// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise preflight geometry and the real plugin state machine with a host double.
#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
#include "../plugins/rubble/rubble.cpp"
static std::map<std::string,double> values;
static std::map<std::string,std::string> strings;
static std::vector<std::string> commandsSent;
static bool clickApply=false;
static int physicsStatus=0,starts=0;
static double number(const char* key){return values[key];}
static int readString(const char* key,char* out,size_t size){auto s=strings[key];if(size){auto n=std::min(size-1,s.size());std::memcpy(out,s.data(),n);out[n]=0;}return int(s.size());}
static void setNumber(const char* key,double value){values[key]=value;}
static void setString(const char* key,const char* value){strings[key]=value;}
static void commandSent(const char* value){commandsSent.push_back(value);}
static int settleMock(int op,const RBSettleOptionsV1*){if(op==0){starts++;return 0;}return op==1?physicsStatus:1;}
static int buttonMock(const char* name){if(std::strcmp(name,"Apply")||!clickApply)return 0;clickApply=false;return 1;}
static std::vector<rubble::Plane> boxPlanes(double x,double y,double z){return {{1,0,0,x},{-1,0,0,x},{0,1,0,y},{0,-1,0,y},{0,0,1,z},{0,0,-1,z}};}
static std::string sourceText(const std::vector<rubble::Plane>& planes,const std::array<double,6>& bounds){std::ostringstream out;out<<planes.size();for(double v:bounds)out<<' '<<v;for(auto p:planes)for(double v:p)out<<' '<<v;return out.str();}
static void applyBox(double x,double y,double z){
    stage=0;values.clear();strings.clear();commandsSent.clear();starts=0;physicsStatus=0;
    holes=3;sides=6;axis=3;fragments=4;minimum=24;maximum=48;spacing=8;automaticSide=release=focusResult=1;
    clickApply=true;draw();assert(stage==4&&commandsSent.back()=="ftebroom_plugin_selection\n");
    strings["ftebroom_plugin_source"]=sourceText(boxPlanes(x,y,z),{-x,-y,-z,x,y,z});values["ftebroom_plugin_preflight"]=1;tick();
}
int main(){
    RBHostV1 api{};api.api_version=RB_PLUGIN_API_VERSION;api.struct_size=sizeof api;
    api.number=number;api.string=readString;api.set_number=setNumber;api.set_string=setString;api.command=commandSent;api.log=[](const char*){};
    api.settle=settleMock;api.text=[](const char*){};api.separator=[](const char*){};api.input_int=[](const char*,int*){return 0;};api.input_float=[](const char*,float*){return 0;};api.checkbox=[](const char*,int*){return 0;};api.combo=[](const char*,int*,const char*){return 0;};api.button=buttonMock;api.disabled=[](int){};api.progress=[](float,const char*){};
    assert(ReyondBroomPluginInit(&api));
    auto slab=boxPlanes(256,256,8);std::array<double,6> bounds{-256,-256,-8,256,256,8};
    auto automatic=rubble::check(slab,bounds,3,3,24,8);assert(automatic.allowed&&automatic.certified&&automatic.axis==2);
    assert(!rubble::check(slab,bounds,1,3,24,8).allowed);
    assert(!rubble::check(slab,bounds,3,16,100,1000).allowed);
    assert(!rubble::check(slab,bounds,3,1,10000,0).allowed);
    auto wall=boxPlanes(128,16,128);auto wallCheck=rubble::check(wall,{-128,-16,-128,128,16,128},3,3,24,8);assert(wallCheck.certified&&wallCheck.axis==1);
    // A wedge needs paired upper/lower planes, not just its bounding rectangle.
    auto wedge=slab;wedge.push_back({.7071067811865475,0,.7071067811865475,0});auto wedgeCheck=rubble::check(wedge,bounds,2,3,24,8);assert(wedgeCheck.certified);for(auto p:wedgeCheck.centers)assert(p[0]<-3);
    applyBox(256,256,8);assert(stage==1&&values["ftebroom_plugin_actual_axis"]==2&&values["ftebroom_plugin_release_negative"]==1);
    values["ftebroom_plugin_status"]=-1;strings["ftebroom_plugin_error"]="Geometry failed";
    for(int n=0;n<8;n++){values["ftebroom_plugin_status"]=-1;tick();assert(stage==1);}assert(attempt==8&&!strings["ftebroom_plugin_hole_layout"].empty());
    values["ftebroom_plugin_status"]=1;tick();assert(stage==2&&starts==1&&strings["ftebroom_plugin_hole_layout"].empty());
    physicsStatus=-1;strings["ftebroom_plugin_physics_error"]="Try another seed";tick();assert(stage==2&&starts==2);
    physicsStatus=1;tick();assert(stage==3);values["ftebroom_settle_applied"]=1;values["ftebroom_settle_moved"]=12;values["ftebroom_settle_dropped"]=12;values["ftebroom_settle_drop"]=100;tick();assert(!busy()&&message.find("12 dropped")!=std::string::npos&&commandsSent.back()=="ftebroom_camera_focus\n");
    applyBox(256,256,8);for(int n=0;n<16;n++){values["ftebroom_plugin_status"]=-1;tick();}assert(!busy()&&attempt==16&&strings["ftebroom_plugin_hole_layout"].empty()&&starts==0);
    applyBox(256,256,8);values["ftebroom_plugin_status"]=-2;tick();assert(!busy()&&attempt==0);
    applyBox(256,256,8);values["ftebroom_plugin_status"]=1;tick();physicsStatus=1;tick();values["ftebroom_settle_applied"]=1;tick();assert(!busy()&&message.find("no fragment dropped")!=std::string::npos);
    applyBox(8,8,8);assert(!busy()&&starts==0&&commandsSent.size()==1);
    std::cout<<"PASS circle-section feasibility, Auto axis, tilted plane, bounded geometry / physics retries, certified fallback, stale guard, no-drop status and focus\n";
}
