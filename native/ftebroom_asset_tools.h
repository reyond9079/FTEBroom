// SPDX-License-Identifier: GPL-2.0-or-later
// Asset tools operate on virtual archive paths through the file manager reader.
static bool paletteOpen=false,newWadOpen=false,newWadForImport=false;
static std::string paletteBytes,paletteError,newWadError;
static std::filesystem::path palettePath,newWadFolder,newWadCreated;
static char newWadName[256]="textures.wad";
static int paletteSelected=0;
static void paletteShow(const std::filesystem::path& path) {
  palettePath=path;paletteError.clear();paletteBytes.clear();paletteOpen=true;
  try{auto bytes=modFileBytes(path);if(bytes.size()!=768)throw std::runtime_error("A Quake palette must contain exactly 768 bytes (256 RGB colors). This LMP is not a palette.");paletteBytes=std::move(bytes);}
  catch(const std::exception& e){paletteError=e.what();}
}
static void newWadShow(const std::filesystem::path& folder,bool importing=false){newWadFolder=folder;newWadForImport=importing;newWadOpen=true;newWadError.clear();}
static void newWadCreate(){
        auto name=std::filesystem::u8path(newWadName);if(name.empty()||name.has_parent_path()||name=="."||name=="..")throw std::runtime_error("Enter a filename, without directories.");
        name.replace_extension(".wad");auto target=newWadFolder/name;
        if(ftebroom::pak::exists(target)||std::filesystem::exists(target))throw std::runtime_error("A file with that name already exists.");
        std::string bytes(gameProfile==4?"WAD3":"WAD2",4);bytes.resize(12,0);bytes[8]=12;
        if(modFilePacked(newWadFolder)){modFileValidate(target);ftebroom::pak::replace(target,bytes);}
        else {if(!std::filesystem::is_directory(newWadFolder))throw std::runtime_error("Choose an existing directory.");std::ofstream out(target,std::ios::binary);out.write(bytes.data(),bytes.size());out.close();if(!out)throw std::runtime_error("Cannot create WAD.");}
        if(newWadForImport)newWadCreated=target;modFilesRefresh=true;newWadOpen=false;
}
static void assetToolsPanel(){
  if(paletteOpen){ImGui::SetNextWindowSize({550*uiScale(),620*uiScale()},ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Palette viewer",&paletteOpen)){
      ImGui::TextWrapped("%s",palettePath.u8string().c_str());
      if(!paletteError.empty())ImGui::TextWrapped("%s",paletteError.c_str());
      else {float cell=std::max(6.f,std::min(28.f*uiScale(),(ImGui::GetContentRegionAvail().x-15*ImGui::GetStyle().ItemSpacing.x)/16));
        for(int i=0;i<256;i++){if(i%16)ImGui::SameLine();ImGui::PushID(i);auto c=(const unsigned char*)paletteBytes.data()+i*3;
          if(ImGui::ColorButton("##color",{c[0]/255.f,c[1]/255.f,c[2]/255.f,1},ImGuiColorEditFlags_NoTooltip,{cell,cell}))paletteSelected=i;
          if(i==paletteSelected)ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),IM_COL32(255,220,50,255),0,0,2);
          if(ImGui::IsItemHovered())ImGui::SetTooltip("Index %d | RGB %d, %d, %d | #%02X%02X%02X",i,c[0],c[1],c[2],c[0],c[1],c[2]);ImGui::PopID();}
        auto c=(const unsigned char*)paletteBytes.data()+paletteSelected*3;ImGui::Text("Index %d   RGB %d, %d, %d   #%02X%02X%02X",paletteSelected,c[0],c[1],c[2],c[0],c[1],c[2]);
        ImGui::TextDisabled("Index 255 is transparent only for materials using masked transparency.");
      }
    }ImGui::End();
  }
  if(newWadOpen){ImGui::SetNextWindowSize({500*uiScale(),230*uiScale()},ImGuiCond_FirstUseEver);
    if(ImGui::Begin("New WAD",&newWadOpen,ImGuiWindowFlags_NoCollapse)){
      ImGui::TextWrapped("Directory: %s",newWadFolder.u8string().c_str());ImGui::InputText("Filename",newWadName,sizeof newWadName);
      if(!newWadError.empty())ImGui::TextWrapped("%s",newWadError.c_str());
      if(ImGui::Button(gameProfile==4?"Create WAD3":"Create WAD2"))try{
        newWadCreate();
      }catch(const std::exception& e){newWadError=e.what();}
      ImGui::SameLine();if(ImGui::Button("Cancel"))newWadOpen=false;
    }ImGui::End();
  }
}
