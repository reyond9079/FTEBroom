// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>
namespace ftebroom::shaderpreview {
inline std::string token(const std::string& source, size_t& i)
{
  for (;;)
  {
    while (i < source.size() && std::isspace((unsigned char)source[i]))
      ++i;
    if (source.compare(i, 2, "//") == 0)
    {
      while (i < source.size() && source[i] != '\n')
        ++i;
    }
    else if (source.compare(i, 2, "/*") == 0)
    {
      auto end = source.find("*/", i + 2);
      i = end == std::string::npos ? source.size() : end + 2;
    }
    else
      break;
  }
  if (i == source.size())
    return {};
  if (source[i] == '{' || source[i] == '}')
    return source.substr(i++, 1);
  if (source[i] == '"')
  {
    size_t start = ++i;
    while (i < source.size() && source[i] != '"')
      ++i;
    auto result = source.substr(start, i - start);
    if (i < source.size())
      ++i;
    return result;
  }
  size_t start = i;
  while (i < source.size() && !std::isspace((unsigned char)source[i]) && source[i] != '{'
         && source[i] != '}')
    ++i;
  return source.substr(start, i - start);
}
struct Material {bool valid;std::string preview;int tags;float opacity;bool masked;std::string blend,alphaTest;};
// A static, unlit editor preview preserves diffuse-stage transparency.
inline Material parse(const std::string& source,size_t& offset){
      int depth=1,tags=0;std::string preview,stagePreview,blend,stageImage,stageBlend,alphaTest,stageAlphaTest;
      float materialOpacity=1,stageOpacity=1;bool editorOpacity=false,masked=false,stageMasked=false,sawLightmap=false;
      auto lower=[](std::string value){for(char& c:value)c=char(std::tolower((unsigned char)c));return value;};
      while(depth&&offset<source.size()){
        auto word=lower(token(source,offset));
        if(word=="{"){if(depth==1){stageImage.clear();stageBlend.clear();stageOpacity=1;stageMasked=false;stageAlphaTest.clear();}++depth;}
        else if(word=="}"){
          if(depth==2&&stagePreview.empty()&&!stageImage.empty()){stagePreview=stageImage;blend=stageBlend;masked=stageMasked;alphaTest=stageAlphaTest;if(!editorOpacity&&!stageBlend.empty())materialOpacity=stageOpacity;}
          --depth;
        }
        else if(depth==1&&word=="qer_editorimage")preview=token(source,offset);
        else if(depth==1&&word=="qer_trans"){
          auto value=token(source,offset);char* end=nullptr;float alpha=std::strtof(value.c_str(),&end);
          if(end!=value.c_str()&&std::isfinite(alpha)){editorOpacity=true;materialOpacity=std::clamp(alpha,0.f,1.f);}
        }
        else if(depth==2&&word=="alphafunc"){auto mode=lower(token(source,offset));if(mode=="ge128"||mode=="gt0"||mode=="lt128"){stageMasked=true;stageAlphaTest=mode;}}
        else if(depth==2&&word=="alphagen"){
          if(lower(token(source,offset))=="const"){auto value=token(source,offset);char* end=nullptr;float alpha=std::strtof(value.c_str(),&end);if(end!=value.c_str()&&std::isfinite(alpha))stageOpacity=std::clamp(alpha,0.f,1.f);}
        }
        else if(depth==2&&word=="blendfunc"){
          auto mode=lower(token(source,offset));
          if(mode=="add"||mode=="filter"||mode=="blend")stageBlend=mode;
          else if(mode.rfind("gl_",0)==0){auto dst=lower(token(source,offset));if(!(mode=="gl_one"&&dst=="gl_zero"))stageBlend=mode+" "+dst;}
        }
        else if(depth==2&&(word=="map"||word=="clampmap")){
          auto image=token(source,offset);if(lower(image)=="$lightmap")sawLightmap=true;
          if(stageImage.empty()&&!image.empty()&&image[0]!='$')stageImage=image;
        }
        else if(depth==1&&word=="surfaceparm"){
          auto parm=lower(token(source,offset));if(parm=="water"||parm=="lava"||parm=="slime")tags|=32;
          if(parm=="trans"||parm=="fog")tags|=64;if(parm=="areaportal")tags|=128;if(parm=="clusterportal")tags|=256;if(parm=="sky")tags|=512;
        }
      }
      // A diffuse/filter stage over a lightmap is opaque in the editor's unlit preview.
      if(sawLightmap&&(blend=="filter"||blend=="gl_dst_color gl_zero"||blend=="gl_zero gl_src_color"))blend.clear();
      if(editorOpacity&&materialOpacity<1)blend="blend";
      if(materialOpacity<1||!blend.empty())tags|=64;
      return {depth==0,preview.empty()?stagePreview:preview,tags,materialOpacity,masked,blend,alphaTest};
}
}
