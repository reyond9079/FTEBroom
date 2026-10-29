// SPDX-License-Identifier: GPL-2.0-or-later
#include "ftebroom_legacy_images.h"
static std::string q2Palette(){
  size_t length=0;auto data=(char*)files->LoadFile("pics/colormap.pcx",&length);
  if(!data)throw std::runtime_error("Quake II needs pics/colormap.pcx in the mod or baseq2 PAK files.");
  std::string bytes(data,length);core->Free(data);return ftebroom::legacy::pcx(bytes).palette;
}
static void q2IndexAssets(){
  char runtime[4096]{};if(!files->NativePath("ftebroom.cfg",(fs_relative)5,runtime,sizeof runtime))throw std::runtime_error("Cannot locate the editor texture cache.");
  auto palette=q2Palette();auto editor=std::filesystem::u8path(runtime).parent_path();auto cache=editor/"textures/_ftebroom_q2";std::filesystem::create_directories(cache);std::filesystem::create_directories(editor/"scripts");
  std::ofstream scripts(editor/"scripts/reyondbroom-quake2.shader");
  for(auto& pair:materials){auto& m=pair.second;if(m.preview.size()<4||m.preview.substr(m.preview.size()-4)!=".wal")continue;
    size_t length=0;auto data=(char*)files->LoadFile(m.preview.c_str(),&length);if(!data)continue;std::string bytes(data,length);core->Free(data);
    auto texture=ftebroom::legacy::wal(bytes,palette);auto key=ftebroom::quake1::textureCacheKey(texture,palette);auto file=cache/(key+".tga");
    auto image=ftebroom::quake1::tga(texture,palette);bool same=false;if(std::filesystem::exists(file))same=q1Read(file)==image;
    if(!same){std::ofstream output(file,std::ios::binary);output.write(image.data(),image.size());if(!output)throw std::runtime_error("Cannot cache Quake II texture.");}
    auto flags=ftebroom::quake1::word(bytes,88);m.tags=(flags&4?512:0)|(flags&8?32:0)|(flags&128?64:0);m.opacity=flags&16?.33f:flags&32?.66f:1.f;
    m.preview="textures/_ftebroom_q2/"+key+".tga";
    scripts<<m.name<<"\n{\ncull back\ndiffusemap "<<m.preview<<"\n{\nmap "<<m.preview<<"\n"<<(m.opacity<1?"blendfunc blend\n":"")<<"}\n"<<(m.opacity<1?"":"{\nmap $lightmap\nblendfunc filter\ndepthfunc equal\n}\n")<<"}\n";
  }
  scripts.close();command("fs_restart\nftebroom_reload_palette\nftebroom_reload_asset_models\n");
}
