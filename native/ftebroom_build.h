// SPDX-License-Identifier: GPL-2.0-or-later
// Build jobs use immutable snapshots beside the source map; previews use a local cache.
static bool buildWindow,buildRunning,buildGeometry=true,buildVis=true,buildLight=true;
static bool buildWasPreview;
static bool gpuBakeRunning=false;
static bool mapGpuRunning=false,mapGpuSnapshotPending=false;
static double mapGpuSnapshotStarted=0,mapGpuSnapshotPoll=0;
static int mapGpuRevision=-1,mapGpuGeneration=0;
static std::filesystem::path mapGpuFolder,mapGpuDocument,mapGpuResult,mapGpuResultDocument;
static int mapGpuResultRevision=-1,mapGpuResultGeneration=-1,mapGpuResultProfile=-1;
static std::string mapGpuMessage;
static std::filesystem::path gpuBakeOutput,gpuBakeDocument;
static std::string buildMessage,buildResultModel,buildPendingModel;
static std::filesystem::path buildResult,buildPending,buildDocument,buildResultDocument;
static int buildRevision,buildResultRevision,buildResultProfile=-1;
static std::string buildSourceMap,buildResultSourceMap;
static bool modCompilePending=false;
static int buildContentRevision=-1,buildResultContentRevision=-1;
static uint64_t buildGeneration=0,buildResultGeneration=0;
static int buildCurrentContentRevision(){
  std::string value;int revision=-1;
  if(qcQuery(4,0,value)&&std::sscanf(value.c_str(),"RBMAP1 %d",&revision)==1)return revision;
  return (int)cvars->GetFloat("ftebroom_revision");
}
static int traceRevisionNow(){return buildCurrentContentRevision();}
static void traceEditingView();
static bool buildLeakFound=false;
static void buildDetectLeak(){
 if(!buildGeometry||buildDocument!=documentFile||buildGeneration!=documentGeneration)return;
 auto path=ftebroom::leak::find(buildPending);if(path.empty()||(buildLeakFound&&tracePaths[0]==path))return;
 if(traceChoose(1,path)){traceRevision=buildContentRevision;traceGeneration=buildGeneration;buildLeakFound=true;
  runAppend("Leak route detected: "+path.u8string()+". Use Leak route to inspect the path.\n");}
}
static double buildStarted;
static int buildSavedLayout,buildSavedFocus,buildSavedMax,buildSavedTypes[4];
static std::vector<std::string> buildStages;
static std::string buildArguments[3],buildCsgArguments;
static std::string buildSettingsFile(){return gameProfile==4?"reyondbroom-halflife-build-options.txt":gameProfile==3?"reyondbroom-quake2-build-options.txt":gameProfile==2?"ftebroom-quake1-build-options.txt":"ftebroom-build-options.txt";}
static void buildLoadSettings()
{
  static int loaded=-1;int profile=gameProfile>=2?gameProfile:0;if(loaded==profile)return;loaded=profile;
  buildGeometry=buildVis=buildLight=true;for(auto& arg:buildArguments)arg.clear();buildCsgArguments.clear();
  std::ifstream settings(documentPath(buildSettingsFile()));int geometry,vis,light;
  if(settings>>geometry>>vis>>light){buildGeometry=geometry!=0;buildVis=vis!=0;buildLight=light!=0;}
  for(auto& argument:buildArguments)settings>>std::quoted(argument);if(gameProfile==4)settings>>std::quoted(buildCsgArguments);
}
static std::string buildRead(const std::filesystem::path& path)
{
  std::ifstream file(path,std::ios::binary);
  if(!file)throw std::runtime_error("Cannot read build snapshot: "+path.u8string());
  return {std::istreambuf_iterator<char>(file),{}};
}
static bool buildValidBsp(const std::filesystem::path& path)
{
  auto bytes=buildRead(path);return gameProfile>=3?ftebroom::quake1::gameBsp(bytes,gameProfile):ftebroom::quake1::bsp(bytes,gameProfile==2);
}
static std::filesystem::path buildHistoryFile(){
  return ftebroom::pak::diskFolder(documentFile.parent_path())/"_ftebroom-build"/(backupStorageName()+"-"+gameProfileKey(gameProfile)+".last-build");
}
static void buildSaveHistory(){
  try{auto file=buildHistoryFile();std::filesystem::create_directories(file.parent_path());std::ofstream output(file);
    output<<gameProfile<<' '<<std::quoted(documentFile.generic_u8string())<<' '<<std::quoted(buildResult.generic_u8string())<<'\n';
  }catch(const std::exception& e){core->Print((std::string("ReyondBroom: cannot save build history: ")+e.what()+"\n").c_str());}
}
static void buildRestoreHistory(){
  if(documentFile.empty()||(!buildResult.empty()&&buildResultDocument==documentFile&&buildResultProfile==gameProfile))return;
  try{std::ifstream input(buildHistoryFile());int profile=-1;std::string document,result;
    if(!(input>>profile>>std::quoted(document)>>std::quoted(result))||profile!=gameProfile||document!=documentFile.generic_u8string())return;
    auto path=std::filesystem::u8path(result);if(!std::filesystem::is_regular_file(path)||!buildValidBsp(path))return;
    buildResult=path;buildResultDocument=documentFile;buildResultProfile=profile;buildResultRevision=-1;buildResultContentRevision=-1;buildResultGeneration=0;buildResultSourceMap.clear();
    buildResultModel="maps/ftebroom-build/"+path.parent_path().filename().u8string()+"/"+path.filename().u8string();
  }catch(const std::exception& e){core->Print((std::string("ReyondBroom: cannot restore build history: ")+e.what()+"\n").c_str());}
}
static void buildExported(const std::filesystem::path& path)
{
  if(documentFile!=buildDocument)throw std::runtime_error("The active map changed during export. Start Build again for the current map.");
  buildRevision=(int)cvars->GetFloat("ftebroom_revision");
  buildContentRevision=buildCurrentContentRevision();buildGeneration=documentGeneration;
  char loaded[512]{};cvars->GetString("mapname",loaded,sizeof loaded);buildSourceMap=loaded;
  runAppend("Exported current editor contents from: "+buildDocument.u8string()+"\nBuild copy: "+path.u8string()+"\nRevision: "+std::to_string(buildRevision)+"\n");
  if(!buildGeometry && buildRead(path)!=buildRead(buildResult.parent_path()/"snapshot.map"))
    throw std::runtime_error("Geometry changed. Enable Geometry/BSP before rebuilding VIS or LIGHT.");
}
static void buildStageFinished()
{
  buildDetectLeak();
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
static void traceEditingView(){if(cvars->GetFloat("ftebroom_bsp_preview"))buildEdit();}
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
    // World materials register lazily on their first rendered frame.
    if(!buildPlaceholderHandles.empty())command("in .2 ftebroom_build_placeholders\n");
  }catch(const std::exception& error){buildMessage=error.what();buildRestoreLayout();}
}
static void mapGpuShow(){
  if(mapGpuResult.empty()||mapGpuResultDocument!=documentFile||mapGpuResultProfile!=gameProfile)return;
  if(mapGpuResultRevision!=buildCurrentContentRevision()||mapGpuResultGeneration!=(int)documentGeneration){mapGpuMessage="The map changed. Bake MAP lighting again before previewing.";return;}
  try{
    auto cache=documentPath("maps/ftebroom-build/"+mapGpuResult.parent_path().filename().u8string());std::filesystem::create_directories(cache);
    std::filesystem::copy_file(mapGpuResult,cache/"snapshot.map",std::filesystem::copy_options::overwrite_existing);
  }catch(const std::exception& e){mapGpuMessage=e.what();return;}
  auto saved=buildResult;auto savedModel=buildResultModel;auto savedDocument=buildResultDocument;int savedProfile=buildResultProfile;
  buildResult=mapGpuResult;buildResultDocument=documentFile;buildResultProfile=gameProfile;
  buildResultModel="maps/ftebroom-build/"+mapGpuResult.parent_path().filename().u8string()+"/snapshot.map";
  buildShow();buildResult=saved;buildResultModel=savedModel;buildResultDocument=savedDocument;buildResultProfile=savedProfile;
}
static std::string unityTexture(const MaterialEntry& material);
static void mapGpuStageAssets(const std::filesystem::path& target){
  if(documentFile!=mapGpuDocument)throw std::runtime_error("The active map changed during export.");
  std::filesystem::copy_file(documentPath("data/gpu-map.scene"),target.parent_path()/"scene.rbgs");
  std::ofstream manifest(target.parent_path()/"materials.txt");
  std::filesystem::create_directories(target.parent_path()/"materials");int index=0;
  std::set<std::string> used;auto scene=q1Read(target.parent_path()/"scene.rbgs");size_t at=12;
  while(at<scene.size()){auto n=ftebroom::quake1::word(scene,at+8),length=ftebroom::quake1::word(scene,at+80);ftebroom::quake1::range(scene,at+84,length+uint64_t(n)*12);used.insert(scene.substr(at+84,length));at+=84+length+size_t(n)*12;}
  for(const auto& pair:materials){const auto& m=pair.second;if(m.preview.empty()||(!used.count(m.name)&&!used.count(m.name.rfind("textures/",0)==0?m.name.substr(9):"textures/"+m.name)))continue;
    auto tga=unityTexture(m);if(tga.empty())continue;
    auto path=target.parent_path()/"materials"/(std::to_string(index++)+".tga");
    std::ofstream image(path,std::ios::binary);image.write(tga.data(),tga.size());if(!image)throw std::runtime_error("Cannot stage GPU material.");
    manifest<<std::quoted(m.name)<<' '<<std::quoted(path.generic_u8string())<<' '<<(m.masked||m.name.rfind("textures/{",0)==0||m.name.rfind("{",0)==0)<<' '<<m.opacity<<'\n';
  }
  if(!manifest)throw std::runtime_error("Cannot write GPU material manifest.");
  mapGpuRevision=buildCurrentContentRevision();mapGpuGeneration=(int)documentGeneration;
}
static void mapGpuExported(const std::filesystem::path& target){
  if(documentFile!=mapGpuDocument)throw std::runtime_error("The active map changed during export.");
  auto relative="maps/ftebroom-build/"+mapGpuFolder.filename().u8string()+"/snapshot.map";
  std::filesystem::create_directories(documentPath(relative).parent_path());std::filesystem::copy_file(target,documentPath(relative));
  cvars->SetFloat("ftebroom_gpu_snapshot_status",0);cvars->SetString("ftebroom_gpu_snapshot_path","");mapGpuSnapshotPending=true;mapGpuSnapshotStarted=ImGui::GetTime();mapGpuSnapshotPoll=mapGpuSnapshotStarted;
  command(("ftebroom_gpu_snapshot "+relative+"\n").c_str());
}
static bool mapGpuReady(){
  if(!mapGpuSnapshotPending)return true;
  int status=(int)cvars->GetFloat("ftebroom_gpu_snapshot_status");
  char snapshotPath[512]{};cvars->GetString("ftebroom_gpu_snapshot_path",snapshotPath,sizeof snapshotPath);
  bool current=std::string(snapshotPath)=="maps/ftebroom-build/"+mapGpuFolder.filename().u8string()+"/snapshot.map";
  if(status==0||!current){if(ImGui::GetTime()-mapGpuSnapshotStarted>120)throw std::runtime_error("GPU MAP snapshot preparation timed out.");
    if(ImGui::GetTime()-mapGpuSnapshotPoll>.2){mapGpuSnapshotPoll=ImGui::GetTime();command(("ftebroom_gpu_snapshot maps/ftebroom-build/"+mapGpuFolder.filename().u8string()+"/snapshot.map\n").c_str());}return false;}
  if(status<0)throw std::runtime_error("Cannot extract brush lighting from the MAP snapshot; patches are not supported.");
  mapGpuStageAssets(mapGpuFolder/"snapshot.map");mapGpuSnapshotPending=false;return true;
}
static void mapGpuBegin(int rays, int samples=4){
  if(runActive||buildRunning||mapGpuRunning)return;
  try{
    if(assetsLoading)throw std::runtime_error("Wait for game resources to finish loading before baking.");
    if(gameProfile!=2)throw std::runtime_error("MAP GPU lighting currently supports Quake 1.");
    if(documentFile.empty())throw std::runtime_error("Save the map once before baking MAP lighting.");
    if(cvars->GetFloat("ftebroom_bsp_preview"))buildEdit();
    auto stamp=std::chrono::system_clock::now().time_since_epoch().count();mapGpuFolder=ftebroom::pak::diskFolder(documentFile.parent_path())/"_ftebroom-build"/("map-gpu-"+std::to_string(stamp));std::filesystem::create_directories(mapGpuFolder);mapGpuDocument=documentFile;
    auto helper=documentPath("ftebroom.cfg").parent_path().parent_path()/"tools/gpu-light";
#ifdef _WIN32
    helper/="gpu-light.exe";
#else
    helper/="gpu-light";
#endif
    std::ostringstream args;args<<"--map "<<std::quoted((mapGpuFolder/"scene.rbgs").generic_u8string())<<' '<<std::quoted((mapGpuFolder/"snapshot.map").generic_u8string())<<' '<<std::quoted((mapGpuFolder/"snapshot.rblm").generic_u8string())<<' '<<rays<<' '<<std::quoted((mapGpuFolder/"materials.txt").generic_u8string())<<' '<<samples;
    runReadyHook=mapGpuReady;mapGpuSnapshotPending=false;runExportHook=mapGpuExported;runStageHook=nullptr;
    runBegin({"GPU lighting directly on MAP (no BSP)",mapGpuFolder.u8string(),false,{{0,"${WORK_DIR_PATH}/snapshot.map",""},{1,helper.u8string(),args.str()}}});
    mapGpuRunning=runActive;gameProfileLocked=runActive;runWindow=false;mapGpuMessage="Exporting MAP polygons and materials...";
  }catch(const std::exception& e){mapGpuMessage=e.what();runReadyHook=nullptr;mapGpuSnapshotPending=false;runExportHook=nullptr;}
}
static std::string modQuote(const std::string& text);
static void modValidate();
static void modPreview();
static void buildBegin()
{
  if(documentTerrain){buildMessage="FTE Engine terrain uses MAP plus section files; BSP compilation is unavailable for this document.";return;}
  if(runActive || runExclusive || buildRunning)return;
  if(documentFile.empty()){buildMessage="Save the map once before building.";return;}
  if(assetsLoading){buildMessage="Wait for game resources to finish loading before building.";return;}
  buildLeakFound=false;buildMessage.clear();runLoad();buildLoadSettings();gameProfileLoad();
  if(gameProfile==1 && gamePreviewAfterBuild){try{modValidate();}catch(const std::exception& e){buildMessage=e.what();return;}}
  std::error_code projectError;
  if(!*baseq3 || !std::filesystem::is_directory(std::filesystem::u8path(baseq3),projectError)){buildMessage="Configure an existing game resource folder in Game / resources.";setupNeeded=true;return;}
  if(documentFile.empty()){buildMessage="Save the map once before building.";return;}
  if(!buildGeometry&&!buildVis&&!buildLight){buildMessage="Select at least one build stage.";return;}
  if(gameProfile<2 && runVariables["q3map2"].empty()){buildMessage="Set the q3map2 executable path.";return;}
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
    if(gameProfile==3){
      auto q2base=std::filesystem::u8path(q2Mod).parent_path()/"baseq2";if(!std::filesystem::is_directory(q2base))q2base=std::filesystem::u8path(q2Mod);
      auto paths="-basedir "+modQuote(q2base.generic_u8string())+" -gamedir "+modQuote(std::filesystem::u8path(q2Mod).generic_u8string())+" -path \"${WORK_DIR_PATH}\" ";
      const char* names[]={"q2bsp","q2vis","q2light"};bool enabled[]={buildGeometry,buildVis,buildLight};
      for(int i=0;i<3;i++)if(enabled[i]){
        if(runVariables[names[i]].empty())throw std::runtime_error(std::string("Configure ")+names[i]+" in Setup compile.");
        profile.tasks.push_back({1,std::string("${")+names[i]+"}",paths+(i==0?"-q2bsp ":"")+buildArguments[i]+" \"${WORK_DIR_PATH}/snapshot."+(i==0?"map":"bsp")+"\""});buildStages.push_back(names[i]);
      }
    }else if(gameProfile==4){
      const char* names[]={"hlcsg","hlbsp","hlvis","hlrad"};
      for(int i=0;i<4;i++)if(i<2?buildGeometry:i==2?buildVis:buildLight){
        if(runVariables[names[i]].empty())throw std::runtime_error(std::string("Configure ")+names[i]+" in Setup compile.");
        profile.tasks.push_back({1,std::string("${")+names[i]+"}",(i==0?std::string("-nowadtextures "):std::string())+(i==0?buildCsgArguments:buildArguments[i-1])+" \"${WORK_DIR_PATH}/snapshot.map\""});buildStages.push_back(names[i]);
      }
    }else if(gameProfile==2) {
      const char* names[]={"qbsp","vis","light"};bool enabled[]={buildGeometry,buildVis,buildLight};
      for(int i=0;i<3;i++)if(enabled[i]){
        if(runVariables[names[i]].empty())throw std::runtime_error(std::string("Set the ")+names[i]+" executable path.");
        std::string args=(i==0 && q1BspFormat==1?"-bsp2 ":"")+buildArguments[i]+" \"${WORK_DIR_PATH}/snapshot."+(i==0?"map":"bsp")+"\"";
        profile.tasks.push_back({1,std::string("${")+names[i]+"}",args});buildStages.push_back(names[i]);
      }
    } else {
    common+="-fs_pakpath \"${WORK_DIR_PATH}\" ";
    if(buildGeometry){profile.tasks.push_back({1,"${q3map2}",common+"-meta "+buildArguments[0]+" \"${WORK_DIR_PATH}/snapshot.map\""});buildStages.push_back("Geometry / BSP");}
    if(buildVis){profile.tasks.push_back({1,"${q3map2}",common+"-vis "+buildArguments[1]+" \"${WORK_DIR_PATH}/snapshot.bsp\""});buildStages.push_back("Visibility / VIS");}
    if(buildLight){profile.tasks.push_back({1,"${q3map2}",common+"-light "+buildArguments[2]+" \"${WORK_DIR_PATH}/snapshot.bsp\""});buildStages.push_back("Lighting / LIGHT");}
    }
    runExportHook=buildExported;runStageHook=buildStageFinished;runBegin(profile);runWindow=false;buildRunning=runActive;gameProfileLocked=buildRunning;buildStarted=ImGui::GetTime();
    if(!buildRunning){runExportHook=nullptr;runStageHook=nullptr;buildMessage=runError;}
    runSaveProfiles();
    {std::ofstream settings(documentPath(buildSettingsFile()));settings<<buildGeometry<<' '<<buildVis<<' '<<buildLight;for(auto& argument:buildArguments)settings<<' '<<std::quoted(argument);if(gameProfile==4)settings<<' '<<std::quoted(buildCsgArguments);}
  }catch(const std::exception& error){buildMessage=error.what();runExportHook=nullptr;runStageHook=nullptr;}
}
static void mapGpuTestTick(){
  if(!cvars->GetFloat("ftebroom_skip_setup")||!cvars->GetFloat("ftebroom_test_map_gpu_auto"))return;
  static int stage=0;static double since=0;
  if(stage==0&&!assetsLoading){char map[4096]{};cvars->GetString("ftebroom_test_map",map,sizeof map);if(!*map)return;recentWindow=false;documentRequest("@"+std::string(map));stage=1;since=ImGui::GetTime();}
  if(stage==1&&!assetsLoading&&!documentFile.empty()&&pendingDocumentFile.empty()&&ImGui::GetTime()-since>3){stage=2;mapGpuBegin((int)cvars->GetFloat("ftebroom_test_bounce_rays"));}
  if(stage==2&&!mapGpuRunning){stage=3;since=ImGui::GetTime();command("ftebroom_camera_restore 0 -200 110 5 90 0 90\n");std::ofstream result(documentPath("data/map-gpu-test-result.txt"));result<<mapGpuMessage<<'\n'<<mapGpuResult.u8string()<<'\n'<<runLog;core->Print(("MAP_GPU_TEST_RESULT "+mapGpuMessage+"\n").c_str());}
  if(stage==3&&ImGui::GetTime()-since>2){stage=4;command("screenshot map103-gpu.png\n");}
}
static void buildTick()
{
  mapGpuTestTick();
  if(buildWasPreview && !modEmbedded && !cvars->GetFloat("ftebroom_bsp_preview"))buildRestoreLayout();
  if(mapGpuRunning&&!runActive){
    {std::ofstream log(mapGpuFolder/"build.log");log<<runLog;}
    mapGpuRunning=false;gameProfileLocked=false;runReadyHook=nullptr;mapGpuSnapshotPending=false;runExportHook=nullptr;runStageHook=nullptr;
    if(runHadError||!std::filesystem::exists(mapGpuFolder/"snapshot.rblm"))mapGpuMessage=runHadError?runError:"MAP bake cancelled. Previous output retained.";
    else{mapGpuResult=mapGpuFolder/"snapshot.map";mapGpuResultDocument=mapGpuDocument;mapGpuResultRevision=mapGpuRevision;mapGpuResultGeneration=mapGpuGeneration;mapGpuResultProfile=gameProfile;mapGpuMessage="MAP GPU bake complete; no BSP was generated.";mapGpuShow();}
  }
  if(gpuBakeRunning&&!runActive){
    gpuBakeRunning=false;gameProfileLocked=false;
    if(gpuBakeDocument!=documentFile){buildMessage="Map changed during GPU bake; output saved without replacing the current preview.";}
    else if(runHadError||!std::filesystem::exists(gpuBakeOutput)){buildMessage=runHadError?runError:"GPU bake cancelled; previous BSP retained.";}
    else{buildResult=gpuBakeOutput;buildResultModel="maps/ftebroom-build/"+buildResult.parent_path().filename().u8string()+"/"+buildResult.filename().u8string();buildSaveHistory();buildMessage="GPU bake complete (experimental neutral-surface bounce).";buildShow();}
  }
  if(!buildRunning||runActive)return;
  buildDetectLeak();
  buildRunning=false;gameProfileLocked=false;runExportHook=nullptr;runStageHook=nullptr;
  bool playAfterBuild=modCompilePending;modCompilePending=false;
  {std::ofstream log(buildPending.parent_path()/"build.log");log<<runLog;}
  if(runHadError){buildMessage=runError;return;}
  if(runStep!=(int)buildStages.size()){buildMessage="Build cancelled. Previous successful BSP is retained.";return;}
  try {
    if(!buildValidBsp(buildPending))throw std::runtime_error("Compiler did not produce a valid BSP for the selected profile.");
    if(buildGeometry&&!buildLeakFound){traceAction("ftebroom_trace unloadpoints");command("ftebroom_trace unloadpoints\n");}
    buildResult=buildPending;buildResultModel=buildPendingModel;buildResultDocument=buildDocument;buildResultRevision=buildRevision;buildResultProfile=gameProfile;buildResultSourceMap=buildSourceMap;
    buildResultContentRevision=buildContentRevision;buildResultGeneration=buildGeneration;
    buildSaveHistory();
    buildMessage="Build complete. Ready to preview.";
    if(gameProfile!=0 && gameProfile!=3 && (gamePreviewAfterBuild||playAfterBuild))modPreview();
    else if(!buildLeakFound)buildShow();
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
    if(gameProfile!=0 && gameProfile!=3 && ImGui::Checkbox("Open mod preview after Build",&gamePreviewAfterBuild))gameProfileSave();
    if(gameProfile==2){for(const char* name:{"qbsp","vis","light"})runText(name,runVariables[name]);}
    else if(gameProfile==3){for(const char* name:{"q2bsp","q2vis","q2light"})runText(name,runVariables[name]);}
    else if(gameProfile==4){for(const char* name:{"hlcsg","hlbsp","hlvis","hlrad"})runText(name,runVariables[name]);}
    else runText("q3map2 executable",runVariables["q3map2"]);
    if(gameProfile==2){
      if(ImGui::Button("Fast preset")){buildGeometry=buildVis=buildLight=true;buildArguments[0]="";buildArguments[1]="-fast";buildArguments[2]="";}
      ImGui::SameLine();if(ImGui::Button("Final preset")){buildGeometry=buildVis=buildLight=true;buildArguments[0]="";buildArguments[1]="";buildArguments[2]="-extra4";}
    }
    ImGui::Checkbox("Geometry / BSP",&buildGeometry);ImGui::Checkbox("Visibility / VIS",&buildVis);ImGui::Checkbox("Lighting / LIGHT",&buildLight);
    ImGui::TextWrapped("VIS and LIGHT without Geometry reuse the last successful BSP for the identical exported map. Build files are written to _ftebroom-build beside the saved map. BSP preview uses a separate editor cache.");
    if(ImGui::CollapsingHeader("Advanced compiler arguments")) {
      if(gameProfile==4)runText("CSG arguments",buildCsgArguments);runText("BSP arguments",buildArguments[0]);runText("VIS arguments",buildArguments[1]);runText("LIGHT arguments",buildArguments[2]);
      ImGui::TextWrapped("Extra arguments before the input filename. Example LIGHT: -fast -samples 3 -bounce 2. Passed directly to the selected compiler, not a command shell.");
    }
    if(gameProfile==2&&ImGui::CollapsingHeader("GPU lighting on MAP (no BSP)")){
      static int mapRays=32;static int mapSamples=1;const char* quality[]={"Fast (1 sample)","Balanced (4 samples)","Fine (16 samples)"};ImGui::Combo("MAP shadow quality",&mapSamples,quality,3);ImGui::SliderInt("MAP bounce rays",&mapRays,0,64);
      ImGui::TextWrapped("Experimental: direct point lights and one diffuse bounce on brush MAP geometry. Uses a BVH, texture alpha masks and separate surface lightmaps. No QBSP, VIS or CPU LIGHT. Patches and moving objects are not yet supported. Point lights, target/mangle spotlights and target-based Quake sunlight are supported. Animated lightstyles bake at static brightness. The source MAP stays unchanged.");
      if(ImGui::Button("Bake MAP lighting"))mapGpuBegin(mapRays,mapSamples==0?1:mapSamples==1?4:16);
      ImGui::SameLine();if(ImGui::Button("Show baked MAP"))mapGpuShow();
      if(!mapGpuMessage.empty())ImGui::TextWrapped("%s",mapGpuMessage.c_str());
    }
    if(gameProfile==2&&ImGui::CollapsingHeader("Experimental GPU lighting")){
      static int rays=32;ImGui::SliderInt("One-bounce rays",&rays,0,64);
      ImGui::TextWrapped("Small static BSP29/BSP2 maps only. Compile Geometry first; VIS and LIGHT may be disabled. Neutral 55%% reflectance, ordinary point lights, no moving brush models. 0 rays = direct light only. Output is a separate BSP/LIT pair; existing files are retained.");
      ImGui::BeginDisabled(buildResult.empty()||buildResultDocument!=documentFile||buildResultProfile!=gameProfile);
      if(ImGui::Button("Bake lighting on GPU")){
        try{
          auto helper=documentPath("ftebroom.cfg").parent_path().parent_path()/"tools/gpu-light";
#ifdef _WIN32
          helper/="gpu-light.exe";
#else
          helper/="gpu-light";
#endif
          gpuBakeOutput=buildResult.parent_path()/("gpu-"+std::to_string(std::chrono::system_clock::now().time_since_epoch().count())+".bsp");
          std::ostringstream args;args<<std::quoted(buildResult.u8string())<<' '<<std::quoted(gpuBakeOutput.u8string())<<' '<<rays;
          runExportHook=nullptr;runStageHook=nullptr;
          runBegin({"Experimental GPU light bake",buildResult.parent_path().u8string(),false,{{1,helper.u8string(),args.str()}}});gpuBakeRunning=runActive;gpuBakeDocument=documentFile;gameProfileLocked=runActive;
        }catch(const std::exception& e){buildMessage=e.what();}
      }ImGui::EndDisabled();
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
    if(gameProfile!=0&&gameProfile!=3){next("Play mod");ImGui::BeginDisabled(runActive||runExclusive||buildRunning||assetsLoading);
      if(compileButton("Play mod",3))modPreview();ImGui::EndDisabled();}
    if(!tracePoints.empty()){next("Leak route");if(ImGui::Button("Leak route"))traceWindow=true;}
    ImGui::PopStyleVar();
}
static void buildOverlay()
{
  if(!buildRunning && !mapGpuRunning && !modLoading && buildMessage.empty())return;
  int pane=-1;for(int i=0;i<viewLayout;i++)if(viewRects[i].type==0&&viewRects[i].w>0){pane=i;break;}
  if(pane<0)return;auto r=viewRects[pane];float width=std::max(120.f,std::min(r.w-16,600.f*uiScale()));
  ImGui::SetNextWindowPos({r.x+r.w*.5f,r.y+12*uiScale()},ImGuiCond_Always,{.5f,0});ImGui::SetNextWindowSize({width,0});ImGui::SetNextWindowBgAlpha(.97f);
  if(ImGui::Begin("##Build viewport status",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoDocking|ImGuiWindowFlags_NoNav)){
    if(modLoading){
      ImGui::TextUnformatted("Loading mod preview...");
      ImGui::TextDisabled("Preparing the engine, map and game resources");
      ImGui::ProgressBar(-float(ImGui::GetTime()),{-1,8*uiScale()},"");
      if(ImGui::Button("Cancel"))modEmbedEnd();
    }else if(mapGpuRunning){
      ImGui::TextUnformatted("GPU lighting on MAP (no BSP)");
      auto progress=runLog.rfind("Progress ");int percent=-1;if(progress!=std::string::npos)std::sscanf(runLog.c_str()+progress,"Progress %d",&percent);
      ImGui::ProgressBar(percent<0?-float(ImGui::GetTime()):percent/100.f,{-1,8*uiScale()},percent<0?"Preparing polygons and materials":"GPU lighting");
      if(ImGui::SmallButton("Cancel bake"))runStop();ImGui::SameLine();if(ImGui::SmallButton("Log"))runWindow=true;
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

static void testLeakBuild(){
 if(!cvars->GetFloat("ftebroom_skip_setup"))return;
 char arg[4096]{};commands->Argv(1,arg,sizeof arg);
 auto expect=[](bool ok,const char* name){core->Print((std::string(ok?"LEAK111_PASS ":"LEAK111_FAIL ")+name+"\n").c_str());};
 if(std::string(arg)=="compile"){buildBegin();runMissingDecision=1;expect(buildRunning,"real_background_compile_started");return;}
 if(std::string(arg)=="compiled"){expect(!buildRunning&&!runHadError&&buildLeakFound&&traceWindow&&!tracePoints.empty(),"real_compile_automatically_loaded_route");expect(!cvars->GetFloat("ftebroom_bsp_preview"),"leaky_compile_stays_in_map_view");traceFocus(0);return;}
 if(std::string(arg)=="stale"){expect(traceRevision!=traceRevisionNow(),"map_edit_marks_route_outdated");return;}
 if(std::string(arg)=="focus"){char point[32]{};commands->Argv(2,point,sizeof point);traceFocus(std::atoi(point));return;}
 auto savedDocument=buildDocument,savedPending=buildPending;auto savedGeneration=buildGeneration;auto savedRevision=buildContentRevision;bool savedGeometry=buildGeometry,savedFound=buildLeakFound;
 buildDocument=documentFile;buildPending=std::filesystem::u8path(arg);buildGeneration=documentGeneration;buildContentRevision=traceRevisionNow();buildGeometry=true;buildLeakFound=false;
 buildStageFinished();expect(buildLeakFound&&!tracePoints.empty()&&traceWindow,"stage_hook_detects_compiler_route");
 expect(traceDocument==documentFile&&traceGeneration==documentGeneration&&traceRevision==traceRevisionNow(),"route_bound_to_source_revision");
 traceFocus(0);expect(tracePointIndex==0,"focus_first_point");
 buildDocument=savedDocument;buildPending=savedPending;buildGeneration=savedGeneration;buildContentRevision=savedRevision;buildGeometry=savedGeometry;buildLeakFound=savedFound;
}
