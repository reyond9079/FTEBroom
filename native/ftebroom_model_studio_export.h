// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_model_edit_math.h"
#include "ftebroom_quake1.h"
#include "ftebroom_model_mdl_export.h"
#include <functional>
#include <map>
namespace ftebroom::studioExport {
using modelEdit::Matrix;using modelEdit::V;
struct Data {
  std::string b;
  unsigned u(size_t p)const{return quake1::word(b,p);}
  float f(size_t p)const{auto n=u(p);float v;std::memcpy(&v,&n,4);if(!std::isfinite(v))throw std::runtime_error("Non-finite studio model value.");return v;}
  void u(size_t p,unsigned n){quake1::range(b,p,4);for(int i=0;i<4;i++)b[p+i]=char(n>>(8*i));}
  void f(size_t p,float v){if(!std::isfinite(v))throw std::runtime_error("Non-finite studio export value.");unsigned n;std::memcpy(&n,&v,4);u(p,n);}
  size_t add(size_t n){if(n>64*1024*1024||b.size()+n>128*1024*1024)throw std::runtime_error("Studio export exceeds 128 MiB.");while(b.size()%4)b+='\0';size_t p=b.size();b.append(n,0);return p;}
  void name(size_t p,size_t n,const std::string& s){quake1::range(b,p,n);if(s.size()>=n)throw std::runtime_error("Studio name is too long.");std::memcpy(&b[p],s.data(),s.size());}
  V v(size_t p)const{return {f(p),f(p+4),f(p+8)};}
  void v(size_t p,V a){for(int i=0;i<3;i++)f(p+4*i,a[i]);}
};
inline Matrix rotation(V a){modelEdit::Node n;for(int i=0;i<3;i++)n.rotation[i]=a[i]*57.29577951308232f;return modelEdit::local(n);}
inline V angles(const Matrix& m){float y=std::asin(std::clamp(-m.m[2][0],-1.f,1.f));if(std::abs(std::cos(y))>1e-5f)return {std::atan2(m.m[2][1],m.m[2][2]),y,std::atan2(m.m[1][0],m.m[0][0])};return {0,y,std::atan2(-m.m[0][1],m.m[1][1])};}
inline int value(const std::string& b,size_t anim,unsigned channel,unsigned frame){
  quake1::range(b,anim,12);unsigned off=(unsigned char)b[anim+channel*2]|((unsigned char)b[anim+channel*2+1]<<8);if(!off)return 0;
  size_t p=anim+off;for(unsigned guard=0;guard<65536;guard++){
    quake1::range(b,p,2);unsigned valid=(unsigned char)b[p],total=(unsigned char)b[p+1];if(!valid||valid>total)throw std::runtime_error("Invalid studio animation run.");quake1::range(b,p,2*(valid+1));
    if(frame<total){unsigned k=std::min(frame+1,valid);return int16_t((unsigned char)b[p+k*2]|((unsigned char)b[p+k*2+1]<<8));}frame-=total;p+=2*(valid+1);
  }throw std::runtime_error("Studio animation is too long.");
}
inline void channel(Data& d,size_t anim,unsigned c,const std::vector<float>& values,float base,float step){
  std::vector<int> q;bool zero=true;for(float v:values){int n=std::lround((v-base)/step);if(n< -32768||n>32767)throw std::runtime_error("Studio animation channel overflow.");q.push_back(n);zero&=n==0;}if(zero)return;
  size_t p=d.b.size(),off=p-anim;if(off>65535)throw std::runtime_error("Studio animation channel offset exceeds 16 bits. Split the animation into shorter sequences.");
  d.b[anim+c*2]=char(off);d.b[anim+c*2+1]=char(off>>8);
  auto put=[&](int n){d.b+=char(n);d.b+=char(n>>8);};
  for(size_t i=0;i<q.size();){unsigned repeat=1;while(repeat<255&&i+repeat<q.size()&&q[i+repeat]==q[i])repeat++;
    if(repeat>1){d.b+=char(1);d.b+=char(repeat);put(q[i]);i+=repeat;}
    else {unsigned n=1;while(n<255&&i+n<q.size()&&(i+n+1==q.size()||q[i+n]!=q[i+n+1]))n++;d.b+=char(n);d.b+=char(n);for(unsigned j=0;j<n;j++)put(q[i++]);}
  }
}
inline void bounds(Data& d,size_t lo,size_t hi,const Matrix& m){auto l=d.v(lo),h=d.v(hi);V low{1e30f,1e30f,1e30f},high{-1e30f,-1e30f,-1e30f};for(int k=0;k<8;k++){V p;for(int a=0;a<3;a++)p[a]=k&(1<<a)?h[a]:l[a];p=m.point(p);for(int a=0;a<3;a++){low[a]=std::min(low[a],p[a]);high[a]=std::max(high[a],p[a]);}}d.v(lo,low);d.v(hi,high);}
inline std::string bake(const modelExport::Mesh& mesh,const std::vector<modelExport::Skin>& skins,float fps){
  using namespace modelExport;
  if(!mesh.frames||mesh.frames>512||mesh.surfaces.empty()||mesh.surfaces.size()!=skins.size()||skins.size()>32)throw std::runtime_error("GoldSrc bake requires 1-512 frames and 1-32 textured surfaces.");
  size_t nv=0,nt=0;for(const auto& s:mesh.surfaces){nv+=s.frames.at(0).size();nt+=s.triangles.size();if(s.frames.size()!=mesh.frames)throw std::runtime_error("Missing animation poses.");}
  if(!nv||nv>2048||nt>20000)throw std::runtime_error("Native GoldSrc bake limit: 2048 vertices and 20000 triangles.");
  // Vertex animation cannot be represented by one rigid GoldSrc root bone.
  // Preserve it exactly as independent motion tracks within the 128-bone limit.
  unsigned bones=mesh.frames>1?nv:1;if(bones>128)throw std::runtime_error("This vertex animation needs more than GoldSrc's 128 rigid bones. Export an existing skeletal studio MDL, reduce the mesh explicitly, or use MD2/MD3. No animation was silently discarded.");
  Data d{std::string(244,0)};d.b.replace(0,4,"IDST");d.u(4,10);d.name(8,64,"ReyondBroom native studio export");
  auto boneAt=d.add(bones*112);d.u(140,bones);d.u(144,boneAt);
  V low{1e30f,1e30f,1e30f},high{-1e30f,-1e30f,-1e30f};std::vector<const Surface*> surfaces;std::vector<unsigned> local;
  for(auto& s:mesh.surfaces)for(unsigned v=0;v<s.frames[0].size();v++){surfaces.push_back(&s);local.push_back(v);for(auto& frame:s.frames)for(int a=0;a<3;a++){low[a]=std::min(low[a],frame[v].position[a]);high[a]=std::max(high[a],frame[v].position[a]);}}
  for(unsigned b=0;b<bones;b++){size_t at=boneAt+b*112;if(mesh.frames>1){const auto& v=surfaces[b]->frames[0][local[b]];d.v(at+64,v.position);float len=std::sqrt(v.normal[0]*v.normal[0]+v.normal[1]*v.normal[1]+v.normal[2]*v.normal[2]);if(len>1e-6f)d.v(at+76,{0,std::acos(std::clamp(v.normal[2]/len,-1.f,1.f)),std::atan2(v.normal[1],v.normal[0])});}d.name(at,32,"track"+std::to_string(b));d.u(at+32,~0u);for(int c=0;c<6;c++)d.u(at+40+c*4,~0u);for(int c=0;c<6;c++)d.f(at+88+c*4,c<3?std::max(1e-5f,(high[c]-low[c]+std::max(std::abs(low[c]),std::abs(high[c])))/32760.f):float(6.283185307179586/32767));}
  size_t seq=d.add(176);d.u(164,1);d.u(168,seq);d.name(seq,32,"baked");d.f(seq+32,fps);d.u(seq+36,1);d.u(seq+56,mesh.frames);d.u(seq+120,1);d.v(seq+96,low);d.v(seq+108,high);size_t group=d.add(104);d.u(172,1);d.u(176,group);
  size_t anim=d.add(bones*12);d.u(seq+124,anim);
  for(unsigned b=0;b<bones;b++){std::array<std::vector<float>,6> channels;for(unsigned f=0;f<mesh.frames;f++){V p{},a{};if(mesh.frames>1){const auto& v=surfaces[b]->frames[f][local[b]];p=v.position;float len=std::sqrt(v.normal[0]*v.normal[0]+v.normal[1]*v.normal[1]+v.normal[2]*v.normal[2]);if(len>1e-6f){a[1]=std::acos(std::clamp(v.normal[2]/len,-1.f,1.f));a[2]=std::atan2(v.normal[1],v.normal[0]);}}for(int c=0;c<6;c++)channels[c].push_back(c<3?p[c]:a[c-3]);}for(int c=0;c<6;c++)channel(d,anim+b*12,c,channels[c],d.f(boneAt+b*112+64+c*4),d.f(boneAt+b*112+88+c*4));}
  auto texAt=d.add(skins.size()*80);d.u(180,skins.size());d.u(184,texAt);
  for(unsigned i=0;i<skins.size();i++){
    auto& skin=skins[i];if(!skin.width||!skin.height||skin.width>512||skin.height>512||skin.width%4||skin.height%4||skin.rgba.size()!=size_t(skin.width)*skin.height*4)throw std::runtime_error("GoldSrc textures must be at most 512x512 with dimensions divisible by four. Resize the source explicitly; automatic size changes are disabled.");
    size_t p=d.add(size_t(skin.width)*skin.height+768);d.name(texAt+i*80,64,"surface"+std::to_string(i));d.u(texAt+i*80+68,skin.width);d.u(texAt+i*80+72,skin.height);d.u(texAt+i*80+76,p);
    // A per-texture palette retains exact colours when there are <=256 unique
    // RGB values; otherwise deterministic RGB332 quantization is explicit.
    std::map<unsigned,unsigned> colours;for(size_t v=0;v<skin.rgba.size();v+=4){unsigned rgb=(skin.rgba[v]<<16)|(skin.rgba[v+1]<<8)|skin.rgba[v+2];if(colours.size()<257)colours.emplace(rgb,colours.size());}
    bool exact=colours.size()<=256;size_t palette=p+size_t(skin.width)*skin.height;
    if(exact){for(auto pair:colours)for(int a=0;a<3;a++)d.b[palette+pair.second*3+a]=char(pair.first>>(16-8*a));}
    else for(unsigned c=0;c<256;c++){d.b[palette+c*3]=char(((c>>5)&7)*255/7);d.b[palette+c*3+1]=char(((c>>2)&7)*255/7);d.b[palette+c*3+2]=char((c&3)*255/3);}
    for(size_t v=0;v<size_t(skin.width)*skin.height;v++){auto* rgba=&skin.rgba[v*4];unsigned rgb=(rgba[0]<<16)|(rgba[1]<<8)|rgba[2];d.b[p+v]=char(exact?colours.at(rgb):((rgba[0]>>5)<<5)|((rgba[1]>>5)<<2)|(rgba[2]>>6));}
  }
  auto refs=d.add(skins.size()*2);d.u(192,skins.size());d.u(196,1);d.u(200,refs);for(unsigned i=0;i<skins.size();i++){d.b[refs+i*2]=char(i);d.b[refs+i*2+1]=char(i>>8);}
  auto body=d.add(76),model=d.add(112),parts=d.add(mesh.surfaces.size()*20);d.u(204,1);d.u(208,body);d.name(body,64,"body");d.u(body+64,1);d.u(body+68,1);d.u(body+72,model);d.name(model,64,"model");d.u(model+72,mesh.surfaces.size());d.u(model+76,parts);d.u(model+80,nv);
  auto vb=d.add(nv),vp=d.add(nv*12),nb=d.add(nv),np=d.add(nv*12);d.u(model+84,vb);d.u(model+88,vp);d.u(model+92,nv);d.u(model+96,nb);d.u(model+100,np);
  for(unsigned v=0;v<nv;v++){d.b[vb+v]=d.b[nb+v]=char(mesh.frames>1?v:0);d.v(vp+v*12,mesh.frames>1?V{}:surfaces[v]->frames[0][local[v]].position);d.v(np+v*12,mesh.frames>1?V{0,0,1}:surfaces[v]->frames[0][local[v]].normal);}
  unsigned base=0;for(unsigned i=0;i<mesh.surfaces.size();i++){const auto& s=mesh.surfaces[i];auto cmd=d.add(s.triangles.size()*26+2);d.u(parts+i*20,s.triangles.size());d.u(parts+i*20+4,cmd);d.u(parts+i*20+8,i);d.u(parts+i*20+12,s.frames[0].size());d.u(parts+i*20+16,base);
    size_t at=cmd;for(auto tri:s.triangles){d.b[at]=3;at+=2;for(auto v:tri){if(v>=s.frames[0].size())throw std::runtime_error("Invalid studio triangle.");auto& vertex=s.frames[0][v];int uv[2]={int(std::lround(vertex.uv[0]*skins[i].width)),int(std::lround(vertex.uv[1]*skins[i].height))};for(int a=0;a<2;a++)if(uv[a]<-32768||uv[a]>32767)throw std::runtime_error("Studio UV exceeds signed 16-bit range.");int values[4]={int(base+v),int(base+v),uv[0],uv[1]};for(int value:values){d.b[at++]=char(value);d.b[at++]=char(value>>8);}}}base+=s.frames[0].size();}
  d.v(88,low);d.v(100,high);d.v(112,low);d.v(124,high);d.u(72,d.b.size());return d.b;
}
// Preserve native sequences, blending, activities, events, bodygroups, skins and
// the rig. Uniform model transforms are representable by GoldSrc rigid bones.
// Nonuniform scaling of animated rigid bones is not generally representable.
inline std::string transform(std::string source,const Matrix& matrix,const std::function<std::string(const std::string&)>& companion){
  Data original{source},out{std::move(source)};quake1::range(original.b,0,244);
  if(original.b.substr(0,4)!="IDST"||original.u(4)!=10)throw std::runtime_error("Expected GoldSrc MDL version 10.");
  float scale=0;for(int i=0;i<3;i++)scale+=matrix.m[i][0]*matrix.m[i][0];scale=std::sqrt(scale);
  if(scale<1e-6f)throw std::runtime_error("Studio model scale must be positive.");
  Matrix r=matrix;for(int i=0;i<3;i++)for(int j=0;j<3;j++)r.m[i][j]/=scale;
  for(int i=0;i<3;i++)for(int j=0;j<3;j++){float dot=0;for(int k=0;k<3;k++)dot+=r.m[k][i]*r.m[k][j];if(std::abs(dot-(i==j?1.f:0.f))>1e-4f)throw std::runtime_error("Native animated GoldSrc export requires uniform model scale. Use Total scale; nonuniform animated bones would introduce shear.");}
  float determinant=r.m[0][0]*(r.m[1][1]*r.m[2][2]-r.m[1][2]*r.m[2][1])-r.m[0][1]*(r.m[1][0]*r.m[2][2]-r.m[1][2]*r.m[2][0])+r.m[0][2]*(r.m[1][0]*r.m[2][1]-r.m[1][1]*r.m[2][0]);if(determinant<0)throw std::runtime_error("Mirrored studio skeletons are not supported.");
  for(int i=0;i<3;i++)r.m[i][3]=0;
  unsigned nb=original.u(140),bo=original.u(144),ns=original.u(164),so=original.u(168),ng=original.u(172),go=original.u(176);
  if(!nb||nb>128||!ns||ns>2048||!ng||ng>256)throw std::runtime_error("Studio rig exceeds native format limits.");quake1::range(original.b,bo,size_t(nb)*112);quake1::range(original.b,so,size_t(ns)*176);quake1::range(original.b,go,size_t(ng)*104);
  bool wrap=false;for(int i=0;i<3;i++){wrap|=std::abs(matrix.m[i][3])>1e-7f;for(int j=0;j<3;j++)wrap|=std::abs(r.m[i][j]-(i==j?1.f:0.f))>1e-7f;}
  unsigned prefix=wrap?1:0,totalBones=nb+prefix;if(totalBones>128)throw std::runtime_error("The model already uses all 128 GoldSrc bones. A rotated/translated rig needs one additional transform root.");
  size_t newBones=out.add(totalBones*112);out.u(140,totalBones);out.u(144,newBones);
  if(wrap){out.name(newBones,32,"ReyondBroom_transform");out.u(newBones+32,~0u);for(int c=0;c<6;c++)out.u(newBones+40+c*4,~0u);out.v(newBones+64,{matrix.m[0][3],matrix.m[1][3],matrix.m[2][3]});out.v(newBones+76,angles(r));}
  for(unsigned b=0;b<nb;b++){
    size_t from=bo+b*112,to=newBones+(b+prefix)*112;out.b.replace(to,112,original.b,from,112);int parent=int32_t(original.u(from+32));if(parent< -1||parent>=int(b))throw std::runtime_error("Invalid studio bone hierarchy.");out.u(to+32,parent<0?(wrap?0:~0u):parent+prefix);
    for(int c=0;c<3;c++){out.f(to+64+c*4,original.f(from+64+c*4)*scale);out.f(to+88+c*4,original.f(from+88+c*4)*scale);}
  }
  // A constant parent retains the original root-motion axes and controllers.
  // Rotating Euler channels themselves would change motion stripping/blending.
  for(unsigned s=0;s<ns;s++){
    size_t seq=so+s*176;unsigned frames=original.u(seq+56),blends=original.u(seq+120),group=original.u(seq+156);if(!frames||frames>4096||!blends||blends>4||group>=ng)throw std::runtime_error("Invalid studio animation sequence.");
    std::string external;const std::string* input=&original.b;size_t animation=original.u(seq+124);
    if(group){auto groupName=quake1::name(original.b,go+group*104+32,64);std::replace(groupName.begin(),groupName.end(),'\\','/');external=companion(groupName);if(external.substr(0,4)!="IDSQ"||quake1::word(external,4)!=10)throw std::runtime_error("Missing studio animation group.");input=&external;}
    else animation+=original.u(go+100);
    quake1::range(*input,animation,size_t(nb)*blends*12);
    size_t newAnimation=out.add(size_t(totalBones)*blends*12);out.u(seq+124,newAnimation);out.u(seq+156,0);out.u(seq+72,original.u(seq+72)+prefix);
    for(unsigned blend=0;blend<blends;blend++)for(unsigned b=0;b<nb;b++)for(int c=0;c<6;c++){
      size_t bone=bo+b*112,anim=animation+(blend*nb+b)*12;float factor=c<3?scale:1,base=original.f(bone+64+c*4)*factor,step=original.f(bone+88+c*4)*factor;
      std::vector<float> values;values.reserve(frames);for(unsigned f=0;f<frames;f++)values.push_back(base+value(*input,anim,c,f)*step);
      channel(out,newAnimation+(blend*totalBones+b+prefix)*12,c,values,base,step==0?1:step);
    }
    bounds(out,seq+96,seq+108,matrix);out.v(seq+76,matrix.point(original.v(seq+76),true));
  }
  out.u(go+100,0);out.u(172,1);
  unsigned controllers=original.u(148),controlAt=original.u(152);if(controllers>256)throw std::runtime_error("Too many bone controllers.");quake1::range(original.b,controlAt,size_t(controllers)*24);
  for(unsigned c=0;c<controllers;c++){size_t at=controlAt+c*24;out.u(at,original.u(at)+prefix);if(original.u(at+4)&7){out.f(at+8,original.f(at+8)*scale);out.f(at+12,original.f(at+12)*scale);}}
  unsigned bodies=original.u(204),bodyAt=original.u(208);if(bodies>128)throw std::runtime_error("Too many studio bodygroups.");quake1::range(original.b,bodyAt,size_t(bodies)*76);
  for(unsigned b=0;b<bodies;b++){unsigned count=original.u(bodyAt+b*76+64),models=original.u(bodyAt+b*76+72);if(count>256)throw std::runtime_error("Too many studio submodels.");quake1::range(original.b,models,size_t(count)*112);for(unsigned i=0;i<count;i++){size_t at=models+i*112;unsigned nv=original.u(at+80),p=original.u(at+88);if(nv>65536)throw std::runtime_error("Too many studio vertices.");quake1::range(original.b,p,size_t(nv)*12);for(unsigned v=0;v<nv;v++){auto point=original.v(p+v*12);for(float& x:point)x*=scale;out.v(p+v*12,point);}out.f(at+68,original.f(at+68)*scale);
    if(wrap){unsigned normals=original.u(at+92);for(auto pair:{std::pair<unsigned,unsigned>{original.u(at+84),nv},{original.u(at+96),normals}}){quake1::range(original.b,pair.first,pair.second);for(unsigned v=0;v<pair.second;v++){unsigned bone=(unsigned char)original.b[pair.first+v];if(bone>=nb)throw std::runtime_error("Invalid studio vertex bone.");out.b[pair.first+v]=char(bone+prefix);}}}
  }}
  unsigned hitboxes=original.u(156),hitAt=original.u(160);if(hitboxes>1024)throw std::runtime_error("Too many studio hitboxes.");quake1::range(original.b,hitAt,size_t(hitboxes)*32);for(unsigned h=0;h<hitboxes;h++){out.u(hitAt+h*32,original.u(hitAt+h*32)+prefix);for(int c=0;c<6;c++)out.f(hitAt+h*32+8+c*4,original.f(hitAt+h*32+8+c*4)*scale);}
  unsigned attachments=original.u(212),attachAt=original.u(216);if(attachments>1024)throw std::runtime_error("Too many studio attachments.");quake1::range(original.b,attachAt,size_t(attachments)*88);for(unsigned a=0;a<attachments;a++){out.u(attachAt+a*88+36,original.u(attachAt+a*88+36)+prefix);for(int c=0;c<3;c++)out.f(attachAt+a*88+40+c*4,original.f(attachAt+a*88+40+c*4)*scale);}
  out.v(76,matrix.point(original.v(76)));bounds(out,88,100,matrix);bounds(out,112,124,matrix);
  // Embed texture companions rather than exporting a model with missing skins.
  if(!original.u(180)){
    Data tex{companion("@textures")};if(tex.b.substr(0,4)!="IDST"||tex.u(4)!=10)throw std::runtime_error("Missing studio texture companion.");unsigned count=tex.u(180),table=tex.u(184);if(!count||count>256)throw std::runtime_error("Invalid studio texture count.");quake1::range(tex.b,table,size_t(count)*80);
    size_t dest=out.add(size_t(count)*80);out.b.replace(dest,size_t(count)*80,tex.b,table,size_t(count)*80);out.u(180,count);out.u(184,dest);
    for(unsigned i=0;i<count;i++){unsigned w=tex.u(table+i*80+68),h=tex.u(table+i*80+72),off=tex.u(table+i*80+76);if(!w||!h||uint64_t(w)*h>16777216)throw std::runtime_error("Invalid studio texture dimensions.");size_t n=size_t(w)*h+768;quake1::range(tex.b,off,n);size_t p=out.add(n);out.b.replace(p,n,tex.b,off,n);out.u(dest+i*80+76,p);}
    unsigned refs=tex.u(192),families=tex.u(196);if(!refs||refs>256||!families||families>256)throw std::runtime_error("Invalid studio skin families.");size_t n=size_t(refs)*families*2;quake1::range(tex.b,tex.u(200),n);auto p=out.add(n);out.b.replace(p,n,tex.b,tex.u(200),n);out.u(192,refs);out.u(196,families);out.u(200,p);
  }
  out.u(72,out.b.size());return out.b;
}
}
