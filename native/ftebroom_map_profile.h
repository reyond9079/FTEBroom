// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <cctype>
#include <sstream>
#include <algorithm>
namespace ftebroom {
struct MapProfile { int profile=-1, format=-1; bool terrain=false; };
inline MapProfile mapProfile(const std::string& bytes) {
  std::istringstream input(bytes.substr(0,16384));std::string line,game,format;MapProfile result;
  while(std::getline(input,line)) {
    if(line.size()>=3 && (unsigned char)line[0]==239 && (unsigned char)line[1]==187 && (unsigned char)line[2]==191)line.erase(0,3);
    auto start=line.find_first_not_of(" \t\r");if(start==line.npos)continue;line.erase(0,start);
    if(line.rfind("//",0)!=0)break;
    auto end=line.find_last_not_of(" \t\r");line.erase(end+1);
    if(line.rfind("// FTEBroom profile=",0)==0)line.replace(3,8,"ReyondBroom");
    if(line=="// ReyondBroom profile=quake1 format=valve220")return {2,1,false};
    if(line=="// ReyondBroom profile=quake1 format=quake-standard")return {2,0,false};
    if(line=="// ReyondBroom profile=quake2 format=valve220")return {3,1,false};
    if(line=="// ReyondBroom profile=halflife format=valve220")return {4,1,false};
    if(line=="// ReyondBroom profile=quake3 format=quake3")return {0,-1,false};
    if(line=="// ReyondBroom profile=when-the-sky-went-dark format=quake3")return {1,-1,false};
    if(line=="// ReyondBroom profile=when-the-sky-went-dark format=fte-terrain")return {1,-1,true};
    if(line.rfind("// Game:",0)==0)game=line.substr(8);
    if(line.rfind("// Format:",0)==0)format=line.substr(10);
  }
  auto clean=[](std::string& s){auto start=s.find_first_not_of(" \t");s=start==s.npos?"":s.substr(start);std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return char(std::tolower(c));});};clean(game);clean(format);
  if(game=="quake"||game=="quake 1"||game=="librequake") {if(format=="standard"||format=="valve")return {2,format=="valve"?1:0,false};}
  if(game=="quake2"||game=="quake 2"||game=="quake ii"||game=="quake2re"||game=="quake ii remastered"||format=="quake2"||format=="quake2 (valve)")return {3,1,false};
  if(game=="half-life"||game=="halflife"||game=="goldsrc"||game=="counter-strike")return {4,1,false};
  bool q3=format=="quake3"||format=="quake3 (legacy)"||format=="quake3 (valve)";
  if(q3 && (game=="quake3"||game=="quake 3"||game=="quake iii arena"||game.empty()))return {0,-1,false};
  if(q3 && (game=="when the sky went dark"||game=="fte custom game"))return {1,-1,false};
  // Valve is also used by unrelated games; never guess a game from it alone.
  return result;
}
}
