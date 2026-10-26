// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_model_export.h"
namespace ftebroom::modelEdit {
using V=std::array<float,3>;
struct Matrix{
  float m[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
  V point(V p,bool direction=false)const{V r{};for(int i=0;i<3;i++){r[i]=direction?0:m[i][3];for(int j=0;j<3;j++)r[i]+=m[i][j]*p[j];}return r;}
};
inline Matrix multiply(const Matrix& a,const Matrix& b){Matrix r;for(int i=0;i<3;i++)for(int j=0;j<4;j++){r.m[i][j]=j==3?a.m[i][3]:0;for(int k=0;k<3;k++)r.m[i][j]+=a.m[i][k]*b.m[k][j];}return r;}
inline Matrix inverse(const Matrix& a){
  Matrix r;double d=double(a.m[0][0])*(double(a.m[1][1])*a.m[2][2]-double(a.m[1][2])*a.m[2][1])-double(a.m[0][1])*(double(a.m[1][0])*a.m[2][2]-double(a.m[1][2])*a.m[2][0])+double(a.m[0][2])*(double(a.m[1][0])*a.m[2][1]-double(a.m[1][1])*a.m[2][0]);
  if(!std::isfinite(d)||std::abs(d)<1e-36)throw std::runtime_error("Transform is singular. Use a nonzero scale.");
  for(int i=0;i<3;i++)for(int j=0;j<3;j++)r.m[i][j]=(double(a.m[(j+1)%3][(i+1)%3])*a.m[(j+2)%3][(i+2)%3]-double(a.m[(j+1)%3][(i+2)%3])*a.m[(j+2)%3][(i+1)%3])/d;
  for(int i=0;i<3;i++){r.m[i][3]=0;for(int j=0;j<3;j++)r.m[i][3]-=r.m[i][j]*a.m[j][3];}return r;
}
struct Node{std::string name;int parent=-1;V position{},rotation{},scale{1,1,1},pivot{},low{},high{};float total=1;};
inline Matrix local(const Node& n){
  Matrix r;for(int axis=0;axis<3;axis++){Matrix q;float a=std::remainder(n.rotation[axis],360.f)*float(3.141592653589793/180),c=std::cos(a),s=std::sin(a);int i=(axis+1)%3,j=(axis+2)%3;q.m[i][i]=q.m[j][j]=c;q.m[i][j]=-s;q.m[j][i]=s;r=multiply(q,r);}
  for(int i=0;i<3;i++){for(int j=0;j<3;j++)r.m[i][j]*=n.scale[j]*n.total;r.m[i][3]=n.position[i]+n.pivot[i];for(int j=0;j<3;j++)r.m[i][3]-=r.m[i][j]*n.pivot[j];}return r;
}
inline Matrix world(const std::vector<Node>& nodes,int index,int depth=0){if(index<0)return {};if(index>=int(nodes.size())||depth>int(nodes.size()))throw std::runtime_error("Invalid mesh hierarchy.");auto r=local(nodes[index]);return nodes[index].parent<0?r:multiply(world(nodes,nodes[index].parent,depth+1),r);}
inline bool descendant(const std::vector<Node>& nodes,int child,int parent){for(size_t n=0;n<=nodes.size()&&child>=0;n++){if(child==parent)return true;child=nodes.at(child).parent;}return false;}
inline void bake(modelExport::Mesh& mesh,const std::vector<Matrix>& transforms){
  if(mesh.surfaces.size()!=transforms.size())throw std::runtime_error("The model surface layout changed. Reopen it before exporting.");
  for(size_t s=0;s<mesh.surfaces.size();s++){const auto& m=transforms[s];auto inv=inverse(m);for(auto& frame:mesh.surfaces[s].frames)for(auto& v:frame){v.position=m.point(v.position);V normal{};float length=0;for(int i=0;i<3;i++){for(int j=0;j<3;j++)normal[i]+=inv.m[j][i]*v.normal[j];length+=normal[i]*normal[i];}length=std::sqrt(length);if(length>0)for(float& x:normal)x/=length;v.normal=normal;for(float x:v.position)if(!std::isfinite(x)||std::abs(x)>1e7f)throw std::runtime_error("Transformed model coordinates exceed export limits.");}}
}
}
