// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace ftebroom {
inline std::filesystem::path legacyResourceDirectory(std::string input,int profile,int runtime=0) {
  const char* game=profile==3?"Quake II":"Half-Life / Counter-Strike";
  auto begin=input.find_first_not_of(" \t\r\n"),end=input.find_last_not_of(" \t\r\n");
  if(begin==std::string::npos)throw std::runtime_error(std::string("Choose the ")+game+" game or mod folder.");
  input=input.substr(begin,end-begin+1);
  if(input.size()>=2&&input.front()=='"'&&input.back()=='"')input=input.substr(1,input.size()-2);
  if(input.empty())throw std::runtime_error(std::string("Choose the ")+game+" game or mod folder.");
  auto path=std::filesystem::weakly_canonical(std::filesystem::absolute(std::filesystem::u8path(input)));
  while(path.has_relative_path()&&path.filename().empty())path=path.parent_path();
  if(!std::filesystem::is_directory(path))throw std::runtime_error(std::string("The ")+game+" game or mod folder does not exist: "+input);
  auto child=path/(profile==3?"baseq2":runtime?"cstrike":"valve");
  if(std::filesystem::is_directory(child))path=child;
  return path;
}
inline bool quakeId1(const std::filesystem::path& path) {
  auto name=path.filename().u8string();
  std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
  return name=="id1";
}
inline std::filesystem::path quakeResourceDirectory(std::string input) {
  auto begin=input.find_first_not_of(" \t\r\n"),end=input.find_last_not_of(" \t\r\n");
  if(begin==std::string::npos)throw std::runtime_error("Choose the Quake 1 game or mod folder.");
  input=input.substr(begin,end-begin+1);
  if(input.size()>=2 && input.front()=='"' && input.back()=='"')input=input.substr(1,input.size()-2);
  if(input.empty())throw std::runtime_error("Choose the Quake 1 game or mod folder.");
  auto path=std::filesystem::weakly_canonical(std::filesystem::absolute(std::filesystem::u8path(input)));
  while(path.has_relative_path() && path.filename().empty())path=path.parent_path();
  if(!std::filesystem::is_directory(path))throw std::runtime_error("The Quake 1 game or mod folder does not exist: "+input);
  if(!quakeId1(path) && std::filesystem::is_directory(path/"id1"))path/="id1";
  return path;
}
}
