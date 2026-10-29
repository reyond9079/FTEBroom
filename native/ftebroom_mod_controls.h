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
static bool modLightingRTL=false,modLightingLoaded=false,modLightingRaw=false;
static float modLightingDistance=4096;
static std::string modLightingSource;
static ImVec2 modLightingToggleCenter;
static void modLightingLoad(){
  if(modLightingLoaded)return;modLightingLoaded=true;
  std::ifstream file(documentPath("mod-lighting.txt"));int enabled;float distance;
  if(file>>enabled>>distance){modLightingRTL=enabled!=0;if(std::isfinite(distance))modLightingDistance=std::clamp(distance,0.f,65536.f);}
}
static std::string modLightingCommand(){
  return "ftebroom_preview_lighting "+std::to_string(modLightingRTL?1:0)+" "+std::to_string(modLightingDistance)+" "+std::to_string(modLightingRaw?1:0)+" \""+modLightingSource+"\"";
}
static void modLightingApply(){
  modInputQueue+=modLightingCommand()+"\n";
  std::ofstream file(documentPath("mod-lighting.txt"));file<<(modLightingRTL?1:0)<<' '<<modLightingDistance<<'\n';
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
      if(ImGui::Checkbox("FTE-native RTL instead of lightmaps",&modLightingRTL))modLightingApply();
      {auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();modLightingToggleCenter={(a.x+b.x)*.5f,(a.y+b.y)*.5f};}
      ImGui::BeginDisabled(!modLightingRTL);
      if(ImGui::DragFloat("RTL distance (map units)",&modLightingDistance,64,0,65536,"%.0f",ImGuiSliderFlags_AlwaysClamp)){
        if(!std::isfinite(modLightingDistance))modLightingDistance=4096;
        modLightingApply();
      }
      ImGui::EndDisabled();
      ImGui::TextWrapped("0 = unlimited. Lights fade over the last 20% of the range outside their influence. Ordinary map lights are imported automatically. Realtime shadows can cost more GPU time.");
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
