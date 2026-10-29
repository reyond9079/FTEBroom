// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_model_mdl_export.h"
#include "ftebroom_quake1.h"
#include "ftebroom_model_edit_math.h"
namespace ftebroom::modelExport {
struct Md2Output {std::string model,skin;};
inline std::string transformMd2(std::string bytes,const modelEdit::Matrix& matrix){
  using namespace quake1;range(bytes,0,68);if(bytes.substr(0,4)!="IDP2"||word(bytes,4)!=8)throw std::runtime_error("Expected MD2 v8.");
  unsigned nv=word(bytes,24),nf=word(bytes,40),stride=word(bytes,16),at=word(bytes,56);if(!nv||nv>2048||!nf||nf>512||stride<40+nv*4)throw std::runtime_error("MD2 exceeds Quake II limits.");range(bytes,at,size_t(stride)*nf);
  auto real=[&](size_t p){auto u=word(bytes,p);float f;std::memcpy(&f,&u,4);if(!std::isfinite(f))throw std::runtime_error("Invalid MD2 coordinate.");return f;};
  auto put=[&](size_t p,float f){unsigned u;std::memcpy(&u,&f,4);for(int i=0;i<4;i++)bytes[p+i]=char(u>>(8*i));};
  static const float normals[][3]={
#include "third_party/quake_anorms.h"
  };
  auto inv=modelEdit::inverse(matrix);std::array<unsigned char,162> remap{};
  for(unsigned n=0;n<162;n++){modelEdit::V v{};for(int i=0;i<3;i++)for(int j=0;j<3;j++)v[i]+=inv.m[j][i]*normals[n][j];float best=-1e30f;for(unsigned k=0;k<162;k++){float d=0;for(int a=0;a<3;a++)d+=v[a]*normals[k][a];if(d>best){best=d;remap[n]=k;}}}
  for(unsigned f=0;f<nf;f++){size_t p=at+f*stride;modelEdit::V scale{real(p),real(p+4),real(p+8)},origin{real(p+12),real(p+16),real(p+20)},lo{1e30f,1e30f,1e30f},hi{-1e30f,-1e30f,-1e30f};std::vector<modelEdit::V> points;
    for(unsigned v=0;v<nv;v++){modelEdit::V a;for(int i=0;i<3;i++)a[i]=origin[i]+scale[i]*(unsigned char)bytes[p+40+v*4+i];a=matrix.point(a);for(int i=0;i<3;i++){if(!std::isfinite(a[i])||std::abs(a[i])>1e7f)throw std::runtime_error("MD2 transform exceeds coordinate limits.");lo[i]=std::min(lo[i],a[i]);hi[i]=std::max(hi[i],a[i]);}points.push_back(a);}
    for(int i=0;i<3;i++){scale[i]=hi[i]>lo[i]?(hi[i]-lo[i])/255.f:1.f;put(p+i*4,scale[i]);put(p+12+i*4,lo[i]);}
    for(unsigned v=0;v<nv;v++){for(int i=0;i<3;i++)bytes[p+40+v*4+i]=char(std::clamp(int(std::lround((points[v][i]-lo[i])/scale[i])),0,255));unsigned normal=(unsigned char)bytes[p+43+v*4];if(normal>=162)throw std::runtime_error("Invalid MD2 normal.");bytes[p+43+v*4]=char(remap[normal]);}
  }return bytes;
}
inline std::string pcx(unsigned w,unsigned h,const std::string& pixels,const std::string& palette){
  if(!w||!h||w>4096||h>4096||pixels.size()!=size_t(w)*h||palette.size()!=768)throw std::runtime_error("Invalid PCX skin.");
  std::string b(128,0);b[0]=10;b[1]=5;b[2]=1;b[3]=8;
  auto u=[&](int p,unsigned v){b[p]=char(v);b[p+1]=char(v>>8);};u(8,w-1);u(10,h-1);u(12,w);u(14,h);b[65]=1;u(66,(w+1)&~1u);u(68,1);
  for(unsigned y=0;y<h;y++)for(unsigned x=0;x<((w+1)&~1u);){unsigned char c=x<w?(unsigned char)pixels[y*w+x]:0;unsigned n=1;while(n<63&&x+n<w&&(unsigned char)pixels[y*w+x+n]==c)n++;if(n>1||c>=192)b+=char(192|n);b+=char(c);x+=n;}
  b+=char(12);b+=palette;return b;
}
// Use the same padded, wrapping-aware atlas and normal quantizer as Quake MDL.
// MD2 stores UVs separately and has per-frame scale/translation, without any
// normalization of the source model's coordinate system.
inline Md2Output md2(const Mesh& mesh,const std::vector<Skin>& skins,const std::string& palette,const std::string& skinName){
  if(mesh.frames>512)throw std::runtime_error("Native Quake II MD2 supports at most 512 frames.");
  auto q=mdl(mesh,skins,palette);auto u=[&](size_t p){return quake1::word(q,p);};
  unsigned w=u(52),h=u(56),nv=u(60),nt=u(64),nf=u(68),frameSize=40+4*nv;
  unsigned st=68+64,tri=st+nv*4,frames=tri+nt*12,end=frames+nf*frameSize;
  Binary out;out.bytes="IDP2";for(unsigned n:{8u,w,h,frameSize,1u,nv,nv,nt,0u,nf,68u,st,tri,frames,end,end})out.u32(n);out.text(skinName,64);
  size_t at=88+size_t(w)*h;quake1::range(q,at,nv*12+nt*16);
  for(unsigned v=0;v<nv;v++){out.u16(u(at+v*12+4));out.u16(u(at+v*12+8));}at+=nv*12;
  for(unsigned t=0;t<nt;t++){for(int a=0;a<3;a++)out.u16(u(at+t*16+4+a*4));for(int a=0;a<3;a++)out.u16(u(at+t*16+4+a*4));}at+=nt*16;
  for(unsigned f=0;f<nf;f++){quake1::range(q,at,28+nv*4);out.bytes+=q.substr(8,24);out.bytes+=q.substr(at+12,16);out.bytes+=q.substr(at+28,nv*4);at+=28+nv*4;}
  return {std::move(out.bytes),pcx(w,h,q.substr(88,size_t(w)*h),palette)};
}
}
