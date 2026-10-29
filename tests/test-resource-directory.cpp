#include "../native/ftebroom_resource_directory.h"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
int main(){
 namespace fs=std::filesystem;
 auto root=fs::temp_directory_path()/("ReyondBroom-resources69-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 fs::create_directories(root/"Game with spaces"/"id1");fs::create_directories(root/"custom mod");
 auto game=root/"Game with spaces",id1=game/"id1",mod=root/"custom mod";
 auto resolve=ftebroom::quakeResourceDirectory;
 assert(resolve(game.u8string())==id1);assert(resolve(id1.u8string())==id1);
 assert(resolve((id1/"").u8string())==id1);
 assert(resolve(mod.u8string())==mod);
 assert(resolve("  \""+game.u8string()+"\" \r\n")==id1);
 fs::create_directories(root/"ID1"/"id1");assert(resolve((root/"ID1").u8string())==root/"ID1");
 std::ofstream(game/"pak0.pak")<<"root data";assert(resolve(game.u8string())==id1);
 std::ofstream(root/"file")<<"data";
 for(auto path:{std::string(),std::string("\"\""),(root/"missing").u8string(),(root/"file").u8string()}){
  bool failed=false;try{resolve(path);}catch(const std::exception&){failed=true;}assert(failed);
 }
 auto q2=root/"Quake II",hl=root/"Half-Life";
 fs::create_directories(q2/"baseq2");fs::create_directories(hl/"valve");fs::create_directories(hl/"cstrike");
 assert(ftebroom::legacyResourceDirectory(q2.u8string(),3)==q2/"baseq2");
 assert(ftebroom::legacyResourceDirectory((q2/"baseq2").u8string(),3)==q2/"baseq2");
 assert(ftebroom::legacyResourceDirectory(" \""+hl.u8string()+"\" \r\n",4,0)==hl/"valve");
 assert(ftebroom::legacyResourceDirectory(hl.u8string(),4,1)==hl/"cstrike");
 assert(ftebroom::legacyResourceDirectory((hl/"cstrike").u8string(),4,1)==hl/"cstrike");
 for(int profile:{3,4}){
   assert(ftebroom::legacyResourceDirectory((mod/"").u8string(),profile)==mod);
   for(auto path:{std::string(),std::string("\"\""),(root/"missing").u8string(),(root/"file").u8string()}){
     bool failed=false;try{ftebroom::legacyResourceDirectory(path,profile);}catch(const std::exception& e){failed=std::string(e.what()).find(profile==3?"Quake II":"Half-Life")!=std::string::npos;}assert(failed);
   }
 }
 fs::remove_all(root);std::cout<<"PASS: id1, game root, custom mod, quotes, spaces, case, trailing separators and invalid paths\n";
}
