// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

namespace ftebroom::quake1 {
struct MapWads {
  std::vector<std::filesystem::path> found, missing;
};
// Resolve a MAP's own references even when the profile already has other WADs.
template<class Exists>
MapWads mapWads(const std::string& source, const std::filesystem::path& map,
               const std::filesystem::path& mod, Exists exists) {
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
      auto& target=exists(resolved)?result.found:result.missing;
      auto value=exists(resolved)?resolved.lexically_normal():path;
      if(std::find(target.begin(),target.end(),value)==target.end())target.push_back(value);
    }
    break;
  }
  return result;
}
}
