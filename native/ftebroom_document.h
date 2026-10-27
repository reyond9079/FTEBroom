#include "ftebroom_map_profile.h"
// SPDX-License-Identifier: GPL-2.0-or-later
#include <chrono>
#include "ftebroom_recent.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

static std::map<std::string, ImVec2> fileBrowserTestItems;
static void fileBrowserTestItem(const std::string& name)
{
  if (!cvars->GetFloat("ftebroom_skip_setup"))
    return;
  auto a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
  fileBrowserTestItems[name] = {(a.x + b.x) * .5f, (a.y + b.y) * .5f};
}
static std::string documentName = "start", documentError, documentNext;
static std::string saveTemporary, saveDestination;
static std::filesystem::path documentFile, pendingDocumentFile, saveNativeTarget, fileBrowserFolder;
static std::string pendingDocumentAlias;
static int saveRevision;
static std::chrono::steady_clock::time_point saveProfileStarted;
static bool savingDocument, savingBackup, savingExport, confirmDocument;
static bool savePakDetached;
static std::string savePakBase,savePakMod;
static void restorePakMounts(){if(!savePakDetached)return;cvars->SetString("ftebroom_assetpath",savePakBase.c_str());cvars->SetString("ftebroom_modassetpath",savePakMod.c_str());savePakDetached=false;command("fs_restart\n");}
static bool exportRequest;
static bool objRequest, fileBrowserObj, savingObj;
static std::filesystem::path objDestination;
static bool documentUntitled;
static bool documentTerrain;
static bool newMapRequest,fileBrowserNew;
static int newMapProfile;
static unsigned long long saveSerial;
static auto lastBackup = std::chrono::steady_clock::now();
static std::filesystem::path recoveryFile, recoverySource;
static bool recoveryRequest, recoveryOpening;
static std::string backupStorageName()
{
  if (documentFile.empty()) return "untitled";
  uint64_t hash = 14695981039346656037ull;
  for (unsigned char c : documentFile.lexically_normal().u8string()) {hash ^= c; hash *= 1099511628211ull;}
  return "map-" + std::to_string(hash);
}

static std::filesystem::path documentPath(const std::string& relative)
{
  char path[4096];
  if (!files->NativePath(relative.c_str(), (fs_relative)5, path, sizeof path))
    return {};
  return std::filesystem::u8path(path);
}
static std::filesystem::path documentDefaultFolder()
{
  if(!documentFile.empty())return documentFile.parent_path();
  std::error_code error;
  if(*baseq3 && std::filesystem::is_directory(std::filesystem::u8path(baseq3),error))
    return std::filesystem::u8path(baseq3)/"maps";
  return documentPath("maps");
}
#include "ftebroom_lines.h"
#include "ftebroom_trace_files.h"
// Only known identifiers are accepted; map comments never contain local configuration.
static std::string documentMetadata(std::string bytes)
{
  if(bytes.rfind("// ReyondBroom profile=",0)==0 || bytes.rfind("// FTEBroom profile=",0)==0)bytes.erase(0,bytes.find('\n')+1);
  const char* profile=gameProfile==2?"quake1":gameProfile==1?"when-the-sky-went-dark":"quake3";
  const char* format=documentTerrain?"fte-terrain":gameProfile==2?(q1MapFormat?"valve220":"quake-standard"):"quake3";
  return std::string("// ReyondBroom profile=")+profile+" format="+format+"\n"+bytes;
}
static void documentSelectProfile(const std::string& bytes)
{
  auto header=ftebroom::mapProfile(bytes);int selected=header.profile;
  if(selected<0&&!pendingDocumentFile.empty()){recentLoad();for(const auto& entry:recentMaps)if(entry.path==pendingDocumentFile.lexically_normal().u8string()){selected=entry.profile;break;}}
  documentTerrain=header.terrain;if(header.format>=0)q1MapFormat=header.format;
  if(selected<0 || selected==gameProfile)return;
  if(gameProfile!=2)q3Root=baseq3;
  gameProfile=selected;
  std::snprintf(baseq3,sizeof baseq3,"%s",gameProfile==2?q1Authoring().c_str():q3Root.c_str());
  gameProfileSave();if(!mountAssets())setupNeeded=true;
}
static void terrainCopySections(const std::filesystem::path& from,const std::filesystem::path& to)
{
  if(!std::filesystem::is_directory(from)||from==to)return;
  for(const auto& entry:std::filesystem::recursive_directory_iterator(from)) {
    if(entry.is_symlink())throw std::runtime_error("Terrain section directories cannot contain symbolic links.");
    if(!entry.is_regular_file())continue;
    auto ext=entry.path().extension();if(ext!=".hms"&&ext!=".png"&&ext!=".tga")continue;
    auto target=to/entry.path().lexically_relative(from);std::filesystem::create_directories(target.parent_path());
    auto temporary=target;temporary+=".tmp";std::filesystem::copy_file(entry.path(),temporary,std::filesystem::copy_options::overwrite_existing);
#ifdef _WIN32
    if(!MoveFileExW(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot save terrain section.");
#else
    std::filesystem::rename(temporary,target);
#endif
  }
}
static bool objSave(const std::filesystem::path& target)
{
  if (savingObj) return false;
  objDestination = target; savingObj = true;
  cvars->SetFloat("ftebroom_obj_status", 0);
  action("ftebroom_export_obj\nftebroom_obj_finish\n");
  return true;
}
static void objSaveFinished()
{
  if (!savingObj) return;
  savingObj = false;
  std::filesystem::path staged, material;
  try
  {
    if (cvars->GetFloat("ftebroom_obj_status") != 1)
      throw std::runtime_error("OBJ export failed.");
    const auto serial = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    const auto materialName = "ftebroom-materials-" + serial + ".mtl";
    material = objDestination.parent_path() / materialName;
    staged = objDestination; staged += "." + serial + ".tmp";
    std::ifstream input(documentPath("data/export.obj"));
    std::ofstream output(staged);
    std::string line;
    while (std::getline(input, line))
      output << (line.rfind("mtllib ",0)==0 ? "mtllib " + materialName : line) << '\n';
    output.close();
    if (!input.eof() || !output) throw std::runtime_error("Cannot stage OBJ export.");
    std::filesystem::copy_file(documentPath("data/export.mtl"), material);
#ifdef _WIN32
    if (!MoveFileExW(staged.c_str(),objDestination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
      throw std::runtime_error("Cannot replace OBJ destination.");
#else
    std::filesystem::rename(staged,objDestination);
#endif
    core->Print("ReyondBroom: OBJ and MTL exported.\n");
  }
  catch (const std::exception& error)
  {
    documentError = error.what(); std::error_code ignored;
    if (!staged.empty()) std::filesystem::remove(staged,ignored);
    if (!material.empty()) std::filesystem::remove(material,ignored);
  }
}
static void documentProceed()
{
  const auto next = documentNext;
  documentNext.clear();
  if (next == "exit")
    action("quit force\n");
  else if (next.rfind("__profile:",0)==0) {
    int selected=std::atoi(next.c_str()+10);if(selected<0||selected>2)return;
    if(gameProfile!=2)q3Root=baseq3;gameProfile=selected;
    std::snprintf(baseq3,sizeof baseq3,"%s",selected==2?q1Authoring().c_str():q3Root.c_str());
    gameProfileSave();if(!mountAssets())setupNeeded=true;
    documentTerrain=false;documentUntitled=true;documentFile.clear();pendingDocumentFile.clear();pendingDocumentAlias.clear();documentName="untitled";
    cvars->SetFloat("ftebroom_terrain_document",0);cvars->SetFloat("ftebroom_terrain_edit",0);
    std::string scene=q1Read(documentPath("maps/start.map"));
    if(selected!=0){size_t at=0;while((at=scene.find("info_player_deathmatch",at))!=scene.npos){scene.replace(at,22,"info_player_start");at+=17;}}
    if(selected==2){size_t at=0;while((at=scene.find("ftebroom/grid",at))!=scene.npos){scene.replace(at,13,"fb_grey");at+=7;}}
    std::ofstream(documentPath("maps/_ftebroom_profile_start.map"),std::ios::binary)<<documentMetadata(scene);
    action("mapedit maps/_ftebroom_profile_start.map\n");
  }
  else if (next == "__new")
  {
    newMapRequest=true;newMapProfile=gameProfile;

  }
  else if (next.rfind("@", 0) == 0)
  {
    try
    {
      pendingDocumentFile = std::filesystem::u8path(next.substr(1));
      documentSelectProfile(q1Read(pendingDocumentFile));
      if(gameProfile==2){std::string member;std::filesystem::path archive;ftebroom::pak::split(pendingDocumentFile,archive,member);bool changed=archive!=q1DocumentPak;q1DocumentPak=archive;auto source=q1Read(pendingDocumentFile);q1MapFormat=ftebroom::quake1::valveMap(source)?1:0;if(q1DiscoverWads(source,pendingDocumentFile)||changed){gameProfileSave();mountAssets();}}
      pendingDocumentAlias =
        "_ftebroom_open_"
        + std::to_string(std::chrono::system_clock::now().time_since_epoch().count())
        + "_" + std::to_string(++saveSerial);
      {auto bytes=q1Read(pendingDocumentFile);std::ofstream out(documentPath("maps/"+pendingDocumentAlias+".map"),std::ios::binary);out<<bytes;out.close();if(!out)throw std::runtime_error("Cannot stage map from file/archive.");}
      if(documentTerrain)terrainCopySections(pendingDocumentFile.parent_path()/pendingDocumentFile.stem(),documentPath("maps/"+pendingDocumentAlias));
      cvars->SetFloat("ftebroom_terrain_dirty",0);cvars->SetFloat("ftebroom_terrain_document",documentTerrain);cvars->SetFloat("ftebroom_terrain_edit",documentTerrain);
      action(("mapedit maps/" + pendingDocumentAlias + ".map\n").c_str());
    }
    catch (const std::exception& error)
    {
      documentError = error.what();
      pendingDocumentFile.clear();
    }
  }
  else if (!next.empty())
    action(("mapedit maps/" + next + ".map\n").c_str());
}
static bool documentSave(
  const std::string& name,
  bool backup = false,
  const std::filesystem::path& destination = {},
  bool compile = false)
{
  if (
    savingDocument
    || (!safeName(name.c_str()) && documentFile.empty() && destination.empty()))
    return false;
  documentError.clear();
  saveProfileStarted=std::chrono::steady_clock::now();
  const auto serial = std::chrono::system_clock::now().time_since_epoch().count();
  saveTemporary =
    "_ftebroom_save_" + std::to_string(serial) + "_" + std::to_string(++saveSerial);
  const std::string storageName = backup ? backupStorageName() :
    safeName(name.c_str())
      ? name
      : "external_" + std::to_string(std::hash<std::string>{}(documentFile.u8string()));
  saveDestination = "maps/" + storageName + ".map";
  saveNativeTarget = destination;
  if (saveNativeTarget.empty() && !backup && name == documentName)
    saveNativeTarget = documentFile;
  if (backup)
  {
    saveNativeTarget.clear();
    int slot = 0;
    auto oldest = std::filesystem::file_time_type::max();
    for (int i = 0; i < std::clamp((int)cvars->GetFloat("ftebroom_autosave_count"),1,200); ++i)
    {
      const auto path =
        documentPath("autosave/" + storageName + "-" + std::to_string(i) + ".map");
      std::error_code error;
      if (!std::filesystem::exists(path, error))
      {
        slot = i;
        break;
      }
      const auto modified = std::filesystem::last_write_time(path, error);
      if (!error && modified < oldest)
      {
        oldest = modified;
        slot = i;
      }
    }
    saveDestination = "autosave/" + storageName + "-" + std::to_string(slot) + ".map";
  }
  if (documentPath("maps/" + saveTemporary + ".map").empty())
  {
    documentError = "Cannot locate the writable editor directory.";
    return false;
  }
  saveRevision = (int)cvars->GetFloat("ftebroom_revision");
  savingDocument = true;
  savingBackup = backup;
  savingExport = compile;
  if (compile)
  {
    cvars->SetFloat("ftebroom_export_status", 0);
    action("ftebroom_export_map\nftebroom_save_finish\n");
    return true;
  }
  if(documentTerrain)command("ftebroom_terrain_save\n");
  action(("mod_terrain_save \"\" " + saveTemporary + "\nftebroom_save_finish\n").c_str());
  return true;
}
static void documentSaveFinished()
{
  if (!savingDocument)
    return;
  const auto saveEngineFinished=std::chrono::steady_clock::now();
  savingDocument = false;
  const auto source =
    documentPath(savingExport ? "data/export-map.map" : "maps/" + saveTemporary + ".map");
  const auto target =
    saveNativeTarget.empty() ? documentPath(saveDestination) : saveNativeTarget;
  std::filesystem::path pakArchive;std::string pakMember;
  if(!savePakDetached&&ftebroom::pak::split(target,pakArchive,pakMember)){
    char base[4096]={},mod[4096]={};cvars->GetString("ftebroom_assetpath",base,sizeof base);cvars->GetString("ftebroom_modassetpath",mod,sizeof mod);savePakBase=base;savePakMod=mod;
    savePakDetached=true;savingDocument=true;cvars->SetString("ftebroom_assetpath","");cvars->SetString("ftebroom_modassetpath","");command("fs_restart\nftebroom_save_finish\n");return;
  }
  try
  {
    if (
      (savingExport && cvars->GetFloat("ftebroom_export_status") != 1) || source.empty()
      || target.empty() || !std::filesystem::exists(source)
      || std::filesystem::file_size(source) == 0)
      throw std::runtime_error("The engine did not write the map.");
    std::filesystem::path archive;std::string member;
    bool packed=ftebroom::pak::split(target,archive,member);
    if(packed){if(savingExport)throw std::runtime_error("Export compiler input to a folder, not a PAK.");auto bytes=ftebroom::quake1::map(q1Read(source),q1MapFormat==0,q1WadList(),true);ftebroom::pak::replace(target,documentMetadata(bytes));}
    else {
    std::filesystem::create_directories(target.parent_path());
    // Stage beside the destination so replacement remains atomic across drives.
    auto adjacent = target;
    adjacent += ".ftebroom-" + std::to_string(saveSerial) + ".tmp";
    if(gameProfile==2){auto converted=ftebroom::quake1::map(q1Read(source),savingExport?false:q1MapFormat==0,savingExport?q1CompilerWads(target.parent_path()):q1WadList(),true);
      std::ofstream output(adjacent,std::ios::binary);output<<(savingExport?converted:documentMetadata(converted));output.close();if(!output)throw std::runtime_error("Cannot write Quake map.");}
    else {std::ofstream output(adjacent,std::ios::binary);auto bytes=q1Read(source);output<<(savingExport?bytes:documentMetadata(bytes));output.close();if(!output)throw std::runtime_error("Cannot write map.");}
#ifdef _WIN32
    if (!MoveFileExW(
          adjacent.c_str(),
          target.c_str(),
          MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
      std::filesystem::remove(adjacent);
      throw std::runtime_error("Cannot replace the destination file.");
    }
#else
    std::filesystem::rename(adjacent, target);
#endif
    }
    if(documentTerrain&&!savingExport) {char loaded[256]={};cvars->GetString("mapname",loaded,sizeof loaded);auto stem=std::filesystem::u8path(loaded).stem().u8string();terrainCopySections(documentPath("maps/"+stem),target.parent_path()/target.stem());}
    restorePakMounts();
    std::error_code cleanupError;
    std::filesystem::remove(source, cleanupError);
    core->Print(("FTEBROOM_SAVED " + saveDestination + "\n").c_str());
    if (savingBackup)
    {
      auto sourcePath = target; sourcePath += ".source";
      std::ofstream(sourcePath) << documentFile.u8string();
      int retained=std::clamp((int)cvars->GetFloat("ftebroom_autosave_count"),1,200);
      for(int i=retained;i<200;i++)
      {
        auto old=documentPath("autosave/"+backupStorageName()+"-"+std::to_string(i)+".map");
        std::error_code ignored;std::filesystem::remove(old,ignored);old+=".source";std::filesystem::remove(old,ignored);
      }
    }
    if (!savingBackup && !savingExport)
    {
      documentIndexLines(target);
      documentFile = target;
      recentRemember(target,gameProfile);
      fileBrowserFolder=target.parent_path();
      cvars->SetString("ftebroom_document_path",target.u8string().c_str());
      documentName = target.stem().u8string();
      documentUntitled = false;
      std::snprintf(mapName, sizeof mapName, "%s", documentName.c_str());
      command(("ftebroom_document_saved " + std::to_string(saveRevision) + "\n").c_str());
      if ((int)cvars->GetFloat("ftebroom_revision") == saveRevision)
        documentProceed();
      else
        documentNext.clear();
    }
    if(cvars->GetFloat("ftebroom_profile_saves")) {
      const auto ended=std::chrono::steady_clock::now();
      auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
      core->Print(("RB_SAVE_PROFILE total_ms="+std::to_string(ms(saveProfileStarted,ended))+" engine_ms="+std::to_string(ms(saveProfileStarted,saveEngineFinished))+" finish_ms="+std::to_string(ms(saveEngineFinished,ended))+"\n").c_str());
    }
  }
  catch (const std::exception& error)
  {
    restorePakMounts();
    documentError = error.what();
    documentNext.clear();
    core->Print(("ReyondBroom save failed: " + documentError + "\n").c_str());
  }
}
static void documentRequest(const std::string& next)
{
  if (savingDocument)
    return;
  if (next.rfind("@", 0) == 0)
  {
    auto path=std::filesystem::u8path(next.substr(1));
    if(!ftebroom::pak::exists(path)){documentError="The requested map is missing or unreadable.";return;}

  }
  else if (next != "exit" && next != "__new" && next.rfind("__profile:",0)!=0)
  {
    qhandle_t handle;
    const int size = files->Open(("maps/" + next + ".map").c_str(), &handle, 1);
    if (handle >= 0)
      files->Close(handle);
    if (size <= 0)
    {
      documentError = "The requested map is missing or empty.";
      return;
    }
  }
  cancelGesture();
  documentNext = next;
  if (cvars->GetFloat("ftebroom_dirty"))
    confirmDocument = true;
  else
    documentProceed();
}
static void crashContextTick()
{
  static quintptr_t last=0;static std::string previous;
  auto source=!pendingDocumentFile.empty()?pendingDocumentFile:documentFile;
  auto now=core->GetMilliseconds();if(source.u8string()==previous&&now-last<2000)return;last=now;previous=source.u8string();
  try {
    auto fallback=documentPath("diagnostics");std::filesystem::create_directories(fallback);
    auto name=source.empty()?"untitled":source.stem().u8string();
    auto fallbackReport=fallback/(name+"_crash.txt");auto target=source.empty()?fallbackReport:source.parent_path()/(name+"_crash.txt");
    auto context=fallback/"session-context.txt";std::ofstream out(context,std::ios::binary|std::ios::trunc);
    out<<"ReyondBroom session context\nMap: "<<source.u8string()<<"\nProfile: "<<gameProfileName()<<"\nRevision: "<<cvars->GetFloat("ftebroom_revision")<<"\nBSP preview: "<<cvars->GetFloat("ftebroom_bsp_preview")<<"\nRecent editor commands:\n"<<crashCommands<<"\nConsole tail:\n";out.close();
    cvars->SetString("ftebroom_crash_path",target.u8string().c_str());cvars->SetString("ftebroom_crash_fallback",fallbackReport.u8string().c_str());cvars->SetString("ftebroom_crash_context",context.u8string().c_str());cvars->SetString("ftebroom_crash_console",documentPath("qconsole.log").u8string().c_str());
  }catch(...){}
}
static void documentTick()
{
  crashContextTick();
  static bool recoveryChecked = false;
  if (!recoveryChecked)
  {
    recoveryChecked = true;
    if (!cvars->GetFloat("ftebroom_skip_setup"))
    {
      std::error_code error;
      auto newest = std::filesystem::file_time_type::min();
      for (std::filesystem::directory_iterator it(documentPath("autosave"),error),end; it!=end; it.increment(error))
      {
        if (error) break;
        if (it->path().extension()!=".map") continue;
        auto sourcePath=it->path(); sourcePath += ".source";
        std::ifstream stream(sourcePath); std::string source; std::getline(stream,source);
        if (!stream && source.empty()) continue;
        auto backupTime=std::filesystem::last_write_time(it->path(),error); if(error){error.clear();continue;}
        auto original=std::filesystem::u8path(source);
        auto originalTime=std::filesystem::last_write_time(original,error);
        bool newer=source.empty() || error || backupTime>originalTime;error.clear();
        if(newer && backupTime>newest){newest=backupTime;recoveryFile=it->path();recoverySource=original;recoveryRequest=true;}
      }
    }
  }
  const auto now = std::chrono::steady_clock::now();
  const double minutes = cvars->GetFloat("ftebroom_autosave_interval");
  if (
    minutes <= 0
    || std::chrono::duration<double>(now - lastBackup).count() < minutes * 60)
    return;
  if (!savingDocument && !sceneDrag && !cvars->GetFloat("ftebroom_bsp_preview") && cvars->GetFloat("ftebroom_dirty"))
  {
    documentSave(documentName, true);
    lastBackup = now;
  }
}
static void documentDialogs()
{
  if (recoveryRequest) {ImGui::OpenPopup("Recover autosave?");recoveryRequest=false;}
  if (uiDialog("Recover autosave?"))
  {
    ImGui::TextWrapped("A newer autosave is available for %s. Restore it as an unsaved document?",recoverySource.empty()?"an untitled map":recoverySource.u8string().c_str());
    if(ImGui::Button("Recover")) {recoveryOpening=true;documentRequest("@"+recoveryFile.u8string());ImGui::CloseCurrentPopup();}
    ImGui::SameLine();if(ImGui::Button("Keep current map")) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
  if (confirmDocument)
  {
    ImGui::OpenPopup("Save changes?");
    confirmDocument = false;
  }
  if (uiDialog("Save changes?"))
  {
    ImGui::Text("Save changes to %s.map?", documentName.c_str());
    if (ImGui::Button("Save"))
    {
      if (documentUntitled)
      {
        saveRequest = true;
        inspector = true;
      }
      else
        documentSave(documentName);
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Don't Save"))
    {
      documentProceed();
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
      documentNext.clear();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
  if(uiClosedDialog=="Document error"){documentError.clear();uiClosedDialog.clear();}
  if(uiClosedDialog=="Save changes?"){documentNext.clear();uiClosedDialog.clear();}
  if (!documentError.empty() && !ImGui::IsPopupOpen("Map files"))
  {
    ImGui::OpenPopup("Document error");
    if (uiDialog("Document error"))
    {
      ImGui::TextWrapped("%s", documentError.c_str());
      if (ImGui::Button("OK"))
      {
        documentError.clear();
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }
}
static void documentSaveCommand()
{
  char name[64];
  commands->Argv(1, name, sizeof name);
  documentSave(*name ? name : documentName);
}
static void testBackups()
{
  if(!cvars->GetFloat("ftebroom_skip_setup"))return;
  char argument[16]={};commands->Argv(1,argument,sizeof argument);
  if(std::atoi(argument)==0){cvars->SetFloat("ftebroom_autosave_count",2);documentSave(documentName,true);return;}
  int count=0;bool valid=true;std::error_code error;
  for(std::filesystem::directory_iterator it(documentPath("autosave"),error),end;it!=end;it.increment(error))
  {
    if(error){valid=false;break;}
    if(it->path().extension()!=".map"||it->path().filename().u8string().rfind(backupStorageName()+"-",0)!=0)continue;
    ++count;auto source=it->path();source+=".source";std::ifstream input(source);std::string text;std::getline(input,text);
    valid=valid&&text==documentFile.u8string()&&std::filesystem::file_size(it->path())>100;
  }
  core->Print(valid&&count==2?"FTEBROOM_BACKUP_PASS bounded_rotation_and_source\n":"FTEBROOM_BACKUP_FAIL bounded_rotation_and_source\n");
}

// Paths stay in native filesystem APIs; only generated ASCII aliases reach FTE commands.
static bool fileBrowserSaving, fileBrowserExport, fileBrowserDefinitions,
  definitionsRequest;
static void definitionsChoose(const std::filesystem::path& path);

static char fileBrowserPath[4096] = {}, fileBrowserName[1024] = {};
static std::filesystem::path overwriteTarget;
static void documentFileBrowser()
{
  if (newMapRequest || openRequest || saveRequest || definitionsRequest || exportRequest || traceRequest || objRequest)
  {
    cancelGesture();
    fileBrowserNew=newMapRequest;newMapRequest=false;
    fileBrowserTrace = traceRequest;
    traceRequest = 0;
    fileBrowserObj = objRequest; objRequest = false;
    fileBrowserSaving = fileBrowserNew || saveRequest || exportRequest || fileBrowserObj;
    fileBrowserExport = exportRequest;
    exportRequest = false;
    fileBrowserDefinitions = definitionsRequest;
    definitionsRequest = false;
    openRequest = saveRequest = false;
    if(!fileBrowserDefinitions && !fileBrowserTrace)fileBrowserFolder=documentDefaultFolder();
    else if(fileBrowserFolder.empty())fileBrowserFolder=documentDefaultFolder();
    std::snprintf(
      fileBrowserPath,
      sizeof fileBrowserPath,
      "%s",
      fileBrowserFolder.u8string().c_str());
    std::snprintf(
      fileBrowserName,
      sizeof fileBrowserName,
      "%s",
      fileBrowserSaving ? (documentName + (fileBrowserObj ? ".obj" : ".map")).c_str() : "");
    if(fileBrowserNew){
      std::snprintf(fileBrowserName,sizeof fileBrowserName,"untitled.map");
      auto root=newMapProfile==2?q1Authoring():std::string(baseq3);
      fileBrowserFolder=root.empty()?documentPath("maps"):std::filesystem::u8path(root)/"maps";
      std::snprintf(fileBrowserPath,sizeof fileBrowserPath,"%s",fileBrowserFolder.u8string().c_str());
    }
    documentError.clear();
    ImGui::OpenPopup("Map files");
  }
  ImGui::SetNextWindowSize({720, 520}, ImGuiCond_FirstUseEver);
  if (uiDialog("Map files",{760,540}))
  {
    if(fileBrowserNew) {
      ImGui::TextUnformatted("Create a new map: choose its game profile and destination.");
      if(ImGui::Combo("Game profile",&newMapProfile,"Quake III Arena\0FTE Custom game\0Quake 1\0")) {
        auto root=newMapProfile==2?q1Authoring():(gameProfile!=2?std::string(baseq3):q3Root);
        fileBrowserFolder=root.empty()?documentPath("maps"):std::filesystem::u8path(root)/"maps";
        std::snprintf(fileBrowserPath,sizeof fileBrowserPath,"%s",fileBrowserFolder.u8string().c_str());
      }
    }
    ImGui::TextUnformatted(
      fileBrowserObj          ? "Export Wavefront OBJ + MTL (omitted layers excluded)"
      : fileBrowserTrace == 1 ? "Load point file (.lin)"
      : fileBrowserTrace == 2  ? "Load portal file (.prt)"
      : fileBrowserExport     ? "Export map for compilation (omitted layers excluded)"
      : fileBrowserDefinitions ? "Open entity definitions"
      : fileBrowserNew         ? "Choose where to create the map"
      : fileBrowserSaving      ? "Save map as"
                               : "Open map");
    bool canGoUp = fileBrowserFolder.has_parent_path() && fileBrowserFolder.parent_path() != fileBrowserFolder;
    ImGui::BeginDisabled(!canGoUp);
    bool upClicked = ImGui::Button("Up one level");
    ImGui::EndDisabled();
    fileBrowserTestItem("up");
    if (upClicked)
    {
      fileBrowserFolder = fileBrowserFolder.parent_path();
      documentError.clear();
      if (!fileBrowserSaving) fileBrowserName[0] = 0;
      std::snprintf(
        fileBrowserPath,
        sizeof fileBrowserPath,
        "%s",
        fileBrowserFolder.u8string().c_str());
    }
#ifdef _WIN32
    ImGui::SameLine();
    if (ImGui::BeginCombo("Drive", fileBrowserFolder.root_name().u8string().c_str()))
    {
      DWORD drives = GetLogicalDrives();
      for (int i = 0; i < 26; i++)
        if (drives & (1u << i))
        {
          char drive[] = "A:/";
          drive[0] += i;
          if (ImGui::Selectable(drive))
          {
            fileBrowserFolder = std::filesystem::u8path(drive);
            std::snprintf(fileBrowserPath, sizeof fileBrowserPath, "%s", drive);
          }
        }
      ImGui::EndCombo();
    }
#endif
    bool go = ImGui::InputText(
      "Folder",
      fileBrowserPath,
      sizeof fileBrowserPath,
      ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    go |= ImGui::Button("Go");
    if (go)
    {
      std::error_code error;
      auto folder = std::filesystem::u8path(fileBrowserPath);
      if (std::filesystem::is_directory(folder, error) || (gameProfile==2 && [&](){try{ftebroom::pak::list(folder);return true;}catch(...){return false;}}()))
      {
        fileBrowserFolder = folder;
        documentError.clear();
      }
      else
        documentError = "Folder does not exist or cannot be read.";
    }
    ImGui::BeginChild("File list", {0, -105}, ImGuiChildFlags_Borders);
    std::error_code error;
    std::vector<std::pair<bool, std::filesystem::path>> entries;
    try {
      auto found=ftebroom::pak::list(fileBrowserFolder);
      for(const auto& entry:found){auto extension=entry.second.extension().u8string();std::transform(extension.begin(),extension.end(),extension.begin(),::tolower);
        bool pak=extension==".pak";bool allowedPak=gameProfile==2&&!fileBrowserDefinitions&&!fileBrowserTrace&&!fileBrowserObj;
        if(pak&&!allowedPak)continue;
        if(entry.first || (fileBrowserObj ? extension==".obj" : fileBrowserTrace ? extension==(fileBrowserTrace==1?".lin":".prt") : fileBrowserDefinitions ? (extension==".ent"||extension==".def"||extension==".fgd") : extension==".map"))entries.push_back(entry);
      }
    }catch(const std::exception& e){if(fileBrowserNew&&!std::filesystem::exists(fileBrowserFolder))ImGui::TextWrapped("This folder will be created when you press Create map. Use Up one level to browse its parent.");else ImGui::TextWrapped("%s",e.what());}
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
      return a.first != b.first ? a.first > b.first
                                : a.second.filename() < b.second.filename();
    });
    for (auto& entry : entries)
    {
      auto name = entry.second.filename().u8string();
      auto label = (entry.first ? (entry.second.extension()==".pak"?"[PAK] ":"[Folder] ") : "") + name;
      bool entryClicked = ImGui::Selectable(
        label.c_str(), name == fileBrowserName, ImGuiSelectableFlags_AllowDoubleClick);
      fileBrowserTestItem(name);
      if (entryClicked)
      {
        if (entry.first)
        {
          fileBrowserFolder = entry.second;
          std::snprintf(
            fileBrowserPath,
            sizeof fileBrowserPath,
            "%s",
            fileBrowserFolder.u8string().c_str());
        }
        else if (!entry.first)
          std::snprintf(fileBrowserName, sizeof fileBrowserName, "%s", name.c_str());
      }
    }
    if (error)
      ImGui::TextWrapped("Cannot read this folder: %s", error.message().c_str());
    ImGui::EndChild();
    ImGui::InputText("File name", fileBrowserName, sizeof fileBrowserName);
    if (!documentError.empty())
      ImGui::TextWrapped("%s", documentError.c_str());
    bool primaryClicked = ImGui::Button(fileBrowserNew ? "Create map" : fileBrowserSaving ? "Save" : "Open");
    fileBrowserTestItem("primary");
    if (primaryClicked)
    {
      auto target = fileBrowserFolder / std::filesystem::u8path(fileBrowserName);
      if (target.extension().empty() && !fileBrowserDefinitions)
        target += fileBrowserObj ? ".obj" : fileBrowserTrace == 1 ? ".lin" : fileBrowserTrace == 2 ? ".prt" : ".map";
      auto extension = target.extension().u8string();
      std::transform(
        extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
          return char(std::tolower(c));
        });
      if (fileBrowserTrace)
      {
        if (traceChoose(fileBrowserTrace, target)) ImGui::CloseCurrentPopup();
      }
      else if (fileBrowserDefinitions)
      {
        if (extension != ".ent" && extension != ".def" && extension != ".fgd")
          documentError = "Choose an .ent, .def or .fgd file.";
        else if (!std::filesystem::is_regular_file(target, error))
          documentError = "Definition file does not exist.";
        else
        {
          definitionsChoose(target);
          ImGui::CloseCurrentPopup();
        }
      }
      else if (!*fileBrowserName || extension != (fileBrowserObj ? ".obj" : ".map"))
        documentError = fileBrowserObj ? "Choose an .obj file." : "Choose a .map file.";
      else if(fileBrowserNew) {
        try {
          if(std::filesystem::exists(target))throw std::runtime_error("Choose a new filename; existing maps are not replaced.");
          std::filesystem::create_directories(target.parent_path());
          const char* profile=newMapProfile==2?"quake1":newMapProfile==1?"when-the-sky-went-dark":"quake3";
          std::ofstream out(target,std::ios::binary);
          out<<"// ReyondBroom profile="<<profile<<" format="<<(newMapProfile==2?(q1MapFormat?"valve220":"quake-standard"):"quake3")<<"\n{\n\"classname\" \"worldspawn\"\n}\n{\n\"classname\" \""<<(newMapProfile!=0?"info_player_start":"info_player_deathmatch")<<"\"\n\"origin\" \"0 0 64\"\n}\n";
          out.close();if(!out)throw std::runtime_error("Cannot create map.");
          documentNext="@"+target.u8string();documentProceed();fileBrowserNew=false;ImGui::CloseCurrentPopup();
        }catch(const std::exception& e){documentError=e.what();}
      }
      else if (fileBrowserSaving)
      {
        if (ftebroom::pak::exists(target))
        {
          overwriteTarget = target;
          ImGui::OpenPopup("Replace map?");
        }
        else if (fileBrowserObj ? objSave(target) : documentSave(documentName, false, target, fileBrowserExport))
          ImGui::CloseCurrentPopup();
      }
      else
      {
        documentError.clear();
        documentRequest("@" + target.u8string());
        if (documentError.empty())
          ImGui::CloseCurrentPopup();
      }
    }
    ImGui::SameLine();
    if (
      ImGui::Button("Cancel")
      || (ImGui::IsKeyPressed(ImGuiKey_Escape) && !ImGui::IsPopupOpen("Replace map?")))
    {
      documentNext.clear();
      documentError.clear();
      ImGui::CloseCurrentPopup();
    }
    if (uiDialog("Replace map?"))
    {
      ImGui::TextWrapped("Replace %s?", overwriteTarget.u8string().c_str());
      if (ImGui::Button("Replace"))
      {
        if (fileBrowserObj ? objSave(overwriteTarget) : documentSave(documentName, false, overwriteTarget, fileBrowserExport))
        {
          ImGui::CloseCurrentPopup();
          fileBrowserSaving = false;
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel"))
        ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }
    // Successful replacement closes the parent on the next frame.
    if (savingDocument || savingObj)
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
}
