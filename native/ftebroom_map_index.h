// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <string_view>
#include <map>
#include <set>
#include <array>
#include <charconv>
#include <cctype>
#include <cstdint>
namespace ftebroom {
// Match the MAP token reader, borrowing spans instead of allocating face tokens.
inline std::string_view mapIndexToken(std::string_view text,size_t& at) {
  for(;;) {
    while(at<text.size() && std::isspace((unsigned char)text[at]))++at;
    if(text.substr(at,2)=="//"){at=text.size();return {};}
    if(text.substr(at,2)=="/*"){auto end=text.find("*/",at+2);at=end==text.npos?text.size():end+2;continue;}
    break;
  }
  if(at==text.size())return {};
  if(text[at]=='{' || text[at]=='}')return text.substr(at++,1);
  if(text[at]=='"'){size_t start=++at;while(at<text.size() && text[at]!='"')++at;auto result=text.substr(start,at-start);if(at<text.size())++at;return result;}
  size_t start=at;while(at<text.size() && !std::isspace((unsigned char)text[at]) && text[at]!='{' && text[at]!='}')++at;
  return text.substr(start,at-start);
}
inline std::string mapLineIndex(std::string_view bytes) {
  std::string out;out.reserve(bytes.size()/24);
  int line=1,depth=0,entity=-1,brush=0,brushLine=0;
  bool quoted=false,escaped=false,comment=false,block=false;
  size_t lineStart=0;std::map<int,int> firstContents;std::set<int> mixed;
  for(size_t at=0;at<bytes.size();++at) {
    char c=bytes[at],next=at+1<bytes.size()?bytes[at+1]:0;
    if(c=='\n') {
      if(depth>=2 && brushLine) {
        auto row=bytes.substr(lineStart,at-lineStart);size_t cursor=0,count=0;std::array<std::string_view,3> tail{};
        bool valve=false,primitive=false,plane=false;
        for(;;){auto word=mapIndexToken(row,cursor);if(word.empty())break;
          if(!count)plane=word=="(";if(count==5)primitive=word==")";
          if(word=="[")valve=true;tail[count%3]=word;++count;}
        if(plane && count>=(primitive?20u:valve?34u:24u)) {
          auto word=tail[(count-3)%3];long long value=0;auto begin=word.data(),end=begin+word.size();
          // stoll accepts a leading plus; from_chars deliberately does not.
          if(begin!=end && *begin=='+')++begin;
          auto parsed=std::from_chars(begin,end,value);
          if(parsed.ec==std::errc{} && parsed.ptr==end && value>=INT32_MIN && value<=INT32_MAX) {
            auto entry=firstContents.emplace(brushLine,(int)value);
            if(!entry.second && entry.first->second!=value)mixed.insert(brushLine);
          }
        }
      }
      lineStart=at+1;++line;comment=false;
    }
    if(comment)continue;
    if(block){if(c=='*' && next=='/'){++at;block=false;}continue;}
    if(quoted){if(escaped)escaped=false;else if(c=='\\')escaped=true;else if(c=='"')quoted=false;continue;}
    if(c=='"'){quoted=true;continue;}
    if(c=='/' && next=='/'){++at;comment=true;continue;}
    if(c=='/' && next=='*'){++at;block=true;continue;}
    if(c=='{') {
      if(!depth){++entity;brush=0;out+=std::to_string(line)+" "+std::to_string(entity)+" -1\n";}
      if(depth==1){brushLine=line;out+=std::to_string(line)+" "+std::to_string(entity)+" "+std::to_string(brush++)+"\n";}
      ++depth;
    }else if(c=='}')--depth;
  }
  for(int item:mixed)out+="mixed "+std::to_string(item)+"\n";
  return out;
}
}
