// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_wad_import.h"
namespace ftebroom {
inline std::string materialKey(std::string name) {
  std::replace(name.begin(),name.end(),'\\','/');
  for(char& c:name)c=char(std::tolower((unsigned char)c));
  if(name.rfind("textures/",0)==0)name.erase(0,9);
  return name;
}
inline bool safeMaterialPath(const std::string& name) {
  if(name.empty()||name.size()>240||name.front()=='/'||name.find_first_of(":\"\r\n\t\\")!=name.npos)return false;
  std::istringstream in(name);std::string part;
  while(std::getline(in,part,'/'))if(part.empty()||part=="."||part=="..")return false;
  return true;
}
inline quake1::Texture placeholderTexture(const std::string& name,const std::string& palette,bool neutral=false) {
  if(palette.size()!=768||!safeMaterialPath(name))throw std::runtime_error("Invalid placeholder texture name or palette: "+name);
  PaletteQuantizer colors(palette,224);
  auto a=neutral?colors.nearest(144,144,144):colors.nearest(96,16,96);
  auto b=neutral?colors.nearest(160,160,160):colors.nearest(240,48,240);
  quake1::Texture t{name,64,64,{},palette};t.pixels.resize(4096);
  for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++)t.pixels[y*64+x]=((x/8+y/8)&1)?a:b;
  return t;
}
inline std::string placeholderWal(const quake1::Texture& t) {
  std::string out(100,0);out.replace(0,std::min(size_t(31),t.name.size()),t.name.substr(0,31));
  auto put=[&](size_t at,uint32_t v){for(int b=0;b<4;b++)out[at+b]=char(v>>(8*b));};
  put(32,t.width);put(36,t.height);
  for(int mip=0;mip<4;mip++){put(40+4*mip,out.size());for(unsigned y=0;y<(t.height>>mip);y++)for(unsigned x=0;x<(t.width>>mip);x++)out+=char(t.pixels[(y<<mip)*t.width+(x<<mip)]);}
  return out;
}
}
