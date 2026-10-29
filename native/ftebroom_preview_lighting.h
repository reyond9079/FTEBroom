// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
#include <istream>
#include <ostream>
#include <string>
#include "imgui.h"
struct PreviewLighting {
  bool enabled=false;
  float distance=4096, radius=1.3f, energy=.35f, ambient=.2f, baked=.7f;
  void soft(){radius=1.6f;energy=.8f;ambient=.35f;baked=0;}
  void clamp(){
    auto bound=[](float& x,float low,float high,float fallback){x=std::isfinite(x)?std::clamp(x,low,high):fallback;};
    bound(distance,0,65536,4096);bound(radius,.25f,4,1.3f);bound(energy,0,4,.35f);bound(ambient,0,1,.2f);bound(baked,0,1,.7f);
  }
  std::string command(bool raw,const std::string& source)const{
    return "ftebroom_preview_lighting "+std::to_string(enabled?1:0)+" "+std::to_string(distance)+" "+std::to_string(raw?1:0)+" \""+source+"\" "+std::to_string(radius)+" "+std::to_string(energy)+" "+std::to_string(ambient)+" "+std::to_string(baked);
  }
  void read(std::istream& in){int on;float d;if(in>>on>>d){enabled=on!=0;distance=d;in>>radius>>energy>>ambient>>baked;}clamp();}
  void write(std::ostream& out)const{out<<(enabled?1:0)<<' '<<distance<<' '<<radius<<' '<<energy<<' '<<ambient<<' '<<baked;}
};
static bool previewLightingWidgets(PreviewLighting& light,bool raw,ImVec2* toggle=nullptr){
  bool changed=ImGui::Checkbox("FTE-native RTL instead of lightmaps",&light.enabled);
  if(toggle){auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();*toggle={(a.x+b.x)*.5f,(a.y+b.y)*.5f};}
  ImGui::BeginDisabled(!light.enabled);
  ImGui::BeginDisabled(raw);
  if(ImGui::Button("Lightmap-like")){light.radius=1.3f;light.energy=.35f;light.ambient=.2f;light.baked=.7f;changed=true;}
  ImGui::EndDisabled();
  auto nextButton=[](const char* label){auto remaining=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x-ImGui::GetItemRectMax().x;
    if(remaining>ImGui::CalcTextSize(label).x+ImGui::GetStyle().FramePadding.x*2+ImGui::GetStyle().ItemSpacing.x)ImGui::SameLine();};
  nextButton("Soft RTL only");if(ImGui::Button("Soft RTL only")){light.soft();changed=true;}
  nextButton("Native RTL");if(ImGui::Button("Native RTL")){light.radius=1;light.energy=1;light.ambient=0;light.baked=0;changed=true;}
  ImGui::TextUnformatted("RTL distance (map units)");ImGui::SetNextItemWidth(-1);
  changed|=ImGui::DragFloat("##RTL distance",&light.distance,64,0,65536,"%.0f",ImGuiSliderFlags_AlwaysClamp);
  ImGui::TextUnformatted("Light reach");ImGui::SetNextItemWidth(-1);
  changed|=ImGui::SliderFloat("##Light reach",&light.radius,.25f,4,"%.2fx");
  ImGui::TextUnformatted("RTL brightness");ImGui::SetNextItemWidth(-1);
  changed|=ImGui::SliderFloat("##RTL brightness",&light.energy,0,2,"%.2f");
  ImGui::TextUnformatted("Directional fill");ImGui::SetNextItemWidth(-1);
  changed|=ImGui::SliderFloat("##Directional fill",&light.ambient,0,1,"%.2f");
  ImGui::BeginDisabled(raw);ImGui::TextUnformatted("Baked light contribution");ImGui::SetNextItemWidth(-1);
  changed|=ImGui::SliderFloat("##Baked light contribution",&light.baked,0,1,"%.2f");ImGui::EndDisabled();
  ImGui::EndDisabled();light.clamp();
  ImGui::TextWrapped("Lightmap-like combines realtime lights with baked fill. Soft RTL only needs no lightmaps; it approximates broad illumination, not bounced light. Directional fill still respects shadow occlusion.");
  if(raw)ImGui::TextWrapped("Current MAP has no baked lightmaps; baked contribution is ignored.");
  ImGui::TextWrapped("Distance 0 = unlimited. Lights fade over the last 20%% of the range outside their influence. Ordinary map lights are imported automatically.");
  return changed;
}
