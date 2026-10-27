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
  std::vector<P> available={P("/external/My Game/wads/first.wad"),P("/external/mod/second.wad")};
  result=ftebroom::quake1::mapWads("\"wad\" \"old/SECOND.wad;old/first.wad\"",P("/external/a.map"),P("/other/mod"),exists,available);
  assert(result.missing.empty() && result.found==std::vector<P>({available[1],available[0]}));
  std::vector<P> sources={available[0],available[1],P("/library/other.wad"),available[0]};
  ftebroom::quake1::prioritizeMapWads(sources,result.found);
  assert(sources==std::vector<P>({P("/library/other.wad"),available[1],available[0]}));
  files.insert(P("/different/second.wad"));available.push_back(P("/different/second.wad"));
  result=ftebroom::quake1::mapWads("\"wad\" \"old/second.wad\"",P("/external/a.map"),P("/other/mod"),exists,available);
  assert(result.found.empty() && result.missing.size()==1);
  std::cout<<"PASS: map-relative WADs, spaces, backslashes, mod fallbacks, deduplication and missing paths\n";
}
