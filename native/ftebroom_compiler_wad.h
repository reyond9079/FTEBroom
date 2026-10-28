// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_quake1.h"
#include <map>
#include <set>
namespace ftebroom::quake1 {
// Copy original mip chains unchanged. The last archive wins, as in the editor.
inline std::string compilerWad(const std::vector<std::string>& archives,const std::set<std::string>& wanted){
    std::map<std::string,std::string> lumps;
    for(const auto& bytes:archives){
        if(bytes.compare(0,4,"WAD2"))throw std::runtime_error("Expected WAD2 compiler resources.");
        auto count=word(bytes,4),offset=word(bytes,8);if(count>65536)throw std::runtime_error("Too many WAD entries.");range(bytes,offset,uint64_t(count)*32);
        for(unsigned i=0;i<count;i++){
            size_t at=offset+i*32;if((unsigned char)bytes[at+12]!=68)continue;
            auto key=name(bytes,at+16,16);for(char& c:key)c=char(std::tolower((unsigned char)c));if(!wanted.count(key))continue;
            if(bytes[at+13])throw std::runtime_error("Compressed WAD textures are unsupported.");
            auto start=word(bytes,at),size=word(bytes,at+4);range(bytes,start,size);auto lump=bytes.substr(start,size);
            if(lump.size()<40)throw std::runtime_error("Truncated WAD mip texture.");lumps[key]=std::move(lump);
        }
    }
    std::string output("WAD2",4);output.resize(12,0);std::string directory;
    auto put=[](std::string& to,size_t at,uint32_t value){for(int b=0;b<4;b++)to[at+b]=char(value>>(b*8));};
    for(const auto& item:lumps){
        if(output.size()+item.second.size()>512u*1024u*1024u)throw std::runtime_error("Compiler WAD exceeds 512 MiB.");
        size_t entry=directory.size();directory.resize(entry+32,0);put(directory,entry,output.size());put(directory,entry+4,item.second.size());put(directory,entry+8,item.second.size());directory[entry+12]=68;
        directory.replace(entry+16,std::min(size_t(15),item.first.size()),item.first.substr(0,15));output+=item.second;
    }
    put(output,4,lumps.size());put(output,8,output.size());output+=directory;return output;
}
}
