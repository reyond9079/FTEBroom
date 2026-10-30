#include "../native/ftebroom_pak.h"
#include <cassert>
#include <cstring>
#include <iostream>
int main(){
  namespace fs=std::filesystem;auto p=fs::temp_directory_path()/("pak-names-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".pak");
  {std::ofstream out(p,std::ios::binary);out.write("PACK",4);ftebroom::pak::put(out,14);ftebroom::pak::put(out,128);out.write("ab",2);
    for(auto name:{"gfx/safe.lmp","models/tank/../ctank/skin.pcx"}){std::string row(56,0);row.replace(0,std::strlen(name),name);out.write(row.data(),56);ftebroom::pak::put(out,12);ftebroom::pak::put(out,1);}}
  bool strict=false;try{ftebroom::pak::directory(p);}catch(...){strict=true;}assert(strict);
  auto entries=ftebroom::pak::directory(p,true);assert(entries.size()==1&&entries[0].name=="gfx/safe.lmp");
  assert(ftebroom::pak::read(p/"gfx/safe.lmp")=="a");
  bool unsafe=false;try{ftebroom::pak::read(p/"models/tank/../ctank/skin.pcx");}catch(...){unsafe=true;}assert(unsafe);
  fs::remove(p);std::cout<<"PASS: safe PAK resource reads, strict writes, traversal rejection\n";
}
