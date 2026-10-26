// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/ftebroom_map_wads.h"
#include <cassert>
#include <set>
#include <iostream>
int main(){
  using P=std::filesystem::path;
  std::set<P> files={P("/external/My Game/wads/first.wad"),P("/external/mod/second.wad"),P("/external/mod/textures/third.wad")};
  auto exists=[&](const P& path){return files.count(path)!=0;};
  auto result=ftebroom::quake1::mapWads("// header\n{\n\"wad\" \"../../wads/first.wad;old/second.wad;old/third.wad;absent.wad;absent.wad\"\n",P("/external/My Game/maps/level/a.map"),P("/external/mod"),exists);
  assert(result.found.size()==3&&result.missing.size()==1);
  assert(result.found[0]==P("/external/My Game/wads/first.wad"));
  assert(result.found[1]==P("/external/mod/second.wad"));
  assert(result.found[2]==P("/external/mod/textures/third.wad"));
  assert(result.missing[0]==P("/external/My Game/maps/level/absent.wad"));
  result=ftebroom::quake1::mapWads("\"wad\" \"..\\..\\wads\\first.wad\"",P("/external/My Game/maps/level/a.map"),P("/external/mod"),exists);
  assert(result.found.size()==1&&result.missing.empty());
  result=ftebroom::quake1::mapWads("// no archives",P("map.map"),P("/external/mod"),exists);
  assert(result.found.empty()&&result.missing.empty());
  std::cout<<"PASS: map-relative WADs, spaces, backslashes, mod fallbacks, deduplication and missing paths\n";
}
