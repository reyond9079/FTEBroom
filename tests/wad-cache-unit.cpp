// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/ftebroom_wad_cache.h"
#include <fstream>
#include <cassert>
#include <iostream>
using namespace ftebroom::quake1;
int main(int argc,char** argv){
  assert(argc==2);auto root=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(root);auto path=root/"cache-test.wad";
  std::string bytes(12+40+340+32,0);bytes.replace(0,4,"WAD2");
  auto put=[&](size_t at,uint32_t value){for(int i=0;i<4;i++)bytes[at+i]=char(value>>(8*i));};
  put(4,1);put(8,392);bytes.replace(12,5,"stone");put(28,16);put(32,16);put(36,40);put(40,296);put(44,360);put(48,376);
  put(392,12);put(396,380);put(400,380);bytes[404]=68;bytes.replace(408,5,"stone");
  auto write=[&](){std::ofstream out(path,std::ios::binary);out.write(bytes.data(),bytes.size());};write();
  int reads=0;auto read=[&](const auto& p){++reads;std::ifstream in(p,std::ios::binary);return std::string((std::istreambuf_iterator<char>(in)),{});};
  WadCache cache;auto first=cache.load(path,read);assert(first->size()==1&&reads==1);
  auto second=cache.load(root/"."/"cache-test.wad",read);assert(first==second&&reads==1);
  auto stamp=std::filesystem::last_write_time(path);bytes[52]=1;write();std::filesystem::last_write_time(path,stamp+std::chrono::seconds(2));
  auto third=cache.load(path,read);assert(third!=first&&reads==2&&third->at(0).pixels[0]==1&&first->at(0).pixels[0]==0);
  std::string palette(768,0);auto key=textureCacheKey(third->at(0),palette);palette[0]=1;assert(textureCacheKey(third->at(0),palette)!=key);assert(textureCacheKey(first->at(0),std::string(768,0))!=key);
  bytes.resize(2);write();bool rejected=false;try{cache.load(path,read);}catch(...){rejected=true;}assert(rejected);
  std::cout<<"PASS decoded WAD reuse, canonical paths, changed resources, immutable references, invalidation and palette/pixel disk identities\n";
}
