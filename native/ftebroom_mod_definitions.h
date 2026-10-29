// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_pak.h"
namespace ftebroom {
inline std::string discoverModDefinitions(const std::filesystem::path& root) {
  namespace fs=std::filesystem;
  if(root.empty()||!fs::is_directory(root))return {};
  std::vector<fs::path> loose,packed;
  auto supported=[](const fs::path& p){auto e=p.extension().u8string();std::transform(e.begin(),e.end(),e.begin(),::tolower);return e==".fgd"||e==".def"||e==".ent";};
  // Nuclide's decls/def/*.def are runtime declarations, not QUAKED editor
  // definitions. Detect the content before replacing the bundled game FGD.
  auto definition=[&](const fs::path& p){
    if(!supported(p))return false;
    try{auto bytes=pak::read(p);if(bytes.size()>16*1024*1024)return false;
      auto ext=p.extension().u8string();std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);
      if(ext==".def")return bytes.find("/*QUAKED")!=bytes.npos||bytes.find("/* QUAKED")!=bytes.npos;
      if(ext==".fgd")return bytes.find("@PointClass")!=bytes.npos||bytes.find("@SolidClass")!=bytes.npos||bytes.find("@include")!=bytes.npos||bytes.find("@BaseClass")!=bytes.npos;
      return bytes.find("<classes")!=bytes.npos||bytes.find("<point")!=bytes.npos||bytes.find("<group")!=bytes.npos;
    }catch(...){return false;}
  };
  size_t count=0;
  for(fs::recursive_directory_iterator it(root,fs::directory_options::skip_permission_denied),end;it!=end;++it) {
    if(++count>50000)break;
    if(it->is_directory()) {auto n=it->path().filename().u8string();if(it.depth()>=6||n=="_ftebroom-build"||n=="cache"||n==".git")it.disable_recursion_pending();continue;}
    if(!it->is_regular_file())continue;
    auto p=it->path();if(definition(p))loose.push_back(p);
    else if(p.extension()==".pak"||p.extension()==".pk3") {
      for(const auto& e:pak::directory(p))if(definition(p/fs::u8path(e.name)))packed.push_back(p/fs::u8path(e.name));
    }
  }
  auto order=[&](const fs::path& a,const fs::path& b){
    auto depth=[&](const fs::path& p){auto r=p.lexically_relative(root);return std::distance(r.begin(),r.end());};
    return depth(a)!=depth(b)?depth(a)<depth(b):a.generic_u8string()<b.generic_u8string();
  };
  // Loose mod definitions override packed defaults; top-level files win over examples.
  std::sort(loose.begin(),loose.end(),order);std::sort(packed.begin(),packed.end(),order);
  return !loose.empty()?loose.front().u8string():!packed.empty()?packed.front().u8string():std::string{};
}
}
