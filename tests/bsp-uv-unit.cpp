// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/ftebroom_bsp_uv.h"
#include <cassert>
#include <fstream>
#include <iostream>
std::string read(const char* p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){
  assert(argc==4);auto b=read(argv[1]),palette=read(argv[2]);auto materials=ftebroom::bspUV::read(b,palette);assert(!materials.empty());size_t edges=0,embedded=0;
  for(auto& m:materials){edges+=m.edges.size();embedded+=!m.image.empty();auto image=ftebroom::bspUV::raster(m,m.width?m.width:128,m.height?m.height:128);assert(image.size()>18);size_t visible=0;for(size_t p=21;p<image.size();p+=4)visible+=(unsigned char)image[p]!=0;assert(visible);}
  auto bad=b;ftebroom::bspAtlas::put(bad,b.substr(0,4)=="IBSP"?8:4,0xfffffff0);bool rejected=false;try{ftebroom::bspUV::read(bad,palette);}catch(const std::exception&){rejected=true;}assert(rejected);
  ftebroom::bspUV::Material wrap;wrap.edges.push_back({{{-.25f,.5f},{1.25f,.5f}}});auto image=ftebroom::bspUV::raster(wrap,32,32);for(int x=0;x<32;x++)assert((unsigned char)image[18+(16*32+x)*4+3]==230);
  wrap.edges[0][0][0]=-100000;rejected=false;try{ftebroom::bspUV::raster(wrap,32,32);}catch(const std::exception&){rejected=true;}assert(rejected);
  auto output=ftebroom::bspUV::raster(materials.front(),materials.front().width?materials.front().width:128,materials.front().height?materials.front().height:128);std::ofstream f(argv[3],std::ios::binary);f<<output;assert(f);
  std::cout<<"PASS BSP UV materials="<<materials.size()<<" edges="<<edges<<" embedded="<<embedded<<" wrapping and corrupt-input guards\n";
}
