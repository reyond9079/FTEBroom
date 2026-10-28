// SPDX-License-Identifier: GPL-2.0-or-later
#include "ftebroom_md3.h"
#include "ftebroom_legacy_models.h"
struct EntityPreviewPart {qhandle_t image=0;std::vector<std::array<ftebroom::MdlVertex,3>> triangles;};
struct EntityPreview {qhandle_t image=0;std::vector<EntityPreviewPart> parts;std::vector<std::pair<size_t,size_t>> order;};
static std::map<std::string,EntityPreview> entityPreviews;
static void clearEntityPreviews(){for(auto& p:entityPreviews)for(auto& part:p.second.parts)if(part.image)draw->UnloadImage(part.image);entityPreviews.clear();}
static std::string modelResource(const std::string& path){if(path=="gfx/palette.lmp")return q1Palette();size_t length=0;auto raw=(char*)files->LoadFile(path.c_str(),&length);if(!raw)return {};std::string result(raw,length);core->Free(raw);return result;}
static qhandle_t modelImage(std::string path){
 auto found=materials.find(path);if(found!=materials.end()&&!found->second.preview.empty())path=found->second.preview;
 for(auto suffix:{"",".tga",".png",".jpg",".jpeg",".pcx"})if(!modelResource(path+suffix).empty())return draw->LoadImage((path+suffix).c_str());
 return 0;
}
static bool entityModelTile(const ftebroom::EntityDefinition& def,ImVec2 tile,float size){
 if(def.brush||def.model.empty()||def.model.find_first_of("{}()\"'")!=std::string::npos)return false;
 if(!ImGui::IsRectVisible(tile,{tile.x+size,tile.y+size}))return true;
 auto key=def.model+":"+def.modelSkin;auto it=entityPreviews.find(key);
 if(it==entityPreviews.end()){
  EntityPreview preview;
  try {
   auto bytes=modelResource(def.model);if(bytes.empty())throw std::runtime_error("Missing model");
   if(bytes.substr(0,4)=="IDST"||bytes.substr(0,4)=="IDP2"){
    std::string companion;
    if(bytes.substr(0,4)=="IDST"&&bytes.size()>=244&&!ftebroom::quake1::word(bytes,180)){
      auto stem=def.model.substr(0,def.model.find_last_of('.'));companion=modelResource(stem+"t.mdl");if(companion.empty())companion=modelResource(stem+"T.mdl");}
    auto parts=bytes.substr(0,4)=="IDP2"?ftebroom::legacyModels::md2(bytes,std::max(0,std::atoi(def.modelSkin.c_str())),std::max(0,std::atoi(def.modelFrame.c_str()))):ftebroom::legacyModels::studio(bytes,companion,std::max(0,std::atoi(def.modelSkin.c_str())));
    for(size_t i=0;i<parts.size();i++){auto& part=parts[i];qhandle_t image=0;
      if(!part.embedded.pixels.empty()){auto tga=ftebroom::quake1::tga(part.embedded,{});auto name="ftebroom/legacy-model-"+std::to_string(entityDefinitionEpoch)+"-"+std::to_string(std::hash<std::string>{}(key))+"-"+std::to_string(i)+".tga";draw->LoadImageData(name.c_str(),"image/tga",tga.data(),tga.size());image=draw->LoadImage(name.c_str());}
      else image=modelImage(part.image);preview.parts.push_back({image,std::move(part.triangles)});
    }
   }else if(bytes.compare(0,4,"IDP3")==0){
    auto surfaces=ftebroom::readMd3(bytes);std::map<std::string,std::string> skins;
    if(def.modelSkin.find(".skin")!=std::string::npos){std::istringstream input(modelResource(def.modelSkin));std::string line;while(std::getline(input,line)){auto comma=line.find(',');if(comma!=line.npos){auto value=line.substr(comma+1);value.erase(0,value.find_first_not_of(" \t"));auto end=value.find_last_not_of(" \r\t");if(end!=value.npos)value.erase(end+1);skins[line.substr(0,comma)]=value;}}}
    for(auto& surface:surfaces){auto name=skins.count(surface.name)?skins[surface.name]:surface.shader;preview.parts.push_back({modelImage(name),std::move(surface.triangles)});}
   }else{
    auto mdl=ftebroom::readMdl(bytes,std::atoi(def.modelSkin.c_str()));auto palette=modelResource("gfx/palette.lmp");
    auto base=def.model.substr(0,def.model.find_last_of('.'));auto skin=modelImage(base+"_"+(def.modelSkin.empty()?"0":def.modelSkin));
    if(!skin){auto pixels=ftebroom::quake1::tga(mdl.skin,palette);auto name="ftebroom/model-"+std::to_string(entityDefinitionEpoch)+"-"+std::to_string(std::hash<std::string>{}(key))+".tga";
     draw->LoadImageData(name.c_str(),"image/tga",pixels.data(),pixels.size());skin=draw->LoadImage(name.c_str());}
    preview.parts.push_back({skin,std::move(mdl.triangles)});
   }
   // Fit all surfaces together; model thumbnails never execute game code.
   float lo[2]={1e30f,1e30f},hi[2]={-1e30f,-1e30f};
   for(auto& part:preview.parts){if(part.image)preview.image=part.image;for(auto& tri:part.triangles)for(auto& v:tri){auto p=v.point;v.point={.7071f*(p[0]-p[1]),.33f*(p[0]+p[1])-.88f*p[2],.62f*(p[0]+p[1])+.47f*p[2]};for(int a=0;a<2;a++){lo[a]=std::min(lo[a],v.point[a]);hi[a]=std::max(hi[a],v.point[a]);}}}
   float extent=std::max({hi[0]-lo[0],hi[1]-lo[1],1.f});
   for(auto& part:preview.parts){for(auto& tri:part.triangles)for(auto& v:tri)for(int a=0;a<2;a++)v.point[a]=.5f+.88f*(v.point[a]-(hi[a]+lo[a])*.5f)/extent;
    std::stable_sort(part.triangles.begin(),part.triangles.end(),[](const auto& a,const auto& b){return a[0].point[2]+a[1].point[2]+a[2].point[2]<b[0].point[2]+b[1].point[2]+b[2].point[2];});}
   for(size_t part=0;part<preview.parts.size();part++)for(size_t tri=0;tri<preview.parts[part].triangles.size();tri++)preview.order.push_back({part,tri});
   std::stable_sort(preview.order.begin(),preview.order.end(),[&](auto a,auto b){auto& x=preview.parts[a.first].triangles[a.second];auto& y=preview.parts[b.first].triangles[b.second];return x[0].point[2]+x[1].point[2]+x[2].point[2]<y[0].point[2]+y[1].point[2]+y[2].point[2];});
  }catch(...){for(auto& part:preview.parts)if(part.image)draw->UnloadImage(part.image);preview={};}
  it=entityPreviews.emplace(key,std::move(preview)).first;
 }
 auto& p=it->second;if(!p.image)return false;auto list=ImGui::GetWindowDrawList();list->PushClipRect(tile,{tile.x+size,tile.y+size},true);
 qhandle_t active=0;
 for(auto entry:p.order){auto& part=p.parts[entry.first];if(!part.image)continue;if(active!=part.image){if(active)list->PopTextureID();active=part.image;list->PushTextureID((ImTextureID)active);}auto tri=part.triangles[entry.second];
  float area=(tri[1].point[0]-tri[0].point[0])*(tri[2].point[1]-tri[0].point[1])-(tri[1].point[1]-tri[0].point[1])*(tri[2].point[0]-tri[0].point[0]);
  if(area<0)std::swap(tri[1],tri[2]);
  list->PrimReserve(3,3);auto idx=list->_VtxCurrentIdx;for(int j=0;j<3;j++)list->PrimWriteIdx(ImDrawIdx(idx+j));for(const auto& v:tri)list->PrimWriteVtx({tile.x+v.point[0]*size,tile.y+v.point[1]*size},{v.u,v.v},IM_COL32_WHITE);
 }if(active)list->PopTextureID();
 list->PopClipRect();return true;
}
