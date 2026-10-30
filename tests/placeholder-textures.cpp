#include "../native/ftebroom_placeholder_texture.h"
#include "../native/ftebroom_legacy_images.h"
#include <cassert>
#include <iostream>
int main(){
  std::string palette;for(int i=0;i<256;i++){palette+=char(i);palette+=char(i);palette+=char(i);}
  auto t=ftebroom::placeholderTexture("missing104",palette);
  for(bool hl:{false,true}){
    auto bytes=ftebroom::addWadTexture({},t,palette,false,hl);
    auto textures=ftebroom::quake1::wad(bytes);assert(textures.size()==1);
    assert(textures[0].name==t.name&&textures[0].pixels==t.pixels&&textures[0].width==64);
    assert(!hl||textures[0].palette==palette);
  }
  auto wal=ftebroom::placeholderWal(t);auto decoded=ftebroom::legacy::wal(wal,palette);
  assert(decoded.pixels==t.pixels&&decoded.width==64&&decoded.height==64);
  assert(ftebroom::materialKey("Textures/E1U1/GRATE2_5")=="e1u1/grate2_5");
  for(auto s:{"../outside","/absolute","a/../b","C:/bad","a\\b","a//b"})assert(!ftebroom::safeMaterialPath(s));
  assert(ftebroom::safeMaterialPath("e1u1/grate2_5"));
  auto neutral=ftebroom::placeholderTexture("__tb_empty",palette,true);assert(neutral.pixels!=t.pixels);
  std::cout<<"PASS: WAD2/WAD3/WAL mip chains, palette, names and path validation\n";
}
