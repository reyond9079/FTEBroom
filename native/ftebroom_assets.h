// SPDX-License-Identifier: GPL-2.0-or-later
// Resource discovery uses FTE's mounted filesystem, including PK3 precedence.
#include <cctype>
#include "ftebroom_shader_preview.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <set>

static plugfsfuncs_t* files;
static char baseq3[1024] = "";
static bool setupNeeded = true, setupInitialized = false, assetsLoading = false;
static int firstSetupStep=-1;
static void uiSaveAppearance();
static std::string assetError;
struct MaterialEntry
{
  std::string name, preview;
  bool shader = false;
  int tags = 0;
  float opacity = 1;
  bool masked = false;
  std::string blend,alphaTest;
  bool looseWal=false;
  std::string source;
};
static std::map<std::string, MaterialEntry> materials;
static std::set<std::string> resourceFiles, resourceDirs;
static int shaderFiles = 0;
static std::string selectedMaterial;
static void resetMaterialBrowser();
static void clearEntityPreviews();
static void publishMaterialTags();
static std::filesystem::path settingsPath()
{
  char path[4096] = {};
  if (!files->NativePath("ftebroom-project.txt", (fs_relative)2, path, sizeof path))
    return {};
  return std::filesystem::u8path(path);
}
#include "ftebroom_game_profile.h"
#include "ftebroom_halflife_runtime.h"
static std::string token(const std::string& source,size_t& offset){return ftebroom::shaderpreview::token(source,offset);}
static int QDECL
collectResource(const char* name, qofs_t, time_t, void*, searchpathfuncs_s*)
{
  std::string path = name;
  if (path.empty())
    return 1;
  if (path.back() == '/')
    resourceDirs.insert(path);
  else
    resourceFiles.insert(path);
  return 1;
}
static void scanDirectory(const std::string& path)
{
  files->EnumerateFiles((fs_relative)4, (path + "*").c_str(), collectResource, nullptr);
}
#include "ftebroom_quake1_assets.h"
#include "ftebroom_quake2_assets.h"
static void profileStartMap();
static void resetBuildPlaceholders();
static void assetsReady()
{
  resetBuildPlaceholders();
  resetMaterialBrowser();
  clearEntityPreviews();
  materials.clear();
  cacheProjectIndexed();
  resourceFiles.clear();
  resourceDirs.clear();
  shaderFiles = 0;
  if(gameWad()) {
    try {q1IndexAssets();cacheProjectIndexed();publishMaterialTags();command("ftebroom_definitions\n");char current[256]={};cvars->GetString("mapname",current,sizeof current);if(std::filesystem::path(current).stem()=="start"&&!cvars->GetFloat("ftebroom_dirty"))profileStartMap();assetsLoading=false;setupNeeded=false;cvars->SetFloat("ftebroom_material_count",float(materials.size()));return;}
    catch(const std::exception& e){assetError=e.what();assetsLoading=false;setupNeeded=true;return;}
  }
  scanDirectory("textures/");
  if(gameProfile==3)files->EnumerateFiles((fs_relative)4,"*.wal",collectResource,nullptr);
  scanDirectory("scripts/");
  // ZIPs need not contain directory records. Match each depth explicitly too.
  std::string nested = "textures/";
  for (int depth = 0; depth < 32; ++depth)
  {
    nested += "*/";
    scanDirectory(nested);
  }
  std::set<std::string> visited;
  while (visited.size() < resourceDirs.size())
  {
    auto pending = resourceDirs;
    for (auto& dir : pending)
      if (visited.insert(dir).second && std::count(dir.begin(), dir.end(), '/') < 32)
        scanDirectory(dir);
  }
  for (auto& path : resourceFiles)
  {
    auto dot = path.find_last_of('.');
    if (dot == std::string::npos)
      continue;
    auto ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
      return char(std::tolower(c));
    });
    if (
      (path.rfind("textures/", 0) == 0 || (gameProfile==3&&ext==".wal"&&path.find('/')==path.npos))
      && (ext == ".tga" || ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".dds" || (gameProfile==3&&(ext == ".wal" || ext == ".pcx"))))
    {
      if (path == "textures/ftebroom/overlay.tga" || path.rfind("textures/_ftebroom_q1/",0)==0 || path.rfind("textures/_ftebroom_q2/",0)==0 || path.rfind("textures/_ftebroom_missing/",0)==0)
        continue;
      auto name = path.substr(0, dot);
      if(gameProfile==3){
        name="textures/"+ftebroom::materialKey(name);
        // Keep WAL's logical dimensions when a legacy texture is available.
        auto previous=materials.find(name);
        if(previous!=materials.end() && !(previous->second.looseWal&&path.find('/')!=path.npos)){
          if(path.find('/')==path.npos&&!previous->second.looseWal)continue;
          auto old=std::filesystem::u8path(previous->second.preview).extension().u8string();
          if(old==".wal" || (ext!=".wal"&&old==".pcx"))continue;
        }
      }
      materials[name] = {name, path, false};
      materials[name].looseWal=gameProfile==3&&path.find('/')==path.npos&&ext==".wal";
      materials[name].source=path;
    }
  }
  for (auto& path : resourceFiles)
  {
    // Generated WAL aliases are output, never authoritative input resources.
    if(gameProfile==3 && path=="scripts/reyondbroom-quake2.shader")continue;
    if (path.size() < 7 || path.substr(path.size() - 7) != ".shader")
      continue;
    size_t size = 0;
    auto data = (char*)files->LoadFile(path.c_str(), &size);
    if (!data)
      continue;
    std::string source(data, size);
    core->Free(data);
    ++shaderFiles;
    size_t offset = 0;
    for (;;)
    {
      auto name = token(source, offset);
      if (name.empty())
        break;
      if (token(source, offset) != "{")
        continue;
      auto material=ftebroom::shaderpreview::parse(source,offset);
      if(material.valid)materials[name]={name,material.preview,true,material.tags,material.opacity,material.masked,material.blend,material.alphaTest};
    }
  }
  if(gameProfile==3){try{q2IndexAssets();}catch(const std::exception& e){assetError=e.what();assetsLoading=false;setupNeeded=true;return;}}
  cacheProjectIndexed();
  publishMaterialTags();
  command("ftebroom_definitions\n");
  assetsLoading = false;
  setupNeeded = false;
  cvars->SetFloat("ftebroom_material_count", float(materials.size()));
  cvars->SetFloat("ftebroom_shader_file_count", float(shaderFiles));
  core->Print(("ReyondBroom: indexed " + std::to_string(materials.size())
               + " materials from " + std::to_string(shaderFiles) + " shader files.\n")
                .c_str());
  if (cvars->GetFloat("ftebroom_skip_setup")&&!cvars->GetFloat("ftebroom_test_quiet_resources"))
    for (auto& pair : materials)
      core->Print(("ReyondBroom material: " + pair.first + "\n").c_str());
}
static void prepareEditorWall(){
  cvars->SetFloat("ftebroom_wall_override",0);
  if(gameProfile!=1||gameWallMode==0)return;
  std::string shader;
  if(gameWallMode==4)shader=gameWallReadFile(std::filesystem::u8path(gameWallFile));
  else if(gameWallMode==3){size_t length=0;auto bytes=(char*)files->LoadFile("glsl/ftebroom_studio_wall.glsl",&length);if(bytes){shader.assign(bytes,length);core->Free(bytes);}}
  else {
    auto root=std::filesystem::u8path(gameWallMode==1?baseq3:gameMod);
    if(!std::filesystem::is_directory(root))throw std::runtime_error("Choose the selected defaultwall resource directory.");
    auto loose=root/"glsl/defaultwall.glsl";
    if(std::filesystem::is_regular_file(loose))shader=ftebroom::pak::read(loose);
    else {std::vector<std::filesystem::path> archives;for(const auto& e:std::filesystem::directory_iterator(root))if(e.is_regular_file()&&(e.path().extension()==".pak"||e.path().extension()==".pk3"))archives.push_back(e.path());std::sort(archives.rbegin(),archives.rend());for(const auto& a:archives){auto path=a/"glsl/defaultwall.glsl";if(ftebroom::pak::exists(path)){shader=ftebroom::pak::read(path);break;}}}
  }
  if(shader.empty()||shader.size()>1024*1024)throw std::runtime_error("The selected defaultwall.glsl is missing or exceeds 1 MiB.");
  qhandle_t handle;if(files->Open("glsl/ftebroom_selected_wall.glsl",&handle,2)<0)throw std::runtime_error("Cannot stage the editor wall shader.");
  files->Write(handle,shader.data(),shader.size());files->Close(handle);cvars->SetFloat("ftebroom_wall_override",1);
}
static void gameProfileRendering()
{
  static bool customActive=false;
  static std::string previousOffset;
  if(gameProfile==1) {
    if(!customActive){char value[64]={};cvars->GetString("r_glsl_offsetmapping",value,sizeof value);previousOffset=value;customActive=true;}
    cvars->SetFloat("r_glsl_offsetmapping",1);
  } else if(customActive) {
    cvars->SetString("r_glsl_offsetmapping",previousOffset.c_str());customActive=false;
  }
}
static bool mountAssets()
{
  assetError.clear();
  gameProfileLoad();
  cvars->SetFloat("ftebroom_allow_engine_terrain",gameProfile==1);
  cvars->SetFloat("ftebroom_quake1",gameWad());
  cvars->SetFloat("ftebroom_game_profile",gameProfile);
  cvars->SetFloat("ftebroom_classic_map",gameClassic());
  try
  {
    if(gameProfile==2) {
      auto selected=ftebroom::quakeResourceDirectory(q1Authoring()).generic_u8string();
      if(selected.size()>=sizeof q1Mod || selected.size()>=sizeof baseq3)throw std::runtime_error("The Quake 1 resource path is too long.");
      if(!q1UseLibre)std::snprintf(q1Mod,sizeof q1Mod,"%s",selected.c_str());
      std::snprintf(baseq3,sizeof baseq3,"%s",selected.c_str());
      if(!gameProfileSave())throw std::runtime_error("Cannot save the resource configuration.");
    }
    if(gameProfile>=3){
      auto selected=ftebroom::legacyResourceDirectory(gameProfile==3?q2Mod:hlMod,gameProfile,hlRuntime).generic_u8string();
      if(selected.size()>=sizeof q2Mod||selected.size()>=sizeof baseq3)throw std::runtime_error("The game resource path is too long.");
      std::snprintf(gameProfile==3?q2Mod:hlMod,1024,"%s",selected.c_str());
      if(!gameProfileSave())throw std::runtime_error("Cannot save the resource configuration.");
    }
    if(gameProfile==4)halfLifeInstallRuntime();
    if(gameProfile>=3)std::snprintf(baseq3,sizeof baseq3,"%s",gameProfile==3?q2Mod:hlMod);
    if(!gameWad()){
      char writable[4096]={};if(files->NativePath("scripts/ftebroom-quake1.shader",(fs_relative)5,writable,sizeof writable)){
        auto generated=std::filesystem::u8path(writable);if(std::filesystem::exists(generated))std::ofstream(generated)<<"// Quake 1 material aliases inactive.\n";
      }
    }
    if(gameProfile!=3){char cache[4096]{};if(files->NativePath("scripts/reyondbroom-quake2.shader",(fs_relative)5,cache,sizeof cache)&&std::filesystem::exists(std::filesystem::u8path(cache)))std::ofstream(std::filesystem::u8path(cache))<<"// Quake II aliases inactive.\n";}
    auto path = std::filesystem::weakly_canonical(std::filesystem::u8path(baseq3));
    if (!*baseq3 || !std::filesystem::is_directory(path))
      throw std::runtime_error(gameProfile==2?"Choose the Quake 1 mod folder, or enable LibreQuake.":"Choose an existing baseq3 folder.");
    bool hasAssets = std::filesystem::is_directory(path / "textures")
                     || std::filesystem::is_directory(path / "scripts");
    for (auto& entry : std::filesystem::directory_iterator(path))
    {
      auto ext = entry.path().extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return char(std::tolower(c));
      });
      if (ext == ".pk3" || (gameClassic() && (ext==".pak" || ext==".wad")))
        hasAssets = true;
    }
    if(gameProfile==2)hasAssets=hasAssets||std::filesystem::exists(path/"gfx/palette.lmp");
    if (!hasAssets && !gameClassic())
      throw std::runtime_error(
        "No PK3 archives, textures or scripts were found in this folder.");
    auto normalized = path.generic_u8string();
    auto base=gameWad()?q1BaseDirectory():std::string{};
    if(gameProfile==3){base=ftebroom::quake2FallbackDirectory(path).generic_u8string();if(!base.empty())core->Print(("ReyondBroom: Quake II fallback resources: "+base+" (selected directory has priority)\n").c_str());}
    cvars->SetString("ftebroom_assetpath", base.empty()?normalized.c_str():base.c_str());
    cvars->SetString("ftebroom_modassetpath",gameProfile==1?gameRuntime().c_str():!base.empty()?normalized.c_str():"");
    std::ofstream settings(settingsPath(), std::ios::binary);
    settings << normalized;
    settings.close();
    if(!settings)throw std::runtime_error("Cannot save the resource configuration.");
    std::snprintf(baseq3,sizeof baseq3,"%s",normalized.c_str());
    prepareEditorWall();
    gameProfileRendering();
    assetsLoading = true;
    command("fs_restart\nftebroom_assets_ready\n");
    return true;
  }
  catch (const std::exception& e)
  {
    assetError = e.what();
    return false;
  }
}
static void loadProjectCommand()
{
  if (commands->IsInsecure())
    return;
  commands->Argv(1, baseq3, sizeof baseq3);
  if (!mountAssets())
    core->Print(("ReyondBroom: " + assetError + "\n").c_str());
}
static void projectResourceFields()
{
  // Empty inputs must remain distinguishable from the resource panel in every theme.
  const bool light=uiTheme==1||uiTheme==3;
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,std::max(1.f,ImGui::GetStyle().FrameBorderSize));
  ImGui::PushStyleColor(ImGuiCol_Border,light?uiColor(120,128,140):uiColor(110,120,135));
  ImGui::PushStyleColor(ImGuiCol_FrameBg,light?uiColor(255,255,255):uiColor(42,46,52));
  gameProfileFields(firstSetupStep>=0);
  if(gameProfile<2) {
    ImGui::TextUnformatted("Map format: Quake 3 (Valve 220)");
    ImGui::TextUnformatted("baseq3 folder");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##baseq3",baseq3,sizeof baseq3);
    ImGui::TextWrapped("Paste the full path to baseq3. PK3 archives and loose textures/scripts are read without modifying your game. Use Save As to choose the map directory.");
  }
  if(!assetError.empty())ImGui::TextWrapped("%s",assetError.c_str());
  ImGui::PopStyleColor(2);
  ImGui::PopStyleVar();
}
#include "ftebroom_setup_theme.h"
static bool resourceDownloadPending=false;
static bool projectLoadRequested(bool clicked)
{
  if(resourceDownloadPending) {
    if(libreBusy())return false;
    resourceDownloadPending=false;
    if(!libreError.empty()||!libreReady())return false;
    clicked=true;
  }
  if(!clicked)return false;
  if(gameProfile==2) {
    if(setupModChoice==0&&!libreReady()) {
      libreStartDownload();resourceDownloadPending=libreBusy();return false;
    }
    q1UseLibre=setupModChoice==0;
  }
  return gameProfileSave()&&mountAssets();
}
static ImVec2 setupFinishCenter,setupLoadCenter;
static void projectSetup()
{
  libreTick();
  if (!setupInitialized)
  {
    setupInitialized = true;
    gameProfileLoad();
    std::ifstream settings(settingsPath(), std::ios::binary);
    std::string saved;
    std::getline(settings, saved);
    if(saved.empty() && !cvars->GetFloat("ftebroom_skip_setup"))firstSetupStep=0;
    std::snprintf(baseq3, sizeof baseq3, "%s", saved.c_str());
    setupNeeded = false;
    if(!saved.empty()) { if(!mountAssets())setupNeeded=cvars->GetFloat("ftebroom_skip_setup")==0; }
    else setupNeeded=cvars->GetFloat("ftebroom_skip_setup")==0;
  }
  if(firstSetupStep==5&&setupNeeded&&!assetsLoading){firstSetupStep=4;setupNeeded=false;}
  if(firstSetupStep>=0) {
    ImGui::OpenPopup("First-time setup");
    const auto display=ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSize({std::min(display.x-24,800*uiScale()),std::min(display.y-64,550*uiScale())},ImGuiCond_Always);
    ImGui::SetNextWindowPos({display.x*.5f,display.y*.5f},ImGuiCond_Always,{.5f,.5f});
    bool open=true;
    if(ImGui::BeginPopupModal("First-time setup",&open,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings)) {
      const char* titles[]={"Choose your language","Choose interface size","Make yourself at home","Choose a game","Choose your mod / resources","Join our Discord"};
      ImGui::TextDisabled("ReyondBroom / Step %d of 6",firstSetupStep+1);
      ImGui::TextUnformatted(titles[firstSetupStep]);
      ImGui::PushStyleColor(ImGuiCol_PlotHistogram,ImGui::GetStyle().Colors[ImGuiCol_CheckMark]);
      ImGui::ProgressBar(float(firstSetupStep+1)/6,{ -1,3*uiScale()},"");
      ImGui::PopStyleColor();
      const float footer=ImGui::GetFrameHeightWithSpacing()+ImGui::GetStyle().ItemSpacing.y*2;
      ImGui::BeginChild("Setup content",{0,-footer});
      if(firstSetupStep==0) {
        for(int i=0;i<7;i++)if(ImGui::RadioButton(uiLanguageNames[i],uiLanguage==i))uiLanguage=i;
      } else if(firstSetupStep==1) {
        setupScaleList();
      } else if(firstSetupStep==2) {
        ImGui::TextWrapped("Choose a theme. You can change it later in View.");
        setupThemeTiles();
      } else if(firstSetupStep==3) {
        setupGameChoices();
      } else if(firstSetupStep==4) projectResourceFields();
      else {
        ImGui::Spacing();
        ImGui::TextWrapped("Share your maps, get help and follow ReyondBroom updates.");
        ImGui::Spacing();
        ImGui::TextLinkOpenURL("https://discord.gg/eDMf53Kny","https://discord.gg/eDMf53Kny");
        ImGui::Spacing();
        ImGui::TextDisabled("Joining is optional. You can start editing right away.");
      }
      ImGui::EndChild();
      ImGui::Separator();
      ImGui::BeginDisabled(firstSetupStep==0||assetsLoading||libreBusy());
      if(ImGui::Button("Back",{110*uiScale(),0}))--firstSetupStep;
      ImGui::EndDisabled();ImGui::SameLine();
      const float buttonWidth=220*uiScale();
      ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x-buttonWidth);
      bool loadRequested=false;
      ImGui::BeginDisabled(assetsLoading||gameProfileLocked||libreBusy());
      if(firstSetupStep<4) {
        if(ImGui::Button("Next",{buttonWidth,0})){uiSaveAppearance();++firstSetupStep;}
      } else if(firstSetupStep==4) {
        loadRequested=ImGui::Button("Load resources and start",{buttonWidth,0});
        auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();setupLoadCenter={(a.x+b.x)*.5f,(a.y+b.y)*.5f};
      }
      else {
        bool finish=ImGui::Button("Start editing",{buttonWidth,0});
        auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        setupFinishCenter={(a.x+b.x)*.5f,(a.y+b.y)*.5f};
        if(finish){uiSaveAppearance();firstSetupStep=-1;ImGui::CloseCurrentPopup();}
      }
      ImGui::EndDisabled();
      if(firstSetupStep==4&&projectLoadRequested(loadRequested)) {
        uiSaveAppearance();firstSetupStep=5;
      }
      ImGui::EndPopup();
    }
    if(!open){resourceDownloadPending=false;firstSetupStep=-1;setupNeeded=false;}
    return;
  }
  if(uiClosedDialog=="Game / resources"){resourceDownloadPending=false;setupNeeded=false;uiClosedDialog.clear();}
  if(setupNeeded){ImGui::OpenPopup("Game / resources");setupNeeded=false;}
  if(uiDialog("Game / resources",{800,550})) {
    projectResourceFields();
    ImGui::BeginDisabled(assetsLoading||gameProfileLocked||libreBusy());
    bool loadRequested=ImGui::Button("Load resources and start");
    ImGui::EndDisabled();
    if(projectLoadRequested(loadRequested))ImGui::CloseCurrentPopup();
    ImGui::SameLine();
    if(ImGui::Button("Close##resources")){resourceDownloadPending=false;setupNeeded=false;ImGui::CloseCurrentPopup();}
    if(assetsLoading)ImGui::TextUnformatted("Loading resources...");
    ImGui::EndPopup();
  }
}

static void importMaterialControls();
#include "ftebroom_materials.h"

static void q2ReferenceMapMaterials(const std::string& source){
  q2MapMaterials.clear();
  for(const auto& name:ftebroom::quake1::mapTextures(source))q2MapMaterials.insert(ftebroom::materialKey(name));
  if(!assetsLoading&&q2ExpandMapMaterials()){resetMaterialBrowser();publishMaterialTags();}
}
