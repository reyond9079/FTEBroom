// SPDX-License-Identifier: GPL-2.0-or-later
// Bounded CPU thumbnails. The viewport and animated editor use FTE's loaders.
#pragma once
#include "ftebroom_md3.h"
namespace ftebroom::legacyModels {
struct Part {std::string image;quake1::Texture embedded;std::vector<std::array<MdlVertex,3>> triangles;};
inline uint16_t half(const std::string& b,size_t at){quake1::range(b,at,2);return uint16_t((unsigned char)b[at])|uint16_t((unsigned char)b[at+1])<<8;}
inline float real(const std::string& b,size_t at){auto u=quake1::word(b,at);float v;std::memcpy(&v,&u,4);if(!std::isfinite(v))throw std::runtime_error("Invalid model coordinate.");return v;}
inline std::vector<Part> md2(const std::string& b,unsigned skin=0,unsigned frame=0){
 using namespace quake1;range(b,0,68);if(b.substr(0,4)!="IDP2"||word(b,4)!=8)throw std::runtime_error("Expected MD2 version 8.");
 auto w=word(b,8),h=word(b,12),stride=word(b,16),ns=word(b,20),nv=word(b,24),nt=word(b,28),nf=word(b,32),frames=word(b,40);
 if(!w||!h||!frames||nv>65536||nt>65536||nf>65536||ns>256||stride<40+uint64_t(nv)*4)throw std::runtime_error("Invalid MD2 limits.");
 auto st=word(b,48),tris=word(b,52);auto pose=word(b,56)+uint64_t(std::min(frame,frames-1))*stride;range(b,st,uint64_t(nt)*4);range(b,tris,uint64_t(nf)*12);range(b,pose,stride);
 Part p;if(ns)p.image=name(b,word(b,44)+std::min(skin,ns-1)*64,64);
 for(unsigned t=0;t<nf;t++){std::array<MdlVertex,3> tri;for(int k=0;k<3;k++){auto v=half(b,tris+t*12+k*2),uv=half(b,tris+t*12+6+k*2);if(v>=nv||uv>=nt)throw std::runtime_error("Invalid MD2 triangle.");
   for(int a=0;a<3;a++)tri[k].point[a]=(unsigned char)b[pose+40+v*4+a]*real(b,pose+a*4)+real(b,pose+12+a*4);
   tri[k].u=float(int16_t(half(b,st+uv*4)))/w;tri[k].v=float(int16_t(half(b,st+uv*4+2)))/h;
 }p.triangles.push_back(tri);}return {std::move(p)};
}
inline std::vector<Part> studio(const std::string& b,const std::string& companion,unsigned skin=0){
 using namespace quake1;range(b,0,244);if(b.substr(0,4)!="IDST"||word(b,4)!=10)throw std::runtime_error("Expected GoldSrc studio MDL v10.");
 auto nb=word(b,140),bo=word(b,144),bodies=word(b,204),bodyAt=word(b,208);if(nb>256||bodies>128)throw std::runtime_error("Studio model limits exceeded.");range(b,bo,uint64_t(nb)*112);range(b,bodyAt,uint64_t(bodies)*76);
 using Matrix=std::array<std::array<float,4>,3>;std::vector<Matrix> bones;
 for(unsigned i=0;i<nb;i++){auto at=bo+i*112;int parent=int32_t(word(b,at+32));if(parent>=int(i)||parent< -1)throw std::runtime_error("Invalid studio bone parent.");
   float x=real(b,at+76),y=real(b,at+80),z=real(b,at+84),sx=std::sin(x),cx=std::cos(x),sy=std::sin(y),cy=std::cos(y),sz=std::sin(z),cz=std::cos(z);
   Matrix m{{{{cy*cz,sx*sy*cz-cx*sz,cx*sy*cz+sx*sz,real(b,at+64)}},{{cy*sz,sx*sy*sz+cx*cz,cx*sy*sz-sx*cz,real(b,at+68)}},{{-sy,sx*cy,cx*cy,real(b,at+72)}}}};
   if(parent>=0){auto local=m;auto& p=bones[parent];for(int a=0;a<3;a++)for(int c=0;c<4;c++){m[a][c]=c==3?p[a][3]:0;for(int k=0;k<3;k++)m[a][c]+=p[a][k]*local[k][c];}}bones.push_back(m);
 }
 const auto& tex=word(b,180)?b:companion;range(tex,0,244);if(tex.substr(0,4)!="IDST"||word(tex,4)!=10)throw std::runtime_error("Missing studio texture companion MDL.");
 auto textures=word(tex,180),textureAt=word(tex,184),refs=word(tex,192),families=word(tex,196),skinAt=word(tex,200);if(textures>256||refs>256||families>256)throw std::runtime_error("Invalid studio skin counts.");range(tex,textureAt,uint64_t(textures)*80);range(tex,skinAt,uint64_t(refs)*families*2);
 std::vector<Part> result;size_t total=0;uint64_t decodedPixels=0;
 for(unsigned body=0;body<bodies;body++){auto model=word(b,bodyAt+body*76+72);range(b,model,112);auto meshes=word(b,model+72),meshAt=word(b,model+76),verts=word(b,model+80),boneAt=word(b,model+84),vertexAt=word(b,model+88);if(meshes>256||verts>65536)throw std::runtime_error("Studio mesh limits exceeded.");range(b,meshAt,uint64_t(meshes)*20);range(b,boneAt,verts);range(b,vertexAt,uint64_t(verts)*12);
   for(unsigned mesh=0;mesh<meshes;mesh++){auto at=meshAt+mesh*20,ref=word(b,at+8);if(ref>=refs||!families)throw std::runtime_error("Invalid studio material reference.");auto index=half(tex,skinAt+(std::min(skin,families-1)*refs+ref)*2);if(index>=textures)throw std::runtime_error("Invalid studio skin.");auto t=textureAt+index*80,w=word(tex,t+68),h=word(tex,t+72),pixels=word(tex,t+76);if(!w||!h||uint64_t(w)*h>16777216)throw std::runtime_error("Invalid studio texture size.");range(tex,pixels,uint64_t(w)*h+768);
     decodedPixels+=uint64_t(w)*h;if(decodedPixels>33554432||result.size()>=256)throw std::runtime_error("Studio thumbnail image budget exceeded.");
     Part p;p.embedded={name(tex,t,64),w,h,{},tex.substr(pixels+size_t(w)*h,768)};if(word(tex,t+64)&64)p.embedded.name="{"+p.embedded.name;p.embedded.pixels.assign(tex.begin()+pixels,tex.begin()+pixels+size_t(w)*h);
     size_t cursor=word(b,at+4);for(;;){int count=int16_t(half(b,cursor));cursor+=2;if(!count)break;bool fan=count<0;unsigned n=unsigned(fan?-count:count);if(n<3||n>65536)throw std::runtime_error("Invalid studio triangle strip.");range(b,cursor,uint64_t(n)*8);std::vector<MdlVertex> strip;
       for(unsigned v=0;v<n;v++){auto id=half(b,cursor+v*8);if(id>=verts)throw std::runtime_error("Invalid studio vertex.");unsigned bone=(unsigned char)b[boneAt+id];if(bone>=bones.size())throw std::runtime_error("Invalid studio vertex bone.");MdlVertex vertex;for(int a=0;a<3;a++){vertex.point[a]=bones[bone][a][3];for(int k=0;k<3;k++)vertex.point[a]+=bones[bone][a][k]*real(b,vertexAt+id*12+k*4);}vertex.u=float(int16_t(half(b,cursor+v*8+4)))/w;vertex.v=float(int16_t(half(b,cursor+v*8+6)))/h;strip.push_back(vertex);}
       cursor+=n*8;for(unsigned v=2;v<n;v++){if(++total>65536)throw std::runtime_error("Studio thumbnail exceeds 65536 triangles.");p.triangles.push_back(fan?std::array<MdlVertex,3>{strip[0],strip[v-1],strip[v]}:v&1?std::array<MdlVertex,3>{strip[v-1],strip[v-2],strip[v]}:std::array<MdlVertex,3>{strip[v-2],strip[v-1],strip[v]});}
     }result.push_back(std::move(p));
   }
 }return result;
}
}
