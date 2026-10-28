// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <stdexcept>
#include <cstdint>
namespace ftebroom::lmp {
struct Image {unsigned width=0,height=0;std::string rgba;};
inline Image decode(const std::string& bytes,const std::string& palette,const std::string& name){
    auto word=[&](size_t p){if(p+4>bytes.size())return uint32_t(0);return uint32_t((unsigned char)bytes[p])|(uint32_t((unsigned char)bytes[p+1])<<8)|(uint32_t((unsigned char)bytes[p+2])<<16)|(uint32_t((unsigned char)bytes[p+3])<<24);};
    Image out;size_t start=8;bool font=false,opaque=false;
    if(bytes.size()>=8){out.width=word(0);out.height=word(4);}
    bool qpic=out.width&&out.height&&out.width<=16384&&out.height<=16384&&uint64_t(out.width)*out.height<=16777216&&8+uint64_t(out.width)*out.height==bytes.size();
    if(!qpic){start=0;
        if(name=="conchars.lmp"||name=="conchars"){out.width=out.height=128;font=true;}
        else if(name=="colormap.lmp"||name=="colormap"){out.width=256;out.height=64;opaque=true;}
        else if(bytes.size()==768){out.width=out.height=16;out.rgba.reserve(1024);for(unsigned i=0;i<256;i++){out.rgba.append(bytes,i*3,3);out.rgba+=char(255);}return out;}
        else throw std::runtime_error("Unsupported LMP image layout: expected a Quake picture header, conchars, colormap or a 768-byte palette.");
    }
    if(uint64_t(out.width)*out.height+start!=bytes.size())throw std::runtime_error("Truncated LMP image.");
    if(palette.size()!=768)throw std::runtime_error("Load Quake resources with gfx/palette.lmp to view this indexed image.");
    out.rgba.reserve(out.width*out.height*4);
    for(size_t i=start;i<bytes.size();i++){unsigned index=(unsigned char)bytes[i];out.rgba.append(palette,index*3,3);out.rgba+=char(!opaque&&index==(font?0:255)?0:255);}
    return out;
}
}
