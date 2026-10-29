// SPDX-License-Identifier: GPL-2.0-or-later
// Data-only Quake resource readers. All offsets are checked before access.
#pragma once
#include <cstdint>
#include <cctype>
#include <string>
#include <vector>
#include <array>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
namespace ftebroom::quake1 {
inline uint32_t word(const std::string& bytes, size_t at) {
  if(at>bytes.size() || bytes.size()-at<4)throw std::runtime_error("Truncated Quake resource.");
  uint32_t value=0;for(int i=0;i<4;i++)value|=uint32_t((unsigned char)bytes[at+i])<<(8*i);return value;
}
inline void range(const std::string& bytes,uint64_t at,uint64_t size) {
  if(at>bytes.size() || size>bytes.size()-at)throw std::runtime_error("Quake resource contains an invalid offset.");
}
inline std::string name(const std::string& bytes,size_t at,size_t count) {
  range(bytes,at,count);auto text=bytes.substr(at,count);auto zero=text.find('\0');if(zero!=std::string::npos)text.resize(zero);return text;
}
struct Texture {std::string name;uint32_t width,height;std::vector<unsigned char> pixels;std::string palette;};
inline std::vector<Texture> wad(const std::string& bytes) {
  bool hl=bytes.compare(0,4,"WAD3")==0;if(!hl&&bytes.compare(0,4,"WAD2"))throw std::runtime_error("Expected WAD2 or WAD3 archive.");
  auto count=word(bytes,4),offset=word(bytes,8);
  if(count>65536)throw std::runtime_error("WAD has too many entries.");
  range(bytes,offset,uint64_t(count)*32);std::vector<Texture> result;
  uint64_t total=0;
  for(uint32_t i=0;i<count;i++) {
    size_t entry=offset+size_t(i)*32;auto start=word(bytes,entry),size=word(bytes,entry+4);range(bytes,start,size);
    if((unsigned char)bytes[entry+12]!=(hl?67:68))continue;
    // Quake's console font uses the same lump type but is raw 128x128 pixels.
    auto lumpName=name(bytes,entry+16,16);std::transform(lumpName.begin(),lumpName.end(),lumpName.begin(),[](unsigned char c){return char(std::tolower(c));});
    if(lumpName=="conchars")continue;
    if(bytes[entry+13])throw std::runtime_error("Compressed WAD textures are unsupported.");
    if(size<40)throw std::runtime_error("Truncated WAD mip texture.");
    Texture t{name(bytes,start,16),word(bytes,start+16),word(bytes,start+20),{}};
    if(t.name.empty() || t.name.size()>15 || t.name.find_first_of("/\\\";\r\n\t ")!=std::string::npos)
      throw std::runtime_error("Invalid Quake texture name.");
    std::transform(t.name.begin(),t.name.end(),t.name.begin(),[](unsigned char c){return char(std::tolower(c));});
    if(!t.width||!t.height||t.width>4096||t.height>4096)throw std::runtime_error("Invalid WAD texture dimensions.");
    for(int mip=0;mip<4;mip++) {auto pos=word(bytes,start+24+4*mip);uint64_t length=uint64_t(t.width>>mip)*(t.height>>mip);
      if(pos<40 || pos>size || length>size-pos)throw std::runtime_error("Invalid WAD mip offset.");}
    size_t first=start+word(bytes,start+24),length=size_t(t.width)*t.height;
    total+=length;if(total>256*1024*1024)throw std::runtime_error("WAD decoded textures exceed 256 MiB.");
    if(hl){size_t end=40;for(int mip=0;mip<4;mip++)end=std::max(end,size_t(word(bytes,start+24+4*mip))+size_t(t.width>>mip)*(t.height>>mip));
      if(end+770>size || (unsigned char)bytes[start+end]!=0 || (unsigned char)bytes[start+end+1]!=1)throw std::runtime_error("Invalid WAD3 palette.");t.palette=bytes.substr(start+end+2,768);}
    t.pixels.assign(bytes.begin()+first,bytes.begin()+first+length);result.push_back(std::move(t));
  }return result;
}
inline std::string tga(const Texture& texture,const std::string& fallback) {
  const auto& palette=texture.palette.empty()?fallback:texture.palette;
  if(palette.size()!=768)throw std::runtime_error("Quake palette.lmp must contain 256 RGB colours.");
  std::string out(18,0);out[2]=2;out[12]=texture.width&255;out[13]=texture.width>>8;
  bool masked=!texture.name.empty()&&texture.name[0]=='{';
  out[14]=texture.height&255;out[15]=texture.height>>8;out[16]=masked?32:24;out[17]=masked?40:32;
  for(auto pixel:texture.pixels){out+=palette[3*pixel+2];out+=palette[3*pixel+1];out+=palette[3*pixel];if(masked)out+=char(pixel==255?0:255);}return out;
}
inline bool gameBsp(const std::string& bytes,int profile) {
  if(profile==3||profile==4){
    bool q2=profile==3;size_t header=q2?8:4;int lumps=q2?19:15;
    if(bytes.size()<header+lumps*8)return false;
    if(q2?(bytes.compare(0,4,"IBSP")||word(bytes,4)!=38):word(bytes,0)!=30)return false;
    for(int i=0;i<lumps;i++){auto at=word(bytes,header+i*8),length=word(bytes,header+i*8+4);if(at>bytes.size()||length>bytes.size()-at||(length&&at<header+lumps*8))return false;}
    auto models=word(bytes,header+(q2?13:14)*8+4);return models>0&&models%(q2?48:64)==0;
  }
  return false;
}
inline bool bsp(const std::string& bytes,bool q1) {
  if(bytes.size()<(q1?124:144))return false;
  auto version=word(bytes,0);int lumps=q1?15:17;size_t header=q1?4:8;
  if(q1 ? (version!=29 && bytes.compare(0,4,"BSP2") && bytes.compare(0,4,"2PSB")) : (bytes.compare(0,4,"IBSP") || word(bytes,4)!=46))return false;
  for(int i=0;i<lumps;i++) {uint64_t at=word(bytes,header+i*8),size=word(bytes,header+i*8+4);if(at>bytes.size() || size>bytes.size()-at || (size && at<header+lumps*8))return false;}
  auto models=word(bytes,header+(q1?14:7)*8+4);return models>0 && models%(q1?64:40)==0;
}
// Convert the engine's explicit UV axes to Quake's axial representation, only if lossless.
inline std::string face(const std::string& line,bool classic,bool engineOutput=false) {
  // The engine already validated its Valve 220 output. Preserve every numeric
  // character, stripping only the three Q3 surface flags without parsing floats.
  if(!classic && engineOutput) {
    size_t at=0,end=0;int count=0;bool valve=false;
    while(at<line.size() && count<31) {
      while(at<line.size() && std::isspace((unsigned char)line[at]))++at;
      if(at==line.size())break;
      if(line[at]=='[')valve=true;
      if(line[at]=='"'){++at;while(at<line.size() && line[at]!='"')++at;if(at<line.size())++at;}
      else while(at<line.size() && !std::isspace((unsigned char)line[at]))++at;
      end=at;++count;
    }
    if(count==31 && valve)return line.substr(0,end);
  }
  std::istringstream input(line);char c;double p[3][3],uv[2][4],rotation,sx,sy;std::string tex;
  for(auto& point:p){if(!(input>>c)||c!='(')throw std::runtime_error("Invalid map plane.");for(auto& v:point)input>>v;input>>c;if(c!=')')throw std::runtime_error("Invalid map plane.");}
  input>>std::quoted(tex);auto prefix=line.substr(0,line.find('['));
  if(line.find('[')==std::string::npos)return line;
  for(auto& axis:uv){input>>c;if(c!='[')throw std::runtime_error("Invalid texture axis.");for(auto& v:axis)input>>v;input>>c;if(c!=']')throw std::runtime_error("Invalid texture axis.");}
  if(!(input>>rotation>>sx>>sy)||sx==0||sy==0)throw std::runtime_error("Invalid texture scale.");
  std::ostringstream out;out<<std::setprecision(17);
  if(!classic){auto end=input.tellg();return line.substr(0,end<0?line.size():size_t(end));}
  double a[3],b[3],n[3];for(int j=0;j<3;j++){a[j]=p[2][j]-p[0][j];b[j]=p[1][j]-p[0][j];}
  for(int j=0;j<3;j++)n[j]=a[(j+1)%3]*b[(j+2)%3]-a[(j+2)%3]*b[(j+1)%3];
  int dominant=2;if(std::abs(n[0])>std::abs(n[dominant]))dominant=0;if(std::abs(n[1])>std::abs(n[dominant]))dominant=1;
  int u=dominant==0?1:0,v=dominant==2?1:2;
  if(std::abs(n[dominant])<1e-12)throw std::runtime_error("Degenerate map plane.");
  double values[2][2],offset[2];
  for(int j=0;j<2;j++){double scale=j?sy:sx;values[j][0]=(uv[j][u]-uv[j][dominant]*n[u]/n[dominant])/scale;
    values[j][1]=-(uv[j][v]-uv[j][dominant]*n[v]/n[dominant])/scale;
    double distance=0;for(int k=0;k<3;k++)distance+=n[k]*p[0][k];
    offset[j]=uv[j][3]+uv[j][dominant]*distance/n[dominant]/scale;}
  double norm=std::hypot(values[0][0],values[0][1]);if(norm<1e-12)throw std::runtime_error("Degenerate texture axes.");
  double cosine=values[0][0]/norm,sine=-values[0][1]/norm;
  double invY=values[1][0]*sine+values[1][1]*cosine;
  if(std::abs(invY)<1e-12 || std::abs(values[1][0]*cosine-values[1][1]*sine)>1e-5*std::max(1.,std::abs(invY)))
    throw std::runtime_error("This UV mapping needs Valve 220. Select Valve 220 in Game / resources before saving; no file was replaced.");
  out<<prefix<<offset[0]<<' '<<offset[1]<<' '<<std::atan2(sine,cosine)*180/3.141592653589793<<' '<<1/norm<<' '<<1/invY;return out.str();
}
// GoldSrc compilers require _light, unlike Quake's scalar light key. Preserve
// explicitly authored _light values and normalize only missing values.
inline std::string halfLifeLights(const std::string& source){
  std::istringstream input(source);std::ostringstream out;std::string line,classname,brightness;int depth=0;bool explicitLight=false;
  while(std::getline(input,line)){
    auto first=line.find_first_not_of(" \t\r");auto trimmed=first==line.npos?std::string():line.substr(first);
    if(trimmed=="{"){if(depth==0){classname.clear();brightness.clear();explicitLight=false;}depth++;}
    else if(trimmed=="}"){
      if(depth==1&&!explicitLight&&(classname=="light"||classname=="light_spot"||classname=="light_environment")){
        float intensity=200;std::istringstream value(brightness);float parsed;if(value>>parsed&&std::isfinite(parsed)&&parsed>=0&&parsed<=1e9f)intensity=parsed;
        out<<"\"_light\" \"255 255 255 "<<intensity<<"\"\n";
      }depth--;
    }else if(depth==1&&trimmed.rfind("\"",0)==0){std::istringstream row(trimmed);std::string key,value;if(row>>std::quoted(key)>>std::quoted(value)){if(key=="classname")classname=value;if(key=="light")brightness=value;if(key=="_light"){if(value.empty())continue;explicitLight=true;}}}
    out<<line<<'\n';
  }return out.str();
}
inline std::string map(const std::string& source,bool classic,const std::string& wads,bool engineOutput=false,bool halfLife=false) {
  std::istringstream input(source);std::ostringstream out;std::string line;bool world=false;
  while(std::getline(input,line)) {
    auto start=line.find_first_not_of(" \t\r");auto trimmed=start==std::string::npos?std::string():line.substr(start);
    if(trimmed.rfind("patchDef",0)==0)throw std::runtime_error("Quake 1 does not support patches. Remove or convert patches before saving/building.");
    if(halfLife&&trimmed.rfind("\"mapversion\"",0)==0)continue;
    if(trimmed.rfind("\"wad\"",0)==0 && !wads.empty())continue;
    if(trimmed.rfind("(",0)==0)line=face(trimmed,classic,engineOutput);
    out<<line<<'\n';
    if(!world && trimmed.find("\"worldspawn\"")!=std::string::npos){world=true;if(halfLife)out<<"\"mapversion\" \"220\"\n";if(!wads.empty())out<<"\"wad\" "<<std::quoted(wads)<<'\n';}
  }return halfLife?halfLifeLights(out.str()):out.str();
}
inline std::vector<std::string> mapTextures(const std::string& source) {
  std::istringstream input(source);std::string line;std::vector<std::string> result;
  while(std::getline(input,line)){std::istringstream row(line);row>>std::ws;if(row.peek()!='(')continue;
    for(int i=0;i<3;i++){char c;double x,y,z;row>>c>>x>>y>>z>>c;}
    std::string texture;if(row>>std::quoted(texture)){std::transform(texture.begin(),texture.end(),texture.begin(),[](unsigned char c){return char(std::tolower(c));});result.push_back(texture);}}
  return result;
}
inline bool valveMap(const std::string& source) {
  std::istringstream input(source);std::string line;while(std::getline(input,line)) {
    auto start=line.find_first_not_of(" \t");if(start!=std::string::npos&&line[start]=='('&&line.find('[',start)!=std::string::npos)return true;
  }return false;
}
}
