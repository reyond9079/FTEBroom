// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <zlib.h>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef LoadImage
#endif
namespace ftebroom::pak {
namespace fs=std::filesystem;
struct Entry {std::string name; uint32_t offset,size;};
inline uint32_t word(const char* p){uint32_t v=0;for(int i=0;i<4;i++)v|=uint32_t((unsigned char)p[i])<<(8*i);return v;}
inline void put(std::ostream& out,uint32_t v){for(int i=0;i<4;i++)out.put(char(v>>(i*8)));}
inline bool safe(const std::string& n){return !n.empty()&&n.size()<56&&n.front()!='/'&&n.find('\\')==n.npos&&n.find(':')==n.npos&&n.find('\0')==n.npos&&n.find_first_of("\r\n")==n.npos;}
inline bool validName(const std::string& n){if(!safe(n))return false;for(const auto& p:fs::u8path(n))if(p==".."||p==".")return false;return true;}
#include "ftebroom_pk3.h"
inline bool split(const fs::path& path,fs::path& archive,std::string& member){archive.clear();member.clear();bool found=false;for(const auto& part:path){if(found){if(!member.empty())member+='/';member+=part.u8string();}else{archive/=part;auto ext=part.extension().u8string();std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);found=ext==".pak"||ext==".pk3";}}if(!found)archive.clear();return found;}
inline std::vector<Entry> directory(const fs::path& path,bool ignoreUnsafeNames=false){
  if(zipFile(path)){std::vector<Entry> entries;for(const auto& e:zipDirectory(path))entries.push_back({e.name,e.offset,e.size});return entries;}
  std::ifstream in(path,std::ios::binary);char h[12];if(!in.read(h,12)||std::string(h,4)!="PACK")throw std::runtime_error("Invalid Quake PAK: "+path.u8string());
  auto offset=word(h+4),length=word(h+8);auto size=fs::file_size(path);
  if(offset<12||length%64||length/64>65536||offset>size||length>size-offset)throw std::runtime_error("Invalid PAK directory bounds.");
  in.seekg(offset);std::vector<Entry> result;std::map<std::string,bool> seen;
  for(uint32_t i=0;i<length/64;i++){char row[64];if(!in.read(row,64))throw std::runtime_error("Truncated PAK directory.");size_t n=0;while(n<56&&row[n])n++;Entry e{std::string(row,n),word(row+56),word(row+60)};
    if(e.offset<12||e.offset>size||e.size>size-e.offset||seen.count(e.name))throw std::runtime_error("Invalid or duplicate PAK entry.");
    // Official Q2 PAKs contain a few non-canonical aliases. Resource discovery
    // may ignore those names; archive rewriting remains strict and lossless.
    if(!validName(e.name)){if(ignoreUnsafeNames)continue;throw std::runtime_error("Invalid or duplicate PAK entry.");}
    seen[e.name]=true;result.push_back(e);}
  return result;
}
inline std::string read(const fs::path& path){fs::path archive;std::string member;uint64_t size=0,offset=0;auto physical=path;
  if(split(path,archive,member)){if(zipFile(archive))return zipRead(archive,member);if(!validName(member))throw std::runtime_error("Unsafe PAK member path.");physical=archive;bool found=false;for(const auto& e:directory(archive,true))if(e.name==member){size=e.size;offset=e.offset;found=true;break;}if(!found)throw std::runtime_error("PAK entry not found: "+member);}
  else size=fs::file_size(path);
  if(size>512*1024*1024)throw std::runtime_error("Resource exceeds 512 MiB.");std::ifstream in(physical,std::ios::binary);in.seekg(offset);std::string result(size,'\0');if(size&&!in.read(result.data(),size))throw std::runtime_error("Cannot read "+path.u8string());return result;
}
inline bool exists(const fs::path& path){try{fs::path a;std::string m;if(!split(path,a,m))return fs::is_regular_file(path);for(const auto& e:directory(a,true))if(e.name==m)return true;}catch(...){}return false;}
inline fs::path diskFolder(const fs::path& path){fs::path a;std::string m;return split(path,a,m)?a.parent_path():path;}
inline std::vector<std::pair<bool,fs::path>> list(const fs::path& folder){
  std::vector<std::pair<bool,fs::path>> result;fs::path archive;std::string prefix;
  if(!split(folder,archive,prefix)){for(auto& e:fs::directory_iterator(folder)){auto ext=e.path().extension().u8string();std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);result.push_back({e.is_directory()||ext==".pak"||ext==".pk3",e.path()});}return result;}
  // Only cache the directory, never the archive payload, during browser rendering.
  static fs::path cached;static fs::file_time_type stamp;static uintmax_t size;static std::vector<Entry> entries;
  auto now=fs::last_write_time(archive);auto length=fs::file_size(archive);if(cached!=archive||stamp!=now||size!=length){entries=directory(archive);cached=archive;stamp=now;size=length;}
  if(!prefix.empty())prefix+='/';std::map<std::string,bool> children;
  for(const auto& e:entries)if(e.name.rfind(prefix,0)==0){auto rest=e.name.substr(prefix.size());auto slash=rest.find('/');if(!rest.empty())children[rest.substr(0,slash)]=slash!=rest.npos;}
  for(const auto& c:children)result.push_back({c.second,folder/fs::u8path(c.first)});return result;
}
// Stream a complete replacement archive, validate it, back up, then atomically install it.
inline void rewrite(const fs::path& archive,const std::map<std::string,std::string>& changes,const std::string& from="",const std::string& to="",const std::string& removed="",bool keepBackup=true){
  if(zipFile(archive)){zipRewrite(archive,changes,from,to,removed,keepBackup);return;}
  for(const auto& change:changes)if(!validName(change.first))throw std::runtime_error("Invalid PAK destination.");
  if(!from.empty()&&(!validName(from)||!validName(to)||to.rfind(from+"/",0)==0))throw std::runtime_error("Invalid PAK rename destination.");
  auto entries=directory(archive);auto stamp=fs::last_write_time(archive);auto size=fs::file_size(archive);
  std::map<std::string,bool> names;bool renamed=from.empty();
  for(auto& e:entries){if(!from.empty()&&(e.name==from||e.name.rfind(from+"/",0)==0)){e.name=to+e.name.substr(from.size());renamed=true;}if(!validName(e.name)||!names.emplace(e.name,true).second)throw std::runtime_error("PAK rename conflicts with an existing entry.");}
  if(!renamed)throw std::runtime_error("PAK rename source does not exist.");
  auto serial=std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());auto temp=archive;temp+=".ftebroom-"+serial+".tmp";auto backup=archive;backup+=".ftebroom-"+serial+".bak";
  try{std::ifstream in(archive,std::ios::binary);std::ofstream out(temp,std::ios::binary);out.write("PACK",4);put(out,0);put(out,0);std::vector<Entry> next;char block[65536];
    auto write=[&](const Entry& e){auto change=changes.find(e.name);bool changed=change!=changes.end();auto at=out.tellp();auto length=changed?change->second.size():size_t(e.size);if(at<0||uint64_t(at)+length>0x7fffffff)throw std::runtime_error("PAK exceeds classic 2 GiB limit.");next.push_back({e.name,uint32_t(at),uint32_t(length)});if(changed)out.write(change->second.data(),length);else{in.seekg(e.offset);size_t left=e.size;while(left){auto n=std::min(left,sizeof block);if(!in.read(block,n))throw std::runtime_error("Cannot copy PAK entry.");out.write(block,n);left-=n;}}};
    for(const auto& e:entries)if(removed.empty()||(e.name!=removed&&e.name.rfind(removed+"/",0)!=0))write(e);for(const auto& change:changes)if(!names.count(change.first))write({change.first,0,0});
    auto offset=out.tellp();if(next.size()>65536||offset<0||uint64_t(offset)+next.size()*64>0x7fffffff)throw std::runtime_error("PAK directory exceeds supported limits.");
    for(const auto& e:next){char name[56]={};std::copy(e.name.begin(),e.name.end(),name);out.write(name,56);put(out,e.offset);put(out,e.size);}out.seekp(4);put(out,uint32_t(offset));put(out,uint32_t(next.size()*64));out.close();in.close();if(!out)throw std::runtime_error("Cannot finish PAK staging.");directory(temp);
    if(fs::last_write_time(archive)!=stamp||fs::file_size(archive)!=size)throw std::runtime_error("PAK changed while saving. Retry after reloading it.");
    if(keepBackup)fs::copy_file(archive,backup);
#ifdef _WIN32
    if(!ReplaceFileW(archive.c_str(),temp.c_str(),nullptr,0,nullptr,nullptr)){auto error=GetLastError();throw std::runtime_error("Cannot replace PAK (Windows error "+std::to_string(error)+"); original and backup are preserved.");}
#else
    fs::rename(temp,archive);
#endif
  }catch(...){std::error_code ignored;fs::remove(temp,ignored);throw;}
}
inline void replace(const fs::path& path,const std::string& bytes){
  fs::path archive;std::string member;if(!split(path,archive,member)||!(zipFile(archive)?zipName(member):validName(member)))throw std::runtime_error("Invalid PAK destination.");rewrite(archive,{{member,bytes}});
}
inline void rename(const fs::path& source,const fs::path& destination){
  fs::path a,b;std::string from,to;if(!split(source,a,from)||!split(destination,b,to)||a!=b||from.empty()||to.empty())throw std::runtime_error("Move PAK entries within the same archive; use Copy across archives or disk folders.");
  if(from!=to)rewrite(a,{},from,to);
}
}

namespace ftebroom::pak { inline void erase(const fs::path& path){fs::path a;std::string m;if(!split(path,a,m)||m.empty())throw std::runtime_error("Choose an archive member.");rewrite(a,{},"","",m,false);} }
