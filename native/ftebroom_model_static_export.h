// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_model_export.h"
namespace ftebroom::modelExport {
// IQM and MD5 export a static, full-precision pose. Vertex animation is never
// silently represented as a skeletal animation with unrelated motion.
inline std::string iqm(const Mesh& mesh){
  if(!mesh.frames||mesh.surfaces.empty())throw std::runtime_error("Empty model.");
  Binary text,meshes,triangles,positions,normals,uv;unsigned vertex=0,triangle=0;
  auto string=[&](const std::string& s){unsigned offset=text.bytes.size();text.bytes+=s;text.bytes+='\0';return offset;};
  for(size_t i=0;i<mesh.surfaces.size();i++){const auto& s=mesh.surfaces[i];for(unsigned value:{string("surface"+std::to_string(i)),string(s.material),vertex,unsigned(s.frames[0].size()),triangle,unsigned(s.triangles.size())})meshes.u32(value);
    for(const auto& t:s.triangles)for(auto v:t)triangles.u32(vertex+v);
    for(const auto& v:s.frames[0]){for(float x:v.position)positions.real(x);for(float x:v.normal)normals.real(x);for(float x:v.uv)uv.real(x);}vertex+=s.frames[0].size();triangle+=s.triangles.size();
  }
  while(text.bytes.size()%4)text.bytes+='\0';
  unsigned ot=124,om=ot+text.bytes.size(),ova=om+meshes.bytes.size(),otr=ova+60,op=otr+triangles.bytes.size(),on=op+positions.bytes.size(),ouv=on+normals.bytes.size(),end=ouv+uv.bytes.size();
  Binary out;out.bytes=std::string("INTERQUAKEMODEL\0",16);
  for(unsigned n:{2u,end,0u,unsigned(text.bytes.size()),ot,unsigned(mesh.surfaces.size()),om,3u,vertex,ova,triangle,otr,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u})out.u32(n);
  out.bytes+=text.bytes;out.bytes+=meshes.bytes;
  for(auto row:std::vector<std::array<unsigned,5>>{{0,0,7,3,op},{2,0,7,3,on},{1,0,7,2,ouv}})for(auto n:row)out.u32(n);
  out.bytes+=triangles.bytes;out.bytes+=positions.bytes;out.bytes+=normals.bytes;out.bytes+=uv.bytes;return out.bytes;
}
inline std::string md5mesh(const Mesh& mesh){
  std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(9)<<"MD5Version 10\ncommandline \"ReyondBroom static pose\"\nnumJoints 1\nnumMeshes "<<mesh.surfaces.size()<<"\njoints {\n\"origin\" -1 ( 0 0 0 ) ( 0 0 0 )\n}\n";
  for(const auto& s:mesh.surfaces){out<<"mesh {\nshader "<<std::quoted(s.material)<<"\nnumverts "<<s.frames[0].size()<<'\n';
    for(size_t i=0;i<s.frames[0].size();i++){auto& v=s.frames[0][i];out<<"vert "<<i<<" ( "<<v.uv[0]<<' '<<v.uv[1]<<" ) "<<i<<" 1\n";}
    out<<"numtris "<<s.triangles.size()<<'\n';for(size_t i=0;i<s.triangles.size();i++){auto& t=s.triangles[i];out<<"tri "<<i<<' '<<t[0]<<' '<<t[1]<<' '<<t[2]<<'\n';}
    out<<"numweights "<<s.frames[0].size()<<'\n';for(size_t i=0;i<s.frames[0].size();i++){auto& p=s.frames[0][i].position;out<<"weight "<<i<<" 0 1 ( "<<p[0]<<' '<<p[1]<<' '<<p[2]<<" )\n";}out<<"}\n";
  }return out.str();
}
}
