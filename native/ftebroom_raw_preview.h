// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include "ftebroom_prefab_data.h"
namespace ftebroom {
// FTE loads brush geometry directly but does not spawn editor-only groups.
// Flatten only those containers in the private preview snapshot.
inline std::string rawPreviewMap(const std::string& bytes){
  prefab::Lexer lexer(bytes);std::vector<prefab::Entity> entities;
  for(auto token=lexer.next();!token.text.empty();token=lexer.next()){
    if(!prefab::brace(token,"{"))throw std::runtime_error("Expected a MAP entity.");
    prefab::Entity entity;
    for(;;){
      token=lexer.next();
      if(prefab::brace(token,"}"))break;
      if(token.text.empty()&&!token.quoted)throw std::runtime_error("Unclosed MAP entity.");
      if(prefab::brace(token,"{")){
        size_t begin=token.begin;int depth=1;
        while(depth){token=lexer.next();if(token.text.empty()&&!token.quoted)throw std::runtime_error("Unclosed MAP geometry.");if(prefab::brace(token,"{"))++depth;if(prefab::brace(token,"}"))--depth;}
        entity.geometry+=bytes.substr(begin,token.end-begin)+"\n";
      }else{
        auto value=lexer.next();if(!value.quoted&&(value.text.empty()||prefab::brace(value,"{")||prefab::brace(value,"}")))throw std::runtime_error("Missing MAP property value.");
        entity.fields[token.text]=value.text;
      }
    }
    entities.push_back(std::move(entity));
  }
  auto world=std::find_if(entities.begin(),entities.end(),[](const auto& e){return e.get("classname")=="worldspawn";});
  if(world==entities.end())throw std::runtime_error("The preview MAP has no worldspawn.");
  for(auto& e:entities)if(e.get("classname")=="func_group")world->geometry+=e.geometry;
  std::string result;
  for(const auto& e:entities){
    if(e.get("classname")=="func_group")continue;
    result+="{\n";
    for(const auto& field:e.fields)if(field.first.rfind("_tb_",0)!=0)result+=prefab::quote(field.first)+" "+prefab::quote(field.second)+"\n";
    result+=e.geometry+"}\n";
  }
  return result;
}
}
