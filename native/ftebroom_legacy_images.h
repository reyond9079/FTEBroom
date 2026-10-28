// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ftebroom_quake1.h"
namespace ftebroom::legacy {
inline quake1::Texture wal(const std::string& bytes,const std::string& palette) {
  using namespace quake1;range(bytes,0,100);
  Texture t{name(bytes,0,32),word(bytes,32),word(bytes,36),{},palette};
  if(!t.width||!t.height||t.width>8192||t.height>8192||uint64_t(t.width)*t.height>16777216)throw std::runtime_error("Invalid WAL dimensions.");
  for(int mip=0;mip<4;mip++){auto pos=word(bytes,40+4*mip);if(pos<100)throw std::runtime_error("Invalid WAL mip offset.");range(bytes,pos,uint64_t(t.width>>mip)*(t.height>>mip));}
  auto pos=word(bytes,40);t.pixels.assign(bytes.begin()+pos,bytes.begin()+pos+size_t(t.width)*t.height);return t;
}
inline quake1::Texture pcx(const std::string& bytes) {
  using namespace quake1;range(bytes,0,128);
  auto u16=[&](size_t at){range(bytes,at,2);return unsigned((unsigned char)bytes[at])|(unsigned((unsigned char)bytes[at+1])<<8);};
  if((unsigned char)bytes[0]!=10||(unsigned char)bytes[2]!=1||(unsigned char)bytes[3]!=8||(unsigned char)bytes[65]!=1)throw std::runtime_error("Only 8-bit indexed PCX images are supported.");
  unsigned left=u16(4),top=u16(6),right=u16(8),bottom=u16(10),stride=u16(66);
  if(right<left||bottom<top)throw std::runtime_error("Invalid PCX bounds.");
  Texture t{"pcx",right-left+1,bottom-top+1,{},{}};
  if(!t.width||!t.height||t.width>8192||t.height>8192||stride<t.width||stride>16384||uint64_t(t.width)*t.height>16777216)throw std::runtime_error("Invalid PCX dimensions.");
  if(bytes.size()<897||(unsigned char)bytes[bytes.size()-769]!=12)throw std::runtime_error("Missing PCX palette.");t.palette=bytes.substr(bytes.size()-768);
  size_t at=128,end=bytes.size()-769;t.pixels.reserve(size_t(t.width)*t.height);
  for(unsigned y=0;y<t.height;y++)for(unsigned x=0;x<stride;){
    if(at>=end)throw std::runtime_error("Truncated PCX pixels.");unsigned c=(unsigned char)bytes[at++],n=1;
    if((c&192)==192){n=c&63;if(!n||at>=end)throw std::runtime_error("Invalid PCX run.");c=(unsigned char)bytes[at++];}
    if(n>stride-x)throw std::runtime_error("PCX run crosses a scanline.");
    for(unsigned i=0;i<n;i++,x++)if(x<t.width)t.pixels.push_back(c);
  }return t;
}
}
