// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/ftebroom_bsp_atlas.h"
#include <fstream>
#include <cassert>
#include <iostream>
std::string read(const char* path){std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
int main(int argc,char** argv){
  using namespace ftebroom;assert(argc==4);auto source=read(argv[1]),palette=read(argv[2]);bool q3=source.substr(0,4)=="IBSP";
  auto result=bspAtlas::create(source,palette,"testatlas",[](const std::string&){bspAtlas::Image image;image.width=image.height=16;image.rgba.resize(16*16*4);for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){auto p=&image.rgba[(y*16+x)*4];p[0]=x*16;p[1]=y*16;p[2]=0;p[3]=255;}return image;});
  assert(quake1::bsp(result.bsp,!q3));assert(!result.images.empty()&&result.images.size()==result.outlines.size());
  auto lump=[](const std::string& b,int i,bool q){size_t at=(q?8:4)+i*8;return b.substr(quake1::word(b,at),quake1::word(b,at+4));};
  // Positions, collision and entity placement remain byte-identical.
  for(int i:q3?std::vector<int>{0,2,3,4,5,6,7,8,9}:std::vector<int>{0,1,3,5,9,10,11,12,13,14})assert(lump(source,i,q3)==lump(result.bsp,i,q3));
  size_t visible=0,transparent=0;for(auto& page:result.outlines)for(size_t p=3;p<page.rgba.size();p+=4){visible+=page.rgba[p]!=0;transparent+=page.rgba[p]==0;}std::cout<<"outline visible="<<visible<<" transparent="<<transparent<<std::endl;assert(visible>100&&transparent>10000);
  auto faces=lump(result.bsp,q3?13:7,q3);assert(!faces.empty());
  if(q3){auto vertices=lump(result.bsp,10,true);for(size_t f=0;f<faces.size();f+=104){unsigned first=quake1::word(faces,f+12),n=quake1::word(faces,f+16);for(unsigned v=0;v<n;v++){for(int a=0;a<2;a++){auto uv=bspAtlas::real(vertices,size_t(first+v)*44+12+a*4);assert(uv>=0&&uv<=1);}}}}
  else{auto tex=lump(result.bsp,2,false);unsigned old=quake1::word(lump(source,2,false),0),n=quake1::word(tex,0);assert(n==old+result.images.size());for(unsigned i=old;i<n;i++){unsigned at=quake1::word(tex,4+i*4);assert(quake1::name(tex,at,16)=="testatlas"+std::to_string(i-old));assert(quake1::word(tex,at+16)==1024);}}
  auto oldFaces=lump(source,q3?13:7,q3),oldVertices=lump(source,q3?10:3,q3),vertices=lump(result.bsp,q3?10:3,q3);
  auto tex=lump(result.bsp,2,false),oldTex=lump(source,2,false),oldTi=lump(source,6,false),ti=lump(result.bsp,6,false);
  bool extended=source.substr(0,4)=="BSP2"||source.substr(0,4)=="2PSB";unsigned stride=q3?104:extended?28:20;
  for(size_t f=0;f<faces.size();f+=stride){
    if(q3){unsigned ov=quake1::word(oldFaces,f+12),nv=quake1::word(faces,f+12);float u=bspAtlas::real(oldVertices,size_t(ov)*44+12),v=bspAtlas::real(oldVertices,size_t(ov)*44+16);int x=int(std::floor(u*16)),y=int(std::floor(v*16));x=(x%16+16)%16;y=(y%16+16)%16;unsigned nx=unsigned(bspAtlas::real(vertices,size_t(nv)*44+12)*1024),ny=unsigned(bspAtlas::real(vertices,size_t(nv)*44+16)*1024);auto& image=result.images.at(quake1::word(faces,f)-lump(source,1,true).size()/72);auto c=&image.rgba[(size_t(ny)*1024+nx)*4];assert(c[0]==x*16&&c[1]==y*16);}
    else {unsigned oti=extended?quake1::word(oldFaces,f+16):bspAtlas::u16(oldFaces,f+10),nti=extended?quake1::word(faces,f+16):bspAtlas::u16(faces,f+10);unsigned first=quake1::word(oldFaces,f+(extended?8:4));auto edges=lump(source,13,false),edgeData=lump(source,12,false);int32_t e=(int32_t)quake1::word(edges,size_t(first)*4);size_t ep=size_t(e<0?-e:e)*(extended?8:4);unsigned vi=extended?quake1::word(edgeData,ep+(e<0?4:0)):bspAtlas::u16(edgeData,ep+(e<0?2:0));float oldUV[2]{},newUV[2]{};for(int a=0;a<2;a++){oldUV[a]=bspAtlas::real(oldTi,size_t(oti)*40+a*16+12);newUV[a]=bspAtlas::real(ti,size_t(nti)*40+a*16+12);for(int k=0;k<3;k++){float p=bspAtlas::real(vertices,size_t(vi)*12+k*4);oldUV[a]+=bspAtlas::real(oldTi,size_t(oti)*40+a*16+k*4)*p;newUV[a]+=bspAtlas::real(ti,size_t(nti)*40+a*16+k*4)*p;}}
      auto sample=[&](const std::string& data,unsigned texture,float* uv){unsigned at=quake1::word(data,4+texture*4),w=quake1::word(data,at+16),h=quake1::word(data,at+20);int x=int(std::floor(uv[0])),y=int(std::floor(uv[1]));x=(x%int(w)+w)%w;y=(y%int(h)+h)%h;return (unsigned char)data[at+quake1::word(data,at+24)+size_t(y)*w+x];};assert(sample(oldTex,quake1::word(oldTi,size_t(oti)*40+32),oldUV)==sample(tex,quake1::word(ti,size_t(nti)*40+32),newUV));
    }
  }
  bool rejected=false;try{auto broken=source;bspAtlas::put(broken,(q3?8:4)+8*3,0xfffffff0);bspAtlas::create(broken,palette,"test",[](const std::string&){return bspAtlas::Image{};});}catch(const std::exception&){rejected=true;}assert(rejected);
  std::ofstream out(argv[3],std::ios::binary);out<<result.bsp;out.close();assert(out);std::cout<<"PASS BSP atlas: "<<(q3?"Q3":"Q1")<<", collision/positions retained, UVs, template and corrupt-input checks\n";
}
