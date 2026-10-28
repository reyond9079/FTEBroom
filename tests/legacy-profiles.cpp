// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/ftebroom_legacy_images.h"
#include "../native/ftebroom_legacy_models.h"
#include "../native/ftebroom_map_profile.h"
#include "../native/ftebroom_entity_defs.h"
#include "../native/ftebroom_bsp_uv.h"
#include "../native/ftebroom_compiler_wad.h"
#include "../native/ftebroom_pak.h"
#include <cassert>
#include <iostream>
static std::string read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){
 assert(argc==3);std::filesystem::path root=std::filesystem::u8path(argv[1]),qa=std::filesystem::u8path(argv[2]);using namespace ftebroom;
 assert(mapProfile("// Game: Quake2\n// Format: Valve\n{").profile==3);
 assert(mapProfile("// Game: Half-Life\n// Format: Valve\n{").profile==4);
 assert(mapProfile("// ReyondBroom profile=halflife format=valve220\n{").profile==4);
 auto defs=entityRead(read(root/"runtime/quake2.fgd"),".fgd");assert(defs.count("info_player_start")&&defs.count("monster_soldier"));
 assert(defs.at("info_player_start").model=="models/monsters/insane/tris.md2");assert(defs.at("monster_soldier").modelSkin=="2");
 auto hl=entityRead(read(root/"runtime/halflife.fgd"),".fgd");assert(hl.count("monster_scientist")&&hl.count("info_player_start"));
 auto wad=read(root/"runtime/halflife-editor.wad");auto textures=quake1::wad(wad);assert(textures.size()==1&&textures[0].palette.size()==768);assert(quake1::tga(textures[0],{}).size()==18+16*16*3);
 auto packed=quake1::compilerWad({wad},{"fb_grey"},true);assert(quake1::wad(packed)[0].palette==textures[0].palette);
 bool bad=false;try{quake1::wad(wad.substr(0,wad.size()-100));}catch(...){bad=true;}assert(bad);
 auto pcx=legacy::pcx(read(qa/"q2/pics/colormap.pcx"));auto wal=legacy::wal(read(qa/"q2/textures/fb_grey.wal"),pcx.palette);assert(wal.width==16&&wal.height==16);
 auto md2=legacyModels::md2(read(qa/"models/test94.md2"));assert(md2.size()==1&&md2[0].triangles.size()==1&&md2[0].triangles[0][2].point[2]==32);
 auto mdl=legacyModels::studio(read(qa/"models/studio94.mdl"),{});assert(mdl.size()==1&&mdl[0].triangles.size()==1&&mdl[0].triangles[0][2].point[2]==32&&mdl[0].embedded.palette.size()==768);
 bad=false;try{legacy::wal(read(qa/"q2/textures/fb_grey.wal").substr(0,100),pcx.palette);}catch(...){bad=true;}assert(bad);
 for(int profile:{3,4}){auto dir=qa/(profile==3?"q2":"hl");auto bsp=read(dir/"maps/room.bsp");assert(quake1::gameBsp(bsp,profile));assert(!quake1::gameBsp(bsp,profile==3?4:3));
   auto uv=bspUV::read(bsp,pcx.palette,{{"fb_grey",read(qa/"q2/textures/fb_grey.wal")}});assert(!uv.empty()&&uv[0].width==16&&!uv[0].image.empty()&&!uv[0].edges.empty());
   auto map=read(dir/"maps/room.map");if(profile==4){auto converted=quake1::map(map,false,"textures.wad",false,true);assert(converted.find("\"mapversion\" \"220\"")!=converted.npos);}
 }
 // Deleting archive entries must retain unrelated bytes and create no backups.
 auto folder=qa/"delete-test";std::filesystem::create_directories(folder);auto pak=folder/"delete.pak";
 {std::ofstream f(pak,std::ios::binary);f.write("PACK",4);pak::put(f,12);pak::put(f,0);}
 pak::replace(pak/"keep.txt","keep");pak::replace(pak/"remove.txt","remove");size_t before=std::distance(std::filesystem::directory_iterator(folder),std::filesystem::directory_iterator{});pak::erase(pak/"remove.txt");
 assert(pak::read(pak/"keep.txt")=="keep"&&!pak::exists(pak/"remove.txt"));assert(size_t(std::distance(std::filesystem::directory_iterator(folder),std::filesystem::directory_iterator{}))==before);
 auto pk3=folder/"delete.pk3";{std::ofstream f(pk3,std::ios::binary);f.write("PK\5\6",4);for(int i=0;i<18;i++)f.put(0);}
 pak::replace(pk3/"keep.txt","keep");pak::replace(pk3/"remove.txt","remove");before=std::distance(std::filesystem::directory_iterator(folder),std::filesystem::directory_iterator{});pak::erase(pk3/"remove.txt");
 assert(pak::read(pk3/"keep.txt")=="keep"&&!pak::exists(pk3/"remove.txt"));assert(size_t(std::distance(std::filesystem::directory_iterator(folder),std::filesystem::directory_iterator{}))==before);
 std::cout<<"LEGACY_PROFILES_PASS Q2/HL headers, FGD models, WAD3 palettes, WAL/PCX, BSP UVs, MAP220, deletion\n";
}
