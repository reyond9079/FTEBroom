// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/ftebroom_settle_physics.h"
#include <iostream>
#include <string>
using namespace ftebroom::settle;
static void require(bool ok,const char* name){if(!ok)throw std::runtime_error(name);std::cout<<"PASS "<<name<<'\n';}
static Body box(Point lo,Point hi){Body b; b.volume=1;for(int a=0;a<3;a++){b.center[a]=(lo[a]+hi[a])/2;b.volume*=hi[a]-lo[a];}for(int i=0;i<8;i++)b.vertices.push_back({i&1?hi[0]:lo[0],i&2?hi[1]:lo[1],i&4?hi[2]:lo[2]});return b;}
template<class F> static void rejects(F f,const char* message,const char* name){try{f();}catch(const std::runtime_error& e){require(std::string(e.what()).find(message)!=std::string::npos,name);return;}throw std::runtime_error(name);}
int main(){try{
 Options o;o.kick=0;o.duration=3;
 auto cube=box({-8,-8,64},{8,8,80}),floor=box({-512,-512,-32},{512,512,0});
 Simulation fall({cube},{floor},{},o);auto result=fall.run();
 double lowest=1e9;for(auto p:cube.vertices)lowest=std::min(lowest,transformPoint(result[0],p)[2]);require(lowest>-.25&&lowest<.25,"brush floor resting contact");
 require(fall.movingCount()==0,"rested fragment diagnostic");
 Simulation unsupported({cube},{},{},o);unsupported.run();require(unsupported.movingCount()==1,"unsupported falling fragment diagnostic");
 std::vector<Triangle> patch={Triangle{Point{-512,-512,0},Point{512,-512,0},Point{512,512,0}},Triangle{Point{-512,-512,0},Point{512,512,0},Point{-512,512,0}}};
 Simulation patchFall({cube},{},patch,o);result=patchFall.run();lowest=1e9;for(auto p:cube.vertices)lowest=std::min(lowest,transformPoint(result[0],p)[2]);require(lowest>-.25&&lowest<.25,"tessellated patch resting contact");
 rejects([&]{Simulation bad({cube,cube},{floor},{},o);},"overlap","initial overlap rejection");
 rejects([&]{Simulation bad({cube,box({0,0,0},{0,0,0})},{floor},{},o);},"closed convex","partial construction cleanup");
 rejects([&]{Simulation cancelled({cube},{floor},{},o);cancelled.run([](double,const auto&){return false;});},"cancelled","cancellation");
 o.bound=100;rejects([&]{Simulation escape({cube},{},{},o);escape.run();},"world bounds","escape detection");
 o.bound=65536;o.kick=8;Simulation a({cube},{floor},{},o),b({cube},{floor},{},o);auto ar=a.run(),br=b.run();require(ar[0].translation==br[0].translation&&ar[0].columns==br[0].columns,"deterministic seed");
 double last=0;Simulation progress({cube},{floor},{},o);progress.run([&](double value,const auto&){if(value<last)throw std::runtime_error("progress regression");last=value;return true;});require(last==1,"progress completion");
 return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
