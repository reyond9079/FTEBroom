// SPDX-License-Identifier: GPL-2.0-or-later
#include "ftebroom_model_edit_math.h"
#include "ftebroom_model_mdl_transform.h"
using ModelEditV=ftebroom::modelEdit::V;
static std::vector<ftebroom::modelEdit::Node> modelEditNodes;
static bool modelEditLoaded=false,modelEditChanged=true,modelEditPlayer=false,modelEditDirty=false;
static bool modelEditOriginAxes=true;
static bool modelEditCull=false;
static ImVec2 modelEditViewMin,modelEditViewSize;
static ModelEditV modelEditRead(const std::string& key);
static void modelEditSet(const std::string& key,ModelEditV v);
static bool modelEditFloor=true,modelEditCameraFree=false,modelEditCameraCaptured=false;
static int modelEditCameraButton=0;
static bool modelEditCameraKeys[6]={},modelEditCameraShift=false,modelEditCameraAlt=false;
static ModelEditV modelEditCameraEye{};
static ImVec2 modelEditCameraCursor;
static bool modelEditCameraRestore=false;
static void modelEditorCameraEnd(bool restore){
  bool captured=modelEditCameraCaptured;modelEditCameraCaptured=false;modelEditCameraButton=0;
  std::fill(std::begin(modelEditCameraKeys),std::end(modelEditCameraKeys),false);modelEditCameraShift=modelEditCameraAlt=false;
  if(captured&&!restore)modelEditCameraRestore=true;
  if(captured&&restore){input->SetMenuFocus(qtrue,"",0,0,1);ImGui::GetIO().AddMousePosEvent(modelEditCameraCursor.x,modelEditCameraCursor.y);}
}
static bool modelEditorZoom(float wheel){
  if(!modelEditorActive||!modelEditCameraFree)return false;
  float yaw=modelViewerYaw*.01745329252f,pitch=modelViewerPitch*.01745329252f,amount=wheel*20*std::clamp(cvars->GetFloat("ftebroom_move_speed"),.01f,20.f);
  if(cvars->GetFloat("ftebroom_wheel_invert"))amount=-amount;
  ModelEditV forward{std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),-std::sin(pitch)};
  for(int a=0;a<3;a++)modelEditCameraEye[a]=std::clamp(modelEditCameraEye[a]+forward[a]*amount,-1e7f,1e7f);return true;
}
static void modelEditCameraBeginFree(){if(!modelEditCameraFree){modelEditCameraEye=modelEditRead("ftebroom_modeleditor_eye");modelEditCameraFree=true;}}
static bool modelEditorInput(int type,int key,float x,float y){
  if(modelEditCameraRestore&&input->HasMenuFocus()){modelEditCameraRestore=false;input->SetMenuFocus(qtrue,"",0,0,1);ImGui::GetIO().AddMousePosEvent(modelEditCameraCursor.x,modelEditCameraCursor.y);}
  if(!modelEditorActive||!modelViewerOpen)return false;
  if(type==3&&modelEditCameraCaptured){
    float yaw=modelViewerYaw*.01745329252f,pitch=modelViewerPitch*.01745329252f;
    if(modelEditCameraButton==2){float speed=std::clamp(cvars->GetFloat("ftebroom_pan_speed"),.01f,20.f);ModelEditV right{std::sin(yaw),-std::cos(yaw),0},up{std::sin(pitch)*std::cos(yaw),std::sin(pitch)*std::sin(yaw),std::cos(pitch)};for(int a=0;a<3;a++)modelEditCameraEye[a]+=(-x*right[a]+y*up[a])*speed;}
    else{float speed=std::clamp(cvars->GetFloat("ftebroom_look_speed"),.01f,5.f);if(cvars->GetFloat("ftebroom_invert_horizontal"))x=-x;if(cvars->GetFloat("ftebroom_invert_vertical"))y=-y;modelViewerYaw-=x*speed;modelViewerPitch=std::clamp(modelViewerPitch+y*speed,-89.f,89.f);}return true;
  }
  if(type!=1&&type!=2)return modelEditCameraCaptured;
  auto code=[&](const char* name){return input->GetKeyCode(name,nullptr);};bool down=type==1;
  bool inside=x>=modelEditViewMin.x&&y>=modelEditViewMin.y&&x<modelEditViewMin.x+modelEditViewSize.x&&y<modelEditViewMin.y+modelEditViewSize.y;
  ImGuiWindow *hovered=nullptr,*underMoving=nullptr;ImGui::FindHoveredWindowEx({x,y},true,&hovered,&underMoving);auto editorWindow=ImGui::FindWindowByName("Model editor");
  inside=inside&&hovered&&editorWindow&&hovered->RootWindow==editorWindow->RootWindow;
  if(key==code("shift"))modelEditCameraShift=down;if(key==code("alt"))modelEditCameraAlt=down;
  const char* names[]={"w","s","a","d","q","x"};
  if(!modelEditCameraCaptured&&!modelEditPlayer)for(int i=0;i<6;i++)if(key==code(names[i])&&((!down&&modelEditCameraKeys[i])||(down&&inside&&!ImGui::GetIO().WantTextInput&&!ImGui::GetIO().KeyCtrl&&!ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId)))){
    if(down)modelEditCameraBeginFree();modelEditCameraKeys[i]=down;return true;
  }
  if(down&&!modelEditCameraCaptured&&!modelEditPlayer&&inside&&!ImGui::GetIO().WantTextInput&&!ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId)&&(key==code("mouse2")||key==code("mouse3"))){
    modelEditCameraBeginFree();modelEditCameraCaptured=true;modelEditCameraButton=key==code("mouse3")?2:1;modelEditCameraCursor={x,y};input->SetMenuFocus(qtrue,nullptr,0,0,1);return true;
  }
  if(!modelEditCameraCaptured)return false;
  if((!down&&(key==code("mouse2")||key==code("mouse3")))||(down&&key==code("escape"))){modelEditorCameraEnd();return true;}
  for(int i=0;i<6;i++)if(key==code(names[i]))modelEditCameraKeys[i]=down;
  return true;
}
static void modelEditorCameraStep(){
  static auto previous=std::chrono::steady_clock::now();auto now=std::chrono::steady_clock::now();float dt=std::clamp(std::chrono::duration<float>(now-previous).count(),0.f,.05f);previous=now;
  if(!modelViewerOpen||modelEditPlayer){modelEditorCameraEnd();return;}
  bool moving=std::any_of(std::begin(modelEditCameraKeys),std::end(modelEditCameraKeys),[](bool held){return held;});
  if((modelEditCameraCaptured||moving)&&!input->HasMenuFocus()){modelEditorCameraEnd(false);return;}
  if(ImGui::GetIO().WantTextInput&&!modelEditCameraCaptured){std::fill(std::begin(modelEditCameraKeys),std::end(modelEditCameraKeys),false);moving=false;}
  if(modelEditCameraCaptured||moving){float yaw=modelViewerYaw*.01745329252f,pitch=modelViewerPitch*.01745329252f;
    ModelEditV forward{std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),-std::sin(pitch)},right{std::sin(yaw),-std::cos(yaw),0},motion{};
    float length=0;for(int a=0;a<3;a++){motion[a]=forward[a]*(int(modelEditCameraKeys[0])-int(modelEditCameraKeys[1]))+right[a]*(int(modelEditCameraKeys[3])-int(modelEditCameraKeys[2]));if(a==2)motion[a]+=int(modelEditCameraKeys[4])-int(modelEditCameraKeys[5]);length+=motion[a]*motion[a];}
    float speed=std::clamp(cvars->GetFloat("ftebroom_fly_speed"),100.f,10000.f)*dt*(modelEditCameraShift?2.f:1.f)*(modelEditCameraAlt?.5f:1.f)/std::max(1.f,std::sqrt(length));
    for(int a=0;a<3;a++)modelEditCameraEye[a]=std::clamp(modelEditCameraEye[a]+motion[a]*speed,-1e7f,1e7f);
  }
  cvars->SetFloat("ftebroom_modeleditor_freecamera",modelEditCameraFree);if(modelEditCameraFree)modelEditSet("ftebroom_modeleditor_camera",modelEditCameraEye);
}

static int modelEditSelected=0,modelEditTool=0,modelEditDisplay=0,modelEditDrag=-1;
static ImVec2 modelEditMouseStart;
static ModelEditV modelEditPositionStart{},modelEditRotationStart{};
static ModelEditV modelEditScaleStart{};
static ModelEditV modelEditAnchorStart{};
static ImVec2 modelEditGizmoStart[3],modelEditGizmoEnd[3];
static std::string modelEditMessage;
static ModelEditV modelEditRead(const std::string& key){char text[256]={};cvars->GetString(key.c_str(),text,sizeof text);for(char& c:text)if(c=='\''||c=='"')c=' ';ModelEditV r{};std::istringstream(text)>>r[0]>>r[1]>>r[2];return r;}
static void modelEditSet(const std::string& key,ModelEditV v){std::ostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(9)<<v[0]<<' '<<v[1]<<' '<<v[2];cvars->SetString(key.c_str(),s.str().c_str());}
static auto modelEditWorld(int i){return ftebroom::modelEdit::world(modelEditNodes,i);}
static bool modelEditValid(){try{for(size_t i=0;i<modelEditNodes.size();i++){auto m=modelEditWorld(i);for(auto& row:m.m)for(float v:row)if(!std::isfinite(v)||std::abs(v)>1e7f)throw std::runtime_error("Transforms are too large. Reduce the parent/child scale or reset the hierarchy.");ftebroom::modelEdit::inverse(m);}return true;}catch(const std::exception& e){modelEditMessage=e.what();return false;}}
static void modelEditBounds(){
  if(modelEditNodes.size()<2)return;
  auto inverseRoot=ftebroom::modelEdit::inverse(modelEditWorld(0));ModelEditV low{1e30f,1e30f,1e30f},high{-1e30f,-1e30f,-1e30f};
  for(size_t i=1;i<modelEditNodes.size();i++){auto m=ftebroom::modelEdit::multiply(inverseRoot,modelEditWorld(i));for(int corner=0;corner<8;corner++){ModelEditV p{};for(int a=0;a<3;a++)p[a]=corner&(1<<a)?modelEditNodes[i].high[a]:modelEditNodes[i].low[a];p=m.point(p);for(int a=0;a<3;a++){low[a]=std::min(low[a],p[a]);high[a]=std::max(high[a],p[a]);}}}
  modelEditNodes[0].low=low;modelEditNodes[0].high=high;
}
static ModelEditV modelEditPivot(){return modelEditWorld(modelEditSelected).point(modelEditNodes[modelEditSelected].pivot);}
static void modelEditMoveTo(ModelEditV target){auto& n=modelEditNodes[modelEditSelected];auto p=ftebroom::modelEdit::inverse(modelEditWorld(n.parent)).point(target);for(int a=0;a<3;a++)n.position[a]=p[a]-n.pivot[a];modelEditChanged=modelEditDirty=true;}
static void modelEditorShow(const std::string& path){
  modelEditorCameraEnd();modelViewerLoad(path);modelEditorActive=true;modelEditLoaded=false;modelEditNodes.clear();modelEditChanged=true;modelEditDirty=false;modelEditSelected=0;modelEditDrag=-1;modelEditMessage.clear();
  auto name=entityLower(std::filesystem::u8path(path).filename().u8string());modelEditPlayer=name.rfind("v_",0)==0;
  modelEditCameraFree=false;cvars->SetFloat("ftebroom_modeleditor_freecamera",0);modelEditDisplay=0;modelEditTool=0;cvars->SetFloat("ftebroom_modeleditor_meshes",0);cvars->SetFloat("ftebroom_modeleditor_active",1);cvars->SetFloat("ftebroom_modeleditor_count",0);
}
static void modelEditLoad(){
  int count=std::clamp(int(cvars->GetFloat("ftebroom_modeleditor_meshes")),0,256);if(modelEditLoaded||!count)return;
  modelEditNodes.resize(count+1);auto& root=modelEditNodes[0];root.name="Model (root)";root.low=modelEditRead("ftebroom_modeleditor_low");root.high=modelEditRead("ftebroom_modeleditor_high");
  for(int i=0;i<count;i++){auto& n=modelEditNodes[i+1];char name[256]={};std::string prefix="ftebroom_modeleditor_mesh_"+std::to_string(i);cvars->GetString((prefix+"_name").c_str(),name,sizeof name);n.name=*name?name:"Mesh "+std::to_string(i);n.parent=0;n.low=modelEditRead(prefix+"_low");n.high=modelEditRead(prefix+"_high");for(int a=0;a<3;a++)n.pivot[a]=(n.low[a]+n.high[a])*.5f;}
  modelEditLoaded=true;modelEditChanged=true;
}
static void modelEditPublish(){
  cvars->SetFloat("ftebroom_modeleditor_floor",modelEditFloor);cvars->SetFloat("ftebroom_modeleditor_axes",modelEditOriginAxes);cvars->SetFloat("ftebroom_modeleditor_player",modelEditPlayer);cvars->SetFloat("ftebroom_modeleditor_display",modelEditDisplay);cvars->SetFloat("ftebroom_modeleditor_cull",modelEditCull);cvars->SetFloat("ftebroom_modeleditor_selected",modelEditSelected);
  if(!modelEditChanged||!modelEditLoaded)return;
  if(!modelEditValid()){cvars->SetFloat("ftebroom_modeleditor_count",0);return;}
  modelEditBounds();
  for(size_t i=0;i<modelEditNodes.size();i++){auto m=modelEditWorld(i);auto prefix="ftebroom_modeleditor_transform_"+std::to_string(i);for(int a=0;a<3;a++)modelEditSet(prefix+"_"+std::to_string(a),{m.m[0][a],m.m[1][a],m.m[2][a]});modelEditSet(prefix+"_origin",{m.m[0][3],m.m[1][3],m.m[2][3]});}
  cvars->SetFloat("ftebroom_modeleditor_count",modelEditNodes.size()-1);modelEditChanged=false;
}
static void modelEditExport(){
  if(!modelEditLoaded){modelEditMessage="Wait for model metadata before exporting.";return;}
  if(modelConverterPending){modelEditMessage="Wait for the current export to finish.";return;}
  if(!modelEditValid())return;
  try {
  std::map<std::string,std::string> overrides;
  if(!modelViewerCustomSkin.empty()){
    std::istringstream lines(modelResource(modelViewerCustomSkin));std::string line;
    auto trim=[](std::string value){auto a=value.find_first_not_of(" \t\r\"");if(a==std::string::npos)return std::string{};auto b=value.find_last_not_of(" \t\r\"");return value.substr(a,b-a+1);};
    while(std::getline(lines,line)){line=trim(line);if(line.empty()||line.rfind("//",0)==0)continue;std::string name,shader;
      auto comma=line.find(',');if(comma!=std::string::npos){name=trim(line.substr(0,comma));shader=trim(line.substr(comma+1));}
      else {std::istringstream input(line);std::string action;input>>action>>std::quoted(name)>>std::quoted(shader);if(action!="replace"){modelEditMessage="This .skin uses composition/colour directives that cannot be exported as diffuse textures. Select an embedded skin or a simple surface mapping.";return;}}
      if(shader.empty()&&name.rfind("tag_",0)==0)continue;
      if(name.empty()||shader.empty()){modelEditMessage="Invalid surface mapping in .skin file.";return;}overrides[name]=shader;
    }
  }
  std::vector<ftebroom::modelEdit::Matrix> transforms;std::vector<std::string> names;for(size_t i=1;i<modelEditNodes.size();i++){transforms.push_back(modelEditWorld(i));names.push_back(modelEditNodes[i].name);}
  modelConverterShow(modelViewerPath);modelConverterEditor=true;modelConverterSourceSkin=modelViewerSkin;
  modelConverterSequence=modelViewerSequence;modelConverterFrames=std::max(1,int(cvars->GetFloat("ftebroom_modelviewer_poses")));modelConverterFPS=modelViewerFPS;
  if(modelConverterFrames==1){modelConverterSequence=-1;modelConverterFrames=std::max(1,int(cvars->GetFloat("ftebroom_modelviewer_count")));}
  else {float duration=cvars->GetFloat("ftebroom_modelviewer_duration");if(duration>0)modelConverterFPS=std::clamp(modelConverterFrames/duration,1.f,120.f);}
  modelConverterTransform=[transforms,names,overrides](ftebroom::modelExport::Mesh& mesh){ftebroom::modelEdit::bake(mesh,transforms);for(size_t i=0;i<mesh.surfaces.size();i++){auto found=overrides.find(names[i]);if(found==overrides.end())found=overrides.find("*");if(found!=overrides.end())mesh.surfaces[i].material=found->second;}};
  if(overrides.empty()&&transforms.size()==1&&modelResource(modelViewerPath).substr(0,4)=="IDPO")modelConverterMdlTransform=[m=transforms[0]](const std::string& source){return ftebroom::modelEdit::transformMdl(source,m);};
  }catch(const std::exception& e){modelEditMessage=e.what();modelConverterOpen=false;}
}
static void modelEditorCloseCheck(bool open){
  static bool pending=false;if(!open){if(modelEditDirty)pending=true;else {modelViewerOpen=false;modelEditorCameraEnd();}}
  if(pending)ImGui::OpenPopup("Close model editor?");
  if(ImGui::BeginPopupModal("Close model editor?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
    ImGui::TextWrapped("Editor transforms are temporary. Export the model before closing if you want to keep them.");
    if(ImGui::Button("Discard and close")){modelViewerOpen=false;modelEditorCameraEnd();modelEditDirty=false;pending=false;ImGui::CloseCurrentPopup();}ImGui::SameLine();
    if(ImGui::Button("Keep editing")){pending=false;ImGui::CloseCurrentPopup();}ImGui::EndPopup();
  }
}
static void modelEditorMenus(){
  if(ImGui::BeginMenuBar()){
    if(ImGui::BeginMenu("File")){if(ImGui::MenuItem("Export model as..."))modelEditExport();ImGui::EndMenu();}
    if(ImGui::BeginMenu("View")){
      ImGui::MenuItem("View from player perspective",nullptr,&modelEditPlayer);ImGui::MenuItem("View bones",nullptr,&modelViewerBones);
      ImGui::MenuItem("Floor grid",nullptr,&modelEditFloor);
      ImGui::MenuItem("Origin axes",nullptr,&modelEditOriginAxes);
      if(ImGui::MenuItem("Frame model / reset camera")){modelEditorCameraEnd();modelEditCameraFree=false;modelViewerYaw=135;modelViewerPitch=15;modelViewerZoom=1;cvars->SetFloat("ftebroom_modeleditor_freecamera",0);}
      ImGui::MenuItem("Backface culling",nullptr,&modelEditCull);
      for(int i=0;i<3;i++)if(ImGui::MenuItem(i==0?"Textured":i==1?"Untextured":"Wireframe",nullptr,modelEditDisplay==i))modelEditDisplay=i;
      ImGui::EndMenu();
    }ImGui::EndMenuBar();
  }
}
static void modelEditTree(int index,int depth=0){
  if(depth>256)return;auto& n=modelEditNodes[index];bool children=false;for(auto& child:modelEditNodes)if(child.parent==index)children=true;
  auto flags=ImGuiTreeNodeFlags_OpenOnArrow|ImGuiTreeNodeFlags_DefaultOpen|(modelEditSelected==index?ImGuiTreeNodeFlags_Selected:0)|(children?0:ImGuiTreeNodeFlags_Leaf);
  ImGui::PushID(index);bool open=ImGui::TreeNodeEx(n.name.c_str(),flags);if(ImGui::IsItemClicked())modelEditSelected=index;
  if(open){for(size_t i=0;i<modelEditNodes.size();i++)if(modelEditNodes[i].parent==index)modelEditTree(i,depth+1);ImGui::TreePop();}ImGui::PopID();
}
static void modelEditorInspector(){
  modelEditLoad();
  ImGui::TextUnformatted("Model editor");ImGui::TextDisabled("Source file is not modified. Export to save.");
  if(modelEditNodes.empty()){ImGui::TextWrapped("Loading editable mesh metadata...");return;}
  if(ImGui::RadioButton("Move",modelEditTool==0))modelEditTool=0;ImGui::SameLine();if(ImGui::RadioButton("Rotate",modelEditTool==1))modelEditTool=1;ImGui::SameLine();if(ImGui::RadioButton("Scale",modelEditTool==2))modelEditTool=2;
  ImGui::SeparatorText("Meshes");ImGui::BeginChild("Mesh hierarchy",{0,150*uiScale()},ImGuiChildFlags_Borders);modelEditTree(0);ImGui::EndChild();
  if(!modelEditValid()){ImGui::TextWrapped("%s",modelEditMessage.c_str());if(ImGui::Button("Reset all transforms")){for(auto& node:modelEditNodes){node.position={};node.rotation={};node.scale={1,1,1};node.total=1;}modelEditChanged=modelEditDirty=true;}return;}
  modelEditBounds();
  auto& n=modelEditNodes[modelEditSelected];ImGui::TextWrapped("%s",n.name.c_str());
  if(modelEditSelected>0&&ImGui::BeginCombo("Parent",modelEditNodes[n.parent].name.c_str())){
    for(size_t p=0;p<modelEditNodes.size();p++)if(!ftebroom::modelEdit::descendant(modelEditNodes,p,modelEditSelected)&&ImGui::Selectable(modelEditNodes[p].name.c_str(),n.parent==int(p))){n.parent=p;modelEditChanged=modelEditDirty=true;}
    ImGui::EndCombo();
  }
  ImGui::TextUnformatted("Position XYZ (world)");ImGui::SetNextItemWidth(-1);auto position=modelEditPivot();if(ImGui::InputFloat3("##position",position.data(),"%.3f")){bool valid=true;for(float v:position)valid&=std::isfinite(v)&&std::abs(v)<=1e6;if(valid)modelEditMoveTo(position);}
  ImGui::TextUnformatted("Rotation XYZ (local)");ImGui::SetNextItemWidth(-1);if(ImGui::InputFloat3("##rotation",n.rotation.data(),"%.2f")){for(float& v:n.rotation)if(!std::isfinite(v))v=0;modelEditChanged=modelEditDirty=true;}
  ModelEditV size{};for(int a=0;a<3;a++)size[a]=(n.high[a]-n.low[a])*n.scale[a]*n.total;
  ImGui::TextUnformatted("Size XYZ (local axes)");ImGui::SetNextItemWidth(-1);if(ImGui::InputFloat3("##size",size.data(),"%.3f")){for(int a=0;a<3;a++){float base=(n.high[a]-n.low[a])*n.total;if(base>1e-6&&std::isfinite(size[a]))n.scale[a]=std::clamp(size[a]/base,.001f,1000.f);}modelEditChanged=modelEditDirty=true;}
  ImGui::TextUnformatted("Scale XYZ");ImGui::SetNextItemWidth(-1);bool edited=ImGui::InputFloat3("##scale",n.scale.data(),"%.4f");
  ImGui::TextUnformatted("Total scale");ImGui::SetNextItemWidth(-1);edited|=ImGui::InputFloat("##total-scale",&n.total,0,0,"%.4f");
  if(edited){for(float& v:n.scale)v=std::isfinite(v)?std::clamp(v,.001f,1000.f):1;n.total=std::isfinite(n.total)?std::clamp(n.total,.001f,1000.f):1;modelEditChanged=modelEditDirty=true;}
  if(ImGui::Button("Reset selected transform")){n.position={};n.rotation={};n.scale={1,1,1};n.total=1;modelEditChanged=modelEditDirty=true;}
  ImGui::TextWrapped("Mesh pivots start at their bounds center. Root pivot is the model origin. Parent changes keep local transforms.");
  if(!modelEditMessage.empty())ImGui::TextWrapped("%s",modelEditMessage.c_str());
  modelEditPublish();
}
static bool modelEditorOverlay(ImVec2 p,ImVec2 size){
  modelEditViewMin=p;modelEditViewSize=size;
  modelEditPublish();if(!modelEditLoaded||!modelEditValid())return modelEditPlayer;
  auto eye=modelEditRead("ftebroom_modeleditor_eye"),angles=modelEditRead("ftebroom_modeleditor_angles");float yaw=angles[1]*.01745329252f,pitch=angles[0]*.01745329252f;
  ModelEditV forward{std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),-std::sin(pitch)},right{std::sin(yaw),-std::cos(yaw),0},up{std::sin(pitch)*std::cos(yaw),std::sin(pitch)*std::sin(yaw),std::cos(pitch)};
  float f=modelEditPlayer?size.x*.5f:size.y/(2*std::tan(45*.00872664626f));
  auto dot=[](ModelEditV a,ModelEditV b){float v=0;for(int i=0;i<3;i++)v+=a[i]*b[i];return v;};
  auto project=[&](ModelEditV v,ImVec2& out){for(int a=0;a<3;a++)v[a]-=eye[a];float z=dot(v,forward);if(z<=.01f)return false;out={p.x+size.x*.5f+f*dot(v,right)/z,p.y+size.y*.5f-f*dot(v,up)/z};return true;};
  auto dl=ImGui::GetWindowDrawList();dl->PushClipRect(p,{p.x+size.x,p.y+size.y},true);
  ImVec2 zero;if(project({0,0,0},zero)){dl->AddCircle(zero,5,IM_COL32(255,220,70,255));dl->AddLine({zero.x-9,zero.y},{zero.x+9,zero.y},IM_COL32(255,220,70,255));dl->AddLine({zero.x,zero.y-9},{zero.x,zero.y+9},IM_COL32(255,220,70,255));dl->AddText({zero.x+8,zero.y+7},IM_COL32(255,220,70,255),"0, 0, 0");}else if(modelEditPlayer)dl->AddText({p.x+10,p.y+10},IM_COL32(255,220,70,255),"Camera / origin: 0, 0, 0");
  auto pivot=modelEditPivot();if(modelEditSelected==0){ModelEditV center{};auto& root=modelEditNodes[0];for(int a=0;a<3;a++)center[a]=(root.low[a]+root.high[a])*.5f;pivot=modelEditWorld(0).point(center);}
  ImVec2 center;bool visible=project(pivot,center);float distance=0;for(int a=0;a<3;a++)distance+=(pivot[a]-eye[a])*(pivot[a]-eye[a]);float length=std::max(.1f,std::sqrt(distance)*.18f);
  ImU32 colours[]={IM_COL32(245,80,75,255),IM_COL32(80,225,105,255),IM_COL32(90,155,255,255)};
  auto mouse=ImGui::GetIO().MousePos;int hovered=-1;float best=9*uiScale();
  auto segment=[&](ImVec2 a,ImVec2 b){float x=b.x-a.x,y=b.y-a.y,t=std::clamp(((mouse.x-a.x)*x+(mouse.y-a.y)*y)/std::max(1e-6f,x*x+y*y),0.f,1.f);return std::hypot(mouse.x-a.x-t*x,mouse.y-a.y-t*y);};
  if(visible)for(int axis=0;axis<3;axis++){
    if(modelEditTool==1){int i=(axis+1)%3,j=(axis+2)%3;ImVec2 last{};bool have=false;for(int k=0;k<=64;k++){auto v=pivot;float angle=k*6.28318530718f/64;v[i]+=std::cos(angle)*length;v[j]+=std::sin(angle)*length;ImVec2 point;bool ok=project(v,point);if(ok&&have){dl->AddLine(last,point,colours[axis],2);float d=segment(last,point);if(d<best){best=d;hovered=axis;}}last=point;have=ok;}}
    else{auto v=pivot;v[axis]+=length;ImVec2 end;if(project(v,end)){modelEditGizmoStart[axis]=center;modelEditGizmoEnd[axis]=end;dl->AddLine(center,end,colours[axis],3);ImVec2 d{end.x-center.x,end.y-center.y};float l=std::hypot(d.x,d.y);if(l>1){d.x/=l;d.y/=l;dl->AddTriangleFilled(end,{end.x-d.x*12-d.y*5,end.y-d.y*12+d.x*5},{end.x-d.x*12+d.y*5,end.y-d.y*12-d.x*5},colours[axis]);}dl->AddText(end,colours[axis],axis==0?"X":axis==1?"Y":"Z");float d2=segment(center,end);if(d2<best){best=d2;hovered=axis;}}}
  }
  bool inside=mouse.x>=p.x&&mouse.y>=p.y&&mouse.x<p.x+size.x&&mouse.y<p.y+size.y;
  if(inside&&hovered>=0&&ImGui::IsMouseClicked(0)){modelEditDrag=hovered;modelEditMouseStart=mouse;modelEditPositionStart=modelEditPivot();modelEditAnchorStart=pivot;modelEditRotationStart=modelEditNodes[modelEditSelected].rotation;modelEditScaleStart=modelEditNodes[modelEditSelected].scale;}
  if(modelEditDrag>=0&&ImGui::IsMouseDown(0)){
    int axis=modelEditDrag;float dx=mouse.x-modelEditMouseStart.x,dy=mouse.y-modelEditMouseStart.y;auto& n=modelEditNodes[modelEditSelected];
    if(modelEditTool==1){float angle=std::atan2(mouse.y-center.y,mouse.x-center.x)-std::atan2(modelEditMouseStart.y-center.y,modelEditMouseStart.x-center.x);n.rotation=modelEditRotationStart;n.rotation[axis]+=angle*57.295779513f;modelEditChanged=modelEditDirty=true;}
    else {auto end=modelEditAnchorStart;end[axis]+=length;ImVec2 a,b;if(project(modelEditAnchorStart,a)&&project(end,b)){float x=b.x-a.x,y=b.y-a.y,delta=(dx*x+dy*y)/std::max(25.f,x*x+y*y)*length;auto target=modelEditPositionStart;target[axis]+=delta;if(modelEditTool==0)modelEditMoveTo(target);else{n.scale[axis]=std::clamp(modelEditScaleStart[axis]*(1+delta/length),.001f,1000.f);modelEditChanged=modelEditDirty=true;}}}
  }
  bool captured=modelEditDrag>=0||inside&&hovered>=0;if(ImGui::IsMouseReleased(0))modelEditDrag=-1;dl->PopClipRect();return captured||modelEditPlayer||modelEditCameraCaptured;
}
