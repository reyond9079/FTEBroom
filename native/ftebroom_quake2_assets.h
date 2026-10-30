// SPDX-License-Identifier: GPL-2.0-or-later
#include "ftebroom_legacy_images.h"
static std::string q2Palette(){
  size_t length=0;auto data=(char*)files->LoadFile("pics/colormap.pcx",&length);
  if(!data)throw std::runtime_error("Quake II needs pics/colormap.pcx in the mod or baseq2 PAK files.");
  std::string bytes(data,length);core->Free(data);return ftebroom::legacy::pcx(bytes).palette;
}
static const MaterialEntry* q2Material(const std::string& source){
  auto name=ftebroom::materialKey(source);auto found=materials.find("textures/"+name);const MaterialEntry* exact=found==materials.end()?nullptr:&found->second;
  if(found!=materials.end()&&found->second.preview.rfind("textures/_ftebroom_missing/",0)!=0)return &found->second;
  auto leaf=name.substr(name.find_last_of('/')+1);found=materials.find("textures/"+leaf);
  return found!=materials.end()&&found->second.looseWal?&found->second:exact;
}
// Resolve document names once so every consumer sees the same real resource.
static std::set<std::string> q2MapMaterials;
static bool q2ExpandMapMaterials(){
  bool changed=false;
  for(const auto& name:q2MapMaterials){
    auto material=q2Material(name);if(!material||!material->looseWal)continue;
    auto key="textures/"+name;auto found=materials.find(key);
    if(found!=materials.end()&&found->second.preview==material->preview)continue;
    auto copy=*material;copy.name=key;materials[key]=std::move(copy);changed=true;
  }
  return changed;
}
static std::string q2WalBytes(const std::string& source){
  auto read=[](const std::string& path){size_t size=0;auto raw=(char*)files->LoadFile(path.c_str(),&size);if(!raw)return std::string{};std::string bytes(raw,size);core->Free(raw);return bytes;};
  auto name=ftebroom::materialKey(source);auto bytes=read("textures/"+name+".wal");
  if(!bytes.empty())return bytes;
  auto material=q2Material(name);return material&&material->looseWal?read(material->source):std::string{};
}
static void q2IndexAssets(){
  char runtime[4096]{};if(!files->NativePath("ftebroom.cfg",(fs_relative)5,runtime,sizeof runtime))throw std::runtime_error("Cannot locate the editor texture cache.");
  auto palette=q2Palette();auto editor=std::filesystem::u8path(runtime).parent_path();auto cache=editor/"textures/_ftebroom_q2";std::filesystem::create_directories(cache);std::filesystem::create_directories(editor/"scripts");
  std::ofstream scripts(editor/"scripts/reyondbroom-quake2.shader");
  auto cacheTexture=[&](const ftebroom::quake1::Texture& texture){
    auto key=ftebroom::quake1::textureCacheKey(texture,palette);auto file=cache/(key+".tga");
    auto image=ftebroom::quake1::tga(texture,palette);if(!std::filesystem::exists(file)||q1Read(file)!=image){std::ofstream output(file,std::ios::binary);output.write(image.data(),image.size());if(!output)throw std::runtime_error("Cannot cache Quake II texture.");}
    return "textures/_ftebroom_q2/"+key+".tga";
  };
  auto empty="textures/__tb_empty";
  if(!materials.count(empty))materials[empty]={empty,cacheTexture(ftebroom::placeholderTexture("__tb_empty",palette,true)),false};
  for(auto& pair:materials){auto& m=pair.second;auto ext=std::filesystem::u8path(m.preview).extension().u8string();for(char& c:ext)c=char(std::tolower((unsigned char)c));
    if(ext==".wal"||ext==".pcx"){
    size_t length=0;auto data=(char*)files->LoadFile(m.preview.c_str(),&length);if(!data)continue;std::string bytes(data,length);core->Free(data);
    auto texture=ext==".wal"?ftebroom::legacy::wal(bytes,palette):ftebroom::legacy::pcx(bytes);
    // Loose exports may omit or retain an unrelated internal WAL name.
    texture.name=ftebroom::materialKey(m.name);m.preview=cacheTexture(texture);
    auto flags=ext==".wal"?ftebroom::quake1::word(bytes,88):0;m.tags=(flags&4?512:0)|(flags&8?32:0)|(flags&48?64:0);m.opacity=flags&16?.33f:flags&32?.66f:1.f;
    }
  }
  q2ExpandMapMaterials();
  for(const auto& pair:materials){const auto& m=pair.second;
    scripts<<m.name<<"\n{\ncull back\ndiffusemap "<<m.preview<<"\n{\nmap "<<m.preview<<"\n"<<(m.opacity<1?"blendfunc blend\n":"")<<"}\n"<<(m.opacity<1?"":"{\nmap $lightmap\nblendfunc filter\ndepthfunc equal\n}\n")<<"}\n";
  }
  scripts.close();command("fs_restart\nftebroom_reload_palette\nftebroom_reload_asset_models\n");
}
