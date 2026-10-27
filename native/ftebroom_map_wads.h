// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace ftebroom::quake1 {
struct MapWads {
  std::vector<std::filesystem::path> found, missing;
};
// Resolve a MAP's own references even when the profile already has other WADs.
template<class Exists>
MapWads mapWads(const std::string& source, const std::filesystem::path& map,
               const std::filesystem::path& mod, Exists exists,
               const std::vector<std::filesystem::path>& available = {}) {
  MapWads result;
  std::istringstream input(source); std::string line;
  while (std::getline(input,line)) {
    auto at=line.find_first_not_of(" \t");
    if(at==line.npos || line.compare(at,5,"\"wad\""))continue;
    auto start=line.find('"',at+5),end=line.rfind('"');
    if(start==line.npos || end<=start)break;
    auto list=line.substr(start+1,end-start-1);
    std::replace(list.begin(),list.end(),'\\','/');
    std::istringstream entries(list);std::string entry;
    while(std::getline(entries,entry,';')) {
      if(entry.empty())continue;
      auto path=std::filesystem::u8path(entry);
      if(path.is_relative())path=map.parent_path()/path;
      path=path.lexically_normal();auto resolved=path;
      if(!exists(resolved))resolved=mod/path.filename();
      if(!exists(resolved))resolved=mod/"textures"/path.filename();
      if(!exists(resolved)) {
        auto leaf=path.filename().u8string();
        std::transform(leaf.begin(),leaf.end(),leaf.begin(),[](unsigned char c){return std::tolower(c);});
        std::filesystem::path candidate;bool ambiguous=false;
        for(const auto& known:available) {
          auto name=known.filename().u8string();
          std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return std::tolower(c);});
          if(name!=leaf || !exists(known))continue;
          if(!candidate.empty() && candidate.lexically_normal()!=known.lexically_normal()){ambiguous=true;break;}
          candidate=known;
        }
        if(!candidate.empty() && !ambiguous)resolved=candidate;
      }
      auto& target=exists(resolved)?result.found:result.missing;
      auto value=exists(resolved)?resolved.lexically_normal():path;
      if(std::find(target.begin(),target.end(),value)==target.end())target.push_back(value);
    }
    break;
  }
  return result;
}
// General libraries are fallbacks; the document's ordered WAD list wins.
inline void prioritizeMapWads(std::vector<std::filesystem::path>& sources,
                              const std::vector<std::filesystem::path>& document) {
  for(const auto& path:document) {
    sources.erase(std::remove_if(sources.begin(),sources.end(),[&](const auto& source){
      return source.lexically_normal()==path.lexically_normal();
    }),sources.end());
    sources.push_back(path);
  }
}

}
