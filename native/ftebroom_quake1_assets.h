// SPDX-License-Identifier: GPL-2.0-or-later
#include "ftebroom_quake1.h"
#include "ftebroom_quake_palette.h"
#include "ftebroom_map_wads.h"
static std::string q1Palette(){
  size_t size=0;auto data=(char*)files->LoadFile("gfx/palette.lmp",&size);
  if(!data)return ftebroom::defaultQuakePalette();
  std::string palette(data,size);core->Free(data);
  if(palette.size()!=768)throw std::runtime_error("Invalid Quake palette.lmp: expected 768 bytes.");
  return palette;
}
static std::map<std::string,std::string> q1MaterialWad,q1WadLabels;
static std::vector<qhandle_t> q1ShaderHandles;
static std::string q1Read(const std::filesystem::path& path) {return ftebroom::pak::read(path);}
static std::vector<std::filesystem::path> q1ResolvedWads;
static std::vector<std::filesystem::path> q1MissingMapWads, q1MissingWads;
static std::filesystem::path q1DocumentPak;
static const std::vector<std::filesystem::path>& q1WadPaths(){return q1ResolvedWads;}
static void q1FindWads(bool strict=false){
  q1MissingWads=q1MissingMapWads;
  q1ResolvedWads.clear();q1WadLabels.clear();std::vector<std::filesystem::path> sources;
  char builtin[4096]={};if(files->NativePath("quake1-editor.wad",(fs_relative)5,builtin,sizeof builtin))sources.push_back(std::filesystem::u8path(builtin));
  std::vector<std::string> roots={q1Authoring(),gameRuntime()};if(roots[0]==roots[1])roots.pop_back();
  auto base=q1BaseDirectory();if(!base.empty())roots.insert(roots.begin(),base);
  for(const auto& root:roots){if(root.empty())continue;std::vector<std::filesystem::path> archives,loose;
    for(std::filesystem::recursive_directory_iterator it(std::filesystem::u8path(root),std::filesystem::directory_options::skip_permission_denied),end;it!=end;++it){
      const auto& e=*it;auto leaf=e.path().filename().u8string();
      if(e.is_directory()&&(leaf=="_ftebroom-build"||leaf=="_ftebroom-wads"||leaf=="cache")){it.disable_recursion_pending();continue;}
      if(!e.is_regular_file())continue;auto ext=e.path().extension().u8string();std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);
      if(ext==".pak")archives.push_back(e.path());else if(ext==".wad")loose.push_back(e.path());
    }
    std::sort(archives.begin(),archives.end());std::sort(loose.begin(),loose.end());
    for(const auto& archive:archives)for(const auto& entry:ftebroom::pak::directory(archive)){auto ext=std::filesystem::u8path(entry.name).extension().u8string();std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);if(ext==".wad")sources.push_back(archive/std::filesystem::u8path(entry.name));}
    sources.insert(sources.end(),loose.begin(),loose.end());
  }
  if(!q1DocumentPak.empty())for(const auto& e:ftebroom::pak::directory(q1DocumentPak)){auto ext=std::filesystem::u8path(e.name).extension().u8string();std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);if(ext==".wad")sources.push_back(q1DocumentPak/std::filesystem::u8path(e.name));}
  std::istringstream input(q1Wads);std::string line;
  while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;auto path=std::filesystem::u8path(line);if(path.is_relative())path=std::filesystem::u8path(q1Authoring())/path;sources.push_back(path);}
  char writable[4096]={};if(!files->NativePath("ftebroom.cfg",(fs_relative)5,writable,sizeof writable))throw std::runtime_error("No writable editor cache.");auto cache=std::filesystem::u8path(writable).parent_path()/"cache/wads";std::filesystem::create_directories(cache);
  std::set<std::filesystem::path> seen;
  for(const auto& source:sources){if(!seen.insert(source.lexically_normal()).second)continue;
    if(!ftebroom::pak::exists(source)){if(std::find(q1MissingWads.begin(),q1MissingWads.end(),source)==q1MissingWads.end())q1MissingWads.push_back(source);continue;}
    std::filesystem::path archive;std::string member;if(ftebroom::pak::split(source,archive,member)){
      auto bytes=q1Read(source);ftebroom::quake1::wad(bytes);auto id=std::to_string(std::hash<std::string>{}(source.generic_u8string()));auto target=cache/(id+"-"+source.filename().u8string());std::ofstream out(target,std::ios::binary);out.write(bytes.data(),bytes.size());out.close();if(!out)throw std::runtime_error("Cannot cache packed WAD.");q1ResolvedWads.push_back(target);q1WadLabels[target.u8string()]=archive.filename().u8string()+"/"+member;
    }else q1ResolvedWads.push_back(source);
  }
  // A user can reconnect a relocated archive without editing the original MAP.
  q1MissingWads.erase(std::remove_if(q1MissingWads.begin(),q1MissingWads.end(),[&](const auto& missing){
    return std::any_of(sources.begin(),sources.end(),[&](const auto& present){return missing.filename()==present.filename() && ftebroom::pak::exists(present);});
  }),q1MissingWads.end());
  for(const auto& path:q1MissingWads)core->Print(("ReyondBroom: missing WAD archive: "+path.u8string()+"\n").c_str());
  if(strict && !q1MissingWads.empty())throw std::runtime_error("Missing WAD archive: "+q1MissingWads.front().u8string()+". Reconnect the archive in Material / Add WAD(s) before compiling.");
}
static std::string q1WadList() {
  std::string list;for(auto& path:q1WadPaths()){auto name=path.generic_u8string();
    if(name.find_first_of(";\"\r\n")!=std::string::npos)throw std::runtime_error("WAD path contains a separator or quote.");
    if(!list.empty())list+=';';list+=name;}
  return list;
}
// Compiler input uses ASCII relative paths, independent of external Unicode names.
static std::string q1CompilerWads(const std::filesystem::path& folder) {
  q1FindWads(true);
  auto directory=folder/"_ftebroom-wads";std::filesystem::create_directories(directory);
  std::string list;int index=0;
  // qbsp resolves the first matching texture; the editor gives the last WAD priority.
  for(auto it=q1WadPaths().rbegin();it!=q1WadPaths().rend();++it) {
    auto relative="_ftebroom-wads/wad-"+std::to_string(index++)+".wad";
    auto bytes=q1Read(*it);auto target=folder/relative;
    std::ofstream output(target,std::ios::binary);output.write(bytes.data(),bytes.size());output.close();
    if(!output)throw std::runtime_error("Cannot stage WAD for the compiler.");
    if(!list.empty())list+=';';list+=relative;
  }
  return list;
}
static bool q1DiscoverWads(const std::string& source,const std::filesystem::path& map) {
  auto resolved=ftebroom::quake1::mapWads(source,map,std::filesystem::u8path(q1Authoring()),ftebroom::pak::exists);
  bool changed=resolved.missing!=q1MissingMapWads;q1MissingMapWads=std::move(resolved.missing);
  std::string list=q1Wads;std::set<std::filesystem::path> known;
  std::istringstream input(list);std::string line;
  while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(!line.empty())known.insert(std::filesystem::u8path(line).lexically_normal());}
  for(const auto& path:resolved.found)if(known.insert(path.lexically_normal()).second){if(!list.empty()&&list.back()!='\n')list+='\n';list+=path.generic_u8string()+"\n";changed=true;}
  if(!changed)return false;
  if(list.size()>=sizeof q1Wads)throw std::runtime_error("Map WAD list is too long.");
  std::snprintf(q1Wads,sizeof q1Wads,"%s",list.c_str());return true;
}
static void q1IndexAssets() {
  q1FindWads();
  for(auto handle:q1ShaderHandles)draw->UnloadImage(handle);q1ShaderHandles.clear();q1MaterialWad.clear();
  auto palette=q1Palette();
  std::map<std::string,ftebroom::quake1::Texture> textures;
  for(const auto& path:q1WadPaths())for(auto& tex:ftebroom::quake1::wad(q1Read(path))){q1MaterialWad[tex.name]=q1WadLabels.count(path.u8string())?q1WadLabels[path.u8string()]:path.filename().u8string();textures[tex.name]=std::move(tex);}
  char writable[4096]={};if(!files->NativePath("ftebroom.cfg",(fs_relative)5,writable,sizeof writable))throw std::runtime_error("No writable editor cache.");
  auto editor=std::filesystem::u8path(writable).parent_path();
  auto cache=editor/"textures/_ftebroom_q1";std::filesystem::create_directories(cache);
  std::filesystem::create_directories(editor/"scripts");
  std::ofstream shaderFile(editor/"scripts/ftebroom-quake1.shader");
  static unsigned generation=0;generation++;
  for(const auto& pair:textures) {
    const auto& t=pair.second;std::ostringstream encoded;encoded<<generation<<'-';for(unsigned char c:t.name)encoded<<std::hex<<std::setw(2)<<std::setfill('0')<<int(c);
    auto image="textures/_ftebroom_q1/"+encoded.str()+".tga";
    auto bytes=ftebroom::quake1::tga(t,palette);std::ofstream out(cache/(encoded.str()+".tga"),std::ios::binary);out.write(bytes.data(),bytes.size());out.close();if(!out)throw std::runtime_error("Cannot write Quake texture cache.");
    int tags=t.name[0]=='*'?32:0;if(t.name.rfind("sky",0)==0)tags|=512;
    auto material="textures/"+t.name;
    auto script="{\n{\nmap "+image+"\nrgbgen vertex\nalphagen vertex\n"+(t.name[0]=='{'?std::string("alphafunc GE128\ndepthwrite\n"):std::string{})+"}\n}";
    shaderFile<<material<<"\n"<<script<<"\n";
    materials[material]={material,image,false,tags};
  }
  shaderFile.close();command("fs_restart\nftebroom_reload_palette\nftebroom_reload_asset_models\n");
  cvars->SetString("ca_newbrushtexture","fb_grey");
}
