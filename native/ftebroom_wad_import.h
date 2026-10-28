// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_quake1.h"
#include "ftebroom_palette_quantize.h"
namespace ftebroom {
inline std::string addWadTexture(std::string original,quake1::Texture texture,const std::string& palette,bool fullPalette=false,bool halfLife=false){
 using namespace quake1;if(palette.size()!=768||texture.name.empty()||texture.name.size()>15||!texture.width||!texture.height||texture.width>4096||texture.height>4096||texture.width%16||texture.height%16||texture.pixels.size()!=size_t(texture.width)*texture.height)throw std::runtime_error("Invalid WAD texture.");
 if(original.empty()){original=halfLife?"WAD3":"WAD2";original.resize(12,0);original[8]=12;}
 range(original,0,12);bool hl=original.substr(0,4)=="WAD3";if(!hl&&original.substr(0,4)!="WAD2")throw std::runtime_error("Choose a WAD2 or WAD3 file.");
 auto count=word(original,4),table=word(original,8);if(count>65535)throw std::runtime_error("WAD has too many entries.");range(original,table,size_t(count)*32);
 for(unsigned i=0;i<count;i++){auto p=table+i*32;range(original,word(original,p),word(original,p+4));std::string name=original.substr(p+16,16);name.resize(name.find('\0')==name.npos?16:name.find('\0'));auto wanted=texture.name;for(char& c:name)if(c>='A'&&c<='Z')c+=32;for(char& c:wanted)if(c>='A'&&c<='Z')c+=32;if(name==wanted)throw std::runtime_error("A texture with this name already exists. Choose a different name.");}
 auto put=[](std::string& s,size_t at,uint32_t v){for(int j=0;j<4;j++)s[at+j]=char(v>>(j*8));};
 std::string lump(40,0);std::copy(texture.name.begin(),texture.name.end(),lump.begin());put(lump,16,texture.width);put(lump,20,texture.height);
 bool masked=texture.name[0]=='{';PaletteQuantizer quantizer(palette,(fullPalette||hl)?(masked?255:256):224);
 auto level=texture.pixels;unsigned w=texture.width,h=texture.height;
 for(int mip=0;mip<4;mip++){
  put(lump,24+mip*4,lump.size());lump.append((const char*)level.data(),level.size());
  if(mip==3)break;std::vector<unsigned char> next((w/2)*(h/2));
  for(unsigned y=0;y<h/2;y++)for(unsigned x=0;x<w/2;x++){
    int rgb[3]={},visible=0;
    for(int yy=0;yy<2;yy++)for(int xx=0;xx<2;xx++){
      auto index=level[(y*2+yy)*w+x*2+xx];if(masked&&index==255)continue;visible++;for(int c=0;c<3;c++)rgb[c]+=(unsigned char)palette[index*3+c];
    }
    next[y*(w/2)+x]=masked&&visible<=2?255:quantizer.nearest(rgb[0]/visible,rgb[1]/visible,rgb[2]/visible);
  }
  level=std::move(next);w/=2;h/=2;
 }
 if(hl){lump+=char(0);lump+=char(1);lump+=palette;}
 if(original.size()+lump.size()+size_t(count+1)*32>0x7fffffff)throw std::runtime_error("WAD exceeds 2 GiB.");
 auto offset=original.size();std::string rows=original.substr(table,count*32);original+=lump;auto directory=original.size();std::string row(32,0);put(row,0,offset);put(row,4,lump.size());put(row,8,lump.size());row[12]=hl?67:68;std::copy(texture.name.begin(),texture.name.end(),row.begin()+16);original+=rows+row;put(original,4,count+1);put(original,8,directory);return original;
}
}
