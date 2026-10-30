// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <filesystem>
#include <fstream>
#include <set>
#include <vector>
#include <regex>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <cctype>
namespace ftebroom::diskcache {
namespace fs=std::filesystem;
struct Result {uintmax_t bytes=0,files=0,errors=0;};
struct Options {bool aggressive=false;std::set<fs::path> protectedFiles;uintmax_t highWater=512ull*1024*1024,lowWater=256ull*1024*1024;};
// Only these reserved generated names are eligible. Ordinary user files are not.
inline bool openCopy(const fs::path& p){static const std::regex r("_ftebroom_open_[0-9]+_[0-9]+\\.map");return std::regex_match(p.filename().u8string(),r);}
inline bool textureCopy(const fs::path& p){static const std::regex r("(v?[0-9]+)-[0-9a-f]+(-[0-9]+)?\\.tga");return std::regex_match(p.filename().u8string(),r);}
inline bool safe(const fs::path& root,const fs::path& path){
 auto relative=path.lexically_relative(root);if(relative.empty()||relative.is_absolute())return false;
 fs::path current=root;std::error_code ec;
 for(const auto& part:relative){if(part=="..")return false;current/=part;auto s=fs::symlink_status(current,ec);if(ec||fs::is_symlink(s))return false;}
 return true;
}
inline bool sourceFile(const fs::path& p){auto e=p.extension().u8string();std::transform(e.begin(),e.end(),e.begin(),[](unsigned char c){return char(std::tolower(c));});return e==".map"||e==".wad"||e==".bsp";}
inline Result clean(const fs::path& input,const Options& options={}){
 Result result;std::error_code ec;auto root=fs::absolute(input,ec).lexically_normal();if(ec||!fs::is_directory(root))return result;
 auto logPath=root/"cache-cleanup.log";if(fs::is_symlink(fs::symlink_status(logPath,ec)))return result;ec.clear();auto logSize=fs::file_size(logPath,ec);ec.clear();
 std::ofstream log(logPath,logSize>2*1024*1024&&logSize!=uintmax_t(-1)?std::ios::trunc:std::ios::app);
 const auto now=fs::file_time_type::clock::now();
 auto protectedFile=[&](const fs::path& p){for(const auto& keep:options.protectedFiles){std::error_code e;if(fs::equivalent(p,keep,e)&&!e)return true;}return false;};
 auto erase=[&](const fs::path& p){if(!safe(root,p)||protectedFile(p))return;std::error_code e;if(!fs::is_regular_file(p,e)||e)return;auto size=fs::file_size(p,e);if(e){result.errors++;return;}if(fs::remove(p,e)){result.files++;result.bytes+=size;if(log&&log.tellp()<1024*1024)log<<"Removed "<<p.lexically_relative(root).u8string()<<" ("<<size<<" bytes)\n";}else if(e)result.errors++;};
 auto list=[&](const fs::path& dir){std::vector<fs::path> out;if(!safe(root,dir))return out;std::error_code e;fs::directory_iterator it(dir,fs::directory_options::skip_permission_denied,e),end;for(;!e&&it!=end;it.increment(e)){auto s=it->symlink_status(e);if(!e&&!fs::is_symlink(s))out.push_back(it->path());}return out;};
 // Shader manifests pin all currently indexed images, irrespective of age/size.
 std::set<std::string> pinned;bool manifestsValid=true,manifestSeen=false;static const std::regex image("textures/_ftebroom_q[12]/([A-Za-z0-9_.-]+)");
 for(const auto& p:list(root/"scripts"))if(p.extension()==".shader"&&fs::is_regular_file(p,ec)){
  manifestSeen=true;std::ifstream in(p,std::ios::binary);if(!in){manifestsValid=false;break;}std::string text((std::istreambuf_iterator<char>(in)),{});if(in.bad()){manifestsValid=false;break;}
  for(std::sregex_iterator it(text.begin(),text.end(),image),end;it!=end;++it)pinned.insert(it->str());
 }
 if(manifestsValid&&manifestSeen)for(const char* kind:{"_ftebroom_q1","_ftebroom_q2"}){
  struct Entry{fs::path path;fs::file_time_type time;uintmax_t size;};std::vector<Entry> candidates;uintmax_t total=0;
  for(const auto& p:list(root/"textures"/kind))if(textureCopy(p)&&fs::is_regular_file(p,ec)){
   auto size=fs::file_size(p,ec);if(ec)continue;total+=size;auto time=fs::last_write_time(p,ec);if(ec)continue;
   if(!pinned.count(p.lexically_relative(root).generic_u8string()))candidates.push_back({p,time,size});
  }
  std::sort(candidates.begin(),candidates.end(),[](const Entry& a,const Entry& b){return a.time<b.time;});
  bool pressure=total>options.highWater;
  for(const auto& entry:candidates)if(options.aggressive||now-entry.time>std::chrono::hours(24)||(pressure&&total>options.lowWater)){
   auto before=result.bytes;erase(entry.path);total-=result.bytes-before;
  }
 }
 // These immutable engine input copies are not saves or recovery snapshots.
 for(const auto& p:list(root/"maps"))if(openCopy(p))erase(p);
 // Preview sessions may contain user-visible MAP/BSP/WAD copies: retain them.
 // Retain the two newest sessions; prune only old derived resources in others.
 auto sessions=list(root/"data/mod-preview");std::vector<fs::path> generated;static const std::regex session("[0-9]+");
 for(const auto& p:sessions)if(std::regex_match(p.filename().u8string(),session)&&fs::is_directory(p,ec))generated.push_back(p);
 std::sort(generated.begin(),generated.end(),[](const fs::path& a,const fs::path& b){std::error_code e;auto ta=fs::last_write_time(a,e),tb=fs::last_write_time(b,e);return ta==tb?a.filename()>b.filename():ta>tb;});
 for(size_t i=2;i<generated.size();i++){
  auto age=now-fs::last_write_time(generated[i],ec);if(ec||(!options.aggressive&&age<std::chrono::hours(24*7)))continue;
  fs::recursive_directory_iterator it(generated[i],fs::directory_options::skip_permission_denied,ec),end;
  for(;!ec&&it!=end;it.increment(ec)){auto p=it->path();if(!safe(root,p)){it.disable_recursion_pending();continue;}if(!sourceFile(p)&&it->is_regular_file(ec))erase(p);}
 }
 if(log)log<<"Summary "<<std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())<<": removed "<<result.files<<" files, "<<result.bytes<<" bytes; errors "<<result.errors<<"\n";
 return result;
}
}
