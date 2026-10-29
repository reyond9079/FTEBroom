// SPDX-License-Identifier: GPL-2.0-or-later
static ImVec2 setupGameCenters[5],setupModCenters[2];
static bool setupChoice(const char* id,const char* title,const char* subtitle,bool selected,const char* logo=nullptr,ImVec2* center=nullptr,float progress=-1,const char* progressLabel=nullptr) {
  ImGui::PushID(id);
  auto start=ImGui::GetCursorScreenPos();ImVec2 size{ImGui::GetContentRegionAvail().x,(progress>=0?110:76)*uiScale()};
  bool clicked=ImGui::InvisibleButton("choice",size);bool hover=ImGui::IsItemHovered();
  if(center)*center={start.x+size.x*.5f,start.y+size.y*.5f};
  auto list=ImGui::GetWindowDrawList();ImVec2 end{start.x+size.x,start.y+size.y};
  list->AddRectFilled(start,end,ImGui::GetColorU32(selected?ImGuiCol_HeaderActive:hover?ImGuiCol_HeaderHovered:ImGuiCol_FrameBg),4*uiScale());
  list->AddRect(start,end,ImGui::GetColorU32(selected?ImGuiCol_CheckMark:ImGuiCol_Border),4*uiScale(),0,selected?2:1);
  float inset=16*uiScale();
  if(logo){auto handle=draw->LoadImage(logo);if(handle)list->AddImage((ImTextureID)handle,{start.x+inset,start.y+14*uiScale()},{start.x+inset+48*uiScale(),start.y+62*uiScale()});inset+=64*uiScale();}
  list->AddText({start.x+inset,start.y+14*uiScale()},ImGui::GetColorU32(ImGuiCol_Text),title);
  list->AddText({start.x+inset,start.y+42*uiScale()},ImGui::GetColorU32(ImGuiCol_Text),subtitle);
  if(progress>=0){
    ImVec2 a{start.x+16*uiScale(),start.y+74*uiScale()},b{end.x-16*uiScale(),end.y-12*uiScale()};
    list->AddRectFilled(a,b,ImGui::GetColorU32(ImGuiCol_FrameBg),3);
    list->AddRectFilled(a,{a.x+(b.x-a.x)*std::clamp(progress,0.f,1.f),b.y},ImGui::GetColorU32(ImGuiCol_CheckMark),3);
    if(progressLabel)list->AddText({a.x+6,a.y+2},ImGui::GetColorU32(ImGuiCol_Text),progressLabel);
  }
  ImGui::PopID();return clicked;
}
static void setupGameChoices() {
  const int order[]={2,3,0,4,1};
  const char* titles[]={"Quake 1","Quake II","Quake III Arena","Half-Life","FTE Custom game"};
  const char* captions[]={"Classic Quake maps, WAD textures and mods","Quake II maps, WAL textures and PAK archives","Quake III maps, patches and shaders","GoldSrc maps, WAD3 textures, FreeHL / FreeCS","Custom FTE games and engine features"};
  const char* logos[]={"ui/games/quake1.png",nullptr,"ui/games/quake3.png",nullptr,"ui/games/fte.png"};
  for(int i=0;i<5;i++)if(setupChoice(titles[i],titles[i],captions[i],gameProfile==order[i],logos[i],&setupGameCenters[i])){
    if(gameProfile<2)q3Root=baseq3;
    gameProfile=order[i];
    std::snprintf(baseq3,sizeof baseq3,"%s",gameProfile==3?q2Mod:gameWad()?q1Authoring().c_str():q3Root.c_str());
  }
}
static bool setupModBrowseOpen=false;
static int setupModBrowseProfile=2;
static ImVec2 setupModBrowseUseCenter;
static char setupModBrowsePath[4096]{};
static std::string setupModBrowseScanned,setupModBrowseError;
static std::vector<std::filesystem::path> setupModDirectories;
static void setupModBrowseStart(int profile) {
  setupModBrowseProfile=profile;
  const char* value=profile==3?q2Mod:profile==4?hlMod:q1Mod;
  auto path=*value?std::filesystem::u8path(value):settingsPath().parent_path();
  std::snprintf(setupModBrowsePath,sizeof setupModBrowsePath,"%s",path.u8string().c_str());
  setupModBrowseOpen=true;setupModBrowseScanned.clear();
}
static void setupModBrowse() {
  if(!setupModBrowseOpen)return;
  if(!ImGui::IsPopupOpen("Choose mod directory"))ImGui::OpenPopup("Choose mod directory");
  ImGui::SetNextWindowSize({650*uiScale(),420*uiScale()},ImGuiCond_FirstUseEver);
  if(ImGui::BeginPopupModal("Choose mod directory",&setupModBrowseOpen)){
    ImGui::InputText("Directory",setupModBrowsePath,sizeof setupModBrowsePath);
    if(ImGui::Button("Up##mod")){auto parent=std::filesystem::u8path(setupModBrowsePath).parent_path();if(!parent.empty())std::snprintf(setupModBrowsePath,sizeof setupModBrowsePath,"%s",parent.u8string().c_str());}
    ImGui::SameLine();if(ImGui::Button("Refresh##mod"))setupModBrowseScanned.clear();
    if(setupModBrowseScanned!=setupModBrowsePath){setupModBrowseScanned=setupModBrowsePath;setupModDirectories.clear();setupModBrowseError.clear();try{
      for(const auto& e:std::filesystem::directory_iterator(std::filesystem::u8path(setupModBrowsePath)))if(e.is_directory())setupModDirectories.push_back(e.path());
      std::sort(setupModDirectories.begin(),setupModDirectories.end());
    }catch(const std::exception& e){setupModBrowseError=e.what();}}
    ImGui::BeginChild("Mod folders",{0,-85*uiScale()},ImGuiChildFlags_Borders);
    ImGuiListClipper clip;clip.Begin(setupModDirectories.size());while(clip.Step())for(int i=clip.DisplayStart;i<clip.DisplayEnd;i++)if(ImGui::Selectable(setupModDirectories[i].filename().u8string().c_str(),false,ImGuiSelectableFlags_NoAutoClosePopups))std::snprintf(setupModBrowsePath,sizeof setupModBrowsePath,"%s",setupModDirectories[i].u8string().c_str());
    ImGui::EndChild();
    if(ImGui::Button("Use this folder")){try{auto path=std::filesystem::absolute(std::filesystem::u8path(setupModBrowsePath));if(!std::filesystem::is_directory(path))throw std::runtime_error("Choose an existing directory.");auto text=path.u8string();if(text.size()>=sizeof q1Mod)throw std::runtime_error("Directory path is too long.");auto target=setupModBrowseProfile==3?q2Mod:setupModBrowseProfile==4?hlMod:q1Mod;std::snprintf(target,1024,"%s",text.c_str());if(setupModBrowseProfile==2)q1UseLibre=false;setupModBrowseOpen=false;}catch(const std::exception& e){setupModBrowseError=e.what();}}
    {auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();setupModBrowseUseCenter={(a.x+b.x)*.5f,(a.y+b.y)*.5f};}
    ImGui::SameLine();if(ImGui::Button("Cancel##mod"))setupModBrowseOpen=false;
    if(!setupModBrowseError.empty())ImGui::TextWrapped("%s",setupModBrowseError.c_str());
    if(!setupModBrowseOpen)ImGui::CloseCurrentPopup();ImGui::EndPopup();
  }
}
static int setupModChoice=-1;
static void setupQuakeModFields() {
  if(setupModChoice<0)setupModChoice=q1UseLibre?0:1;
  uint64_t done=0,total=0;std::string phase;char progressLabel[128]={};
  if(libreBusy()){std::ifstream(libreInstall()/"progress.txt")>>done>>total>>phase;std::snprintf(progressLabel,sizeof progressLabel,"%s  %.1f / %.1f MiB",phase.empty()?"Connecting...":phase.c_str(),done/1048576.,total/1048576.);}
  ImGui::BeginDisabled(libreBusy());
  if(setupChoice("libre","LibreQuake",libreReady()?"Installed and ready to use":"Downloads automatically when you start",setupModChoice==0,nullptr,&setupModCenters[0],libreBusy()?(total?float(double(done)/total):0.f):-1,progressLabel)){setupModChoice=0;q1UseLibre=libreReady();}
  if(setupChoice("own","Choose my own mod","Select a directory with your mod resources",setupModChoice==1,nullptr,&setupModCenters[1])){setupModChoice=1;q1UseLibre=false;}
  ImGui::EndDisabled();
  if(setupModChoice==0 && !libreError.empty())ImGui::TextWrapped("%s",libreError.c_str());
  if(setupModChoice==1) {
    ImGui::SetNextItemWidth(-110*uiScale());ImGui::InputText("##moddirectory",q1Mod,sizeof q1Mod);ImGui::SameLine();
    if(ImGui::Button("Browse...##mod"))setupModBrowseStart(2);
    ImGui::TextWrapped("Choose the game or mod folder. If it contains id1, that folder is selected automatically. Otherwise the selected folder is used as the mod. A sibling id1 supplies base assets for a custom mod, with mod overrides. Mapping uses the standard Quake palette when the mod has no palette. Playing an expansion such as AD still requires the base game.");
  }
  if(ImGui::CollapsingHeader("Advanced settings")){
    ImGui::InputTextMultiline("Additional WAD2 archives (optional, one path per line)",q1Wads,sizeof q1Wads,{ImGui::GetContentRegionAvail().x*.6f,60*uiScale()});
    ImGui::InputText("Entity definitions (optional .def / .fgd / .ent)",q1Definitions,sizeof q1Definitions);
    ImGui::TextWrapped("Leave entity definitions empty to discover the mod's definitions automatically, including PAK archives. Built-in definitions are used only if none are found.");
    ImGui::Combo("Save MAP format",&q1MapFormat,"Quake standard\0Valve 220\0");
    ImGui::Combo("Target BSP",&q1BspFormat,"BSP29 (classic limits)\0BSP2 (modern engines)\0");
  }
  setupModBrowse();
}
