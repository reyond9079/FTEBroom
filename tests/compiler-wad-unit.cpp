#include "../native/ftebroom_compiler_wad.h"
#include <cassert>
#include <iostream>
using namespace ftebroom::quake1;
std::string archive(const std::string& name,unsigned char pixel){
 std::string bytes("WAD2",4);bytes.resize(12,0);bytes[4]=1;bytes[8]=137;
 std::string lump(125,0);lump.replace(0,name.size(),name);lump[16]=8;lump[20]=8;lump[24]=40;lump[28]=104;lump[32]=120;lump[36]=124;for(int i=40;i<125;i++)lump[i]=pixel;bytes+=lump;
 std::string entry(32,0);entry[0]=12;entry[4]=entry[8]=125;entry[12]=68;entry.replace(16,name.size(),name);return bytes+entry;
}
int main(){
 auto a=archive("clip",1),b=archive("clip",2),c=archive("other",3);
 auto out=compilerWad({a,c,b},{"clip"});auto textures=wad(out);assert(textures.size()==1&&textures[0].name=="clip"&&textures[0].pixels[0]==2);
 assert(out.substr(12,125)==b.substr(12,125));assert(wad(compilerWad({a,b},{})).empty());
 bool rejected=false;try{compilerWad({"WAD2"},{"clip"});}catch(...){rejected=true;}assert(rejected);
 std::cout<<"PASS compiler WAD: selected names, last archive priority, unchanged mip bytes and invalid data\n";
}
