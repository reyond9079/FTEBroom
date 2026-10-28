// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_bsp_atlas.h"
#include "ftebroom_legacy_images.h"
namespace ftebroom::bspUV {
using UV=std::array<float,2>;
struct Material {std::string name;unsigned width=0,height=0;std::string image;std::vector<std::array<UV,2>> edges;};
inline std::vector<Material> read(const std::string& source,const std::string& palette,const std::map<std::string,std::string>& wals={}){
  using quake1::word;using quake1::range;using bspAtlas::real;using bspAtlas::u16;
  bool q2=source.substr(0,4)=="IBSP"&&word(source,4)==38,hl=source.size()>=4&&word(source,0)==30,q3=source.substr(0,4)=="IBSP"&&!q2,bsp2=source.substr(0,4)=="BSP2"||source.substr(0,4)=="2PSB";
  if((q3&&word(source,4)!=46)||(!q3&&!q2&&!hl&&!bsp2&&word(source,0)!=29))throw std::runtime_error("UV view supports BSP29, BSP2 and Quake III IBSP46.");
  unsigned header=(q3||q2)?8:4,count=q3?17:q2?19:15;range(source,header,count*8);
  std::vector<std::string> lumps(count);for(unsigned i=0;i<count;i++){auto off=word(source,header+i*8),size=word(source,header+i*8+4);range(source,off,size);lumps[i]=source.substr(off,size);}
  std::vector<Material> result;std::map<unsigned,size_t> indices;uint64_t pixels=0,edges=0;
  auto material=[&](unsigned id)->Material&{
    auto found=indices.find(id);if(found!=indices.end())return result[found->second];
    if(result.size()>=4096)throw std::runtime_error("UV material limit exceeded.");
    Material m;
    if(q2){range(lumps[5],size_t(id)*76,76);m.name=quake1::name(lumps[5],size_t(id)*76+40,32);auto found=wals.find(m.name);if(found!=wals.end()){auto t=legacy::wal(found->second,palette);m.width=t.width;m.height=t.height;m.image=quake1::tga(t,palette);}else m.width=m.height=512;}
    else if(q3){range(lumps[1],size_t(id)*72,72);m.name=quake1::name(lumps[1],size_t(id)*72,64);}
    else{
      auto& l=lumps[2];auto n=word(l,0);if(id>=n)throw std::runtime_error("Invalid BSP texture index.");auto off=word(l,4+size_t(id)*4);range(l,off,40);
      m.name=quake1::name(l,off,16);m.width=word(l,off+16);m.height=word(l,off+20);
      if(!m.width||!m.height||m.width>8192||m.height>8192)throw std::runtime_error("Invalid UV texture size.");
      auto data=word(l,off+24);std::string colours=palette;
      if(hl&&data){size_t end=40;for(int mip=0;mip<4;mip++)end=std::max(end,size_t(word(l,off+24+4*mip))+size_t(m.width>>mip)*(m.height>>mip));range(l,off+end,770);if(u16(l,off+end)!=256)throw std::runtime_error("Invalid GoldSrc texture palette.");colours=l.substr(off+end+2,768);}
      if(data&&colours.size()==768){
        pixels+=uint64_t(m.width)*m.height;if(pixels>33554432)throw std::runtime_error("Embedded UV images exceed 32 million pixels.");
        range(l,size_t(off)+data,size_t(m.width)*m.height);bspAtlas::Image image;image.width=m.width;image.height=m.height;image.rgba.resize(size_t(m.width)*m.height*4);
        for(size_t p=0;p<size_t(m.width)*m.height;p++){unsigned c=(unsigned char)l[size_t(off)+data+p];for(int a=0;a<3;a++)image.rgba[p*4+a]=(unsigned char)colours[c*3+a];image.rgba[p*4+3]=!m.name.empty()&&m.name[0]=='{'&&c==255?0:255;}
        m.image=bspAtlas::tga(image);
      }
    }
    indices[id]=result.size();result.push_back(std::move(m));return result.back();
  };
  auto edge=[&](Material& m,UV a,UV b){for(float v:{a[0],a[1],b[0],b[1]})if(!std::isfinite(v)||std::abs(v)>1e7f)throw std::runtime_error("Invalid BSP UV coordinate.");if(++edges>2000000)throw std::runtime_error("UV edge limit exceeded.");m.edges.push_back({a,b});};
  auto& surfaces=lumps[q3?13:q2?6:7];unsigned stride=q3?104:bsp2?28:20;if(surfaces.size()%stride)throw std::runtime_error("Invalid BSP face records.");
  for(size_t at=0;at<surfaces.size();at+=stride){
    if(q3){
      unsigned type=word(surfaces,at+8);if(type!=1&&type!=2&&type!=3)continue;auto& m=material(word(surfaces,at));
      unsigned first=word(surfaces,at+12),n=word(surfaces,at+16);range(lumps[10],size_t(first)*44,size_t(n)*44);
      auto uv=[&](unsigned v){if(v>=n)throw std::runtime_error("Invalid BSP mesh index.");return UV{real(lumps[10],size_t(first+v)*44+12),real(lumps[10],size_t(first+v)*44+16)};};
      if(type==2){
        unsigned w=word(surfaces,at+96),h=word(surfaces,at+100);if(w<3||h<3||w>129||h>129||uint64_t(w)*h!=n||!(w&1)||!(h&1))throw std::runtime_error("Invalid BSP patch.");
        auto curve=[&](UV a,UV b,UV c){UV previous=a;for(int i=1;i<=16;i++){float t=i/16.f,s=1-t;UV next{a[0]*s*s+2*b[0]*s*t+c[0]*t*t,a[1]*s*s+2*b[1]*s*t+c[1]*t*t};edge(m,previous,next);previous=next;}};
        // Quadratic patch row/column curves expose its UV control network.
        for(unsigned y=0;y<h;y++)for(unsigned x=0;x+2<w;x+=2)curve(uv(y*w+x),uv(y*w+x+1),uv(y*w+x+2));
        for(unsigned x=0;x<w;x++)for(unsigned y=0;y+2<h;y+=2)curve(uv(y*w+x),uv((y+1)*w+x),uv((y+2)*w+x));
      }else{
        unsigned start=word(surfaces,at+20),total=word(surfaces,at+24);if(total%3)throw std::runtime_error("Invalid BSP triangle count.");range(lumps[11],size_t(start)*4,size_t(total)*4);
        for(unsigned i=0;i<total;i+=3){UV a=uv(word(lumps[11],size_t(start+i)*4)),b=uv(word(lumps[11],size_t(start+i+1)*4)),c=uv(word(lumps[11],size_t(start+i+2)*4));edge(m,a,b);edge(m,b,c);edge(m,c,a);}
      }
    }else{
      unsigned ti=bsp2?word(surfaces,at+16):u16(surfaces,at+10),ts=q2?76:40,tl=q2?5:6,el=q2?11:12,sl=q2?12:13,vl=q2?2:3;range(lumps[tl],size_t(ti)*ts,ts);auto& m=material(q2?ti:word(lumps[tl],size_t(ti)*ts+32));
      unsigned n=bsp2?word(surfaces,at+12):u16(surfaces,at+8),first=word(surfaces,at+(bsp2?8:4));if(n>65536)throw std::runtime_error("Invalid BSP face size.");range(lumps[sl],size_t(first)*4,size_t(n)*4);std::vector<UV> points;
      for(unsigned i=0;i<n;i++){int32_t e=int32_t(word(lumps[sl],size_t(first+i)*4));if(e==INT32_MIN)throw std::runtime_error("Invalid BSP edge.");size_t record=size_t(e<0?-e:e)*(bsp2?8:4);unsigned v=bsp2?word(lumps[el],record+(e<0?4:0)):u16(lumps[el],record+(e<0?2:0));range(lumps[vl],size_t(v)*12,12);UV p{};
        for(int a=0;a<2;a++){p[a]=real(lumps[tl],size_t(ti)*ts+a*16+12);for(int k=0;k<3;k++)p[a]+=real(lumps[tl],size_t(ti)*ts+a*16+k*4)*real(lumps[vl],size_t(v)*12+k*4);p[a]/=a?m.height:m.width;}points.push_back(p);
      }
      for(size_t i=0;i<points.size();i++)edge(m,points[i],points[(i+1)%points.size()]);
    }
  }
  return result;
}
// Rasterize once per material, rather than emitting thousands of ImGui vertices
// every frame. Repeated UV tiles are wrapped without clamping the actual UVs.
inline std::string raster(const Material& material,unsigned width,unsigned height){
  if(!width||!height||width>8192||height>8192||uint64_t(width)*height>16777216)throw std::runtime_error("UV overlay image is too large.");
  bspAtlas::Image image;image.width=width;image.height=height;image.rgba.resize(size_t(width)*height*4);uint64_t work=0;
  auto line=[&](double x0,double y0,double x1,double y1){
    int x=int(std::round(std::clamp(x0,0.0,1.0)*(width-1))),y=int(std::round(std::clamp(y0,0.0,1.0)*(height-1))),endx=int(std::round(std::clamp(x1,0.0,1.0)*(width-1))),endy=int(std::round(std::clamp(y1,0.0,1.0)*(height-1)));
    int dx=std::abs(endx-x),dy=-std::abs(endy-y),sx=x<endx?1:-1,sy=y<endy?1:-1,error=dx+dy;
    for(;;){if(++work>32000000)throw std::runtime_error("UV overlay complexity exceeds the safe limit.");auto p=(size_t(y)*width+x)*4;image.rgba[p]=255;image.rgba[p+1]=210;image.rgba[p+2]=40;image.rgba[p+3]=230;if(x==endx&&y==endy)break;int e=2*error;if(e>=dy){error+=dy;x+=sx;}if(e<=dx){error+=dx;y+=sy;}}
  };
  for(auto e:material.edges){double x=e[0][0],y=e[0][1],dx=double(e[1][0])-x,dy=double(e[1][1])-y;std::vector<double> cuts{0,1};
    for(int axis=0;axis<2;axis++){double a=axis?y:x,d=axis?dy:dx;if(std::abs(d)<1e-10)continue;double low=std::min(a,a+d),high=std::max(a,a+d);if(high-low>32768)throw std::runtime_error("UV edge spans too many repeated tiles.");for(double k=std::floor(low)+1;k<high;k++){cuts.push_back((k-a)/d);if(++work>32000000)throw std::runtime_error("UV wrapping exceeds the safe limit.");}}
    std::sort(cuts.begin(),cuts.end());for(size_t i=1;i<cuts.size();i++){double a=cuts[i-1],b=cuts[i],mid=(a+b)/2,tx=std::floor(x+dx*mid),ty=std::floor(y+dy*mid);line(x+dx*a-tx,y+dy*a-ty,x+dx*b-tx,y+dy*b-ty);}
  }
  return bspAtlas::tga(image);
}
}
