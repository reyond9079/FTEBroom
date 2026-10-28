// SPDX-License-Identifier: GPL-2.0-or-later
#include "ftebroom_prefab_data.h"
struct PrefabCollection {std::filesystem::path path;ftebroom::MapProfile profile;ftebroom::prefab::Collection data;};
static std::vector<PrefabCollection> prefabCollections;
static bool prefabWindow=false,prefabLoaded=false;
static char prefabPath[4096]{},prefabFilter[256]{};
static int prefabSize=120;
static std::string prefabError,prefabBrowse,prefabScanned;
static std::vector<ModFileEntry> prefabFiles;
static void prefabSave(){std::filesystem::create_directories(documentPath("data"));std::ofstream out(documentPath("data/prefab-collections.txt"));for(const auto& c:prefabCollections)out<<std::quoted(c.path.u8string())<<'\n';if(!out)prefabError="Cannot save prefab collections.";}
static void prefabAdd(const std::filesystem::path& path){
  auto bytes=modFileBytes(path);auto profile=ftebroom::mapProfile(bytes);if(profile.terrain)throw std::runtime_error("Terrain maps are not prefab collections.");
  auto data=ftebroom::prefab::parse(bytes);auto found=std::find_if(prefabCollections.begin(),prefabCollections.end(),[&](const auto& c){return c.path==path;});
  PrefabCollection collection{path,profile,std::move(data)};
  if(found!=prefabCollections.end())*found=std::move(collection);else{if(prefabCollections.size()>=32)throw std::runtime_error("At most 32 prefab collections can be open.");prefabCollections.push_back(std::move(collection));}
}
static void prefabShow(){
  prefabWindow=true;if(prefabLoaded)return;prefabLoaded=true;
  std::ifstream in(documentPath("data/prefab-collections.txt"));std::string path;
  while(prefabCollections.size()<32&&in>>std::quoted(path))try{prefabAdd(std::filesystem::u8path(path));}catch(const std::exception& e){prefabError=e.what();}
}
static void prefabInsert(int collection,int item,float x,float y){
  if(collection<0||size_t(collection)>=prefabCollections.size())throw std::runtime_error("Collection was removed; drag again.");auto& c=prefabCollections[collection];
  if(item<0||size_t(item)>=c.data.groups.size())throw std::runtime_error("Prefab was removed; drag again.");
  if(c.profile.profile>=0&&(c.profile.profile>=2?c.profile.profile:0)!=(gameProfile>=2?gameProfile:0))throw std::runtime_error("Choose a collection from the same game family.");
  organizationLoad();auto base=c.data.entities[c.data.groups[item]].get("_tb_name"),name=base;std::set<std::string> names;for(const auto& n:organizationNodes)if(n.kind==2)names.insert(n.name);
  for(int suffix=1;names.count(name);suffix++)name=base+"_"+std::to_string(suffix);
  auto bytes=ftebroom::prefab::extract(c.data,c.data.groups[item],name);qhandle_t handle;
  if(files->Open("data/import-in.map",&handle,2)<0)throw std::runtime_error("Cannot stage prefab.");files->Write(handle,bytes.data(),bytes.size());files->Close(handle);
  command(("ftebroom_import_file 0 "+std::to_string(x)+" "+std::to_string(y)+"\n").c_str());
}
static void prefabPanel(){
  if(!prefabWindow)return;auto viewport=ImGui::GetMainViewport();ImGui::SetNextWindowSize({std::min(560*uiScale(),viewport->WorkSize.x-20),std::min(480*uiScale(),viewport->WorkSize.y-30)},ImGuiCond_FirstUseEver);ImGui::SetNextWindowPos({viewport->WorkPos.x+viewport->WorkSize.x*.5f,viewport->WorkPos.y+viewport->WorkSize.y*.5f},ImGuiCond_FirstUseEver,{.5f,.5f});
  if(ImGui::Begin("Prefab collections",&prefabWindow)){
    try{
      ImGui::InputText("Collection MAP",prefabPath,sizeof prefabPath);ImGui::SameLine();if(ImGui::SmallButton("Browse...")){prefabBrowse=documentDefaultFolder().u8string();prefabScanned.clear();}
      if(ImGui::Button("Add / reload collection")){prefabAdd(std::filesystem::u8path(prefabPath));prefabSave();prefabError.clear();}
      if(ImGui::BeginDragDropTarget()){if(auto payload=ImGui::AcceptDragDropPayload("FTEBROOM_MOD_FILE")){auto path=std::filesystem::u8path((const char*)payload->Data);if(entityLower(path.extension().u8string())==".map"){try{prefabAdd(path);prefabSave();}catch(const std::exception& e){prefabError=e.what();}}}ImGui::EndDragDropTarget();}
      if(!prefabBrowse.empty()){
        if(ImGui::SmallButton("Up##prefab"))prefabBrowse=std::filesystem::u8path(prefabBrowse).parent_path().u8string();ImGui::SameLine();if(ImGui::SmallButton("Close browser##prefab"))prefabBrowse.clear();
        if(prefabScanned!=prefabBrowse){prefabScanned=prefabBrowse;prefabFiles.clear();if(!prefabBrowse.empty())for(const auto& entry:modFileList(std::filesystem::u8path(prefabBrowse)))if(entry.first||entityLower(entry.second.extension().u8string())==".map"||entityLower(entry.second.extension().u8string())==".pak"||entityLower(entry.second.extension().u8string())==".pk3")prefabFiles.push_back(entry);}
        ImGui::TextWrapped("%s",prefabBrowse.c_str());ImGui::BeginChild("Prefab files",{0,120*uiScale()},ImGuiChildFlags_Borders);
        ImGuiListClipper clipper;clipper.Begin(prefabFiles.size());while(clipper.Step())for(int i=clipper.DisplayStart;i<clipper.DisplayEnd;i++){auto entry=prefabFiles[i];auto ext=entityLower(entry.second.extension().u8string());bool folder=entry.first||ext==".pak"||ext==".pk3";auto label=(folder?"[Folder] ":"")+entry.second.filename().u8string();if(ImGui::Selectable(label.c_str())){if(folder)prefabBrowse=entry.second.u8string();else{std::snprintf(prefabPath,sizeof prefabPath,"%s",entry.second.u8string().c_str());prefabBrowse.clear();}}}ImGui::EndChild();
      }
      ImGui::InputText("Filter prefabs",prefabFilter,sizeof prefabFilter);ImGui::SliderInt("Card size",&prefabSize,80,240);ImGui::TextWrapped("Drag a named group into a viewport. Each placement is independent; Undo removes the complete instance. Reload after changing a collection MAP.");
      for(int c=0;c<(int)prefabCollections.size();c++){
        ImGui::PushID(c);auto& collection=prefabCollections[c];bool expanded=ImGui::CollapsingHeader(collection.path.filename().u8string().c_str(),ImGuiTreeNodeFlags_DefaultOpen);
        if(ImGui::BeginPopupContextItem()){if(ImGui::MenuItem("Remove collection")){prefabCollections.erase(prefabCollections.begin()+c);prefabSave();ImGui::EndPopup();ImGui::PopID();break;}ImGui::TextWrapped("%s",collection.path.u8string().c_str());ImGui::EndPopup();}
        if(expanded){
          std::vector<int> visible;auto filter=entityLower(prefabFilter);for(int i=0;i<(int)collection.data.groups.size();i++)if(entityLower(collection.data.entities[collection.data.groups[i]].get("_tb_name")).find(filter)!=std::string::npos)visible.push_back(i);
          int columns=std::max(1,int(ImGui::GetContentRegionAvail().x/(prefabSize*uiScale()+ImGui::GetStyle().ItemSpacing.x)));float height=64*uiScale();ImGuiListClipper clipper;clipper.Begin((visible.size()+columns-1)/columns,height+ImGui::GetStyle().ItemSpacing.y);
          while(clipper.Step())for(int row=clipper.DisplayStart;row<clipper.DisplayEnd;row++)for(int column=0;column<columns;column++){int at=row*columns+column;if(at>=(int)visible.size())break;int item=visible[at];if(column)ImGui::SameLine();ImGui::PushID(item);auto name=collection.data.entities[collection.data.groups[item]].get("_tb_name");
            ImGui::Button(name.c_str(),{prefabSize*uiScale(),height});if(ImGui::IsItemHovered())ImGui::SetTooltip("%s\n%s",name.c_str(),collection.path.u8string().c_str());
            if(ImGui::BeginDragDropSource()){int payload[2]={c,item};ImGui::SetDragDropPayload("FTEBROOM_PREFAB",payload,sizeof payload);ImGui::TextUnformatted(name.c_str());ImGui::EndDragDropSource();}ImGui::PopID();
          }
        }ImGui::PopID();
      }
    }catch(const std::exception& e){prefabError=e.what();}
    if(!prefabError.empty())ImGui::TextWrapped("%s",prefabError.c_str());
  }ImGui::End();
}
