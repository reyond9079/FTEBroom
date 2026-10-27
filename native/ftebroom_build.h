// SPDX-License-Identifier: GPL-2.0-or-later
// Build jobs use immutable snapshots beside the source map; previews use a local cache.
static bool buildWindow,buildRunning,buildGeometry=true,buildVis=true,buildLight=true;
static bool buildWasPreview;
static std::string buildMessage,buildResultModel,buildPendingModel;
static std::filesystem::path buildResult,buildPending,buildDocument,buildResultDocument;
static int buildRevision,buildResultRevision,buildResultProfile=-1;
static double buildStarted;
static int buildSavedLayout,buildSavedFocus,buildSavedMax,buildSavedTypes[4];
static std::vector<std::string> buildStages;
static std::string buildArguments[3];
static void buildLoadSettings()
{
  static int loaded=-1;int profile=gameProfile==2?2:0;if(loaded==profile)return;loaded=profile;
  buildGeometry=buildVis=buildLight=true;for(auto& arg:buildArguments)arg.clear();
  std::ifstream settings(documentPath((gameProfile==2?"ftebroom-quake1-build-options.txt":"ftebroom-build-options.txt")));int geometry,vis,light;
  if(settings>>geometry>>vis>>light){buildGeometry=geometry!=0;buildVis=vis!=0;buildLight=light!=0;}
  for(auto& argument:buildArguments)settings>>std::quoted(argument);
}
static std::string buildRead(const std::filesystem::path& path)
{
  std::ifstream file(path,std::ios::binary);
  if(!file)throw std::runtime_error("Cannot read build snapshot: "+path.u8string());
  return {std::istreambuf_iterator<char>(file),{}};
}
static bool buildValidBsp(const std::filesystem::path& path)
{
  return ftebroom::quake1::bsp(buildRead(path),gameProfile==2);
}
static void buildExported(const std::filesystem::path& path)
{
  if(documentFile!=buildDocument)throw std::runtime_error("The active map changed during export. Start Build again for the current map.");
  buildRevision=(int)cvars->GetFloat("ftebroom_revision");
  runAppend("Exported current editor contents from: "+buildDocument.u8string()+"\nBuild copy: "+path.u8string()+"\nRevision: "+std::to_string(buildRevision)+"\n");
  if(!buildGeometry && buildRead(path)!=buildRead(buildResult.parent_path()/"snapshot.map"))
    throw std::runtime_error("Geometry changed. Enable Geometry/BSP before rebuilding VIS or LIGHT.");
}
static void buildStageFinished()
{
  if(buildGeometry && runStep==1){auto portal=buildPending;portal.replace_extension(".prt");
    if(std::filesystem::exists(portal))std::filesystem::copy_file(portal,buildPending.parent_path()/"saved-portals.prt",std::filesystem::copy_options::overwrite_existing);}
}
static void buildRestoreLayout()
{
  if(!buildWasPreview)return;
  viewLayout=buildSavedLayout;viewFocus=buildSavedFocus;viewMax=buildSavedMax;
  std::copy(std::begin(buildSavedTypes),std::end(buildSavedTypes),viewTypes);
  cvars->SetFloat("ftebroom_view_layout",viewLayout);viewActivate(viewFocus);buildWasPreview=false;
}
static void buildEdit()
{
  command("ftebroom_bsp edit\n");buildRestoreLayout();
}
static void buildShow()
{
  if(buildResult.empty() || (buildResultDocument!=documentFile || buildResultProfile!=gameProfile))return;
  cancelGesture();cameraEnd();
  if(!buildWasPreview){buildSavedLayout=viewLayout;buildSavedFocus=viewFocus;buildSavedMax=viewMax;
    std::copy(std::begin(viewTypes),std::end(viewTypes),buildSavedTypes);buildWasPreview=true;}
  viewLayout=1;viewFocus=0;viewMax=-1;viewTypes[0]=0;cvars->SetFloat("ftebroom_view_layout",1);viewActivate(0);
  try {
    auto cache=documentPath(buildResultModel).parent_path();std::filesystem::create_directories(cache);
    for(const auto& file:std::filesystem::directory_iterator(buildResult.parent_path()))
      if(file.path().extension()!=".map" && file.path().extension()!=".log")
        std::filesystem::copy(file.path(),cache/file.path().filename(),std::filesystem::copy_options::overwrite_existing|std::filesystem::copy_options::recursive);
    command(("fs_flush\nftebroom_bsp "+buildResultModel+"\n").c_str());
  }catch(const std::exception& error){buildMessage=error.what();buildRestoreLayout();}
}
static std::string modQuote(const std::string& text);
static void modValidate();
static void modPreview();
static void buildBegin()
{
  if(documentTerrain){buildMessage="FTE Engine terrain uses MAP plus section files; BSP compilation is unavailable for this document.";return;}
  if(runActive || buildRunning)return;
  if(documentFile.empty()){buildMessage="Save the map once before building.";return;}
  if(assetsLoading){buildMessage="Wait for game resources to finish loading before building.";return;}
  buildMessage.clear();runLoad();buildLoadSettings();gameProfileLoad();
  if(gameProfile==1 && gamePreviewAfterBuild){try{modValidate();}catch(const std::exception& e){buildMessage=e.what();return;}}
  std::error_code projectError;
  if(!*baseq3 || !std::filesystem::is_directory(std::filesystem::u8path(baseq3),projectError)){buildMessage="Configure an existing game resource folder in Game / resources.";setupNeeded=true;return;}
  if(documentFile.empty()){buildMessage="Save the map once before building.";return;}
  if(!buildGeometry&&!buildVis&&!buildLight){buildMessage="Select at least one build stage.";return;}
  if(gameProfile!=2 && runVariables["q3map2"].empty()){buildMessage="Set the q3map2 executable path.";return;}
  if(!buildGeometry && (buildResult.empty() || (buildResultDocument!=documentFile || buildResultProfile!=gameProfile))){
    buildMessage="Build Geometry/BSP first for this map.";return;}
  if(cvars->GetFloat("ftebroom_bsp_preview"))buildEdit();
  try {
    auto stamp=std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::string relative="maps/ftebroom-build/"+std::to_string(stamp);
    auto folder=ftebroom::pak::diskFolder(documentDefaultFolder())/"_ftebroom-build"/std::to_string(stamp);std::filesystem::create_directories(folder);
    buildPending=folder/"snapshot.bsp";buildPendingModel=relative+"/snapshot.bsp";buildDocument=documentFile;
    if(!buildGeometry){for(const auto& file:std::filesystem::directory_iterator(buildResult.parent_path()))
      if(file.is_regular_file() && file.path().extension()!=".map")std::filesystem::copy_file(file.path(),folder/file.path().filename());
      if(!std::filesystem::exists(folder/"snapshot.prt")&&std::filesystem::exists(folder/"saved-portals.prt"))
        std::filesystem::copy_file(folder/"saved-portals.prt",folder/"snapshot.prt");
      if(buildVis && !std::filesystem::exists(folder/"snapshot.prt"))throw std::runtime_error("VIS needs a portal file. Rebuild Geometry on a sealed map.");}
    RunProfile profile{"Viewport build: "+documentFile.filename().u8string(),folder.u8string(),false,{{0,"${WORK_DIR_PATH}/snapshot.map",""}}};
    buildStages={"Exporting "+documentFile.filename().u8string()};
    std::string common="-game quake3 -fs_basepath \"${GAME_DIR_PATH}/..\" -fs_game baseq3 -threads ${CPU_COUNT} ";
    if(gameProfile==1 && *gameMod && std::filesystem::is_directory(std::filesystem::u8path(gameMod))){
      auto mod=std::filesystem::absolute(std::filesystem::u8path(gameMod)).lexically_normal();
      common="-game quake3 -fs_basepath "+modQuote(mod.parent_path().generic_u8string())+
        " -fs_basegame "+modQuote(mod.filename().u8string())+" -fs_game "+modQuote(mod.filename().u8string())+
        " -fs_pakpath "+modQuote(std::filesystem::u8path(baseq3).generic_u8string())+" -threads ${CPU_COUNT} ";
    }
    if(gameProfile==2) {
      const char* names[]={"qbsp","vis","light"};bool enabled[]={buildGeometry,buildVis,buildLight};
      for(int i=0;i<3;i++)if(enabled[i]){
        if(runVariables[names[i]].empty())throw std::runtime_error(std::string("Set the ")+names[i]+" executable path.");
        std::string args=(i==0 && q1BspFormat==1?"-bsp2 ":"")+buildArguments[i]+" \"${WORK_DIR_PATH}/snapshot."+(i==0?"map":"bsp")+"\"";
        profile.tasks.push_back({1,std::string("${")+names[i]+"}",args});buildStages.push_back(names[i]);
      }
    } else {
    if(buildGeometry){profile.tasks.push_back({1,"${q3map2}",common+"-meta "+buildArguments[0]+" \"${WORK_DIR_PATH}/snapshot.map\""});buildStages.push_back("Geometry / BSP");}
    if(buildVis){profile.tasks.push_back({1,"${q3map2}",common+"-vis "+buildArguments[1]+" \"${WORK_DIR_PATH}/snapshot.bsp\""});buildStages.push_back("Visibility / VIS");}
    if(buildLight){profile.tasks.push_back({1,"${q3map2}",common+"-light "+buildArguments[2]+" \"${WORK_DIR_PATH}/snapshot.bsp\""});buildStages.push_back("Lighting / LIGHT");}
    }
    runExportHook=buildExported;runStageHook=buildStageFinished;runBegin(profile);runWindow=false;buildRunning=runActive;gameProfileLocked=buildRunning;buildStarted=ImGui::GetTime();
    if(!buildRunning){runExportHook=nullptr;runStageHook=nullptr;buildMessage=runError;}
    runSaveProfiles();
    {std::ofstream settings(documentPath((gameProfile==2?"ftebroom-quake1-build-options.txt":"ftebroom-build-options.txt")));settings<<buildGeometry<<' '<<buildVis<<' '<<buildLight;for(auto& argument:buildArguments)settings<<' '<<std::quoted(argument);}
  }catch(const std::exception& error){buildMessage=error.what();runExportHook=nullptr;runStageHook=nullptr;}
}
static void buildTick()
{
  if(buildWasPreview && !modEmbedded && !cvars->GetFloat("ftebroom_bsp_preview"))buildRestoreLayout();
  if(!buildRunning||runActive)return;
  buildRunning=false;gameProfileLocked=false;runExportHook=nullptr;runStageHook=nullptr;
  {std::ofstream log(buildPending.parent_path()/"build.log");log<<runLog;}
  if(runHadError){buildMessage=runError;return;}
  if(runStep!=(int)buildStages.size()){buildMessage="Build cancelled. Previous successful BSP is retained.";return;}
  try {
    if(!buildValidBsp(buildPending))throw std::runtime_error("Compiler did not produce a valid BSP for the selected profile.");
    buildResult=buildPending;buildResultModel=buildPendingModel;buildResultDocument=buildDocument;buildResultRevision=buildRevision;buildResultProfile=gameProfile;
    buildMessage="Build complete. Ready to preview.";
    if(gameProfile!=0 && gamePreviewAfterBuild)modPreview();
    else buildShow();
  }catch(const std::exception& error){buildMessage=error.what();}
}
#include "ftebroom_mod_preview.h"
static void buildPanel()
{
  if(!buildWindow)return;runLoad();buildLoadSettings();
  ImGui::SetNextWindowSize({580,330},ImGuiCond_FirstUseEver);
  if(ImGui::Begin("Setup compile",&buildWindow,uiPanelFlags())){
    ImGui::BeginDisabled(runActive||buildRunning);
    gameProfileLoad();
    ImGui::Text("Profile: %s",gameProfileName());
    if(ImGui::Button("Game / resources..."))setupNeeded=true;
    if(gameProfile!=0 && ImGui::Checkbox("Open mod preview after Build",&gamePreviewAfterBuild))gameProfileSave();
    if(gameProfile==2){for(const char* name:{"qbsp","vis","light"})runText(name,runVariables[name]);}
    else runText("q3map2 executable",runVariables["q3map2"]);
    if(gameProfile==2){
      if(ImGui::Button("Fast preset")){buildGeometry=buildVis=buildLight=true;buildArguments[0]="";buildArguments[1]="-fast";buildArguments[2]="";}
      ImGui::SameLine();if(ImGui::Button("Final preset")){buildGeometry=buildVis=buildLight=true;buildArguments[0]="";buildArguments[1]="";buildArguments[2]="-extra4";}
    }
    ImGui::Checkbox("Geometry / BSP",&buildGeometry);ImGui::Checkbox("Visibility / VIS",&buildVis);ImGui::Checkbox("Lighting / LIGHT",&buildLight);
    ImGui::TextWrapped("VIS and LIGHT without Geometry reuse the last successful BSP for the identical exported map. Build files are written to _ftebroom-build beside the saved map. BSP preview uses a separate editor cache.");
    if(ImGui::CollapsingHeader("Advanced compiler arguments")) {
      runText("BSP arguments",buildArguments[0]);runText("VIS arguments",buildArguments[1]);runText("LIGHT arguments",buildArguments[2]);
      ImGui::TextWrapped("Extra arguments before the input filename. Example LIGHT: -fast -samples 3 -bounce 2. Passed directly to the selected compiler, not a command shell.");
    }
    if(ImGui::Button("Compile"))buildBegin();ImGui::EndDisabled();
    ImGui::SameLine();if(ImGui::Button("Full log / advanced profiles"))runWindow=true;
    if(!buildMessage.empty())ImGui::TextWrapped("%s",buildMessage.c_str());
  }ImGui::End();
}
// Original vector symbols: hammer, sliders, cube and play. No font glyph dependency.
static float compileButtonWidth(const char* label)
{
  return ImGui::CalcTextSize(label,nullptr,true).x+54*uiScale();
}
static bool compileButton(const char* label,int icon)
{
  const float u=uiScale(),size=20*u;
  const auto p=ImGui::GetCursorScreenPos();
  bool clicked=ImGui::Button((std::string("##compile-")+label).c_str(),{compileButtonWidth(label),34*u});
  auto list=ImGui::GetWindowDrawList();auto color=ImGui::GetColorU32(ImGuiCol_Text);
  const float x=p.x+12*u,y=p.y+(ImGui::GetItemRectSize().y-size)*.5f;
  auto point=[&](float a,float b){return ImVec2{x+a*u,y+b*u};};
  auto line=[&](float a,float b,float c,float d){list->AddLine(point(a,b),point(c,d),color,1.7f*u);};
  if(icon==0){line(3,17,12,8);line(1,15,10,6);line(1,15,3,17);line(8,4,12,0);line(12,0,19,7);line(19,7,15,11);line(15,11,8,4);}
  else if(icon==1){for(int row=0;row<3;row++){float v=3+7*row,k=row==1?13:6;line(1,v,k-2,v);line(k+2,v,19,v);list->AddRect(point(k-2,v-2),point(k+2,v+2),color,0,0,1.5f*u);}}
  else if(icon==2){line(2,5,10,1);line(10,1,18,5);line(18,5,18,15);line(18,15,10,19);line(10,19,2,15);line(2,15,2,5);line(2,5,10,9);line(10,9,18,5);line(10,9,10,19);}
  else if(icon==3){list->AddTriangleFilled(point(4,1),point(18,10),point(4,19),color);}
  else {line(17,10,2,10);line(2,10,8,4);line(2,10,8,16);}
  const char* text=label;const char* end=nullptr;FteBroomTranslate(text,end);
  list->AddText({p.x+38*u,p.y+(34*u-ImGui::GetFontSize())*.5f},color,text,end);
  return clicked;
}
static void buildToolbar(bool inlineTools=false)
{
    if(inlineTools) {
      const float width=compileButtonWidth("Compile");
      const float right=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x;
      if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+width<=right)ImGui::SameLine();
    }
    bool preview=cvars->GetFloat("ftebroom_bsp_preview")!=0;
    if(modEmbedded){ImGui::TextUnformatted("Mod preview");ImGui::SameLine();
      if(ImGui::RadioButton("Free camera",!modPlay))modSetPlay(false);ImGui::SameLine();
      if(ImGui::RadioButton("Play mod",modPlay))modSetPlay(true);
      ImGui::SameLine();if(ImGui::Button("Back to editing"))modEmbedEnd();
      ImGui::TextWrapped(modPlay?(modCaptured?"Playing | Escape: release mouse":"Click viewport to play | Escape: release mouse"):"RMB: look | WASD: fly | Q/X: up/down | Shift: fast");
      return;}
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{14*uiScale(),9*uiScale()});
    const float right=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x;
    auto next=[&](const char* label){
      const float width=compileButtonWidth(label);
      if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+width<=right)ImGui::SameLine();
    };
    ImGui::BeginDisabled(runActive||buildRunning);
    ImGui::PushStyleColor(ImGuiCol_Button,uiColor(47,99,160));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,uiColor(58,122,195));
    ImGui::PushStyleColor(ImGuiCol_Text,uiColor(255,255,255));
    if(compileButton("Compile",0))buildBegin();
    ImGui::PopStyleColor(3);ImGui::EndDisabled();next("Setup compile");
    if(compileButton("Setup compile",1))buildWindow=true;
    next(preview?"Back to editing":"Show BSP");
    if(preview){if(compileButton("Back to editing",4))buildEdit();}
    else {ImGui::BeginDisabled(buildResult.empty()||(buildResultDocument!=documentFile || buildResultProfile!=gameProfile)||buildRunning);
      if(compileButton("Show BSP",2))buildShow();ImGui::EndDisabled();}
    gameProfileLoad();
    if(gameProfile!=0){next("Preview mod");ImGui::BeginDisabled(runActive||buildRunning||buildResult.empty()||(buildResultDocument!=documentFile || buildResultProfile!=gameProfile));
      if(compileButton("Preview mod",3))modPreview();ImGui::EndDisabled();}
    ImGui::PopStyleVar();
}
static void buildOverlay()
{
  if(!buildRunning && !modLoading && buildMessage.empty())return;
  int pane=-1;for(int i=0;i<viewLayout;i++)if(viewRects[i].type==0&&viewRects[i].w>0){pane=i;break;}
  if(pane<0)return;auto r=viewRects[pane];float width=std::max(120.f,std::min(r.w-16,600.f*uiScale()));
  ImGui::SetNextWindowPos({r.x+r.w*.5f,r.y+12*uiScale()},ImGuiCond_Always,{.5f,0});ImGui::SetNextWindowSize({width,0});ImGui::SetNextWindowBgAlpha(.97f);
  if(ImGui::Begin("##Build viewport status",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoDocking|ImGuiWindowFlags_NoNav)){
    if(modLoading){
      ImGui::TextUnformatted("Loading mod preview...");
      ImGui::TextDisabled("Preparing the engine, map and game resources");
      ImGui::ProgressBar(-float(ImGui::GetTime()),{-1,8*uiScale()},"");
      if(ImGui::Button("Cancel"))modEmbedEnd();
    }else if(buildRunning){int step=std::min(runStep,(int)buildStages.size()-1);
      ImGui::Text("%s | %.0f s",buildStages[step].c_str(),ImGui::GetTime()-buildStarted);
      ImGui::ProgressBar(float(runStep)/float(buildStages.size()),{-1,6},"");
      ImGui::TextDisabled("%d / %d stages complete (not a time estimate)",runStep,(int)buildStages.size());
      auto end=runLog.find_last_not_of("\r\n");if(end!=std::string::npos){auto start=runLog.find_last_of("\r\n",end);auto line=runLog.substr(start==std::string::npos?0:start+1,end-(start==std::string::npos?0:start+1)+1);ImGui::TextWrapped("%s",line.substr(0,240).c_str());}
      if(ImGui::SmallButton("Cancel build"))runStop();ImGui::SameLine();if(ImGui::SmallButton("Log"))runWindow=true;
    }else if(!buildMessage.empty()){
      if(documentFile.empty())ImGui::TextColored({1,.75f,.25f,1},"Map not saved");
      if(runHadError)ImGui::PushStyleColor(ImGuiCol_Text,{1,.35f,.3f,1});
      ImGui::TextWrapped("%s",buildMessage.c_str());if(runHadError)ImGui::PopStyleColor();
      if(runHadError && ImGui::SmallButton("Show error log"))runWindow=true;
      if(ImGui::SmallButton("Dismiss"))buildMessage.clear();
    }
    if(!buildResult.empty()&&buildResultDocument==documentFile&&buildResultRevision!=(int)cvars->GetFloat("ftebroom_revision"))
      ImGui::TextColored({1,.75f,.25f,1},"BSP is from an earlier document revision.");
  }ImGui::End();
}
static void testBuild()
{
  if(!cvars->GetFloat("ftebroom_skip_setup"))return;
  char arg[32]={};commands->Argv(1,arg,sizeof arg);int stage=std::atoi(arg);
  auto expect=[](bool ok,const char* name){core->Print((std::string(ok?"FTEBROOM_BUILD_PASS ":"FTEBROOM_BUILD_FAIL ")+name+"\n").c_str());};
  if(stage==73){expect(!cvars->GetFloat("ftebroom_bsp_preview")&&!buildWasPreview,"escape_returns_to_map");}
#ifdef _WIN32
  if(stage==71){
    RECT host{},child{},position{};GetClientRect(modHost,&host);GetClientRect(modChild,&child);GetWindowRect(modChild,&position);
    POINT corner{position.left,position.top};ScreenToClient(modHost,&corner);
    auto display=ImGui::GetIO().DisplaySize;auto r=viewRects[0];
    float sx=host.right/std::max(1.f,display.x),sy=host.bottom/std::max(1.f,display.y);
    expect(modChild && std::abs(child.right-int(r.w*sx))<=1 && std::abs(child.bottom-int(r.h*sy))<=1 && std::abs(corner.x-int(r.x*sx))<=1 && std::abs(corner.y-int(r.y*sy))<=1,"preview_fills_viewport");
  }
  if(stage==72){SetWindowPos(modHost,nullptr,0,0,1120,760,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);}
  if(stage==30){expect(modEmbedded && !modLoading && modChild && GetParent(modChild)==modHost && runLog.find("ftebroom_preview_ready")!=std::string::npos,"embedded_child");expect(runLog.find("ftebroom_camera_ack")!=std::string::npos,"camera_control_ack");}
  if(stage==31){command("ftebroom_camera_move 32 -160 96\n");}
  if(stage==34){modSetPlay(true);modInputQueue+="bind w +forward\n";modQueueKey(input->GetKeyCode("w",nullptr),0,true);}
  if(stage==35){modReleaseInput();expect(runLog.find("ftebroom_preview_player ")!=std::string::npos,"runtime_player_camera");}
  if(stage==36){std::ofstream(documentPath("embedded-play.log"))<<runLog;modSetPlay(false);expect(!modCaptured,"play_input_released");}
  if(stage==37){expect(modEmbedded && modPlay && modCaptured && sceneLook,"play_click_capture");}
  if(stage==38){expect(modEmbedded && modPlay && !modCaptured && !sceneLook && modInputQueue=="ftebroom_preview_input reset\n","play_escape_releases_without_stopping");}
  if(stage==32){std::ofstream(documentPath("embedded-runtime.log"))<<runLog;expect(modEmbedded && runLog.find("ftebroom_preview_applied 32 -160 96")!=std::string::npos,"runtime_free_camera");modEmbedEnd();}
  if(stage==33){expect(!modEmbedded&&!runActive,"embedded_stopped");char state[512];cvars->GetString("ftebroom_camera_state",state,sizeof state);float a[7],b[7];expect(cameraParse(state,a)&&cameraParse(modSavedCamera,b)&&std::abs(a[0]-b[0])<.01&&std::abs(a[1]-b[1])<.01&&std::abs(a[2]-b[2])<.01,"editor_camera_restored");}
#endif
  if(stage==20){
    gameProfileLoad();gameProfile=1;gamePreviewAfterBuild=cvars->GetFloat("ftebroom_test_autopreview")!=0;
    cvars->GetString("ftebroom_test_mod",gameMod,sizeof gameMod);
    cvars->GetString("ftebroom_test_engine",gameEngine,sizeof gameEngine);
    expect(gameProfileSave(),"profile_saved");
    try{modValidate();expect(true,"mod_validated");}catch(...){expect(false,"mod_validated");}
    command("ftebroom_definitions\n");
  }
  if(stage==21){modPreview();expect(runActive,"mod_process_requested");}
  if(stage==22){expect(runActive && !runHadError,"mod_process_alive");runStop();}
  if(stage==23){setupNeeded=true;}
  if(stage==24){gameProfileLoad();expect(gameProfile==1,"profile_restored");expect(gamePreviewAfterBuild==(cvars->GetFloat("ftebroom_test_autopreview")!=0),"profile_option_restored");}
  if(stage==55){openRequest=true;}
  if(stage==56){expect(fileBrowserTestItems.count("room.map")&&fileBrowserTestItems.count("up"),"pak_browser_navigation");}
  if(stage==50){char archive[4096]={};cvars->GetString("ftebroom_test_archive",archive,sizeof archive);documentRequest("@"+(std::filesystem::u8path(archive)/"maps/room.map").u8string());}
  if(stage==51){expect(documentFile.generic_u8string().find("pak2.pak/maps/room.map")!=std::string::npos,"pak_open_document");expect(materials.count("textures/q1test")&&q1MaterialWad.count("fb_grey"),"pak_wad_catalog");documentSave(documentName);}
  if(stage==52){expect(documentError.empty()&&ftebroom::pak::read(documentFile).find("\"wad\"")!=std::string::npos,"pak_saved_map");gamePreviewAfterBuild=false;buildGeometry=buildVis=buildLight=true;buildBegin();expect(buildRunning,"pak_build_started");}
  if(stage==53){expect(!buildRunning&&!runHadError&&!buildResult.empty(),"pak_build_complete");expect(entityDefinitions.count("monster_ogre")&&entityDefinitions.count("info_player_start"),"quake_entity_profile");expect(!entityDefinitions.count("weapon_railgun"),"no_quake3_entities");selectEntityTab=true;cvars->SetFloat("ftebroom_test_model_previews",1);cvars->SetString("ftebroom_test_entity_filter","monster_ogre");}
  if(stage==54){bool shown=false;for(const auto& p:entityPreviews)shown|=p.second.image!=0;expect(shown,"mod_model_thumbnail");expect(q1UseLibre&&definitionSource=="librequake.def","librequake_definitions");}
  if(stage==40){
    gameProfileLoad();gamePreviewAfterBuild=false;runLoad();buildLoadSettings();
    documentFile=documentPath("maps/build-fixture.map");
    for(auto& args:buildArguments)args.clear();buildGeometry=buildVis=buildLight=true;
    buildBegin();expect(gameProfile==2 && buildRunning,"quake1_background_started");
    expect(materials.count("textures/q1test")!=0 && entityDefinitions.count("monster_ogre")!=0,"quake1_assets_entities");
    std::ofstream(documentPath("quake1-status.txt"))<<assetError<<"\n"<<definitionError<<"\n"<<buildMessage;
  }
  if(stage==41){expect(!buildRunning&&!runHadError&&!buildResult.empty(),"quake1_stages_complete");
    std::ofstream(documentPath("quake1-build.log"))<<runLog<<"\n"<<buildMessage;
    if(!buildResult.empty()){expect(buildValidBsp(buildResult),"quake1_bsp_valid");buildShow();}}
  if(stage==42){expect(cvars->GetFloat("ftebroom_bsp_preview")!=0,"quake1_bsp_visible");buildEdit();}
  if(stage==43){buildGeometry=false;buildVis=false;buildLight=true;buildBegin();expect(buildRunning,"quake1_light_only_started");}
  if(stage==44){expect(!buildRunning&&!runHadError,"quake1_light_only_complete");q1BspFormat=1;buildGeometry=buildVis=buildLight=true;buildBegin();}
  if(stage==45){expect(!buildRunning&&!runHadError&&!buildResult.empty()&&buildRead(buildResult).substr(0,4)=="BSP2","quake1_bsp2");documentSave("quake1-roundtrip",false,documentPath("maps/quake1-roundtrip.map"));}
  if(stage==0){char compiler[4096]={};cvars->GetString("ftebroom_test_compiler",compiler,sizeof compiler);
    runLoad();if(*compiler)runVariables["q3map2"]=compiler;documentFile=documentPath("maps/build-fixture.map");
    buildArguments[0]="-v";buildArguments[2]="-fast -samples 2";
    buildGeometry=buildVis=buildLight=true;buildBegin();expect(buildRunning,"background_started");}
  if(stage==1){expect(!buildRunning&&!runHadError&&!buildResult.empty(),"all_stages_completed");
    expect(cvars->GetFloat("ftebroom_bsp_preview")!=0 && !modEmbedded,"automatic_bsp_preview_without_mod");
    if(!buildResult.empty()){expect(buildValidBsp(buildResult),"valid_bsp");buildShow();}}
  if(stage==2){expect(cvars->GetFloat("ftebroom_bsp_preview")!=0,"preview_active");
    expect(cvars->GetFloat("ftebroom_bsp_entities")==1,"compiled_brush_entity");
    expect(cvars->GetFloat("r_fullbright")==0,"lightmaps_enabled");command("mod_terrain_save \"\" build-direct-save\n");}
  if(stage==3){expect(!cvars->GetFloat("ftebroom_bsp_preview"),"editing_restored");
    expect(cvars->GetFloat("r_fullbright")==1,"editor_lighting_restored");
    buildGeometry=false;buildVis=false;buildLight=true;buildBegin();expect(buildRunning,"light_only_started");}
  if(stage==4){expect(!buildRunning&&!runHadError&&buildStages.size()==2,"light_only_completed");buildShow();}
  if(stage==5){buildEdit();buildGeometry=buildVis=buildLight=false;buildBegin();expect(!buildRunning&&buildMessage=="Select at least one build stage.","empty_stages_rejected");}
  static std::filesystem::path previousResult;static std::string savedCompiler;
  if(stage==6){previousResult=buildResult;savedCompiler=runVariables["q3map2"];runVariables["q3map2"]=documentPath("missing-compiler.exe").u8string();buildGeometry=true;buildBegin();}
  if(stage==7){expect(runHadError&&!buildRunning,"compiler_failure_reported");expect(buildResult==previousResult,"failed_build_retains_result");runVariables["q3map2"]=savedCompiler;}
  if(stage==8){buildBegin();runStop();}
  if(stage==9){expect(!buildRunning&&buildMessage.find("cancelled")!=std::string::npos,"cancellation_reported");expect(buildResult==previousResult,"cancel_retains_result");}
  if(stage==10){buildGeometry=false;buildLight=true;buildBegin();}
  if(stage==11){expect(runHadError&&!buildRunning&&runError.find("Geometry changed")!=std::string::npos,"changed_snapshot_rejected");expect(buildResult==previousResult,"changed_snapshot_retains_result");}
}
