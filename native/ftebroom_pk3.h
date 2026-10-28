// SPDX-License-Identifier: GPL-2.0-or-later
// Included inside ftebroom::pak. ZIP32 store/deflate; no encrypted or ZIP64 entries.
struct ZipEntry {std::string name;uint32_t offset=0,compressed=0,size=0,crc=0;uint16_t method=0,flags=0;};
inline uint16_t half(const char* p){return (unsigned char)p[0]|uint16_t((unsigned char)p[1])<<8;}
inline void putHalf(std::ostream& out,uint16_t v){out.put(char(v));out.put(char(v>>8));}
inline bool zipFile(const fs::path& path){auto ext=path.extension().u8string();std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);return ext==".pk3";}
inline bool zipName(std::string name){if(!name.empty()&&name.back()=='/')name.pop_back();if(name.empty()||name.size()>65534||name.front()=='/'||name.find_first_of("\\:\r\n")!=name.npos||name.find('\0')!=name.npos)return false;for(const auto& p:fs::u8path(name))if(p==".."||p==".")return false;return true;}
inline std::vector<ZipEntry> zipDirectory(const fs::path& path){
  std::ifstream in(path,std::ios::binary);auto size=fs::file_size(path);if(size<22)throw std::runtime_error("Truncated PK3.");
  size_t tail=std::min<uintmax_t>(size,65557);std::string bytes(tail,0);in.seekg(size-tail);if(!in.read(bytes.data(),tail))throw std::runtime_error("Cannot read PK3 directory.");
  size_t end=bytes.npos;for(size_t i=tail-22;;--i){if(word(bytes.data()+i)==0x06054b50&&i+22+half(bytes.data()+i+20)==tail){end=i;break;}if(!i)break;}
  if(end==bytes.npos)throw std::runtime_error("PK3 end-of-directory was not found.");auto h=bytes.data()+end;
  unsigned count=half(h+10);uint64_t offset=word(h+16),length=word(h+12);
  if(half(h+4)||half(h+6)||half(h+8)!=count||count==65535||offset+length>size-tail+end)throw std::runtime_error("Split or ZIP64 PK3 archives are unsupported.");
  in.seekg(offset);std::vector<ZipEntry> result;std::map<std::string,bool> seen;uint64_t consumed=0;
  for(unsigned i=0;i<count;i++){
    char row[46];if(!in.read(row,46)||word(row)!=0x02014b50)throw std::runtime_error("Invalid PK3 central directory.");
    auto n=half(row+28),extra=half(row+30),comment=half(row+32);consumed+=46+n+extra+comment;if(consumed>length)throw std::runtime_error("PK3 directory overflow.");
    ZipEntry e;e.name.resize(n);if(!in.read(e.name.data(),n))throw std::runtime_error("Truncated PK3 name.");in.seekg(extra+comment,std::ios::cur);
    e.flags=half(row+8);e.method=half(row+10);e.crc=word(row+16);e.compressed=word(row+20);e.size=word(row+24);e.offset=word(row+42);
    if(!zipName(e.name)||!seen.emplace(e.name,true).second||(e.flags&1)||(e.method!=0&&e.method!=8)||e.compressed==0xffffffff||e.size==0xffffffff||uint64_t(e.offset)+30>offset)throw std::runtime_error("Unsupported, unsafe or duplicate PK3 entry.");
    result.push_back(e);
  }
  return result;
}
inline uint64_t zipData(std::ifstream& in,const ZipEntry& e,uint64_t archiveSize){
  in.seekg(e.offset);char h[30];if(!in.read(h,30)||word(h)!=0x04034b50||half(h+8)!=e.method||(half(h+6)&1))throw std::runtime_error("Invalid PK3 local header.");
  std::string name(half(h+26),0);if(!in.read(name.data(),name.size())||name!=e.name)throw std::runtime_error("PK3 local name differs from directory.");
  uint64_t offset=uint64_t(e.offset)+30+name.size()+half(h+28);if(offset>archiveSize||e.compressed>archiveSize-offset)throw std::runtime_error("PK3 payload exceeds archive.");return offset;
}
inline std::string zipRead(const fs::path& archive,const std::string& member){
  for(const auto& e:zipDirectory(archive))if(e.name==member){
    if(e.size>512*1024*1024||e.compressed>512*1024*1024)throw std::runtime_error("PK3 member exceeds 512 MiB.");
    std::ifstream in(archive,std::ios::binary);auto offset=zipData(in,e,fs::file_size(archive));in.seekg(offset);std::string packed(e.compressed,0),result(e.size,0);if(e.compressed&&!in.read(packed.data(),e.compressed))throw std::runtime_error("Truncated PK3 payload.");
    if(e.method==0){if(e.size!=e.compressed)throw std::runtime_error("Invalid stored PK3 size.");result=std::move(packed);}
    else {z_stream z{};z.next_in=(Bytef*)packed.data();z.avail_in=packed.size();char empty;z.next_out=(Bytef*)(result.empty()?&empty:result.data());z.avail_out=std::max<size_t>(1,result.size());if(inflateInit2(&z,-MAX_WBITS)!=Z_OK)throw std::runtime_error("Cannot initialize PK3 decoder.");int status=inflate(&z,Z_FINISH);bool valid=status==Z_STREAM_END&&z.total_out==e.size&&z.total_in==e.compressed;inflateEnd(&z);if(!valid)throw std::runtime_error("Invalid compressed PK3 data.");}
    if(crc32(0,(const Bytef*)result.data(),result.size())!=e.crc)throw std::runtime_error("PK3 CRC mismatch.");return result;
  }throw std::runtime_error("PK3 member not found.");
}
inline void zipRewrite(const fs::path& archive,const std::map<std::string,std::string>& changes,const std::string& from,const std::string& to,const std::string& removed,bool keepBackup=true){
  auto entries=zipDirectory(archive);auto stamp=fs::last_write_time(archive);auto size=fs::file_size(archive);auto serial=std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());auto temp=archive;temp+=".ftebroom-"+serial+".tmp";auto backup=archive;backup+=".ftebroom-"+serial+".bak";
  for(const auto& c:changes)if(!zipName(c.first))throw std::runtime_error("Invalid PK3 destination.");
  if(!from.empty()&&(!zipName(from)||!zipName(to)||to.rfind(from+"/",0)==0))throw std::runtime_error("Invalid PK3 rename.");
  bool matched=from.empty()&&removed.empty();std::map<std::string,bool> names;std::vector<ZipEntry> written;
  try {std::ifstream in(archive,std::ios::binary);std::ofstream out(temp,std::ios::binary);
    auto write=[&](ZipEntry e,bool original){
      if(!removed.empty()&&(e.name==removed||e.name.rfind(removed+"/",0)==0)){matched=true;return;}
      uint64_t sourceOffset=original?zipData(in,e,size):0;
      if(!from.empty()&&(e.name==from||e.name.rfind(from+"/",0)==0)){e.name=to+e.name.substr(from.size());matched=true;}
      if(!zipName(e.name)||!names.emplace(e.name,true).second)throw std::runtime_error("PK3 name conflict.");
      auto changed=changes.find(e.name);if(changed!=changes.end()){if(changed->second.size()>512*1024*1024)throw std::runtime_error("PK3 member too large.");e.size=e.compressed=changed->second.size();e.crc=crc32(0,(const Bytef*)changed->second.data(),e.size);e.method=0;}
      auto pos=out.tellp();if(pos<0||uint64_t(pos)+30+e.name.size()+e.compressed>0xffff0000ULL)throw std::runtime_error("PK3 requires unsupported ZIP64.");e.offset=uint32_t(pos);e.flags&=2048;
      put(out,0x04034b50);putHalf(out,20);putHalf(out,e.flags);putHalf(out,e.method);put(out,0);put(out,e.crc);put(out,e.compressed);put(out,e.size);putHalf(out,e.name.size());putHalf(out,0);out.write(e.name.data(),e.name.size());
      if(changed!=changes.end())out.write(changed->second.data(),changed->second.size());else{in.seekg(sourceOffset);char block[65536];uint32_t left=e.compressed;while(left){auto n=std::min<size_t>(left,sizeof block);if(!in.read(block,n))throw std::runtime_error("Cannot copy PK3 member.");out.write(block,n);left-=n;}}
      written.push_back(e);
    };
    for(auto e:entries)write(e,true);for(const auto& c:changes)if(!names.count(c.first)){ZipEntry e;e.name=c.first;write(e,false);}
    if(!matched)throw std::runtime_error("PK3 source not found.");if(written.size()>=65535)throw std::runtime_error("Too many PK3 entries.");auto central=out.tellp();
    for(const auto& e:written){put(out,0x02014b50);putHalf(out,20);putHalf(out,20);putHalf(out,e.flags);putHalf(out,e.method);put(out,0);put(out,e.crc);put(out,e.compressed);put(out,e.size);putHalf(out,e.name.size());putHalf(out,0);putHalf(out,0);putHalf(out,0);putHalf(out,0);put(out,0);put(out,e.offset);out.write(e.name.data(),e.name.size());}
    auto end=out.tellp();if(end<0||uint64_t(end)>0xffff0000ULL)throw std::runtime_error("PK3 directory requires ZIP64.");put(out,0x06054b50);put(out,0);putHalf(out,written.size());putHalf(out,written.size());put(out,uint32_t(end-central));put(out,uint32_t(central));putHalf(out,0);out.close();in.close();if(!out)throw std::runtime_error("Cannot finish PK3.");zipDirectory(temp);
    if(fs::last_write_time(archive)!=stamp||fs::file_size(archive)!=size)throw std::runtime_error("PK3 changed during save.");if(keepBackup)fs::copy_file(archive,backup);
#ifdef _WIN32
    if(!ReplaceFileW(archive.c_str(),temp.c_str(),nullptr,0,nullptr,nullptr))throw std::runtime_error("Cannot replace PK3; original and backup preserved.");
#else
    fs::rename(temp,archive);
#endif
  }catch(...){std::error_code ignored;fs::remove(temp,ignored);throw;}
}
