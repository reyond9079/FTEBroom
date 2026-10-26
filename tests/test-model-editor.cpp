// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/ftebroom_model_edit_math.h"
#include "../native/ftebroom_model_static_export.h"
#include "../native/ftebroom_model_mdl_transform.h"
#include "../native/ftebroom_model_mdl_export.h"
#include "../native/ftebroom_mdl.h"
#include "../native/ftebroom_md3.h"
#include <cassert>
#include <iostream>
using namespace ftebroom::modelEdit;
static bool near(float a,float b){return std::abs(a-b)<.001f;}
int main(){
  std::vector<Node> nodes(3);nodes[0].position={10,-20,30};nodes[0].rotation={0,0,90};nodes[0].scale={2,3,4};nodes[1].parent=0;nodes[2].parent=1;
  auto p=world(nodes,2).point({1,2,3});assert(near(p[0],4)&&near(p[1],-18)&&near(p[2],42));
  auto original=inverse(world(nodes,2)).point(p);for(int a=0;a<3;a++)assert(near(original[a],a+1));
  assert(descendant(nodes,2,0)&&descendant(nodes,2,1)&&!descendant(nodes,0,2));
  nodes[1].position={1,0,0};auto moved=world(nodes,2).point({1,2,3});assert(near(moved[1],p[1]+2));
  nodes[1].position={};nodes[1].pivot={5,0,0};nodes[1].total=2;auto pivot=world(nodes,1).point(nodes[1].pivot);auto expected=world(nodes,0).point(nodes[1].pivot);for(int a=0;a<3;a++)assert(near(pivot[a],expected[a]));
  const std::string frame="\"models/texture\" 3 1\n0 0 0 0 0 1 0 0\n16 0 0 0 0 1 1 0\n0 16 0 0 0 1 0 1\n0 1 2\n";
  auto mesh=ftebroom::modelExport::parse("FTEBROOM_MESH 1 2 1\n"+frame+frame);auto before=mesh;
  std::vector<Matrix> transforms{world(nodes,1)};bake(mesh,transforms);assert(mesh.surfaces[0].material==before.surfaces[0].material);
  for(size_t f=0;f<2;f++)for(size_t i=0;i<3;i++){auto point=transforms[0].point(before.surfaces[0].frames[f][i].position);for(int a=0;a<3;a++)assert(near(point[a],mesh.surfaces[0].frames[f][i].position[a]));auto& n=mesh.surfaces[0].frames[f][i].normal;assert(near(n[0]*n[0]+n[1]*n[1]+n[2]*n[2],1));}
  auto md3=ftebroom::modelExport::md3(mesh);auto decoded=ftebroom::readMd3(md3);assert(decoded.size()==1);
  auto iqm=ftebroom::modelExport::iqm(mesh);assert(iqm.substr(0,15)=="INTERQUAKEMODEL");unsigned fileSize=0;std::memcpy(&fileSize,iqm.data()+20,4);assert(fileSize==iqm.size());
  auto md5=ftebroom::modelExport::md5mesh(mesh);assert(md5.find("numJoints 1")!=std::string::npos&&md5.find("models/texture")!=std::string::npos);
  ftebroom::modelExport::Skin skin;skin.width=skin.height=2;skin.rgba.assign(16,255);std::string palette(768,0);
  auto mdl=ftebroom::modelExport::mdl(before,{skin},palette);assert(transformMdl(mdl,{})==mdl);auto transformed=transformMdl(mdl,transforms[0]);
  unsigned w=ftebroom::quake1::word(mdl,52),h=ftebroom::quake1::word(mdl,56);size_t immutable=4+size_t(w)*h+before.surfaces[0].frames[0].size()*12+before.surfaces[0].triangles.size()*16;
  assert(mdl.substr(84,immutable)==transformed.substr(84,immutable));auto oldMdl=ftebroom::readMdl(mdl),newMdl=ftebroom::readMdl(transformed);
  for(int v=0;v<3;v++){auto expected=transforms[0].point(oldMdl.triangles[0][v].point);for(int a=0;a<3;a++)assert(std::abs(expected[a]-newMdl.triangles[0][v].point[a])<.5f);}
  assert(ftebroom::quake1::word(transformed,68)==2&&transformed.size()==mdl.size());
  auto grouped=mdl.substr(0,84+immutable);grouped[68]=1;grouped[69]=grouped[70]=grouped[71]=0;
  ftebroom::modelExport::Binary group;group.u32(1);group.u32(2);group.bytes.append(8,'\0');group.real(.1f);group.real(.2f);size_t frameStart=84+immutable,poseSize=24+before.surfaces[0].frames[0].size()*4;
  group.bytes+=mdl.substr(frameStart+4,poseSize);group.bytes+=mdl.substr(frameStart+8+poseSize,poseSize);grouped+=group.bytes;
  auto groupedOut=transformMdl(grouped,transforms[0]);assert(groupedOut.substr(frameStart+16,8)==grouped.substr(frameStart+16,8));assert(groupedOut.substr(84,immutable)==grouped.substr(84,immutable));
  bool truncated=false;try{transformMdl(grouped.substr(0,grouped.size()-3),transforms[0]);}catch(...){truncated=true;}assert(truncated);
  bool invalid=false;nodes[0].scale={0,1,1};try{inverse(world(nodes,0));}catch(...){invalid=true;}assert(invalid);
  nodes[0].parent=2;invalid=false;try{world(nodes,0);}catch(...){invalid=true;}assert(invalid);
  std::cout<<"PASS hierarchy, pivots, nonuniform scale, inverse transforms, baked frames/normals/materials and MD3/IQM/MD5 outputs\n";
}
