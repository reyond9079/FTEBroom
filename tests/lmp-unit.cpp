#include "../native/ftebroom_lmp.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <iterator>
#include <cstring>
int main(int argc,char** argv){
 using ftebroom::lmp::decode;std::string palette;for(int i=0;i<256;i++){palette+=char(i);palette+=char(255-i);palette+=char(0);}
 std::string qpic(8,0);qpic[0]=2;qpic[4]=2;qpic+=char(0);qpic+=char(1);qpic+=char(254);qpic+=char(255);
 auto image=decode(qpic,palette,"test.lmp");assert(image.width==2&&image.height==2&&image.rgba.size()==16&&image.rgba[15]==0&&image.rgba[3]==char(255));
 auto font=decode(std::string(16384,0),palette,"conchars.lmp");assert(font.width==128&&font.rgba[3]==0);
 assert(decode(std::string(16384,1),palette,"colormap.lmp").width==256);
 assert(decode(palette,{},"palette.lmp").rgba.size()==1024);
 bool rejected=false;try{decode(qpic.substr(0,11),palette,"bad.lmp");}catch(...){rejected=true;}assert(rejected);
 rejected=false;try{decode(qpic,{},"test.lmp");}catch(...){rejected=true;}assert(rejected);
 if(argc>1){std::ifstream in(argv[1],std::ios::binary);std::string pak((std::istreambuf_iterator<char>(in)),{});assert(pak.substr(0,4)=="PACK");auto word=[&](int p){unsigned n;std::memcpy(&n,pak.data()+p,4);return n;};auto off=word(4),size=word(8);std::string realPalette;for(unsigned i=off;i<off+size;i+=64)if(std::string(pak.data()+i)=="gfx/palette.lmp")realPalette=pak.substr(word(i+56),word(i+60));int pictures=0;
  for(unsigned i=off;i<off+size;i+=64){std::string name(pak.data()+i);if(name.size()>4&&name.substr(name.size()-4)==".lmp"){auto view=decode(pak.substr(word(i+56),word(i+60)),realPalette,name.substr(name.find_last_of('/')+1));assert(view.width&&view.height);pictures++;}}
  std::cout<<"PASS LibreQuake PAK LMP pictures "<<pictures<<"\n";
 }
 std::cout<<"PASS LMP pixels, palette, font, colormap and invalid data\n";
}
