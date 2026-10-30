#include "ftebroom_quake2_contents.h"
// SPDX-License-Identifier: GPL-2.0-or-later
// Missing-resource consent belongs to a single immutable export, not a profile.
static bool runExportPrepared=false,runMissingAwaiting=false;
static int runMissingDecision=0;
static std::vector<std::string> runMissingNames;
static std::set<std::string> runPlaceholderNames;
static std::vector<std::pair<std::string,qhandle_t>> buildPlaceholderHandles;
// These aliases also keep real loose WAL resources available to BSP previews.
static std::map<std::string,std::string> buildAliasImages;
static void buildRemapPlaceholders(){
  for(const auto& entry:buildPlaceholderHandles){
    auto name="textures/"+ftebroom::materialKey(entry.first);auto found=materials.find(name);if(found==materials.end())continue;
    auto stem=std::filesystem::u8path(found->second.preview).stem().u8string();
    std::ostringstream q;q<<std::quoted(name)<<' '<<std::quoted("ftebroom/build-placeholder/"+stem);command(("ftebroom_material_remap "+q.str()+" 0\n").c_str());
  }
}
static void resetBuildPlaceholders(){
  for(auto& entry:buildPlaceholderHandles){std::ostringstream q;q<<std::quoted(entry.first);command(("ftebroom_material_remap "+q.str()+" "+q.str()+" 0\n").c_str());draw->UnloadImage(entry.second);}
  buildPlaceholderHandles.clear();
  buildAliasImages.clear();
}
static void buildWriteTexture(const std::filesystem::path& path,const std::string& bytes){
  std::filesystem::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);out.write(bytes.data(),bytes.size());out.close();if(!out)throw std::runtime_error("Cannot stage compile texture: "+path.u8string());
}
static void buildRegisterMaterialAlias(const std::string& name,const std::string& image){
  auto material="textures/"+name;if(buildAliasImages[material]==image)return;buildAliasImages[material]=image;
  auto alias="ftebroom/build-placeholder/"+std::filesystem::u8path(image).stem().u8string();
  auto script="{\ndiffusemap "+image+"\n{\nmap "+image+"\n}\n{\nmap $lightmap\nblendfunc filter\ndepthfunc equal\n}\n}";
  auto handle=draw->LoadImageShader(alias.c_str(),script.c_str());buildPlaceholderHandles.emplace_back(material,handle);
  std::ostringstream q;q<<std::quoted(material)<<' '<<std::quoted(alias);command(("ftebroom_material_remap "+q.str()+" 0\n").c_str());
}
static void buildPublishPlaceholder(const std::string& name,const ftebroom::quake1::Texture& t,const std::filesystem::path& folder){
  auto key=ftebroom::quake1::textureCacheKey(t,t.palette);auto image="textures/_ftebroom_missing/"+key+".tga";
  buildWriteTexture(documentPath(image),ftebroom::quake1::tga(t,t.palette));
  auto material="textures/"+name;materials[material]={material,image,false};buildRegisterMaterialAlias(name,image);
  runPlaceholderNames.insert(name);runAppend("Placeholder texture: "+name+"\n");
}
static bool runPrepareExport(const std::filesystem::path& target){
  if(!runExportPrepared){
    std::filesystem::create_directories(target.parent_path());
    std::filesystem::copy_file(documentPath("data/export-map.map"),target,std::filesystem::copy_options::overwrite_existing);
    auto source=q1Read(target);runMissingNames.clear();runPlaceholderNames.clear();
    if(gameProfile==3){size_t corrected=0;auto prepared=ftebroom::q2CompileContents(source,corrected);if(corrected){source=std::move(prepared);buildWriteTexture(target,source);runAppend("Quake II: removed legacy inferred SOLID alongside one explicit non-solid visible type (WINDOW/AUX/LAVA/SLIME/WATER/MIST) on "+std::to_string(corrected)+" faces in the compile copy; source MAP unchanged.\n");}}

    if(gameWad()){
      auto wads=q1CompilerWads(target.parent_path(),source,&runMissingNames);
      buildWriteTexture(target,ftebroom::quake1::map(source,false,wads,false,gameProfile==4));
      if(!q1MissingWads.empty())runAppend("Unavailable WAD archives: "+std::to_string(q1MissingWads.size())+". Only unresolved textures used by this snapshot require placeholders.\n");
    }else{
      std::set<std::string> names;for(auto name:ftebroom::quake1::mapTextures(source))names.insert(ftebroom::materialKey(name));
      // Engine exports curved surfaces as patchDef2/3 followed by their material.
      std::istringstream rows(source);std::string row;int patch=0;
      while(std::getline(rows,row)){auto at=row.find_first_not_of(" \t\r");if(at==row.npos)continue;auto text=row.substr(at);if(text=="patchDef2"||text=="patchDef3"){patch=1;continue;}if(patch&&text=="{")continue;if(patch){std::istringstream value(text);std::string name;if(value>>std::quoted(name))names.insert(ftebroom::materialKey(name));patch=0;}}
      auto q2pal=gameProfile==3?q2Palette():std::string{};bool aliasesChanged=false;
      for(const auto& name:names){
        if(!ftebroom::safeMaterialPath(name))throw std::runtime_error("Unsafe material path: "+name);
        bool neutral=name=="__tb_empty"||name=="fb_grey";
        if(gameProfile==3){
          auto bytes=q2WalBytes(name);
          if(!bytes.empty()){
            ftebroom::legacy::wal(bytes,q2pal);buildWriteTexture(target.parent_path()/std::filesystem::u8path("textures/"+name+".wal"),bytes);
            auto root=q2Material(name);if(root&&root->looseWal){auto copy=*root;copy.name="textures/"+name;auto current=materials.find(copy.name);if(current==materials.end()||current->second.preview!=copy.preview){materials[copy.name]=copy;aliasesChanged=true;}buildRegisterMaterialAlias(name,copy.preview);}
            continue;
          }
          if(neutral){auto t=ftebroom::placeholderTexture(name,q2pal,true);buildWriteTexture(target.parent_path()/std::filesystem::u8path("textures/"+name+".wal"),ftebroom::placeholderWal(t));continue;}
        }
        auto found=materials.find("textures/"+name);
        if(found==materials.end())found=std::find_if(materials.begin(),materials.end(),[&](const auto& entry){return ftebroom::materialKey(entry.first)==name;});
        bool absent=found==materials.end();
        if(!absent&&!found->second.shader){size_t size=0;auto bytes=files->LoadFile(found->second.preview.c_str(),&size);absent=!bytes||!size;if(bytes)core->Free(bytes);}
        if(absent || (found!=materials.end()&&found->second.preview.rfind("textures/_ftebroom_missing/",0)==0))runMissingNames.push_back(name);
      }
      if(aliasesChanged){++materialDataRevision;publishMaterialTags();}
    }
    runExportPrepared=true;
  }
  if(!runMissingNames.empty()){
    if(!runMissingDecision){runMissingAwaiting=true;return false;}
    if(runMissingDecision<0)throw std::runtime_error("Compile cancelled: reconnect the missing textures and try again.");
    auto palette=gameProfile==3?q2Palette():q1Palette();
    std::string wad;if(gameWad())wad=q1Read(target.parent_path()/"textures.wad");
    for(const auto& name:runMissingNames){auto t=ftebroom::placeholderTexture(name,palette);
      if(gameWad()){if(name.size()>15)throw std::runtime_error("WAD texture names cannot exceed 15 characters: "+name);wad=ftebroom::addWadTexture(std::move(wad),t,palette,false,gameProfile==4);}
      else buildWriteTexture(target.parent_path()/std::filesystem::u8path("textures/"+name+(gameProfile==3?".wal":".tga")),gameProfile==3?ftebroom::placeholderWal(t):ftebroom::quake1::tga(t,palette));
      buildPublishPlaceholder(name,t,target.parent_path());
    }
    if(gameWad())buildWriteTexture(target.parent_path()/"textures.wad",wad);
    std::ofstream list(target.parent_path()/"placeholder-textures.txt");for(const auto& name:runMissingNames)list<<name<<'\n';
    ++materialDataRevision;publishMaterialTags();runMissingNames.clear();
  }
  return true;
}
static void runMissingPanel(){
  if(!runMissingAwaiting)return;
  ImGui::OpenPopup("Missing compile textures");ImGui::SetNextWindowSize({580,400},ImGuiCond_Appearing);bool open=true;
  if(ImGui::BeginPopupModal("Missing compile textures",&open,ImGuiWindowFlags_NoSavedSettings)){
    ImGui::TextWrapped("%zu textures used by this map are unavailable. Create visible checkerboard placeholders for this compile? Original MAP, WADs and game resources are not changed.",runMissingNames.size());
    ImGui::TextWrapped("The same choice applies to normal compilers and GPU MAP baking. Reconnect the original resources for final results.");
    if(ImGui::BeginChild("Missing textures",{0,-ImGui::GetFrameHeightWithSpacing()*2},ImGuiChildFlags_Borders))for(const auto& name:runMissingNames)ImGui::TextUnformatted(name.c_str());ImGui::EndChild();
    if(ImGui::Button("Create placeholders and continue")){runMissingDecision=1;runMissingAwaiting=false;ImGui::CloseCurrentPopup();}
    ImGui::SameLine();if(ImGui::Button("Cancel")){runMissingDecision=-1;runMissingAwaiting=false;ImGui::CloseCurrentPopup();}ImGui::EndPopup();
  }
  if(!open){runMissingDecision=-1;runMissingAwaiting=false;}
}
