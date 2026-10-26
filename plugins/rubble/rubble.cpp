// SPDX-License-Identifier: GPL-3.0-or-later
#include "reyondbroom_plugin.h"
#include <algorithm>
#include <cmath>
#include <string>
static const RBHostV1 *host;
static int holes=3,sides=6,axis=1,fragments=4,seed=1337,stage=0;
static float minimum=24,maximum=48,spacing=8,friction=.7f,bounce=.1f,impulse=32,duration=8;
static int release=1,negative=0;
static float clearance=2;
static std::string message;
static int busy(){return stage!=0;}
static std::string text(const char *name){char out[2048]={};host->string(name,out,sizeof out);return out;}
static void stop(const std::string& error){message=error;stage=0;}
static void tick(){
    if(stage==1){
        int status=int(host->number("ftebroom_plugin_status"));
        if(status<0){stop(text("ftebroom_plugin_error"));return;}
        if(status>0){
            RBSettleOptionsV1 options{friction,bounce,impulse,duration,uint32_t(seed)};
            if(host->settle(0,&options)<0){stop("Cannot start Settle. Undo restores the holes and fragments.");return;}
            stage=2;message="Simulating the fragments...";
        }
    }else if(stage==2){
        int status=host->settle(1,nullptr);
        if(status<0){stop(text("ftebroom_plugin_physics_error"));return;}
        if(status>0){if(host->settle(2,nullptr)<0){stop("Settle could not be applied.");return;}stage=3;}
    }else if(stage==3){
        std::string error=text("ftebroom_settle_error");
        if(!error.empty()){stop(error);return;}
        if(host->number("ftebroom_settle_applied")){stop("Done. Fragments remain selected. Undo once restores their positions; Undo again restores the source brush.");}
    }
}
static void draw(){
    host->text("Select one whole convex brush above a floor. Only the plugs inside the new holes become falling fragments.");
    bool editingDisabled=busy();if(editingDisabled)host->disabled(1);
    host->separator("Perforate");host->input_int("Holes",&holes);host->input_int("Polygon sides",&sides);
    host->combo("Through axis",&axis,"X\0Y\0Z\0");host->input_float("Minimum diameter",&minimum);host->input_float("Maximum diameter",&maximum);host->input_float("Spacing",&spacing);
    host->separator("Fracture");host->input_int("Fragments per hole",&fragments);host->input_int("Seed",&seed);
    host->separator("Settle");host->input_float("Friction",&friction);host->input_float("Bounce",&bounce);host->input_float("Initial impulse",&impulse);host->input_float("Duration (seconds)",&duration);
    host->checkbox("Release fragments outside the wall",&release);if(release){host->checkbox("Negative side of through axis",&negative);host->input_float("Release clearance",&clearance);}
    if(host->button("Apply")){
        bool finite=true;for(float value:{minimum,maximum,spacing,friction,bounce,impulse,duration,clearance})finite=finite&&std::isfinite(value);
        if(!finite||axis<0||axis>2||holes<1||holes>16||sides<3||sides>32||holes*sides>128||fragments<2||fragments>128||holes*fragments>128||minimum<=.1f||maximum<minimum||spacing<0||duration<.1f||duration>20||friction<0||friction>1||bounce<0||bounce>1||impulse<0||impulse>256||clearance<0||clearance>256){message="Invalid settings: holes x sides <= 128, total fragments <= 128, positive diameters and 0.1..20 seconds.";}
        else{
            host->set_number("ftebroom_hole_count",holes);host->set_number("ftebroom_hole_sides",sides);host->set_number("ftebroom_hole_axis",axis);host->set_number("ftebroom_hole_seed",seed);
            host->set_number("ftebroom_hole_min",minimum);host->set_number("ftebroom_hole_max",maximum);host->set_number("ftebroom_hole_spacing",spacing);host->set_number("ftebroom_hole_keep",1);
            host->set_number("ftebroom_fracture_count",fragments);host->set_number("ftebroom_fracture_seed",seed);host->set_number("ftebroom_fracture_separation",0);host->set_number("ftebroom_fracture_entities",0);host->set_string("ftebroom_fracture_material","");
            host->set_number("ftebroom_plugin_release_axis",axis);host->set_number("ftebroom_plugin_release_gap",clearance);host->set_number("ftebroom_plugin_release_negative",negative);
            host->set_number("ftebroom_plugin_status",0);host->command(release?"ftebroom_plugin_batch perforate fracture release\n":"ftebroom_plugin_batch perforate fracture\n");stage=1;message="Creating holes and fragments...";
        }
    }
    if(editingDisabled)host->disabled(0);
    if(busy()){host->progress(float(host->number("ftebroom_plugin_progress")),message.c_str());if(stage==2&&host->button("Cancel physics")){host->settle(3,nullptr);stop("Cancelled. Generated holes/fragments remain; Undo restores the original brush.");}}
    if(!message.empty())host->text(message.c_str());
}
static void shutdown(){if(stage==2||stage==3)host->settle(3,nullptr);stage=0;}
extern "C" RB_PLUGIN_EXPORT const RBPluginV1 *ReyondBroomPluginInit(const RBHostV1 *api){
    if(!api||api->api_version!=RB_PLUGIN_API_VERSION||api->struct_size<sizeof(RBHostV1))return nullptr;
    host=api;static const RBPluginV1 plugin{RB_PLUGIN_API_VERSION,sizeof(RBPluginV1),"org.reyondbroom.rubble",draw,tick,busy,shutdown};return &plugin;
}
