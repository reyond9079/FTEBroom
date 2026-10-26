// SPDX-License-Identifier: GPL-3.0-or-later
#include "reyondbroom_plugin.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include "feasibility.h"
static const RBHostV1 *host;
static int holes=3,sides=6,axis=3,fragments=4,seed=1337,stage=0;
static float minimum=24,maximum=48,spacing=8,friction=.7f,bounce=.1f,impulse=32,duration=8;
static int release=1,negative=0,automaticSide=1,focusResult=1;
static float clearance=2;
static std::string message;
static rubble::Check feasibility;
static int attempt=0,physicsAttempt=0,actualSeed=0,baseSeed=0;
static std::string layout;
static int busy(){return stage!=0;}
static std::string text(const char *name){char out[16384]={};host->string(name,out,sizeof out);return out;}
static void stop(const std::string& error){message=error;stage=0;host->set_string("ftebroom_plugin_hole_layout","");host->log(message.c_str());}
static void createGeometry(){
    actualSeed=(baseSeed+attempt*104729)&0xffffff;
    host->set_number("ftebroom_hole_seed",actualSeed);host->set_number("ftebroom_fracture_seed",actualSeed);
    // Try random sizes first; the constructive minimum-size layout is a safe
    // fallback, not a claim that unsuccessful random packing is impossible.
    host->set_string("ftebroom_plugin_hole_layout",attempt>=8?layout.c_str():"");
    host->set_number("ftebroom_plugin_status",0);host->set_number("ftebroom_plugin_actual_seed",actualSeed);
    host->command(release?"ftebroom_plugin_batch guard perforate fracture release\n":"ftebroom_plugin_batch guard perforate fracture\n");
    stage=1;message="Creating holes and fragments: seed "+std::to_string(actualSeed)+", attempt "+std::to_string(attempt+1)+" / 16.";
}
static bool startPhysics(){
    RBSettleOptionsV1 options{friction,bounce,impulse,duration,uint32_t((actualSeed+physicsAttempt*8191)&0xffffff)};
    if(host->settle(0,&options)<0)return false;
    host->set_number("ftebroom_plugin_physics_seed",options.seed);stage=2;
    message="Simulating fragments: attempt "+std::to_string(physicsAttempt+1)+" / 8.";return true;
}
static void tick(){
    if(stage==4){
        int status=int(host->number("ftebroom_plugin_preflight"));if(!status)return;
        if(status<0){stop("Select exactly one whole convex brush, not faces or vertices.");return;}
        std::istringstream source(text("ftebroom_plugin_source"));int count=0;std::array<double,6> bounds{};
        source>>count;for(double& value:bounds)source>>value;
        if(!source||count<4||count>64){stop("Cannot read source brush planes.");return;}
        std::vector<rubble::Plane> planes(count);for(auto& plane:planes)for(double& value:plane)source>>value;
        if(!source){stop("Incomplete source brush planes.");return;}
        feasibility=rubble::check(planes,bounds,axis,holes,minimum,spacing);
        if(!feasibility.allowed){stop(feasibility.reason);return;}
        double largestPlug=sides*.5*std::pow(maximum*.5,2)*std::sin(6.283185307179586/sides)*(bounds[feasibility.axis+3]-bounds[feasibility.axis]);
        if(largestPlug<=fragments*.1){stop("Even the maximum-size hole cannot contain the requested number of valid fragments. Increase diameter / brush thickness or reduce Fragments per hole.");return;}
        layout.clear();if(feasibility.certified){std::ostringstream out;out<<std::setprecision(9);for(auto p:feasibility.centers)out<<p[0]<<' '<<p[1]<<' '<<minimum*.5f<<' ';layout=out.str();}
        host->set_number("ftebroom_hole_axis",feasibility.axis);host->set_number("ftebroom_plugin_release_axis",feasibility.axis);
        int side=negative;if(automaticSide){
            if(feasibility.axis==2)side=1;
            else {std::istringstream camera(text("ftebroom_camera_state"));double eye[3]={};camera>>eye[0]>>eye[1]>>eye[2];side=camera&&eye[feasibility.axis]<(bounds[feasibility.axis]+bounds[feasibility.axis+3])*.5;}
        }
        host->set_number("ftebroom_plugin_release_negative",side);host->set_number("ftebroom_plugin_actual_axis",feasibility.axis);
        host->log(feasibility.certified?"Minimum-size clearance-circle layout verified before seed search.":"Necessary bounds passed; packing feasibility remains inconclusive. Searching at most 16 seeds.");
        attempt=physicsAttempt=0;baseSeed=uint32_t(seed)&0xffffff;createGeometry();
    }else if(stage==1){
        int status=int(host->number("ftebroom_plugin_status"));
        if(status<0){std::string error=text("ftebroom_plugin_error");if(status==-2||++attempt>=16){stop(error+" Seed search stopped; no geometry changes were committed.");return;}createGeometry();return;}
        if(status>0){
            host->set_string("ftebroom_plugin_hole_layout","");
            if(!startPhysics()){stop("Cannot start Settle. Undo restores the holes and fragments.");return;}
        }
    }else if(stage==2){
        int status=host->settle(1,nullptr);
        if(status<0){std::string error=text("ftebroom_plugin_physics_error");if(error.find("changed")!=std::string::npos||++physicsAttempt>=8||!startPhysics())stop(error+" Geometry remains selected; Undo restores the source brush.");return;}
        if(status>0){
            if(host->number("ftebroom_settle_moving")>0){host->settle(3,nullptr);if(++physicsAttempt>=8||!startPhysics())stop("Fragments are still moving or falling without support. Add a floor below the release side or increase Duration. Falling positions were not applied; generated geometry remains selected.");return;}
            host->set_number("ftebroom_plugin_group_after_settle",1);int applied=host->settle(2,nullptr);host->set_number("ftebroom_plugin_group_after_settle",0);
            if(applied<0){stop("Settle could not be applied.");return;}stage=3;
        }
    }else if(stage==3){
        std::string error=text("ftebroom_settle_error");
        if(!error.empty()){stop(error);return;}
        if(host->number("ftebroom_settle_applied")){
            int dropped=int(host->number("ftebroom_settle_dropped")),moved=int(host->number("ftebroom_settle_moved"));
            std::ostringstream result;result<<(dropped?"Done. ":"Applied, but no fragment dropped more than 1 map unit. ")<<holes*fragments<<" fragments grouped and selected; "<<moved<<" moved, "<<dropped<<" dropped. Mean drop: "<<std::fixed<<std::setprecision(1)<<host->number("ftebroom_settle_drop")<<" units. Seed "<<actualSeed<<". Undo once restores positions; twice restores the source brush.";
            stop(result.str());if(focusResult)host->command("ftebroom_camera_focus\n");
        }
    }
}
static void draw(){
    host->text("Select one whole convex brush above a floor. Only the plugs inside the new holes become falling fragments.");
    bool editingDisabled=busy();if(editingDisabled)host->disabled(1);
    host->separator("Perforate");host->input_int("Holes",&holes);host->input_int("Polygon sides",&sides);
    host->combo("Through axis",&axis,"X\0Y\0Z\0Auto (brush thickness)\0");host->input_float("Minimum diameter",&minimum);host->input_float("Maximum diameter",&maximum);host->input_float("Spacing",&spacing);
    host->separator("Fracture");host->input_int("Fragments per hole",&fragments);host->input_int("Seed",&seed);
    host->separator("Settle");host->input_float("Friction",&friction);host->input_float("Bounce",&bounce);host->input_float("Initial impulse",&impulse);host->input_float("Duration (seconds)",&duration);
    host->checkbox("Release fragments outside the wall",&release);if(release){host->checkbox("Automatic release side (down / toward camera)",&automaticSide);if(!automaticSide)host->checkbox("Negative side of through axis",&negative);host->input_float("Release clearance",&clearance);}
    host->checkbox("Frame fragments after applying",&focusResult);
    if(host->button("Apply")){
        bool finite=true;for(float value:{minimum,maximum,spacing,friction,bounce,impulse,duration,clearance})finite=finite&&std::isfinite(value);
        if(!finite||axis<0||axis>3||holes<1||holes>16||sides<3||sides>32||holes*sides>128||fragments<2||fragments>128||holes*fragments>128||minimum<=.1f||maximum<minimum||spacing<0||duration<.1f||duration>20||friction<0||friction>1||bounce<0||bounce>1||impulse<0||impulse>256||clearance<0||clearance>256){message="Invalid settings: holes x sides <= 128, total fragments <= 128, positive diameters and 0.1..20 seconds.";}
        else{
            host->set_number("ftebroom_hole_count",holes);host->set_number("ftebroom_hole_sides",sides);host->set_number("ftebroom_hole_axis",axis);host->set_number("ftebroom_hole_seed",seed);
            host->set_number("ftebroom_hole_min",minimum);host->set_number("ftebroom_hole_max",maximum);host->set_number("ftebroom_hole_spacing",spacing);host->set_number("ftebroom_hole_keep",1);
            host->set_number("ftebroom_fracture_count",fragments);host->set_number("ftebroom_fracture_seed",seed);host->set_number("ftebroom_fracture_separation",0);host->set_number("ftebroom_fracture_entities",0);host->set_string("ftebroom_fracture_material","");
            host->set_number("ftebroom_plugin_release_axis",axis);host->set_number("ftebroom_plugin_release_gap",clearance);host->set_number("ftebroom_plugin_release_negative",negative);
            host->set_string("ftebroom_plugin_hole_layout","");host->set_number("ftebroom_plugin_preflight",0);host->command("ftebroom_plugin_selection\n");stage=4;message="Checking the source planes, minimum diameter, count and spacing...";
        }
    }
    if(editingDisabled)host->disabled(0);
    if(busy()){host->progress(float(host->number("ftebroom_plugin_progress")),message.c_str());if(stage==2&&host->button("Cancel physics")){host->settle(3,nullptr);stop("Cancelled. Generated holes/fragments remain; Undo restores the original brush.");}}
    if(!message.empty())host->text(message.c_str());
    if(!busy()&&host->button("Frame fragments"))host->command("ftebroom_camera_focus\n");
}
static void shutdown(){if(stage==2||stage==3)host->settle(3,nullptr);stage=0;}
extern "C" RB_PLUGIN_EXPORT const RBPluginV1 *ReyondBroomPluginInit(const RBHostV1 *api){
    if(!api||api->api_version!=RB_PLUGIN_API_VERSION||api->struct_size<sizeof(RBHostV1))return nullptr;
    host=api;static const RBPluginV1 plugin{RB_PLUGIN_API_VERSION,sizeof(RBPluginV1),"org.reyondbroom.rubble",draw,tick,busy,shutdown};return &plugin;
}
