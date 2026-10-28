// SPDX-License-Identifier: GPL-2.0-or-later
// Map format, authoring assets and runtime game are independent settings.
#include "ftebroom_librequake.h"
#include "ftebroom_resource_directory.h"
#include "ftebroom_wall_file.h"
#include "ftebroom_mod_definitions.h"
static int gameProfile;
static int gameWallMode=0;
static char gameWallFile[4096]="";
static void gameWallBrowse();
static void gameWallBrowser();
static bool gameProfileLocked;
static char gameMod[1024]="";
static char gameEngine[1024]="";
static bool gamePreviewAfterBuild=false;
static char gameDefinitions[1024]="";
static char gameQC[1024]="";
static char q1Mod[1024]="",q1Wads[8192]="",q1Definitions[1024]="";
static char q2Mod[1024]="",hlMod[1024]="",hlWads[8192]="",hlDefinitions[1024]="",q2Definitions[1024]="";
static int hlRuntime=0;
static bool gameClassic(){return gameProfile>=2;}
static bool gameWad(){return gameProfile==2||gameProfile==4;}
static const char* gameProfileNames="Quake III Arena\0FTE Custom game\0Quake 1\0Quake II\0Half-Life\0";
static const char* gameProfileKey(int p){return p==4?"halflife":p==3?"quake2":p==2?"quake1":p==1?"when-the-sky-went-dark":"quake3";}
static std::string q3Root;
static int q1MapFormat=1,q1BspFormat=0;
static const char* gameProfileName(){return gameProfile==4?"Half-Life":gameProfile==3?"Quake II":gameProfile==2?"Quake 1":gameProfile==1?"FTE Custom game":"Quake III Arena";}
static std::string q1Authoring(){return gameProfile==4?std::string(hlMod):q1UseLibre?libreMod():q1Mod;}
static std::string gameRuntime(){return gameProfile==3?std::string(q2Mod):gameWad()?q1Authoring():gameMod;}
// Standard Quake layout: id1 supplies base assets, the selected mod overrides them.
static std::string q1BaseDirectory(){
  auto mod=std::filesystem::u8path(q1Authoring());
  if(gameProfile==4){auto base=mod.parent_path()/"valve";return base!=mod&&std::filesystem::is_directory(base)?base.generic_u8string():std::string{};}
  if(mod.empty()||q1UseLibre||ftebroom::quakeId1(mod))return {};
  auto base=mod.parent_path()/"id1";
  return base!=mod&&std::filesystem::is_directory(base)?base.generic_u8string():std::string{};
}
static void gameProfileLoad()
{
  static bool loaded=false;if(loaded)return;loaded=true;
  std::ifstream input(settingsPath().parent_path()/"ftebroom-game-profile.txt");
  int selected=0,automatic=0;std::string mod,engine;
  if(input>>selected>>automatic>>std::quoted(mod)>>std::quoted(engine)) {
    std::string defs,qc;
    if(input>>std::quoted(defs)>>std::quoted(qc)){std::snprintf(gameDefinitions,sizeof gameDefinitions,"%s",defs.c_str());std::snprintf(gameQC,sizeof gameQC,"%s",qc.c_str());}
    // Earlier versions enabled mod launch implicitly. Require a new explicit opt-in.
    int previewPolicy=0;input>>previewPolicy;input>>gameWallMode;gameWallMode=std::clamp(gameWallMode,0,4);
    std::string wall;if(input>>std::quoted(wall))std::snprintf(gameWallFile,sizeof gameWallFile,"%s",wall.c_str());
    gameProfile=std::clamp(selected,0,4);gamePreviewAfterBuild=previewPolicy==1 && automatic!=0;
    std::snprintf(gameMod,sizeof gameMod,"%s",mod.c_str());
    std::snprintf(gameEngine,sizeof gameEngine,"%s",engine.c_str());
  }
  std::ifstream legacy(settingsPath().parent_path()/"reyondbroom-legacy-games.txt");
  std::string q2,hl,hw,hd,qd;
  if(legacy>>std::quoted(q2)>>std::quoted(hl)>>std::quoted(hw)>>std::quoted(hd)>>std::quoted(qd)>>hlRuntime){
    std::snprintf(q2Mod,sizeof q2Mod,"%s",q2.c_str());std::snprintf(hlMod,sizeof hlMod,"%s",hl.c_str());
    std::snprintf(hlWads,sizeof hlWads,"%s",hw.c_str());std::snprintf(hlDefinitions,sizeof hlDefinitions,"%s",hd.c_str());std::snprintf(q2Definitions,sizeof q2Definitions,"%s",qd.c_str());hlRuntime=std::clamp(hlRuntime,0,1);
  }
  std::ifstream quake(settingsPath().parent_path()/"ftebroom-quake1.txt");
  std::string root,wads,defs;
  if(quake>>std::quoted(root)>>std::quoted(mod)>>std::quoted(wads)>>std::quoted(defs)>>q1MapFormat>>q1BspFormat>>std::quoted(q3Root)) {
    // Migrate the old base/mod pair to one resource directory, preferring the mod.
    std::snprintf(q1Mod,sizeof q1Mod,"%s",(mod.empty()?root:mod).c_str());
    std::snprintf(q1Wads,sizeof q1Wads,"%s",wads.c_str());std::snprintf(q1Definitions,sizeof q1Definitions,"%s",defs.c_str());
    q1MapFormat=std::clamp(q1MapFormat,0,1);q1BspFormat=std::clamp(q1BspFormat,0,1);
    int libre=0;if(quake>>libre)q1UseLibre=libre!=0;
  }
}
static bool gameProfileSave()
{
  auto target=settingsPath().parent_path()/"ftebroom-game-profile.txt";
  std::ofstream output(target,std::ios::binary);
  output<<gameProfile<<' '<<gamePreviewAfterBuild<<' '<<std::quoted(gameMod)<<' '<<std::quoted(gameEngine)<<' '<<std::quoted(gameDefinitions)<<' '<<std::quoted(gameQC)<<" 1 "<<gameWallMode<<' '<<std::quoted(gameWallFile)<<"\n";
  output.close();
  std::ofstream quake(settingsPath().parent_path()/"ftebroom-quake1.txt");
  quake<<std::quoted(std::string())<<' '<<std::quoted(q1Mod)<<' '<<std::quoted(q1Wads)<<' '<<std::quoted(q1Definitions)<<' '<<q1MapFormat<<' '<<q1BspFormat<<' '<<std::quoted(q3Root)<<' '<<q1UseLibre<<'\n';
  quake.close();std::ofstream legacy(settingsPath().parent_path()/"reyondbroom-legacy-games.txt");
  legacy<<std::quoted(q2Mod)<<' '<<std::quoted(hlMod)<<' '<<std::quoted(hlWads)<<' '<<std::quoted(hlDefinitions)<<' '<<std::quoted(q2Definitions)<<' '<<hlRuntime<<'\n';legacy.close();
  return bool(output)&&bool(quake)&&bool(legacy);
}
#include "ftebroom_setup_choices.h"
static void gameProfileFields(bool wizard=false)
{
  gameProfileLoad();
  ImGui::BeginDisabled(gameProfileLocked);
  int old=gameProfile;
  if(!wizard && ImGui::Combo("Game profile",&gameProfile,gameProfileNames)) {
    if(old<2)q3Root=baseq3;
    std::snprintf(baseq3,sizeof baseq3,"%s",gameProfile==3?q2Mod:gameWad()?q1Authoring().c_str():q3Root.c_str());
  }
  if(gameProfile==2) {
    setupQuakeModFields();
  }
  if(gameProfile==3||gameProfile==4){
    auto path=gameProfile==3?q2Mod:hlMod;ImGui::InputText(gameProfile==3?"Quake II mod directory":"Half-Life mod directory",path,1024);
    ImGui::TextWrapped(gameProfile==3?"Choose baseq2 or a mod directory. A sibling baseq2 supplies base resources. PAK archives and WAL textures are supported.":"Choose valve for Half-Life or cstrike for Counter-Strike 1.5. A sibling valve supplies base resources. WAD3 textures retain their own palettes.");
    if(gameProfile==4){ImGui::Combo("Game runtime",&hlRuntime,"FreeHL (Half-Life)\0FreeCS (Counter-Strike 1.5)\0");ImGui::TextWrapped("Runtime installation adds a separate ReyondBroom package. Original game files are not replaced. Retail assets are required; arbitrary DLL mods are not supported by FreeHL/FreeCS.");}
    if(ImGui::CollapsingHeader("Advanced settings")){
      ImGui::InputText("Entity definitions (optional .fgd / .def / .ent)",gameProfile==3?q2Definitions:hlDefinitions,1024);
      if(gameProfile==4)ImGui::InputTextMultiline("Additional WAD3 archives (optional, one path per line)",hlWads,sizeof hlWads,{0,60*uiScale()});
      ImGui::TextUnformatted(gameProfile==3?"MAP: Valve 220 with Q2 surface flags | BSP: IBSP 38":"MAP: Valve 220 | BSP: GoldSrc 30");
    }
  }
  if(gameProfile==1) {
    ImGui::InputText("Runtime mod directory (optional for mapping)",gameMod,sizeof gameMod);
    ImGui::TextWrapped("Preview engine: bundled FTE (required for embedded free camera).");
    ImGui::InputText("Entity definitions (optional .def / .fgd)",gameDefinitions,sizeof gameDefinitions);
    ImGui::InputText("QuakeC source folder (optional)",gameQC,sizeof gameQC);
    ImGui::Checkbox("Open mod preview after Build",&gamePreviewAfterBuild);
    ImGui::Combo("Editor defaultwall",&gameWallMode,"Current FTE default\0Map source folder\0Mod folder\0Neutral studio (Unity-like)\0Custom GLSL file\0");
    if(gameWallMode==4){ImGui::InputText("GLSL file",gameWallFile,sizeof gameWallFile);ImGui::SameLine();if(ImGui::Button("Browse...##wall"))gameWallBrowse();ImGui::TextWrapped("Use an FTE-compatible defaultwall program. Includes resolve through the mounted game resources. Apply/load resources to save and activate this path.");}
    ImGui::TextWrapped("Applied when loading resources. Only the editor material changes; mod files and build output are untouched. Neutral studio is an original approximation, not Unity PBR.");
    ImGui::TextWrapped("Quake 3 BSP with Quake game logic. Preview runs your mod inside the 3D viewport, using a temporary home and read-only filesystem. Only mod assets are loaded; baseq3 is the authoring library.");
  }
  ImGui::EndDisabled();
  gameWallBrowser();
}

static std::string gameDefinitionPath()
{
  gameProfileLoad();if(gameProfile>=3){
    auto overridePath=gameProfile==3?q2Definitions:hlDefinitions;if(*overridePath)return overridePath;
    auto found=ftebroom::discoverModDefinitions(std::filesystem::u8path(gameRuntime()));
    return !found.empty()?found:gameProfile==3?"quake2.fgd":hlRuntime?"counterstrike.fgd":"halflife.fgd";
  }if(gameProfile==2){
    if(*q1Definitions)return q1Definitions;
    auto detected=ftebroom::discoverModDefinitions(std::filesystem::u8path(q1Authoring()));
    return !detected.empty()?detected:q1UseLibre?"librequake.def":"quake1.ent";
  }if(gameProfile!=1)return {};
  if(*gameDefinitions)return gameDefinitions;
  auto root=std::filesystem::u8path(gameQC);
  if(!*gameQC || !std::filesystem::is_directory(root))return {};
  std::vector<std::filesystem::path> sources;
  for(const auto& file:std::filesystem::directory_iterator(root))
    if(file.is_regular_file() && file.path().extension()==".qc")sources.push_back(file.path());
  std::sort(sources.begin(),sources.end());std::string result;
  for(const auto& path:sources){
    if(std::filesystem::file_size(path)>16*1024*1024)continue;
    std::ifstream input(path);std::string source((std::istreambuf_iterator<char>(input)),{});
    size_t at=0;while((at=source.find("/*QUAKED",at))!=std::string::npos){
      auto end=source.find("*/",at);if(end==std::string::npos)break;
      result+=source.substr(at,end+2-at)+"\n";at=end+2;
      if(result.size()>16*1024*1024)throw std::runtime_error("Mod entity definitions exceed 16 MiB.");
    }
  }
  if(result.empty())throw std::runtime_error("No QUAKED descriptions found. Choose a .def or .fgd file for this mod.");
  auto path=settingsPath().parent_path()/"ftebroom-mod-entities.def";
  std::ofstream output(path);output<<result;output.close();
  if(!output)throw std::runtime_error("Cannot write the local mod entity definition cache.");
  return path.u8string();
}
