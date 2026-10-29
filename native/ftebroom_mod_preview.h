// SPDX-License-Identifier: GPL-2.0-or-later
// Keep the editor VM alive while the actual mod runs in a separate process.
#include "ftebroom_embed.h"
static std::string modQuote(const std::string& text)
{
  if(text.find_first_of("\"\r\n")!=std::string::npos)throw std::runtime_error("A profile path contains a quote or newline.");
  return "\""+text+"\"";
}
static void modValidate()
{
#ifndef _WIN32
  if(gameProfile!=4)throw std::runtime_error("Embedded mod preview is currently Windows-only. Use BSP preview or launch your game separately with a Run task.");
#endif
  auto mod=std::filesystem::u8path(gameRuntime());
  if(!std::filesystem::is_regular_file(runBundledEngine()))throw std::runtime_error("The bundled preview engine is missing.");
  if(!std::filesystem::is_directory(mod) || (gameProfile!=4 && !std::filesystem::is_regular_file(mod/"progs.dat") && !std::filesystem::is_regular_file(mod/"pak0.pak")))throw std::runtime_error("The runtime mod directory must contain progs.dat.");
  auto name=mod.filename().u8string();
  if(name.empty() || name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")!=std::string::npos)
    throw std::runtime_error("The runtime mod folder name must contain only letters, digits, _ or -.");
}
static void modPreview()
{
  if(runActive || buildRunning)return;
  try {
    if(gameProfile==4)halfLifeInstallRuntime();
    modValidate();
    if(buildResult.empty() || (buildResultDocument!=documentFile || buildResultProfile!=gameProfile))throw std::runtime_error("Build this map before opening the mod preview.");
    auto name=documentFile.stem().u8string();
    if(name.empty() || name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")!=std::string::npos)
      throw std::runtime_error("For mod preview, save the map with a name containing letters, digits, _ or -.");
    auto mod=std::filesystem::absolute(std::filesystem::u8path(gameRuntime())).lexically_normal();
    auto root=buildResult.parent_path()/"mod-preview";
    auto home=root/"home", maps=home/mod.filename()/"maps";
    std::filesystem::create_directories(maps);
    if(gameProfile==4)halfLifeStageRuntime(home/mod.filename());
    // Preserve generated external lightmap names while retaining the real map name.
    for(const auto& file:std::filesystem::directory_iterator(buildResult.parent_path())) {
      auto ext=file.path().extension();
      if(file.path()==root || ext==".map" || ext==".log" || ext==".prt")continue;
      std::filesystem::copy(file.path(),maps/file.path().filename(),std::filesystem::copy_options::overwrite_existing|std::filesystem::copy_options::recursive);
    }
    std::filesystem::copy_file(buildResult,maps/(name+".bsp"),std::filesystem::copy_options::overwrite_existing);
    for(const char* ext:{".lit",".lux"}){auto extra=buildResult;extra.replace_extension(ext);if(std::filesystem::exists(extra))std::filesystem::copy_file(extra,maps/(name+ext),std::filesystem::copy_options::overwrite_existing); }
    if(cvars->GetFloat("ftebroom_bsp_preview"))buildEdit();
#ifdef _WIN32
    modEmbedBegin();
#endif
    auto embeddedEngine=runBundledEngine();
#ifndef _WIN32
    if(gameProfile==4){
      // An engine beside the editor's default.fmf/installed.lst can inherit
      // the editor installation even with -basedir. Isolate its binary path.
      auto isolated=root/"engine"/embeddedEngine.filename();std::filesystem::create_directories(isolated.parent_path());
      std::filesystem::copy_file(embeddedEngine,isolated,std::filesystem::copy_options::overwrite_existing);
      std::filesystem::permissions(isolated,std::filesystem::perms::owner_exec|std::filesystem::perms::group_exec|std::filesystem::perms::others_exec,std::filesystem::perm_options::add);
      embeddedEngine=isolated;
    }
#endif
    std::string gameSwitch=gameProfile==4?std::string("-game valve "):std::string();
#ifdef _WIN32
    std::string previewSwitch="-plugin -ftebroom-preview ";
#else
    std::string previewSwitch="-ftebroom-preview ";
#endif
    std::string args=previewSwitch+"-noupdates -quake -basedir "+modQuote(mod.parent_path().generic_u8string())+" "+gameSwitch+"-game "+modQuote(mod.filename().u8string())+
      " -homedir "+modQuote(home.generic_u8string())+" -usehome -readonly -window -width 1280 -height 800 +set viewsize 120 +set sv_public 0 +set deathmatch 0 +set coop 0 +set sv_nopvs 0 +set r_drawviewmodel 0 +set cl_idlefps 60 +set cl_maxfps 60 +set vid_wait 0 "+(gameProfile==1?std::string("+set r_glsl_offsetmapping 1 "):std::string())+(gameProfile==4?std::string("+set forceqmenu 1 +exec quake.rc +set r_fullbright 0 +set r_vertexlight 0 +set com_protocolname Nuclide +set g_gametype ")+(hlRuntime?"counterstrike ":"singleplayer "):std::string())+"+map "+name;
    if(cvars->GetFloat("ftebroom_skip_setup"))args+=" +set ftebroom_preview_trace 1";
    RunProfile preview{std::string(gameProfileName())+": mod preview",root.u8string(),true,{{1,embeddedEngine.u8string(),args}}};
    runBegin(preview);runWindow=false;
    if(!runActive){modEmbedEnd();throw std::runtime_error(runError);}
#ifdef _WIN32
    buildMessage="Choose Free camera to inspect, or Play mod to control the player.";
#else
    buildMessage="FreeHL / FreeCS is running in a separate window. Close it to return to editing.";
#endif
  }catch(const std::exception& error){modEmbedEnd();buildMessage=error.what();}
}
