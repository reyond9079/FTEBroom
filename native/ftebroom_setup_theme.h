// SPDX-License-Identifier: GPL-2.0-or-later
// Small native previews share the live palette, including retro frame edges.
static ImVec2 setupThemeCenters[5];
static ImVec2 setupScaleCenters[5];
static void setupScaleList() {
  const char* names[]={"Small","Medium","Large","Extra Large","Largest"};
  ImGui::TextWrapped("Choose a comfortable reading size. Changes apply immediately and can be adjusted later in Preferences.");
  ImGui::Spacing();
  ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,ImVec2(0,.5f));
  for(int i=0;i<5;i++) {
    ImGui::PushID(i);
    if(ImGui::Selectable(names[i],uiScaleChoice==i,ImGuiSelectableFlags_NoAutoClosePopups,{0,52*uiScale()})) {
      uiScaleChoice=i;uiSaveAppearance();
    }
    auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    setupScaleCenters[i]={(a.x+b.x)*.5f,(a.y+b.y)*.5f};
    ImGui::PopID();
  }
  ImGui::PopStyleVar();
}
static void setupThemeTiles()
{
  const int order[]={0,3,1,2,4};
  const float gap=12*uiScale();
  const auto available=ImGui::GetContentRegionAvail();
  const ImVec2 tile{(available.x-gap)*.5f,std::max(100.f,(available.y-gap*2)/3)};
  for(int slot=0;slot<5;slot++) {
    const int theme=order[slot];
    if(slot%2)ImGui::SameLine(0,gap);
    ImGui::PushID(theme);
    auto pos=ImGui::GetCursorScreenPos();
    bool clicked=ImGui::InvisibleButton("Theme",tile);
    bool hovered=ImGui::IsItemHovered();
    setupThemeCenters[theme]={pos.x+tile.x*.5f,pos.y+tile.y*.5f};
    // Snapshot an actual theme without changing the surrounding window's style.
    auto savedStyle=ImGui::GetStyle();int savedTheme=uiTheme;
    uiTheme=theme;uiStyle();auto preview=ImGui::GetStyle();
    uiTheme=savedTheme;ImGui::GetStyle()=savedStyle;
    auto color=[&](ImGuiCol id){return ImGui::ColorConvertFloat4ToU32(preview.Colors[id]);};
    auto draw=ImGui::GetWindowDrawList();bool retro=theme==1||theme==2;
    float round=retro?0:6*uiScale(),pad=10*uiScale();
    draw->AddRectFilled(pos,{pos.x+tile.x,pos.y+tile.y},color(ImGuiCol_WindowBg),round);
    auto rect=[&](float x,float y,float w,float h,ImGuiCol col,float r=0.f){draw->AddRectFilled({pos.x+x,pos.y+y},{pos.x+x+w,pos.y+y+h},color(col),r);};
    float previewHeight=tile.y-ImGui::GetTextLineHeight()-pad*2;
    rect(pad,pad,tile.x-pad*2,previewHeight,ImGuiCol_ChildBg,retro?0:3);
    rect(pad,pad,tile.x-pad*2,16*uiScale(),ImGuiCol_TitleBgActive);
    float top=pad+22*uiScale(),side=tile.x*.23f;
    rect(pad,top,side,previewHeight-22*uiScale(),ImGuiCol_WindowBg);
    rect(tile.x-side-pad,top,side,previewHeight-22*uiScale(),ImGuiCol_WindowBg);
    for(int row=0;row<3;row++) {
      float y=top+6*uiScale()+row*18*uiScale();
      rect(pad*1.5f,y,side-pad,11*uiScale(),row==0?ImGuiCol_HeaderActive:ImGuiCol_Button,retro?0:2);
      rect(tile.x-side-pad*.5f,y,side-pad,11*uiScale(),ImGuiCol_FrameBg,retro?0:2);
      if(retro)draw->AddRect({pos.x+pad*1.5f,pos.y+y},{pos.x+side+pad*.5f,pos.y+y+11*uiScale()},color(ImGuiCol_BorderShadow));
    }
    // A tiny viewport with a grid and a selected brush gives the previews context.
    float left=pos.x+side+pad*2,right=pos.x+tile.x-side-pad*2;
    float bottom=pos.y+pad+previewHeight,mid=(left+right)*.5f;
    draw->PushClipRect({left,pos.y+top},{right,bottom},true);
    auto grid=ImGui::ColorConvertFloat4ToU32(preview.Colors[ImGuiCol_TextDisabled]);
    for(int line=0;line<6;line++) {
      float y=pos.y+top+(bottom-pos.y-top)*line/5;
      draw->AddLine({left,y},{right,y},grid);
      float x=left+(right-left)*line/5;draw->AddLine({mid,pos.y+top},{x,bottom},grid);
    }
    float box=std::min(30*uiScale(),(right-left)*.25f),cy=pos.y+top+(bottom-pos.y-top)*.5f;
    draw->AddRectFilled({mid-box,cy-box*.5f},{mid+box,cy+box*.7f},color(ImGuiCol_Button));
    draw->AddRect({mid-box,cy-box*.5f},{mid+box,cy+box*.7f},color(ImGuiCol_CheckMark),0,0,2);
    draw->PopClipRect();
    draw->AddText({pos.x+pad,pos.y+tile.y-ImGui::GetTextLineHeight()-pad},color(ImGuiCol_Text),uiThemeNames[theme]);
    bool selected=uiTheme==theme;
    draw->AddRect(pos,{pos.x+tile.x,pos.y+tile.y},selected?ImGui::GetColorU32(ImGuiCol_CheckMark):ImGui::GetColorU32(hovered?ImGuiCol_ButtonHovered:ImGuiCol_Border),round,0,selected?3.f:1.f);
    if(selected)draw->AddCircleFilled({pos.x+tile.x-pad*1.5f,pos.y+pad*1.5f},4*uiScale(),ImGui::GetColorU32(ImGuiCol_CheckMark));
    if(clicked){uiTheme=theme;uiSaveAppearance();}
    ImGui::PopID();
  }
}
