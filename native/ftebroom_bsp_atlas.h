// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_quake1.h"
#include <cstring>
#include <functional>
#include <map>
namespace ftebroom::bspAtlas {
struct Image {unsigned width=0,height=0;std::vector<unsigned char> rgba,indexed;};
struct Result {std::string bsp;std::vector<Image> images,outlines;};
inline void put(std::string& s,size_t at,uint32_t v){quake1::range(s,at,4);for(int i=0;i<4;i++)s[at+i]=char(v>>(8*i));}
inline void put16(std::string& s,size_t at,unsigned v){quake1::range(s,at,2);s[at]=v;s[at+1]=v>>8;}
inline unsigned u16(const std::string& s,size_t at){quake1::range(s,at,2);return (unsigned char)s[at]|((unsigned char)s[at+1]<<8);}
inline float real(const std::string& s,size_t at){auto bits=quake1::word(s,at);float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f))throw std::runtime_error("Non-finite BSP coordinate.");return f;}
inline void setReal(std::string& s,size_t at,float f){uint32_t bits;std::memcpy(&bits,&f,4);put(s,at,bits);}
inline std::string tga(const Image& image){std::string out(18,0);out[2]=2;out[12]=image.width;out[13]=image.width>>8;out[14]=image.height;out[15]=image.height>>8;out[16]=32;out[17]=40;for(size_t i=0;i<image.rgba.size();i+=4){out+=char(image.rgba[i+2]);out+=char(image.rgba[i+1]);out+=char(image.rgba[i]);out+=char(image.rgba[i+3]);}return out;}
struct Face {size_t record=0;unsigned original=0,first=0;std::vector<std::array<float,2>> uv;float min[2]{},max[2]{};unsigned w=0,h=0,x=0,y=0,page=0;};
inline Result create(const std::string& source,const std::string& palette,const std::string& prefix,const std::function<Image(const std::string&)>& load){
  using quake1::word;using quake1::range;
  bool q3=source.substr(0,4)=="IBSP",bsp2=source.substr(0,4)=="BSP2"||source.substr(0,4)=="2PSB";
  if((q3&&word(source,4)!=46)||(!q3&&!bsp2&&word(source,0)!=29))throw std::runtime_error("Atlas supports Quake BSP29/BSP2 and Quake III IBSP46.");
  if(!q3&&palette.size()!=768)throw std::runtime_error("Load the Quake palette before exporting an atlas.");
  unsigned header=q3?8:4,count=q3?17:15;range(source,header,count*8);
  std::vector<std::string> lumps(count);for(unsigned i=0;i<count;i++){auto off=word(source,header+i*8),size=word(source,header+i*8+4);range(source,off,size);lumps[i]=source.substr(off,size);}
  if(!lumps[q3?14:8].empty())throw std::runtime_error("Create the atlas before running LIGHT; existing lightmaps cannot be remapped safely.");
  uint64_t sourcePixels=0;std::map<unsigned,Image> textures;std::vector<Face> faces;auto& surfaces=lumps[q3?13:7];auto& texinfo=lumps[q3?1:6];
  unsigned stride=q3?104:bsp2?28:20;if(surfaces.size()%stride)throw std::runtime_error("Invalid BSP face records.");
  if(surfaces.size()/stride>4096)throw std::runtime_error("Atlas export supports up to 4096 compiled faces. Export a smaller selection.");
  auto texture=[&](unsigned index)->const Image&{
    auto found=textures.find(index);if(found!=textures.end())return found->second;Image image;std::string name;
    if(q3){range(texinfo,size_t(index)*72,72);name=quake1::name(texinfo,size_t(index)*72,64);if((word(texinfo,size_t(index)*72+64)&4)||(word(texinfo,size_t(index)*72+68)&0x38))throw std::runtime_error("Sky and liquid materials cannot be baked into an opaque atlas.");image=load(name);}
    else {auto& l=lumps[2];auto n=word(l,0);if(index>=n)throw std::runtime_error("Invalid BSP texture index.");auto off=word(l,4+index*4);range(l,off,40);name=quake1::name(l,off,16);
      if(name.empty()||name[0]=='*'||name[0]=='{'||name.rfind("sky",0)==0||name[0]=='+')throw std::runtime_error("Atlas export requires static opaque materials; water, sky, masked and animated materials retain their effects only without an atlas.");
      image.width=word(l,off+16);image.height=word(l,off+20);if(!image.width||!image.height||uint64_t(image.width)*image.height>16777216)throw std::runtime_error("Invalid BSP texture size.");
      auto pixels=word(l,off+24);if(!pixels)throw std::runtime_error("BSP texture is not embedded: "+name);range(l,size_t(off)+pixels,size_t(image.width)*image.height);if(sourcePixels+uint64_t(image.width)*image.height>33554432)throw std::runtime_error("Atlas source images exceed 32 million pixels; export a smaller selection.");image.rgba.resize(size_t(image.width)*image.height*4);image.indexed.assign(l.begin()+off+pixels,l.begin()+off+pixels+size_t(image.width)*image.height);
      for(size_t p=0;p<size_t(image.width)*image.height;p++){unsigned c=(unsigned char)l[off+pixels+p];for(int a=0;a<3;a++)image.rgba[p*4+a]=(unsigned char)palette[c*3+a];image.rgba[p*4+3]=255;}
    }
    if(!image.width||!image.height||uint64_t(image.width)*image.height>16777216||image.rgba.size()!=size_t(image.width)*image.height*4)throw std::runtime_error("Cannot decode atlas material: "+name);
    sourcePixels+=uint64_t(image.width)*image.height;if(sourcePixels>33554432)throw std::runtime_error("Atlas source images exceed 32 million pixels; export a smaller selection.");
    for(size_t a=3;a<image.rgba.size();a+=4)if(image.rgba[a]!=255)throw std::runtime_error("Transparent materials cannot be baked into an opaque atlas.");
    return textures.emplace(index,std::move(image)).first->second;
  };
  for(size_t at=0;at<surfaces.size();at+=stride){Face f;f.record=at;
    if(q3){unsigned type=word(surfaces,at+8);if(type!=1&&type!=3)throw std::runtime_error("Atlas export requires planar or triangle brush surfaces.");f.original=word(surfaces,at);f.first=word(surfaces,at+12);unsigned n=word(surfaces,at+16);if(n>65536)throw std::runtime_error("Too many surface vertices.");range(lumps[10],size_t(f.first)*44,size_t(n)*44);auto& image=texture(f.original);
      for(unsigned v=0;v<n;v++)f.uv.push_back({real(lumps[10],size_t(f.first+v)*44+12)*image.width,real(lumps[10],size_t(f.first+v)*44+16)*image.height});
    } else {unsigned ti=bsp2?word(surfaces,at+16):u16(surfaces,at+10);range(texinfo,size_t(ti)*40,40);f.original=word(texinfo,size_t(ti)*40+32);texture(f.original);f.first=ti;
      unsigned n=bsp2?word(surfaces,at+12):u16(surfaces,at+8),first=word(surfaces,at+(bsp2?8:4));if(n>65536)throw std::runtime_error("Too many face edges.");range(lumps[13],size_t(first)*4,size_t(n)*4);
      for(unsigned e=0;e<n;e++){int32_t edge=(int32_t)word(lumps[13],size_t(first+e)*4);if(edge==INT32_MIN)throw std::runtime_error("Invalid BSP edge.");size_t record=size_t(edge<0?-edge:edge)*(bsp2?8:4);unsigned v=bsp2?word(lumps[12],record+(edge<0?4:0)):u16(lumps[12],record+(edge<0?2:0));range(lumps[3],size_t(v)*12,12);std::array<float,2> uv{};
        for(int a=0;a<2;a++){uv[a]=real(texinfo,size_t(ti)*40+a*16+12);for(int k=0;k<3;k++)uv[a]+=real(texinfo,size_t(ti)*40+a*16+k*4)*real(lumps[3],size_t(v)*12+k*4);}f.uv.push_back(uv);}
    }
    if(f.uv.size()<3)throw std::runtime_error("Degenerate BSP face.");
    for(int a=0;a<2;a++){f.min[a]=f.max[a]=f.uv[0][a];for(auto uv:f.uv){f.min[a]=std::min(f.min[a],uv[a]);f.max[a]=std::max(f.max[a],uv[a]);}f.min[a]=std::floor(f.min[a]);f.max[a]=std::ceil(f.max[a]);if(f.max[a]-f.min[a]>1006)throw std::runtime_error("A face spans more than 1006 texture pixels. Reduce its texture tiling or export without an atlas; resolution is never silently reduced.");}
    f.w=unsigned(f.max[0]-f.min[0])+17;f.h=unsigned(f.max[1]-f.min[1])+17;f.w=(f.w+7)&~7u;f.h=(f.h+7)&~7u;faces.push_back(std::move(f));
  }
  if(faces.empty())throw std::runtime_error("Compiler produced no visible faces.");
  Result result;unsigned x=0,y=0,row=0,page=0;
  auto addPage=[&](){if(result.images.size()>=16)throw std::runtime_error("Atlas exceeds 16 pages. Export a smaller selection.");Image a;a.width=a.height=1024;a.rgba.resize(1024*1024*4);if(!q3)a.indexed.resize(1024*1024);result.images.push_back(a);result.outlines.push_back(std::move(a));};addPage();
  for(auto& f:faces){if(x+f.w>1024){x=0;y+=row;row=0;}if(y+f.h>1024){x=y=row=0;++page;addPage();}f.x=x+8;f.y=y+8;f.page=page;x+=f.w;row=std::max(row,f.h);
    auto& im=texture(f.original);auto& out=result.images[page];
    for(unsigned yy=0;yy<f.h;yy++)for(unsigned xx=0;xx<f.w;xx++){
      int u=int(f.min[0])+int(std::clamp(xx,8u,unsigned(f.max[0]-f.min[0])+8))-8,v=int(f.min[1])+int(std::clamp(yy,8u,unsigned(f.max[1]-f.min[1])+8))-8;
      u=(u%int(im.width)+im.width)%im.width;v=(v%int(im.height)+im.height)%im.height;
      std::copy_n(&im.rgba[(size_t(v)*im.width+u)*4],4,&out.rgba[(size_t(f.y-8+yy)*1024+f.x-8+xx)*4]);
      if(!q3)out.indexed[size_t(f.y-8+yy)*1024+f.x-8+xx]=im.indexed[size_t(v)*im.width+u];
    }
    auto point=[&](unsigned i){return std::array<float,2>{f.uv.at(i)[0]-f.min[0]+f.x,f.uv.at(i)[1]-f.min[1]+f.y};};
    auto line=[&](unsigned a,unsigned b){auto p=point(a),q=point(b);int n=std::max(1,int(std::ceil(std::max(std::fabs(p[0]-q[0]),std::fabs(p[1]-q[1])))));for(int k=0;k<=n;k++){int px=int(std::round(p[0]+(q[0]-p[0])*k/n)),py=int(std::round(p[1]+(q[1]-p[1])*k/n));if(px>=0&&px<1024&&py>=0&&py<1024){auto dst=&result.outlines[page].rgba[(py*1024+px)*4];dst[0]=dst[1]=dst[2]=dst[3]=255;}}};
    if(q3){unsigned start=word(surfaces,f.record+20),n=word(surfaces,f.record+24);range(lumps[11],size_t(start)*4,size_t(n)*4);if(n%3)throw std::runtime_error("Invalid triangle indices.");std::map<std::pair<unsigned,unsigned>,int> edges;for(unsigned i=0;i<n;i+=3){unsigned a[3];for(int k=0;k<3;k++){a[k]=word(lumps[11],size_t(start+i+k)*4);if(a[k]>=f.uv.size())throw std::runtime_error("Invalid surface vertex index.");}for(int k=0;k<3;k++)++edges[std::minmax(a[k],a[(k+1)%3])];}for(auto e:edges)if(e.second==1)line(e.first.first,e.first.second);
      unsigned first=lumps[10].size()/44;for(unsigned i=0;i<f.uv.size();i++){auto v=lumps[10].substr(size_t(f.first+i)*44,44);auto uv=point(i);setReal(v,12,uv[0]/1024);setReal(v,16,uv[1]/1024);lumps[10]+=v;}put(surfaces,f.record+12,first);
      put(surfaces,f.record,unsigned(texinfo.size()/72)+f.page);
    }else{
      for(unsigned i=0;i<f.uv.size();i++)line(i,(i+1)%f.uv.size());
      auto ti=texinfo.substr(size_t(f.first)*40,40);for(int a=0;a<2;a++)setReal(ti,a*16+12,real(ti,a*16+12)-f.min[a]+(a?f.y:f.x));put(ti,32,word(lumps[2],0)+f.page);put(ti,36,0);
      unsigned index=texinfo.size()/40;if(!bsp2&&index>32767)throw std::runtime_error("Atlas requires too many texinfo entries for BSP29. Select BSP2.");if(bsp2)put(surfaces,f.record+16,index);else put16(surfaces,f.record+10,index);texinfo+=ti;
    }
  }
  if(q3){for(unsigned i=0;i<result.images.size();i++){auto name=prefix+std::to_string(i);if(name.size()>63)throw std::runtime_error("Atlas resource path exceeds 63 characters.");std::string shader(72,0);std::copy(name.begin(),name.end(),shader.begin());texinfo+=shader;}}
  else {
    auto old=lumps[2];unsigned n=word(old,0);range(old,4,size_t(n)*4);std::string tex(4+(n+result.images.size())*4,0);put(tex,0,n+result.images.size());
    for(unsigned i=0;i<n;i++){auto offset=word(old,4+i*4);if(offset==0xffffffff){put(tex,4+i*4,offset);continue;}range(old,offset,40);auto w=word(old,offset+16),h=word(old,offset+20);size_t length=40;if(word(old,offset+24))length+=size_t(w)*h*85/64;range(old,offset,length);put(tex,4+i*4,tex.size());tex+=old.substr(offset,length);}
    for(unsigned i=0;i<result.images.size();i++){auto name=prefix+std::to_string(i);if(name.size()>15)throw std::runtime_error("Quake atlas name exceeds 15 characters.");put(tex,4+(n+i)*4,tex.size());std::string mt(40,0);std::copy(name.begin(),name.end(),mt.begin());put(mt,16,1024);put(mt,20,1024);
      // Preserve source indices, including fullbright colors and duplicate palette entries.
      auto pixels=result.images[i].indexed;
      unsigned w=1024;for(int mip=0;mip<4;mip++){put(mt,24+mip*4,mt.size());mt.append((const char*)pixels.data(),pixels.size());if(mip<3){std::vector<unsigned char> smaller(w*w/4);for(unsigned yy=0;yy<w/2;yy++)for(unsigned xx=0;xx<w/2;xx++)smaller[yy*(w/2)+xx]=pixels[(yy*2)*w+xx*2];pixels=std::move(smaller);w/=2;}}tex+=mt;
    }lumps[2]=std::move(tex);
  }
  result.bsp=source.substr(0,header);result.bsp.resize(header+count*8,0);for(unsigned i=0;i<count;i++){while(result.bsp.size()%4)result.bsp+='\0';put(result.bsp,header+i*8,result.bsp.size());put(result.bsp,header+i*8+4,lumps[i].size());result.bsp+=lumps[i];}return result;
}
}
