// SPDX-License-Identifier: GPL-2.0-or-later
static void modelConverterShow(const std::string& source);
static bool modelEditorActive=false;
static void modelEditorInspector();
static void bspUVPanel();
static void modelEditorMenus();
static bool modelEditorOverlay(ImVec2 position,ImVec2 size);
static void modelEditorShow(const std::string& path);
static void modelEditorCloseCheck(bool open);
static void modelEditorCameraStep();
static void modelEditorCameraEnd(bool restore=true);
static bool modelEditorZoom(float wheel);
static bool modelViewerOpen=false,modelViewerPlay=false,modelViewerBones=false;
static bool modelViewerStudio=false;
static std::string modelViewerPath;
static int modelViewerSequence=0,modelViewerPose=0,modelViewerBone=1;
static float modelViewerYaw=135,modelViewerPitch=15,modelViewerZoom=1,modelViewerFPS=10;
static int modelViewerSkin=0,modelViewerSkinTextureIndex=-1;
static qhandle_t modelViewerSkinTexture=0,modelViewerImage=0;
static std::vector<std::string> modelViewerSkinFiles;
static std::string modelViewerCustomSkin;
static ImVec2 modelViewerSkinCombo,modelViewerSkinChoices[256];
static int QDECL modelViewerCollectSkin(const char* name,qofs_t,time_t,void*,searchpathfuncs_s*){
  if(modelViewerSkinFiles.size()<256)modelViewerSkinFiles.push_back(name);return 1;
}
static double modelViewerClock=0;
static bool modelFile(const std::filesystem::path& path) {
  auto ext=entityLower(path.extension().u8string());return ext==".mdl"||ext==".md3"||ext==".md2"||ext==".iqm"||ext==".md5mesh"||ext==".mdx"||ext==".zym"||ext==".dpm"||ext==".psk"||ext==".obj";
}
static bool modelViewableFile(const std::filesystem::path& path){auto ext=entityLower(path.extension().u8string());return modelFile(path)||ext==".bsp"||ext==".sp2"||ext==".spr";}
static bool modelViewerIsBsp(){return entityLower(std::filesystem::u8path(modelViewerPath).extension().u8string())==".bsp";}
static void modelViewerLoad(const std::string& path) {
  modelEditorActive=false;cvars->SetFloat("ftebroom_modeleditor_active",0);
  modelViewerPath=path;modelViewerOpen=true;modelViewerPlay=false;modelViewerSequence=modelViewerPose=0;modelViewerClock=0;modelViewerZoom=1;
  modelViewerSkin=0;modelViewerSkinTextureIndex=-1;modelViewerCustomSkin.clear();modelViewerSkinFiles.clear();
  if(modelViewerSkinTexture){draw->UnloadImage(modelViewerSkinTexture);modelViewerSkinTexture=0;}
  auto modelPath=std::filesystem::u8path(path);auto parent=modelPath.parent_path();auto stem=modelPath.stem().u8string();
  // FTE enumeration includes loose files and mounted PAK/PK3 resources.
  files->EnumerateFiles((fs_relative)4,(parent/(stem+"*.skin")).generic_u8string().c_str(),modelViewerCollectSkin,nullptr);
  std::sort(modelViewerSkinFiles.begin(),modelViewerSkinFiles.end());modelViewerSkinFiles.erase(std::unique(modelViewerSkinFiles.begin(),modelViewerSkinFiles.end()),modelViewerSkinFiles.end());
  int embedded=1;modelViewerStudio=false;try{auto bytes=modelResource(path);modelViewerStudio=bytes.substr(0,4)=="IDST";if(bytes.substr(0,4)=="IDPO")embedded=std::clamp(int(ftebroom::quake1::word(bytes,48)),1,256);else if(modelViewerStudio)embedded=std::clamp(int(ftebroom::quake1::word(bytes,196)),1,256);}catch(...){}
  cvars->SetFloat("ftebroom_modelviewer_embeddedskins",embedded);cvars->SetFloat("ftebroom_modelviewer_skins",embedded);cvars->SetFloat("ftebroom_modelviewer_skin",0);cvars->SetString("ftebroom_modelviewer_customskin","");
  cvars->SetString("ftebroom_modelviewer_path",path.c_str());cvars->SetFloat("ftebroom_modelviewer_count",0);cvars->SetFloat("ftebroom_modelviewer_poses",0);
}
static void modelViewerPanel() {
  if(modelEditorActive)modelEditorCameraStep();
  cvars->SetFloat("ftebroom_modelviewer_open",0);cvars->SetFloat("ftebroom_modeleditor_active",modelEditorActive&&modelViewerOpen);if(!modelViewerOpen){bspUVPanel();return;}
  ImGui::SetNextWindowSize(modelEditorActive?ImVec2(1100*uiScale(),760*uiScale()):ImVec2(760,650),ImGuiCond_FirstUseEver);
  bool keepOpen=modelViewerOpen;
  if(ImGui::Begin(modelEditorActive?"Model editor":"Model viewer",&keepOpen,modelEditorActive?ImGuiWindowFlags_MenuBar:0)) {
    cvars->SetFloat("ftebroom_modelviewer_open",1);
    if(modelEditorActive){modelEditorMenus();ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1);auto bg=ImGui::GetStyle().Colors[ImGuiCol_WindowBg];ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(std::min(1.f,bg.x+.08f),std::min(1.f,bg.y+.08f),std::min(1.f,bg.z+.08f),1));ImGui::BeginTable("Model editor layout",2,ImGuiTableFlags_Resizable|ImGuiTableFlags_BordersInnerV);ImGui::TableSetupColumn("Preview",ImGuiTableColumnFlags_WidthStretch);ImGui::TableSetupColumn("Inspector",ImGuiTableColumnFlags_WidthFixed,285*uiScale());ImGui::TableNextRow();ImGui::TableSetColumnIndex(1);ImGui::BeginChild("Model inspector",{0,0},ImGuiChildFlags_Borders);modelEditorInspector();ImGui::EndChild();ImGui::TableSetColumnIndex(0);ImGui::BeginChild("Model preview");}
    ImGui::TextWrapped("%s",modelViewerPath.c_str());
    if(!modelEditorActive&&ImGui::Button("Convert..."))modelConverterShow(modelViewerPath);
    int poses=1;
    if(!modelViewerIsBsp()){
    int skins=std::clamp(int(cvars->GetFloat("ftebroom_modelviewer_skins")),1,256);
    char skinName[256]={};cvars->GetString("ftebroom_modelviewer_skinname",skinName,sizeof skinName);
    auto selectedSkin=std::string("Skin ")+std::to_string(modelViewerSkin)+(*skinName?std::string(" — ")+skinName:"");
    ImGui::SetNextItemWidth(300);
    bool skinComboOpen=ImGui::BeginCombo("Skin",selectedSkin.c_str());
    {auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();modelViewerSkinCombo={(a.x+b.x)*.5f,(a.y+b.y)*.5f};}
    if(skinComboOpen){
      for(int i=0;i<skins;i++){
        char name[256]={};cvars->GetString(("ftebroom_modelviewer_skin_name_"+std::to_string(i)).c_str(),name,sizeof name);
        auto label=std::string("Skin ")+std::to_string(i)+(*name?std::string(" — ")+name:"");
        if(ImGui::Selectable(label.c_str(),modelViewerSkin==i)){modelViewerSkin=i;cvars->SetFloat("ftebroom_modelviewer_skin",i);}
        {auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();modelViewerSkinChoices[i]={(a.x+b.x)*.5f,(a.y+b.y)*.5f};}
        if(modelViewerSkin==i)ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    if(!modelViewerSkinFiles.empty()&&ImGui::BeginCombo("Material set (.skin)",modelViewerCustomSkin.empty()?"Embedded / default":modelViewerCustomSkin.c_str())){
      if(ImGui::Selectable("Embedded / default",modelViewerCustomSkin.empty())){modelViewerCustomSkin.clear();cvars->SetString("ftebroom_modelviewer_customskin","");}
      for(const auto& path:modelViewerSkinFiles)if(ImGui::Selectable(path.c_str(),modelViewerCustomSkin==path)){modelViewerCustomSkin=path;cvars->SetString("ftebroom_modelviewer_customskin",path.c_str());}
      ImGui::EndCombo();
    }
    if(ImGui::CollapsingHeader("Skin texture"))try{
      if(modelViewerSkinTextureIndex!=modelViewerSkin){
        modelViewerSkinTextureIndex=modelViewerSkin;
        if(modelViewerSkinTexture){draw->UnloadImage(modelViewerSkinTexture);modelViewerSkinTexture=0;}
        auto bytes=modelResource(modelViewerPath);
        if(bytes.substr(0,4)=="IDPO"){
          auto mdl=ftebroom::readMdl(bytes,modelViewerSkin);auto tga=ftebroom::quake1::tga(mdl.skin,modelResource("gfx/palette.lmp"));
          draw->LoadImageData("ftebroom/viewer-skin.tga","image/tga",tga.data(),tga.size());modelViewerSkinTexture=draw->LoadImage("ftebroom/viewer-skin.tga");
        }else if(bytes.substr(0,4)=="IDST"){
          auto companion=std::filesystem::u8path(modelViewerPath);companion.replace_filename(companion.stem().u8string()+"T.mdl");
          std::string extra;try{extra=modelResource(companion.generic_u8string());}catch(...){}
          auto parts=ftebroom::legacyModels::studio(bytes,extra,modelViewerSkin);
          if(!parts.empty()){auto tga=ftebroom::quake1::tga(parts[0].embedded,{});draw->LoadImageData("ftebroom/viewer-skin.tga","image/tga",tga.data(),tga.size());modelViewerSkinTexture=draw->LoadImage("ftebroom/viewer-skin.tga");}
        }else if(bytes.substr(0,4)=="IDP2"){
          auto parts=ftebroom::legacyModels::md2(bytes,modelViewerSkin);
          if(!parts.empty()){auto texture=ftebroom::legacy::pcx(modelResource(parts[0].image));auto tga=ftebroom::quake1::tga(texture,{});draw->LoadImageData("ftebroom/viewer-skin.tga","image/tga",tga.data(),tga.size());modelViewerSkinTexture=draw->LoadImage("ftebroom/viewer-skin.tga");}
        }
      }
      if(modelViewerSkinTexture){float w=0,h=0;draw->ImageSize(modelViewerSkinTexture,&w,&h);float scale=std::min(200.f/std::max(1.f,w),120.f/std::max(1.f,h));ImGui::Image((ImTextureID)modelViewerSkinTexture,{w*scale,h*scale});}
      else ImGui::TextDisabled("This format stores external materials; use the skin selector to preview them on the model.");
    }catch(const std::exception& e){ImGui::TextWrapped("%s",e.what());}
    if(cvars->GetFloat("ftebroom_modelviewer_count")<=0)ImGui::TextWrapped("Loading model, or model unavailable. Check its file and referenced resources.");
    int sequences=std::max(1,(int)cvars->GetFloat("ftebroom_modelviewer_count"));
    poses=std::max(1,(int)cvars->GetFloat("ftebroom_modelviewer_poses"));
    float duration=cvars->GetFloat("ftebroom_modelviewer_duration");
    char name[256]={};cvars->GetString("ftebroom_modelviewer_name",name,sizeof name);
    if(ImGui::SliderInt(poses>1?"Animation":"Frame",&modelViewerSequence,0,sequences-1)){modelViewerClock=0;modelViewerPose=0;modelViewerPlay=false;}
    ImGui::Text("%s | %i sequence(s), %i pose(s)",name,sequences,poses);
    if(poses>1 && ImGui::SliderInt("Animation frame",&modelViewerPose,0,poses-1)){modelViewerPlay=false;modelViewerClock=duration>0?modelViewerPose*duration/poses:0;}
    if(ImGui::Button("##model-play-pause",{34*uiScale(),ImGui::GetFrameHeight()}))modelViewerPlay=!modelViewerPlay;
    {auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();ImVec2 c{(a.x+b.x)*.5f,(a.y+b.y)*.5f};float r=(b.y-a.y)*.25f;auto dl=ImGui::GetWindowDrawList();auto colour=ImGui::GetColorU32(ImGuiCol_Text);
      if(modelViewerPlay){dl->AddRectFilled({c.x-r,c.y-r},{c.x-r*.25f,c.y+r},colour);dl->AddRectFilled({c.x+r*.25f,c.y-r},{c.x+r,c.y+r},colour);}
      else dl->AddTriangleFilled({c.x-r*.6f,c.y-r},{c.x-r*.6f,c.y+r},{c.x+r,c.y},colour);
      if(ImGui::IsItemHovered())ImGui::SetTooltip(modelViewerPlay?"Pause":"Play");}
    ImGui::SameLine();ImGui::SetNextItemWidth(140);ImGui::SliderFloat("Frames / second",&modelViewerFPS,1,60,"%.1f");
    if(modelViewerPlay){modelViewerClock+=ImGui::GetIO().DeltaTime*(poses>1?(duration>0?modelViewerFPS*duration/poses:1):1);if(poses>1&&duration>0){modelViewerClock=std::fmod(modelViewerClock,duration);modelViewerPose=std::min(poses-1,int(modelViewerClock*poses/duration));}else modelViewerSequence=int(modelViewerClock*modelViewerFPS)%sequences;}
    ImGui::Text("Frame %i / %i",poses>1?modelViewerPose:modelViewerSequence,poses>1?poses-1:sequences-1);
    int bones=(int)cvars->GetFloat("ftebroom_modelviewer_bones");
    ImGui::BeginDisabled(!bones);ImGui::Checkbox("Show bones",&modelViewerBones);
    if(bones){ImGui::SameLine();ImGui::SetNextItemWidth(180);ImGui::SliderInt("Bone",&modelViewerBone,1,bones);cvars->GetString("ftebroom_modelviewer_bonename",name,sizeof name);ImGui::Text("%s",name);}ImGui::EndDisabled();
    if(!bones)ImGui::TextDisabled("No skeletal bones (vertex-animated/static model).");
    }else ImGui::TextDisabled("BSP preview — static geometry");
    ImGui::TextDisabled("LMB: orbit | RMB + WASD / QX: fly | MMB: pan | Wheel: zoom");
    auto size=ImGui::GetContentRegionAvail();size.x=std::max(64.f,size.x);size.y=std::max(64.f,size.y);
    auto& image=modelViewerImage;if(!image)image=draw->LoadImageShader("ftebroom/modelviewer-image","{\n program default2d\n {\n map $rt:ftebroom_modelviewer\n rgbgen vertex\n alphagen vertex\n }\n }");
    const auto imagePos=ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddImage((ImTextureID)image,imagePos,{imagePos.x+size.x,imagePos.y+size.y});
    ImGui::InvisibleButton("model-orbit",size,ImGuiButtonFlags_MouseButtonLeft);
    bool orbitActive=ImGui::IsItemActive(),orbitHovered=ImGui::IsItemHovered();
    bool capture=modelEditorActive&&modelEditorOverlay(imagePos,size);
    if(!capture&&orbitActive&&ImGui::IsMouseDragging(0,0)){modelViewerYaw-=ImGui::GetIO().MouseDelta.x*.4f;modelViewerPitch=std::clamp(modelViewerPitch-ImGui::GetIO().MouseDelta.y*.4f,-89.f,89.f);}
    if(!capture&&orbitHovered&&!modelEditorZoom(ImGui::GetIO().MouseWheel))modelViewerZoom=std::clamp(modelViewerZoom*std::pow(1.12f,-ImGui::GetIO().MouseWheel),.1f,20.f);
    cvars->SetFloat("ftebroom_modelviewer_width",size.x);cvars->SetFloat("ftebroom_modelviewer_height",size.y);
    cvars->SetFloat("ftebroom_modelviewer_sequence",modelViewerSequence);cvars->SetFloat("ftebroom_modelviewer_time",poses>1?modelViewerClock:0);
    cvars->SetFloat("ftebroom_modelviewer_yaw",modelViewerYaw);cvars->SetFloat("ftebroom_modelviewer_pitch",modelViewerPitch);cvars->SetFloat("ftebroom_modelviewer_zoom",modelViewerZoom);
    cvars->SetFloat("ftebroom_modelviewer_showbones",modelViewerBones);cvars->SetFloat("ftebroom_modelviewer_bone",modelViewerBone);
    if(modelEditorActive){ImGui::EndChild();ImGui::EndTable();ImGui::PopStyleColor();ImGui::PopStyleVar();}
  }ImGui::End();
  if(modelEditorActive)modelEditorCloseCheck(keepOpen);else modelViewerOpen=keepOpen;
  bspUVPanel();
}
