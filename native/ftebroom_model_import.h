// SPDX-License-Identifier: GPL-2.0-or-later
#include "ftebroom_model_mdl_export.h"
#include <atomic>
struct ModelImportRequest {std::filesystem::path source,folder;int profile=0;};
static std::vector<ModelImportRequest> modelImportQueue;
static std::filesystem::path modelImportExternal,modelImportStage;
static int modelImportProfile=-1;
static char modelImportBlender[4096]={};
static std::future<std::string> modelImportJob;
static std::atomic<bool> modelImportDownloading{false};
static std::filesystem::path modelImportRuntimeRoot(){return settingsPath().parent_path()/"downloads/blender-5.0.0";}
static std::filesystem::path modelImportRuntime(){
#ifdef _WIN32
  return modelImportRuntimeRoot()/"blender-5.0.0-windows-x64/blender.exe";
#else
  return modelImportRuntimeRoot()/"blender-5.0.0-linux-x64/blender";
#endif
}
static bool modelImportRuntimeReady(){return std::filesystem::is_regular_file(modelImportRuntime())&&std::filesystem::is_regular_file(modelImportRuntime().parent_path()/"ftebroom-ready.txt");}
static bool modelImportSource(const std::filesystem::path& path) {
  auto ext=entityLower(path.extension().u8string());
  return modelFile(path)||ext==".fbx"||ext==".glb"||ext==".gltf"||ext==".blend"||ext==".md5"||ext==".smd"||ext==".dmx";
}
static bool modelImportBridge(const std::filesystem::path& path) {
  auto ext=entityLower(path.extension().u8string());return ext==".fbx"||ext==".blend"||ext==".glb"||ext==".gltf"||ext==".smd"||ext==".dmx";
}
static void modelImportEnqueue(const std::filesystem::path& source,const std::filesystem::path& folder) {
  if(!std::filesystem::is_regular_file(source)||std::filesystem::is_symlink(source))throw std::runtime_error("Import a regular model file.");
  if(std::filesystem::file_size(source)>512*1024*1024)throw std::runtime_error("Model import limit is 512 MiB.");
  if(modelImportQueue.size()>=32)throw std::runtime_error("Finish the pending model imports first (maximum 32).");
  modFileValidate(folder);modelImportQueue.push_back({source,folder,gameProfile});
}
static std::filesystem::path modelImportNewStage(){
  auto path=modFileRoot()/"models"/("import-"+std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
  modFileValidate(path);if(!std::filesystem::create_directories(path))throw std::runtime_error("Cannot create conversion staging directory.");return path;
}
static void modelImportFindBlender(){
  auto path=modelImportRuntime().u8string();
  std::snprintf(modelImportBlender,sizeof modelImportBlender,"%s",path.c_str());
}
static std::string modelImportRun(const std::vector<std::string>& args,const std::filesystem::path& output){
  try {
#ifdef _WIN32
    // Windows command-line quoting (including trailing backslashes), no shell.
    std::wstring line;
    for(const auto& arg:args){auto wide=std::filesystem::u8path(arg).wstring();if(!line.empty())line+=L' ';line+=L'"';unsigned slashes=0;for(auto ch:wide){if(ch==L'\\'){slashes++;continue;}line.append(ch==L'"'?slashes*2+1:slashes,L'\\');slashes=0;line+=ch;}line.append(slashes*2,L'\\');line+=L'"';}
    STARTUPINFOW si{};si.cb=sizeof si;PROCESS_INFORMATION pi{};
    if(!CreateProcessW(nullptr,line.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi))throw std::runtime_error("Cannot start the conversion component. Retry the download or check the installation permissions.");
    CloseHandle(pi.hThread);WaitForSingleObject(pi.hProcess,INFINITE);CloseHandle(pi.hProcess);
#else
    auto values=args;std::vector<char*> argv;for(auto& a:values)argv.push_back(a.data());argv.push_back(nullptr);pid_t process=-1;
    if(posix_spawnp(&process,argv[0],nullptr,nullptr,argv.data(),environ))throw std::runtime_error("Cannot start the conversion component. The downloader requires Python 3; Blender also requires the system libraries listed in the Linux documentation.");
    int status=0;while(waitpid(process,&status,0)<0&&errno==EINTR){}
#endif
    if(!std::filesystem::is_regular_file(output/"success.txt")){std::ifstream in(output/"error.txt");std::string error((std::istreambuf_iterator<char>(in)),{});throw std::runtime_error(error.empty()?"Blender import failed; no complete model was produced.":error);}
  }catch(const std::exception& e){return e.what();}return {};
}
static ftebroom::modelExport::Skin modelImportDecode(const std::string& bytes){
  if(bytes.size()>128&&(unsigned char)bytes[0]==10){auto t=ftebroom::legacy::pcx(bytes);return modelImportDecode(ftebroom::quake1::tga(t,{}));}
  int w=0,h=0,n=0;if(bytes.empty()||bytes.size()>64*1024*1024||!stbi_info_from_memory((const stbi_uc*)bytes.data(),bytes.size(),&w,&h,&n)||w<=0||h<=0||size_t(w)*h>16777216)throw std::runtime_error("Missing or unsupported model texture.");
  auto p=stbi_load_from_memory((const stbi_uc*)bytes.data(),bytes.size(),&w,&h,&n,4);if(!p)throw std::runtime_error("Cannot decode model texture.");
  ftebroom::modelExport::Skin skin;skin.width=w;skin.height=h;skin.rgba.assign(p,p+size_t(w)*h*4);stbi_image_free(p);return skin;
}
static ftebroom::modelExport::Skin modelImportHalfLifeSkin(std::string bytes,unsigned index,const std::string& resource){
  using namespace ftebroom::quake1;
  range(bytes,0,244);if(bytes.substr(0,4)!="IDST"||word(bytes,4)!=10)throw std::runtime_error("Expected a Half-Life MDL version 10.");
  if(!word(bytes,180)){
    auto source=modelImportExternal.empty()?std::filesystem::u8path(resource):modelImportExternal;
    auto texture=source.parent_path()/(source.stem().u8string()+"t.mdl");
    bytes=modelImportExternal.empty()?modelResource(texture.generic_u8string()):(std::filesystem::is_regular_file(texture)?q1Read(texture):std::string());
    if(bytes.empty()){texture=source.parent_path()/(source.stem().u8string()+"T.mdl");bytes=modelImportExternal.empty()?modelResource(texture.generic_u8string()):q1Read(texture);}range(bytes,0,244);
  }
  auto count=word(bytes,180),offset=word(bytes,184);if(count>256||index>=count)throw std::runtime_error("Invalid Half-Life texture index.");
  size_t at=size_t(offset)+index*80;range(bytes,at,80);unsigned w=word(bytes,at+68),h=word(bytes,at+72),data=word(bytes,at+76);
  if(!w||!h||size_t(w)*h>16777216)throw std::runtime_error("Invalid Half-Life skin dimensions.");
  size_t pixels=size_t(w)*h;range(bytes,data,pixels+768);ftebroom::modelExport::Skin skin;skin.width=w;skin.height=h;skin.rgba.resize(pixels*4);
  for(size_t p=0;p<pixels;p++){unsigned colour=(unsigned char)bytes[data+p];for(int a=0;a<3;a++)skin.rgba[p*4+a]=(unsigned char)bytes[data+pixels+colour*3+a];skin.rgba[p*4+3]=255;}return skin;
}
static std::string modelImportTga(const ftebroom::modelExport::Skin& s){
  std::string bytes(18,0);bytes[2]=2;bytes[12]=s.width;bytes[13]=s.width>>8;bytes[14]=s.height;bytes[15]=s.height>>8;bytes[16]=32;bytes[17]=40;
  for(size_t i=0;i<s.rgba.size();i+=4){bytes+=char(s.rgba[i+2]);bytes+=char(s.rgba[i+1]);bytes+=char(s.rgba[i]);bytes+=char(s.rgba[i+3]);}return bytes;
}
static std::string modelImportTexture(const std::string& material){
  auto name=std::filesystem::u8path(material);std::vector<std::filesystem::path> candidates;
  if(!modelImportStage.empty())candidates.push_back(modelImportStage/name.filename());
  if(!modelImportExternal.empty()){
    auto parent=modelImportExternal.parent_path();candidates.push_back(parent/name);candidates.push_back(parent/name.filename());
    // Models often reference paths relative to the game root rather than their
    // own directory. Search ancestors for this explicit dependency only.
    for(int i=0;i<4&&!parent.empty();i++,parent=parent.parent_path())candidates.push_back(parent/name);
  }
  for(const auto& base:candidates)for(const auto* suffix:{"",".tga",".png",".jpg",".jpeg",".pcx"}){
    auto path=base;path+=suffix;if(std::filesystem::is_regular_file(path)&&std::filesystem::file_size(path)<=64*1024*1024)return q1Read(path);
  }
  for(const auto* suffix:{"",".tga",".png",".jpg",".jpeg",".pcx"}){auto bytes=modelResource(material+suffix);if(!bytes.empty())return bytes;}
  throw std::runtime_error("Missing texture: "+material+". Keep the original texture files beside the source model or in its game directory.");
}
