// SPDX-License-Identifier: GPL-2.0-or-later
static bool uiResolutionWindow=false;
static int uiResolutionWidth=1280,uiResolutionHeight=800;
static std::string uiResolutionError;
static void uiResolutionRead(){
  float logical[2];unsigned pixels[2];
  if(draw->GetVideoSize(logical,pixels)){uiResolutionWidth=int(pixels[0]);uiResolutionHeight=int(pixels[1]);}
}
static bool uiResolutionApply(){
  if(uiResolutionWidth<640||uiResolutionWidth>7680||uiResolutionHeight<480||uiResolutionHeight>4320){uiResolutionError="Choose a resolution between 640 x 480 and 7680 x 4320.";return false;}
  if(modEmbedded||cvars->GetFloat("ftebroom_mod_preview")||cvars->GetFloat("ftebroom_terrain_document")){uiResolutionError="Return to MAP editing to change resolution.";return false;}
  std::ofstream out(documentPath("display-resolution.cfg"),std::ios::binary);
  out<<"seta vid_width "<<uiResolutionWidth<<"\nseta vid_height "<<uiResolutionHeight<<"\nseta vid_desktopsettings 0\n";out.close();
  if(!out){uiResolutionError="Cannot save the display resolution.";return false;}
  cvars->SetFloat("vid_width",uiResolutionWidth);cvars->SetFloat("vid_height",uiResolutionHeight);cvars->SetFloat("vid_desktopsettings",0);
  uiResolutionError.clear();command("ftebroom_renderer_restart\n");return true;
}
static void uiResolutionControls(){
  ImGui::TextUnformatted("Resolution");
  float logical[2];unsigned pixels[2]={0,0};draw->GetVideoSize(logical,pixels);
  ImGui::TextDisabled("Current framebuffer: %u x %u",pixels[0],pixels[1]);
  const int sizes[][2]={{1024,768},{1280,720},{1280,800},{1600,900},{1920,1080},{2560,1440},{3840,2160}};
  char current[64];std::snprintf(current,sizeof current,"%d x %d",uiResolutionWidth,uiResolutionHeight);
  if(ImGui::BeginCombo("Preset",current)){
    for(auto& size:sizes){char name[64];std::snprintf(name,sizeof name,"%d x %d",size[0],size[1]);if(ImGui::Selectable(name,uiResolutionWidth==size[0]&&uiResolutionHeight==size[1])){uiResolutionWidth=size[0];uiResolutionHeight=size[1];}}
    ImGui::EndCombo();
  }
  ImGui::InputInt("Width",&uiResolutionWidth);ImGui::InputInt("Height",&uiResolutionHeight);
  if(ImGui::Button("Use current window size"))uiResolutionRead();
  if(ImGui::Button("Apply resolution (restart renderer)"))uiResolutionApply();
  if(!uiResolutionError.empty())ImGui::TextWrapped("%s",uiResolutionError.c_str());
}
static void uiResolutionDialog(){
  if(!uiResolutionWindow)return;
  ImGui::SetNextWindowSize({430*uiScale(),310*uiScale()},ImGuiCond_FirstUseEver);
  if(ImGui::Begin("Resolution",&uiResolutionWindow))uiResolutionControls();ImGui::End();
}
