// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <vector>
#include <sstream>
#include <cmath>
#include <filesystem>
#include <stdexcept>
namespace ftebroom::leak {
using Point=std::array<double,3>;
inline std::vector<Point> parse(const std::string& text){
 if(text.size()>32*1024*1024)throw std::runtime_error("Leak file exceeds 32 MiB.");
 std::istringstream in(text);std::vector<Point> points;size_t count=0;
 while(in>>std::ws&&in.peek()!=EOF){
  if(++count>262144)throw std::runtime_error("Too many leak points.");
  bool bracket=in.peek()=='(';if(bracket)in.get();Point p;
  if(!(in>>p[0]>>p[1]>>p[2]))throw std::runtime_error("Invalid leak point.");
  for(auto v:p)if(!std::isfinite(v)||std::abs(v)>1e7)throw std::runtime_error("Invalid leak coordinate.");
  if(bracket){char end;if(!(in>>end)||end!=')')throw std::runtime_error("Invalid leak point delimiter.");}
  if(points.empty()||points.back()!=p)points.push_back(p);
 }
 if(points.empty())throw std::runtime_error("Leak file is empty.");return points;
}
inline std::filesystem::path find(const std::filesystem::path& bsp){
 for(const char* extension:{".pts",".lin"}){auto p=bsp;p.replace_extension(extension);std::error_code ec;if(std::filesystem::is_regular_file(p,ec)&&std::filesystem::file_size(p,ec)>0&&!ec)return p;}return {};
}
}
