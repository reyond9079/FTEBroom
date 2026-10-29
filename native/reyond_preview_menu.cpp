// SPDX-License-Identifier: GPL-2.0-or-later
// In-process Escape overlay for isolated Linux mod previews (native and SDL2).
#include <cstdint>
typedef int uploadfmt_t;
typedef int clipboardtype_t;
typedef struct qdownload_s qdownload_t;
typedef struct downloadlist_s downloadlist_t;
enum dlfailreason_e : int;
enum com_tokentype_e : int;
enum fs_relative : int;
#define VARGS
#include "imgui.h"
#include "plugin.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include "ftebroom_localization.h"
static plugcorefuncs_t* core;
static plugcmdfuncs_t* commands;
static plugcvarfuncs_t* cvars;
static plug2dfuncs_t* draw;
static pluginputfuncs_t* input;
static plugfsfuncs_t* files;
static qhandle_t atlas;
static bool ready=false,opened=false,raw=false;
static quintptr_t lastTime;
#include "ftebroom_imgui_draw.h"
#include "ftebroom_preview_lighting.h"
static PreviewLighting lighting;
static std::string source;
static float sensitivity=3;
static bool inverted=false;
static int capture=-1;
static ImVec2 lightingToggle;
static float screenWidth=1280,screenHeight=800;
struct Binding{const char* label;const char* action;const char* initial;int key;};
static Binding bindings[]={
 {"Forward","+forward","w"},{"Backward","+back","s"},{"Left","+moveleft","a"},{"Right","+moveright","d"},
 {"Jump / swim up","+jump","space"},{"Attack","+attack","mouse1"},{"Next weapon","impulse 10","mwheelup"},
 {"Previous weapon","impulse 12","mwheeldown"},{"Run / walk","+speed","shift"},{"Swim down","+movedown","c"}
};
static std::vector<std::string> appliedKeys;
static void send(const std::string& text){commands->AddText((text+"\n").c_str(),qfalse);}
static void publish(){
 std::ostringstream line;line<<"reyond_preview_settings ";lighting.write(line);line<<' '<<sensitivity<<' '<<inverted;
 for(const auto& b:bindings)line<<' '<<b.key;
 std::printf("%s\n",line.str().c_str());std::fflush(stdout);
}
static void applyLighting(){send(lighting.command(raw,source));publish();}
static void applyControls(){
 for(const auto& key:appliedKeys)send("unbind \""+key+"\"");appliedKeys.clear();
 for(const auto& b:bindings){const char* text=input->GetKeyName(b.key,0);std::string key=text?text:"";
  if(key.empty()||key.find_first_of("\";\\\r\n")!=std::string::npos)continue;
  send("bind \""+key+"\" \""+b.action+"\"");appliedKeys.push_back(key);}
 send("set sensitivity "+std::to_string(sensitivity));send(std::string("set m_pitch ")+(inverted?"-0.022":"0.022"));publish();
}
static void closeMenu(){opened=false;capture=-1;input->SetMenuFocus(qfalse,"",0,0,1);}
static void toggle(){
 if(opened){closeMenu();return;}
 opened=true;capture=-1;
 // Release movement/attack before acquiring UI focus so a held key cannot stick.
 send("-forward\n-back\n-moveleft\n-moveright\n-jump\n-attack\n-speed\n-movedown");
 input->SetMenuFocus(qtrue,"shape:1:",0,0,1);
 std::printf("reyond_preview_menu_open\n");std::fflush(stdout);
}
static void uploadFontAtlas()
{
  auto& io=ImGui::GetIO();
  unsigned char* pixels;
  int w, h;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
  std::vector<unsigned char> tga(18 + w * h * 4, 0);
  tga[2] = 2;
  tga[12] = w & 255;
  tga[13] = w >> 8;
  tga[14] = h & 255;
  tga[15] = h >> 8;
  tga[16] = 32;
  tga[17] = 0x28;
  for (int i = 0; i < w * h; ++i)
  {
    tga[18 + i * 4] = pixels[i * 4 + 2];
    tga[19 + i * 4] = pixels[i * 4 + 1];
    tga[20 + i * 4] = pixels[i * 4];
    tga[21 + i * 4] = pixels[i * 4 + 3];
  }
  atlas = draw->LoadImageData("reyond-preview/font.tga", "image/tga", tga.data(), tga.size());
  atlas = draw->LoadImage("reyond-preview/font.tga");
  io.Fonts->SetTexID((ImTextureID)atlas);
}

static void initialize(){
 if(ready)return;ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
 io.BackendFlags|=ImGuiBackendFlags_RendererHasVtxOffset;io.ConfigFlags|=ImGuiConfigFlags_NoMouseCursorChange;
 uiBuildGlyphRanges();char fontPath[4096]={};
 files->NativePath("reyond-preview/menu.otf",(fs_relative)5,fontPath,sizeof fontPath);
 ImFont* loaded=nullptr;if(std::ifstream(fontPath).good())loaded=io.Fonts->AddFontFromFileTTF(fontPath,17,nullptr,uiCJKRanges.Data);
 if(!loaded){ImFontConfig font;font.SizePixels=17;io.Fonts->AddFontDefault(&font);}
 ImGui::StyleColorsDark();auto& style=ImGui::GetStyle();style.WindowPadding={16,16};style.FramePadding={8,5};style.ItemSpacing={8,9};
 style.Colors[ImGuiCol_FrameBg]={.13f,.15f,.18f,1};style.Colors[ImGuiCol_Border]={.38f,.42f,.48f,1};style.FrameBorderSize=1;
 uploadFontAtlas();ready=true;
}
static qboolean QDECL event(int type,int key,int unicode,float x,float y,float w,float h){
 if(!opened)return qfalse;initialize();auto& io=ImGui::GetIO();io.AddMousePosEvent(x,y);
 if(type==1||type==2){bool down=type==1;
  if(capture>=0){if(down){if(key!=input->GetKeyCode("escape",nullptr)&&key>0&&key<1024){
   int old=bindings[capture].key;for(auto& b:bindings)if(b.key==key)b.key=old;bindings[capture].key=key;}
   capture=-1;}return qtrue;}
  if(key==input->GetKeyCode("escape",nullptr)){if(down)closeMenu();return qtrue;}
  for(int i=0;i<5;i++){auto name="mouse"+std::to_string(i+1);if(key==input->GetKeyCode(name.c_str(),nullptr))io.AddMouseButtonEvent(i,down);}
  if(down&&key==input->GetKeyCode("mwheelup",nullptr))io.AddMouseWheelEvent(0,1);
  if(down&&key==input->GetKeyCode("mwheeldown",nullptr))io.AddMouseWheelEvent(0,-1);
  const std::pair<const char*,ImGuiKey> keys[]={{"backspace",ImGuiKey_Backspace},{"del",ImGuiKey_Delete},{"leftarrow",ImGuiKey_LeftArrow},{"rightarrow",ImGuiKey_RightArrow},{"home",ImGuiKey_Home},{"end",ImGuiKey_End},{"enter",ImGuiKey_Enter},{"tab",ImGuiKey_Tab},{"ctrl",ImGuiMod_Ctrl},{"shift",ImGuiMod_Shift}};
  for(const auto& k:keys)if(key==input->GetKeyCode(k.first,nullptr))io.AddKeyEvent(k.second,down);
  if(down&&unicode>=32&&unicode<=0x10ffff)io.AddInputCharacter(unicode);
 }
 if(type==0){
  screenWidth=w;screenHeight=h;
  auto now=core->GetMilliseconds();io.DeltaTime=lastTime?std::clamp((now-lastTime)/1000.f,.001f,.1f):1.f/60;lastTime=now;io.DisplaySize={w,h};
  // Let the engine own one native cursor; do not draw a second ImGui cursor.
  io.MouseDrawCursor=false;ImGui::NewFrame();
  ImGui::SetNextWindowPos({w*.5f,h*.5f},ImGuiCond_Appearing,{.5f,.5f});
  ImGui::SetNextWindowSize({std::min(650.f,w-32),std::min(800.f,h-32)},ImGuiCond_Appearing);
  bool visible=opened;
  if(ImGui::Begin("Mod controls",&visible,ImGuiWindowFlags_NoSavedSettings)){
   ImGui::TextWrapped("Controls apply to this preview. Your mod files are unchanged.");
   ImGui::BeginChild("Preview settings",{0,-ImGui::GetFrameHeightWithSpacing()*2});
   if(ImGui::CollapsingHeader("Lighting",ImGuiTreeNodeFlags_DefaultOpen))if(previewLightingWidgets(lighting,raw,&lightingToggle))applyLighting();
   ImGui::Separator();ImGui::SliderFloat("Mouse sensitivity",&sensitivity,.1f,20,"%.1f");ImGui::Checkbox("Invert mouse Y",&inverted);
   for(int i=0;i<10;i++){ImGui::PushID(i);ImGui::TextUnformatted(bindings[i].label);ImGui::SameLine(250);
    const char* name=input->GetKeyName(bindings[i].key,0);if(ImGui::Button(capture==i?"Press a key...":name?name:"Unbound",{170,0}))capture=i;ImGui::PopID();}
   ImGui::EndChild();ImGui::Separator();
   if(ImGui::Button("Apply")){applyControls();applyLighting();}ImGui::SameLine();if(ImGui::Button("Continue"))visible=false;
   ImGui::SameLine();if(ImGui::Button("Exit mod preview"))send("quit force");
   ImGui::TextDisabled("The mod simulation continues while this menu is open.");
  }
  ImGui::End();render();if(!visible)closeMenu();
 }
 return qtrue;
}
// Integration tests exercise the real ImGui checkbox and Escape event routing.
static void testMenu(){
 if(commands->IsInsecure())return;
 char stage[32];commands->Argv(1,stage,sizeof stage);
 if(!strcmp(stage,"press")||!strcmp(stage,"release")){
  if(opened&&ready)event(!strcmp(stage,"press")?1:2,input->GetKeyCode("mouse1",nullptr),0,lightingToggle.x,lightingToggle.y,screenWidth,screenHeight);
 }else if(!strcmp(stage,"escape"))event(1,input->GetKeyCode("escape",nullptr),0,0,0,screenWidth,screenHeight);
 else {std::printf("reyond_preview_test opened=%i rtl=%i distance=%g\n",opened,lighting.enabled,lighting.distance);std::fflush(stdout);}
}
static std::string arg(int index){char text[4096];commands->Argv(index,text,sizeof text);return text;}
static void configure(){
 // Startup command is editor-generated, but validate paths before executing it.
 if(commands->IsInsecure()||(commands->Argc()!=21&&commands->Argc()!=22))return;
 if(commands->Argc()==22)uiLanguage=std::clamp(std::atoi(arg(21).c_str()),0,6);
 std::istringstream values(arg(1)+" "+arg(2)+" "+arg(5)+" "+arg(6)+" "+arg(7)+" "+arg(8));lighting.read(values);
 raw=arg(3)=="1";source=arg(4);
 if(source.size()>127||(!source.empty()&&source.rfind("maps/",0)!=0)||source.find("..")!=std::string::npos||source.find_first_of(":\";\\\r\n")!=std::string::npos){source.clear();return;}
 sensitivity=std::clamp(float(std::atof(arg(9).c_str())),.1f,20.f);inverted=arg(10)=="1";
 for(int i=0;i<10;i++){int key=std::atoi(arg(11+i).c_str());if(key>0&&key<1024)bindings[i].key=key;}
 applyControls();applyLighting();std::printf("reyond_preview_menu_ready\n");std::fflush(stdout);
}
static void QDECL shutdownMenu(){if(ready){ImGui::DestroyContext();ready=false;}opened=false;}
static void QDECL videoChanged(int,int,qboolean restarted){if(ready&&restarted)uploadFontAtlas();}
extern "C" qboolean NATIVEEXPORT FTEPlug_Init(plugcorefuncs_t* api){
 core=api;commands=(plugcmdfuncs_t*)core->GetEngineInterface(plugcmdfuncs_name,sizeof(*commands));
 cvars=(plugcvarfuncs_t*)core->GetEngineInterface(plugcvarfuncs_name,sizeof(*cvars));
 draw=(plug2dfuncs_t*)core->GetEngineInterface(plug2dfuncs_name,sizeof(*draw));input=(pluginputfuncs_t*)core->GetEngineInterface(pluginputfuncs_name,sizeof(*input));
 files=(plugfsfuncs_t*)core->GetEngineInterface(plugfsfuncs_name,sizeof(*files));
 if(!commands||!cvars||!draw||!input||!files)return qfalse;
 for(auto& b:bindings)b.key=input->GetKeyCode(b.initial,nullptr);
 core->ExportFunction("MenuEvent",(funcptr_t)event);core->ExportFunction("Shutdown",(funcptr_t)shutdownMenu);core->ExportFunction("UpdateVideo",(funcptr_t)videoChanged);
 commands->AddCommand("reyond_preview_test",testMenu,"Exercise the preview UI for integration tests.");
 commands->AddCommand("reyond_preview_menu",toggle,"Open the isolated mod preview controls.");
 commands->AddCommand("reyond_preview_config",configure,"Configure the isolated mod preview controls.");return qtrue;
}
