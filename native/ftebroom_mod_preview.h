// SPDX-License-Identifier: GPL-2.0-or-later
// Keep the editor VM alive while the actual mod runs in a separate process.
#include "ftebroom_embed.h"
#include "ftebroom_raw_preview.h"
static bool modChoiceOpen=false,modRawExporting=false;
static std::filesystem::path modRawDocument;
static int modRawProfile;
static std::chrono::steady_clock::time_point modRawStarted;
static bool modHasCompiledMap(){
  std::error_code error;
  return !buildResult.empty()&&buildResultDocument==documentFile&&buildResultProfile==gameProfile&&std::filesystem::is_regular_file(buildResult,error);
}
static bool modCompiledCurrent(){
  return modHasCompiledMap()&&buildResultContentRevision==buildCurrentContentRevision()&&buildResultGeneration==documentGeneration;
}
static void modRawMaterials(const std::filesystem::path& source,const std::filesystem::path& home){
  auto names=ftebroom::quake1::mapTextures(q1Read(source));std::set<std::string> used(names.begin(),names.end());
  std::filesystem::create_directories(home/"scripts");std::ofstream shaders(home/"scripts/reyondbroom-raw-preview.shader");
  for(const auto& name:used)if(ftebroom::previewToolMaterial(name)){
    shaders<<name<<"\n{\nsurfaceparm nodraw\nsurfaceparm noshadows\n}\n";
    if(name.rfind("textures/",0)!=0)shaders<<"textures/"<<name<<"\n{\nsurfaceparm nodraw\nsurfaceparm noshadows\n}\n";
  }
  if(!gameWad()&&gameProfile!=3){shaders.close();if(!shaders)throw std::runtime_error("Cannot stage preview tool materials.");return;}
  for(const auto& usedName:used){
    auto shortName=ftebroom::materialKey(usedName);if(ftebroom::previewToolMaterial(shortName))continue;
    const MaterialEntry* material=nullptr;
    if(gameProfile==3){material=q2Material(shortName);if(material&&!material->looseWal&&material->preview.rfind("textures/_ftebroom_missing/",0)!=0)continue;}
    else {auto found=materials.find("textures/"+shortName);if(found!=materials.end())material=&found->second;}
    if(!material)continue;
    const auto& image=material->preview;
    if(!ftebroom::rawPreviewImagePath(image))throw std::runtime_error("Invalid generated preview image path: "+image);
    auto bytes=modelResource(image);if(bytes.empty())throw std::runtime_error("A preview image is missing: "+usedName);
    auto target=home/std::filesystem::u8path(image);std::filesystem::create_directories(target.parent_path());
    std::ofstream output(target,std::ios::binary);output.write(bytes.data(),bytes.size());output.close();if(!output)throw std::runtime_error("Cannot stage raw-preview material.");
    auto pass="\n{\nmap "+image+"\nrgbgen identitylighting\n"+(shortName.rfind("{",0)==0?std::string("alphafunc GE128\ndepthwrite\n"):std::string())+"}\n";
    shaders<<"textures/"<<shortName<<"\n{"<<pass<<"}\n"<<shortName<<"\n{"<<pass<<"}\n";
  }
  shaders.close();if(!shaders)throw std::runtime_error("Cannot stage raw-preview material aliases.");
}
static std::string modQuote(const std::string& text)
{
  if(text.find_first_of("\"\r\n")!=std::string::npos)throw std::runtime_error("A profile path contains a quote or newline.");
  return "\""+text+"\"";
}
static void modValidate()
{
  auto mod=std::filesystem::u8path(gameRuntime());
  if(!std::filesystem::is_regular_file(runBundledEngine()))throw std::runtime_error("The bundled preview engine is missing.");
  if(!std::filesystem::is_directory(mod) || (gameProfile!=4 && !std::filesystem::is_regular_file(mod/"progs.dat") && !std::filesystem::is_regular_file(mod/"pak0.pak")))throw std::runtime_error("The runtime mod directory must contain progs.dat.");
  auto name=mod.filename().u8string();
  if(name.empty() || name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")!=std::string::npos)
    throw std::runtime_error("The runtime mod folder name must contain only letters, digits, _ or -.");
}
static void modLaunch(const std::filesystem::path& source,bool raw)
{
  if(runActive || buildRunning)return;
  try {
    if(gameProfile==4)halfLifeInstallRuntime();
    modValidate();
    if(!std::filesystem::is_regular_file(source))throw std::runtime_error("The selected preview file is missing.");
    auto name=documentFile.empty()?std::string("untitled"):documentFile.stem().u8string();
    for(auto& c:name)if(!std::isalnum(static_cast<unsigned char>(c))||static_cast<unsigned char>(c)>127)c='_';
    if(name.empty())name="preview";
    if(name.size()>48)name.resize(48);
    auto mod=std::filesystem::absolute(std::filesystem::u8path(gameRuntime())).lexically_normal();
    auto root=source.parent_path()/"mod-preview";
    auto home=root/"home", maps=home/mod.filename()/"maps";
    std::filesystem::create_directories(maps);
    if(gameProfile==4)halfLifeStageRuntime(home/mod.filename());
    // Preserve generated external lightmap names while retaining the real map name.
    if(!raw)for(const auto& file:std::filesystem::directory_iterator(source.parent_path())) {
      auto ext=file.path().extension();
      if(file.path()==root || ext==".map" || ext==".log" || ext==".prt")continue;
      std::filesystem::copy(file.path(),maps/file.path().filename(),std::filesystem::copy_options::overwrite_existing|std::filesystem::copy_options::recursive);
    }
    std::filesystem::copy_file(source,maps/(name+(raw?".map":".bsp")),std::filesystem::copy_options::overwrite_existing);
    if(raw)modRawMaterials(source,home/mod.filename());
    auto lightSource=raw?source:source.parent_path()/"snapshot.map";
    modLightingSource.clear();modLightingRaw=raw;modLightingLoad();
    if(std::filesystem::is_regular_file(lightSource)){
      modLightingSource="maps/reyond-preview-lights.ent";
      auto lights=ftebroom::previewLightEntities(q1Read(lightSource));
      std::ofstream output(home/mod.filename()/modLightingSource,std::ios::binary);output<<lights;output.close();
      if(!output)throw std::runtime_error("Cannot stage preview light entities.");
    }
    if(!raw)for(const char* ext:{".lit",".lux"}){auto extra=source;extra.replace_extension(ext);if(std::filesystem::exists(extra))std::filesystem::copy_file(extra,maps/(name+ext),std::filesystem::copy_options::overwrite_existing); }
    if(cvars->GetFloat("ftebroom_bsp_preview"))buildEdit();
#ifdef _WIN32
    modEmbedBegin();
#endif
    auto embeddedEngine=runBundledEngine();
#ifndef _WIN32
    {
      // An engine beside the editor's default.fmf/installed.lst can inherit
      // the editor installation even with -basedir. Isolate its binary path.
      auto isolated=root/"engine"/embeddedEngine.filename();std::filesystem::create_directories(isolated.parent_path());
      std::filesystem::copy_file(embeddedEngine,isolated,std::filesystem::copy_options::overwrite_existing);
      std::filesystem::permissions(isolated,std::filesystem::perms::owner_exec|std::filesystem::perms::group_exec|std::filesystem::perms::others_exec,std::filesystem::perm_options::add);
      auto menu=runBundledEngine().parent_path()/"fteplug_reyondpreview_amd64.so";
      if(!std::filesystem::is_regular_file(menu))throw std::runtime_error("The Linux preview-menu module is missing. Reinstall the complete editor archive.");
      std::filesystem::copy_file(menu,isolated.parent_path()/menu.filename(),std::filesystem::copy_options::overwrite_existing);
      auto fontHome=home/mod.filename()/"reyond-preview";std::filesystem::create_directories(fontHome);
      std::filesystem::copy_file(documentPath("fonts/FTEBroom-CJK.otf"),fontHome/"menu.otf",std::filesystem::copy_options::overwrite_existing);
      runPreviewSettings=modPreviewReadSettings;
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
      " -homedir "+modQuote(home.generic_u8string())+" -usehome -readonly -window -width 1280 -height 800 +set viewsize 120 +set sv_public 0 +set deathmatch 0 +set coop 0 +set sv_nopvs 0 +set ftebroom_preview_play 1 +set r_drawviewmodel 1 +set chase_active 0 +set cl_idlefps 60 +set cl_maxfps 60 +set vid_wait 0 "+(gameProfile==1?std::string("+set r_glsl_offsetmapping 1 "):std::string())+(gameProfile==4?std::string("+set forceqmenu 1 +exec quake.rc +set r_fullbright ")+(raw?"1":"0")+std::string(" +set r_vertexlight 0 +set pm_thirdPerson 0 +set r_showViewModels 1 +set com_protocolname Nuclide +set g_gametype ")+(hlRuntime?"counterstrike ":"singleplayer "):std::string())+(raw?std::string("+set r_fullbright 1 "):std::string())+"+map "+(raw?modQuote("maps/"+name+".map"):name);
    args+=" +"+modLightingCommand();
#ifndef _WIN32
    modControlsLoad();
    args+=" +plug_load reyondpreview +reyond_preview_config "+std::to_string(modLightingRTL?1:0)+" "+std::to_string(modLightingDistance)+" "+std::to_string(raw?1:0)+" "+modQuote(modLightingSource)+" "+std::to_string(modLighting.radius)+" "+std::to_string(modLighting.energy)+" "+std::to_string(modLighting.ambient)+" "+std::to_string(modLighting.baked)+" "+std::to_string(modSensitivity)+" "+std::to_string(modInvertMouse?1:0);
    for(const auto& b:modControls)args+=" "+std::to_string(b.key);
    args+=" "+std::to_string(uiLanguage);
#endif
    if(cvars->GetFloat("ftebroom_skip_setup"))args+=" +set ftebroom_preview_trace 1";
    RunProfile preview{std::string(gameProfileName())+": mod preview",root.u8string(),true,{{1,embeddedEngine.u8string(),args}}};
    runBegin(preview);runWindow=false;
    if(!runActive){modEmbedEnd();throw std::runtime_error(runError);}
#ifdef _WIN32
    buildMessage="Play mod: click the viewport to control the player. Escape opens mod controls.";
#else
    buildMessage="Play mod is running in a separate window. Escape opens controls and lighting; Exit mod preview returns to editing.";
#endif
  }catch(const std::exception& error){modEmbedEnd();buildMessage=error.what();}
}
static void modPreview(){
  if(runActive||runExclusive||buildRunning||assetsLoading)return;
  buildRestoreHistory();
  if(modCompiledCurrent())modLaunch(buildResult,false);
  else {modChoiceOpen=true;buildMessage.clear();}
}
static void modPreviewChoose(int choice){
  if(runActive||runExclusive||buildRunning||assetsLoading)return;
  try{
    if(choice==0){
      if(documentFile.empty())throw std::runtime_error("Save the map once before compiling it.");
      modValidate();
      buildLoadSettings();buildGeometry=true;modCompilePending=true;buildBegin();
      if(!buildRunning){modCompilePending=false;throw std::runtime_error(buildMessage.empty()?"Cannot start compilation.":buildMessage);}
    }else if(choice==1){
      if(!modHasCompiledMap())throw std::runtime_error("There is no compiled version for this map and game profile.");
      modLaunch(buildResult,false);if(!runActive)throw std::runtime_error(buildMessage);
    }else if(choice==2){
      modValidate();
      modRawDocument=documentFile;modRawProfile=gameProfile;modRawStarted=std::chrono::steady_clock::now();
      cvars->SetFloat("ftebroom_export_status",0);command("ftebroom_export_map raw-preview\n");
      modRawExporting=runExclusive=gameProfileLocked=true;buildMessage="Preparing the current MAP for Play mod...";
    }else return;
    modChoiceOpen=false;
  }catch(const std::exception& e){buildMessage=e.what();}
}
static void modPreviewTick(){
#ifndef _WIN32
  if(!runActive)runPreviewSettings=nullptr;
#endif
  if(!modRawExporting)return;
  try{
    if(documentFile!=modRawDocument||gameProfile!=modRawProfile)throw std::runtime_error("The active map changed while preparing the preview. Start Play mod again.");
    int status=(int)cvars->GetFloat("ftebroom_export_status");
    if(!status){if(std::chrono::steady_clock::now()-modRawStarted>std::chrono::seconds(120))throw std::runtime_error("MAP preview export timed out.");return;}
    if(status!=1)throw std::runtime_error("Cannot export the current MAP for Play mod.");
    auto stamp=std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto source=documentPath("data/mod-preview")/std::to_string(stamp)/"current.map";std::filesystem::create_directories(source.parent_path());
    auto bytes=ftebroom::rawPreviewMap(q1Read(documentPath("data/export-map.map")));
    std::ofstream output(source,std::ios::binary);output.write(bytes.data(),bytes.size());output.close();
    if(!output)throw std::runtime_error("Cannot write the MAP preview snapshot.");
    modRawExporting=runExclusive=gameProfileLocked=false;modLaunch(source,true);
  }catch(const std::exception& e){modRawExporting=runExclusive=gameProfileLocked=false;buildMessage=e.what();}
}
static void modPreviewPanel(){
  if(modChoiceOpen&&!ImGui::IsPopupOpen("Map has uncompiled changes"))ImGui::OpenPopup("Map has uncompiled changes");
  if(uiDialog("Map has uncompiled changes",{620,300})){
    ImGui::TextWrapped(modHasCompiledMap()?"The current editor contents are not verified against the last successful compilation. Choose what to play.":"This map has no compiled version for the current game profile. Choose what to play.");
    ImGui::BeginDisabled(runActive||runExclusive||buildRunning||assetsLoading);
    if(ImGui::Button("Compile changes and play"))modPreviewChoose(0);
    ImGui::BeginDisabled(!modHasCompiledMap());
    if(ImGui::Button("Play last compiled BSP"))modPreviewChoose(1);
    ImGui::EndDisabled();
    if(ImGui::Button("Play current MAP without compiling"))modPreviewChoose(2);
    ImGui::EndDisabled();
    ImGui::TextWrapped("MAP preview uses the current editor contents, including unsaved changes. It has no baked lightmaps or compiled visibility and can load more slowly than BSP.");
    if(!buildMessage.empty())ImGui::TextWrapped("%s",buildMessage.c_str());
    if(ImGui::Button("Cancel"))modChoiceOpen=false;
    if(!modChoiceOpen)ImGui::CloseCurrentPopup();ImGui::EndPopup();
  }
  if(uiClosedDialog=="Map has uncompiled changes"){modChoiceOpen=false;uiClosedDialog.clear();}
}
