// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_quake1.h"
#include <filesystem>
#include <memory>
#include <map>
namespace ftebroom::quake1 {
class WadCache {
  struct Entry {
    uintmax_t size=0;std::filesystem::file_time_type modified{};
    size_t bytes=0;uint64_t used=0;
    std::shared_ptr<const std::vector<Texture>> textures;
  };
  std::map<std::filesystem::path,Entry> entries;
  size_t resident=0;uint64_t clock=0;
public:
  template<class Read> std::shared_ptr<const std::vector<Texture>> load(const std::filesystem::path& path,Read read) {
    auto key=std::filesystem::weakly_canonical(path);
    auto size=std::filesystem::file_size(key);auto modified=std::filesystem::last_write_time(key);
    auto found=entries.find(key);
    if(found!=entries.end() && found->second.size==size && found->second.modified==modified){found->second.used=++clock;return found->second.textures;}
    auto textures=std::make_shared<const std::vector<Texture>>(wad(read(key)));
    size_t bytes=0;for(const auto& t:*textures)bytes+=sizeof(t)+t.name.size()+t.pixels.size();
    if(found!=entries.end()){resident-=found->second.bytes;entries.erase(found);}
    constexpr size_t limit=256u*1024*1024;
    while(!entries.empty() && resident+bytes>limit){auto oldest=entries.begin();for(auto it=entries.begin();it!=entries.end();++it)if(it->second.used<oldest->second.used)oldest=it;resident-=oldest->second.bytes;entries.erase(oldest);}
    if(bytes<=limit){entries.emplace(key,Entry{size,modified,bytes,++clock,textures});resident+=bytes;}
    return textures;
  }
};
// A disk image is trusted only after byte validation/write, while its immutable
// decoded source, palette and on-disk metadata are unchanged. Weak ownership
// preserves the decoded WAD cache's memory limit and detects resource reloads.
class WadImageCache {
  struct Entry {
    std::weak_ptr<const std::vector<Texture>> owner;
    size_t index;std::string palette;uintmax_t size;
    std::filesystem::file_time_type modified;
  };
  std::map<std::filesystem::path,Entry> entries;
public:
  void prune(){for(auto it=entries.begin();it!=entries.end();)if(it->second.owner.expired())it=entries.erase(it);else ++it;}
  bool unchanged(const std::filesystem::path& path,const std::shared_ptr<const std::vector<Texture>>& owner,size_t index,const std::string& palette) const {
    if(!owner)return false;
    auto found=entries.find(path);if(found==entries.end())return false;
    const auto& entry=found->second;
    if(entry.owner.lock()!=owner || entry.index!=index || entry.palette!=palette)return false;
    std::error_code ec;auto size=std::filesystem::file_size(path,ec);if(ec || size!=entry.size)return false;
    auto modified=std::filesystem::last_write_time(path,ec);return !ec && modified==entry.modified;
  }
  void validated(const std::filesystem::path& path,const std::shared_ptr<const std::vector<Texture>>& owner,size_t index,const std::string& palette){
    std::error_code ec;auto size=std::filesystem::file_size(path,ec);if(ec)return;
    auto modified=std::filesystem::last_write_time(path,ec);if(ec)return;
    if(entries.size()>=16384){prune();if(entries.size()>=16384)entries.clear();}
    entries.insert_or_assign(path,Entry{owner,index,palette,size,modified});
  }
};
// Versioned content identity. Disk hits are compared byte-for-byte before reuse.
inline std::string textureCacheKey(const Texture& t,const std::string& palette) {
  uint64_t hash=14695981039346656037ull;
  auto feed=[&](unsigned char c){hash=(hash^c)*1099511628211ull;};
  for(unsigned char c:palette)feed(c);for(unsigned char c:t.name)feed(c);
  for(auto value:{t.width,t.height})for(int shift=0;shift<32;shift+=8)feed((value>>shift)&255);
  for(auto c:t.pixels)feed(c);
  std::ostringstream out;out<<"v1-"<<std::hex<<hash;return out.str();
}
}
