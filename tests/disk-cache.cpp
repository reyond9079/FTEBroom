// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/ftebroom_disk_cache.h"
#include <cassert>
#include <iostream>
using namespace ftebroom::diskcache;
int main(int argc,char** argv){
 if(argc==3&&std::string(argv[1])=="--clean"){
  Options o;o.aggressive=true;auto r=clean(fs::u8path(argv[2]),o);
  std::cout<<"Removed files="<<r.files<<" bytes="<<r.bytes<<" errors="<<r.errors<<"\n";return r.errors?1:0;
 }
 auto root=fs::temp_directory_path()/("reyondbroom-cache-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 fs::create_directories(root);
 auto put=[&](std::string name,std::string data="test",int hours=0){auto p=root/name;fs::create_directories(p.parent_path());std::ofstream(p)<<data;if(hours)fs::last_write_time(p,fs::file_time_type::clock::now()-std::chrono::hours(hours));return p;};
 auto pinned=put("textures/_ftebroom_q1/v1-aaaa.tga","pixels",72);
 auto unused=put("textures/_ftebroom_q1/v1-bbbb.tga","pixels",72);
 auto recent=put("textures/_ftebroom_q1/v1-cccc.tga");
 auto unknown=put("textures/_ftebroom_q1/my-painting.tga","pixels",72);
 auto source=put("maps/source.map"),wad=put("textures/source.wad"),autosave=put("autosave/map.map"),bsp=put("maps/map.bsp");
 auto staging=put("maps/_ftebroom_open_1234_1.map"),protectedMap=put("maps/_ftebroom_open_1234_2.map");
 auto notStaging=put("maps/_ftebroom_open_my-map.map");
 put("scripts/ftebroom-quake1.shader","map textures/_ftebroom_q1/v1-aaaa.tga");
 auto outside=root.parent_path()/(root.filename().u8string()+"-external");fs::create_directories(outside);std::ofstream(outside/"v1-dddd.tga")<<"safe";
 std::error_code symlinkError;fs::create_directory_symlink(outside,root/"textures/_ftebroom_q2",symlinkError);
 Options options;options.protectedFiles.insert(protectedMap);auto r=clean(root,options);
 assert(r.files==2&&!fs::exists(unused)&&!fs::exists(staging));
 for(const auto& p:{pinned,recent,unknown,source,wad,autosave,bsp,protectedMap,notStaging,outside/"v1-dddd.tga"})assert(fs::exists(p));
 options.highWater=1;options.lowWater=1;clean(root,options);assert(!fs::exists(recent)&&fs::exists(pinned));
 fs::remove(root/"scripts/ftebroom-quake1.shader");auto noManifest=put("textures/_ftebroom_q1/v1-eeee.tga","pixels",72);clean(root,options);assert(fs::exists(noManifest));
 auto sessionMap=put("data/mod-preview/100/current.map"),sessionWad=put("data/mod-preview/100/a.wad"),sessionBsp=put("data/mod-preview/100/a.bsp");
 auto derived=put("data/mod-preview/100/home/cache.bin");put("data/mod-preview/200/cache.bin");put("data/mod-preview/300/cache.bin");options.aggressive=true;
 clean(root,options);assert(!fs::exists(derived));for(const auto& p:{sessionMap,sessionWad,sessionBsp})assert(fs::exists(p));
 // A normal start only sweeps staging; it must not parse history or scan TGAs.
 auto deferred=put("textures/_ftebroom_q1/v1-ffff.tga","pixels",72);
 Options startup;startup.fullScan=false;startup.aggressive=true;
 auto quick=clean(root,startup);assert(!quick.fullScan&&fs::exists(deferred));
 assert(fullScanDue(root,200000));markFullScan(root,200000);
 assert(!fullScanDue(root,200001)&&!fullScanDue(root,286399));
 assert(fullScanDue(root,286400)&&fullScanDue(root,199999));
 put("scripts/ftebroom-quake1.shader","// history-only fixture");
 std::vector<fs::path> projects;
 for(int i=0;i<6;i++){
  auto image="textures/_ftebroom_q1/v1-ab"+std::to_string(i)+".tga";
  projects.push_back(put(image,"old project texture",72));
  assert(rememberProject(root,"project "+std::to_string(i),{image}));
 }
 Options pressure;pressure.highWater=1;pressure.lowWater=1;pressure.aggressive=true;
 auto history=recentProjects(root);assert(history.valid&&history.keep.size()==5);
 clean(root,pressure);assert(!fs::exists(projects[0]));
 for(int i=1;i<6;i++)assert(fs::exists(projects[i]));
 projects[0]=put("textures/_ftebroom_q1/v1-ab0.tga","reopened",72);
 assert(rememberProject(root,"project 0",{"textures/_ftebroom_q1/v1-ab0.tga"}));
 clean(root,pressure);assert(fs::exists(projects[0])&&!fs::exists(projects[1]));
 assert(recentProjects(root).keep.size()==5);
 // Invalid history fails closed, while quick staging sweeps still skip it.
 put("cache-projects/project-9999999999999999999-1.txt","invalid history");
 auto safety=put("textures/_ftebroom_q1/v1-ab7.tga","safe",72);
 assert(clean(root,startup).errors==0);
 assert(clean(root,pressure).errors==1&&fs::exists(safety));
 fs::remove_all(root);fs::remove_all(outside);
 std::cout<<"PASS: pinned caches, five recent projects, LRU reopening, budget pressure, 24h scheduling/clock rollback, fast staging-only startup, malformed-history safety and source/link protection\n";
}
