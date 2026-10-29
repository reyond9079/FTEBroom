// SPDX-License-Identifier: GPL-2.0-or-later
struct ModControl {const char* label;const char* action;const char* initial;int key=0;};
static ModControl modControls[]={
  {"Forward","+forward","w"},{"Backward","+back","s"},{"Left","+moveleft","a"},{"Right","+moveright","d"},
  {"Jump / swim up","+jump","space"},{"Attack","+attack","mouse1"},{"Next weapon","impulse 10","mwheelup"},
  {"Previous weapon","impulse 12","mwheeldown"},{"Run / walk","+speed","shift"},{"Swim down","+movedown","c"}
};
static bool modMenuDrawn=false;
static std::vector<std::string> modAppliedKeys;
static float modSensitivity=3;static bool modInvertMouse=false,modControlsLoaded=false;
#include "ftebroom_preview_lighting.h"
static PreviewLighting modLighting;
static bool& modLightingRTL=modLighting.enabled;
static float& modLightingDistance=modLighting.distance;
static bool modLightingLoaded=false,modLightingRaw=false;
static bool modLightingSettingsRaw=false;
static std::string modLightingSource;
static ImVec2 modLightingToggleCenter;
static const char* modLightingFile(){return modLightingRaw?"mod-lighting-raw.txt":"mod-lighting.txt";}
static void modLightingLoad(){
  if(modLightingLoaded&&modLightingSettingsRaw==modLightingRaw)return;
  auto enabled=modLighting.enabled;auto distance=modLighting.distance;
  modLighting=PreviewLighting{};
  if(modLightingRaw){modLighting.soft();modLighting.enabled=enabled;modLighting.distance=distance;}
  modLightingLoaded=true;modLightingSettingsRaw=modLightingRaw;
  std::ifstream file(documentPath(modLightingFile()));modLighting.read(file);
}
static std::string modLightingCommand(){
  return modLighting.command(modLightingRaw,modLightingSource);
}
static void modLightingApply(){
  modInputQueue+=modLightingCommand()+"\n";
  std::ofstream file(documentPath(modLightingFile()));modLighting.write(file);file<<'\n';
}
static void modControlsLoad()
{
  if(modControlsLoaded)return;modControlsLoaded=true;
  for(auto& binding:modControls)binding.key=input->GetKeyCode(binding.initial,nullptr);
  std::ifstream in(documentPath("mod-controls.txt"));float sensitivity;int inverted;
  if(in>>sensitivity>>inverted){modSensitivity=std::clamp(sensitivity,.1f,20.f);modInvertMouse=inverted!=0;
    for(auto& binding:modControls){int key;if(in>>key && key>0 && key<1024)binding.key=key;}}
}
static std::string modControlName(int key)
{
  const char* name=input->GetKeyName(key,0);return name?name:"";
}
static void modControlsApply()
{
  modControlsLoad();
  modLightingLoad();
  for(const auto& key:modAppliedKeys)modInputQueue+="unbind \""+key+"\"\n";modAppliedKeys.clear();
  for(auto& binding:modControls){auto key=modControlName(binding.key);if(!key.empty() && key.find_first_of("\";\\\r\n")==std::string::npos)
    {modInputQueue+="bind \""+key+"\" \""+binding.action+"\"\n";modAppliedKeys.push_back(key);}}
  modInputQueue+="set sensitivity "+std::to_string(modSensitivity)+"\nset m_pitch "+std::string(modInvertMouse?"-0.022":"0.022")+"\n";
  modInputQueue+=modLightingCommand()+"\n";
}
static void modOpenControls(){modReleaseInput();modControlsLoad();modMenuOpen=true;modControlCapture=-1;}
static void modControlsPanel()
{
  modMenuDrawn=false;
  if(modEmbedded && modMenuOpen && !ImGui::IsPopupOpen("Mod controls"))ImGui::OpenPopup("Mod controls");
  if(!ImGui::IsPopupOpen("Mod controls"))return;
  auto screen=ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowSize({std::min(600*uiScale(),screen.x-40),std::min(720*uiScale(),screen.y-40)},ImGuiCond_Appearing);
  if(ImGui::BeginPopupModal("Mod controls",&modMenuOpen,ImGuiWindowFlags_NoSavedSettings)){
    if(!modEmbedded || !modMenuOpen){ImGui::CloseCurrentPopup();ImGui::EndPopup();return;}
    modMenuDrawn=true;
    ImGui::TextWrapped("Controls apply to this preview. Your mod files are unchanged.");
    ImGui::BeginChild("Preview settings",{0,-ImGui::GetFrameHeightWithSpacing()*3},ImGuiChildFlags_None);
    if(ImGui::CollapsingHeader("Lighting",ImGuiTreeNodeFlags_DefaultOpen)){
      if(previewLightingWidgets(modLighting,modLightingRaw,&modLightingToggleCenter))modLightingApply();
      ImGui::Separator();
    }
    ImGui::SliderFloat("Mouse sensitivity",&modSensitivity,.1f,20.f,"%.1f");ImGui::Checkbox("Invert mouse Y",&modInvertMouse);
    for(int i=0;i<(int)std::size(modControls);++i){ImGui::PushID(i);ImGui::TextUnformatted(modControls[i].label);ImGui::SameLine(240*uiScale());
      auto label=modControlCapture==i?std::string("Press a key..."):modControlName(modControls[i].key);
      if(ImGui::Button(label.c_str(),{160*uiScale(),0}))modControlCapture=i;ImGui::PopID();}
    ImGui::EndChild();ImGui::Separator();
    if(ImGui::Button("Apply")){
      modReleaseInput();modControlsApply();std::ofstream out(documentPath("mod-controls.txt"));out<<modSensitivity<<' '<<modInvertMouse;
      for(auto& binding:modControls)out<<' '<<binding.key;out<<'\n';modControlCapture=-1;
    }
    ImGui::SameLine();if(ImGui::Button("Continue")){modMenuOpen=false;modControlCapture=-1;ImGui::CloseCurrentPopup();}
    ImGui::SameLine();if(ImGui::Button("Exit mod preview")){modMenuOpen=false;modControlCapture=-1;modEmbedEnd();ImGui::CloseCurrentPopup();}
    ImGui::TextDisabled("The mod simulation continues while this menu is open.");
    ImGui::EndPopup();
  }
}

#ifndef _WIN32
static void modPreviewReadSettings(const std::string& line){
  std::istringstream in(line);PreviewLighting next;int enabled,invert;float sensitivity;int keys[10];
  if(!(in>>enabled>>next.distance>>next.radius>>next.energy>>next.ambient>>next.baked>>sensitivity>>invert))return;
  next.enabled=enabled!=0;next.clamp();if(!std::isfinite(sensitivity)||sensitivity<.1f||sensitivity>20)return;
  for(auto& key:keys)if(!(in>>key)||key<=0||key>=1024)return;
  modLighting=next;modLightingLoaded=true;modLightingSettingsRaw=modLightingRaw;modSensitivity=sensitivity;modInvertMouse=invert!=0;modControlsLoaded=true;
  std::ofstream lighting(documentPath(modLightingFile()));next.write(lighting);lighting<<'\n';
  std::ofstream controls(documentPath("mod-controls.txt"));controls<<sensitivity<<' '<<invert;
  for(int i=0;i<10;i++){modControls[i].key=keys[i];controls<<' '<<keys[i];}controls<<'\n';
}
#endif
