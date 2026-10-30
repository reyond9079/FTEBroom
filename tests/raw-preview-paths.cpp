#include "../native/ftebroom_raw_preview.h"
#include <cassert>
#include <iostream>
int main(){
  using ftebroom::rawPreviewImagePath;
  for(auto p:{"textures/_ftebroom_q1/a.tga","textures/_ftebroom_q2/a.tga","textures/_ftebroom_missing/a.tga"})assert(rawPreviewImagePath(p));
  for(auto p:{"textures/_ftebroom_q1/../evil.tga","textures/_ftebroom_q1/sub/../../evil.tga","textures/_ftebroom_q1/evil.cfg","textures/_ftebroom_q1_evil/a.tga","C:/outside.tga","textures/_ftebroom_missing/..\\outside.tga"})assert(!rawPreviewImagePath(p));
  std::cout<<"PASS: generated MAP-preview image paths and traversal rejection\n";
}
