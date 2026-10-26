// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_model_edit_math.h"
namespace ftebroom::modelEdit {
// Rewrite only MDL geometry/derived bounds. Skin bytes, UVs, names, frame-group
// intervals and flags remain byte-for-byte identical to the original model.
inline std::string transformMdl(std::string bytes,const Matrix& matrix){
  bool identity=true;for(int i=0;i<3;i++)for(int j=0;j<4;j++)identity&=std::abs(matrix.m[i][j]-(i==j?1.f:0.f))<1e-7f;if(identity)return bytes;
  auto require=[&](size_t p,size_t n){if(p>bytes.size()||n>bytes.size()-p)throw std::runtime_error("Truncated Quake MDL.");};
  auto word=[&](size_t p){require(p,4);unsigned v=0;for(int a=0;a<4;a++)v|=unsigned((unsigned char)bytes[p+a])<<(a*8);return v;};
  auto real=[&](size_t p){unsigned v=word(p);float f;std::memcpy(&f,&v,4);if(!std::isfinite(f))throw std::runtime_error("Invalid MDL header.");return f;};
  auto put=[&](size_t p,float value){unsigned v;std::memcpy(&v,&value,4);for(int a=0;a<4;a++)bytes[p+a]=char(v>>(a*8));};
  require(0,84);if(bytes.substr(0,4)!="IDPO"||word(4)!=6)throw std::runtime_error("Lossless skin export requires Quake MDL version 6.");
  unsigned skins=word(48),width=word(52),height=word(56),vertices=word(60),triangles=word(64),frames=word(68);
  if(!skins||skins>256||!width||width>4096||!height||height>4096||!vertices||vertices>65536||triangles>131072||!frames||frames>4096)throw std::runtime_error("MDL exceeds editor transform limits.");
  size_t cursor=84,pixels=size_t(width)*height;
  for(unsigned skin=0;skin<skins;skin++){
    unsigned type=word(cursor);cursor+=4;
    if(type==0){require(cursor,pixels);cursor+=pixels;}
    else if(type==1){unsigned count=word(cursor);cursor+=4;if(!count||count>256)throw std::runtime_error("Invalid MDL skin group.");size_t size=size_t(count)*(4+pixels);require(cursor,size);cursor+=size;}
    else throw std::runtime_error("Invalid MDL skin type.");
  }
  require(cursor,size_t(vertices)*12+size_t(triangles)*16);cursor+=size_t(vertices)*12+size_t(triangles)*16;
  struct Frame{size_t bounds,points;};struct Group{size_t bounds,first,count;};std::vector<Frame> poses;std::vector<Group> groups;
  auto pose=[&](){require(cursor,24+size_t(vertices)*4);poses.push_back({cursor,cursor+24});cursor+=24+size_t(vertices)*4;if(poses.size()*vertices>4000000)throw std::runtime_error("MDL animation exceeds editor transform limits.");};
  for(unsigned frame=0;frame<frames;frame++){
    unsigned type=word(cursor);cursor+=4;
    if(type==0)pose();
    else if(type==1){unsigned count=word(cursor);cursor+=4;if(!count||count>4096)throw std::runtime_error("Invalid MDL frame group.");size_t bounds=cursor;require(cursor,8+size_t(count)*4);cursor+=8+size_t(count)*4;groups.push_back({bounds,poses.size(),count});for(unsigned i=0;i<count;i++)pose();}
    else throw std::runtime_error("Invalid MDL frame type.");
  }
  V sourceScale{real(8),real(12),real(16)},sourceOrigin{real(20),real(24),real(28)},low{1e30f,1e30f,1e30f},high{-1e30f,-1e30f,-1e30f};std::vector<V> points;points.reserve(poses.size()*vertices);
  for(auto f:poses)for(unsigned i=0;i<vertices;i++){V p{};for(int a=0;a<3;a++)p[a]=sourceOrigin[a]+sourceScale[a]*(unsigned char)bytes[f.points+i*4+a];p=matrix.point(p);for(int a=0;a<3;a++){if(!std::isfinite(p[a])||std::abs(p[a])>1e7f)throw std::runtime_error("Transformed MDL coordinates exceed limits.");low[a]=std::min(low[a],p[a]);high[a]=std::max(high[a],p[a]);}points.push_back(p);}
  V scale{};for(int a=0;a<3;a++){scale[a]=high[a]>low[a]?(high[a]-low[a])/255.f:1.f;put(8+a*4,scale[a]);put(20+a*4,low[a]);}
  V eye{real(36),real(40),real(44)};eye=matrix.point(eye);for(int a=0;a<3;a++)put(36+a*4,eye[a]);
  float radius=0;for(auto p:points){float d=0;for(float x:p)d+=x*x;radius=std::max(radius,std::sqrt(d));}put(32,radius);
  static const float normals[][3]={
#include "third_party/quake_anorms.h"
  };
  auto inv=inverse(matrix);std::array<unsigned char,162> normalMap{};
  for(unsigned n=0;n<162;n++){V v{};for(int a=0;a<3;a++)for(int b=0;b<3;b++)v[a]+=inv.m[b][a]*normals[n][b];float best=-1e30f;for(unsigned k=0;k<162;k++){float d=0;for(int a=0;a<3;a++)d+=v[a]*normals[k][a];if(d>best){best=d;normalMap[n]=k;}}}
  size_t point=0;
  for(auto f:poses){unsigned lo[3]={255,255,255},hi[3]={0,0,0};for(unsigned i=0;i<vertices;i++,point++){
      for(int a=0;a<3;a++){unsigned q=std::clamp(int(std::round((points[point][a]-low[a])/scale[a])),0,255);bytes[f.points+i*4+a]=char(q);lo[a]=std::min(lo[a],q);hi[a]=std::max(hi[a],q);}
      unsigned normal=(unsigned char)bytes[f.points+i*4+3];if(normal>=162)throw std::runtime_error("Invalid MDL vertex normal.");bytes[f.points+i*4+3]=char(normalMap[normal]);
    }for(int a=0;a<3;a++){bytes[f.bounds+a]=char(lo[a]);bytes[f.bounds+4+a]=char(hi[a]);}}
  for(auto g:groups)for(int a=0;a<3;a++){unsigned lo=255,hi=0;for(size_t n=g.first;n<g.first+g.count;n++){lo=std::min(lo,unsigned((unsigned char)bytes[poses[n].bounds+a]));hi=std::max(hi,unsigned((unsigned char)bytes[poses[n].bounds+4+a]));}bytes[g.bounds+a]=char(lo);bytes[g.bounds+4+a]=char(hi);}
  return bytes;
}
}
