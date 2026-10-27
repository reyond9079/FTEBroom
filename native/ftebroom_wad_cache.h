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
