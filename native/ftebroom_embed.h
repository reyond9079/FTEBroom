// SPDX-License-Identifier: GPL-2.0-or-later
// Windows child rendering avoids frame readback and keeps the mod VM isolated.
static std::string modSavedCamera;
static float modSavedFPS;
static double modLastSend,modPendingSince;
static uint64_t modAckCount;
static bool modAwaiting;
static bool modPlay=true,modCaptured,modMenuOpen,modLoading;
static uint64_t modSentAcks;
static int modControlCapture=-1;
static void modControlsApply();
static std::string modLastWindow,modLastCamera,modInputQueue;
static float modMouseX,modMouseY;
static void modReleaseInput()
{
  modCaptured=false;modMouseX=modMouseY=0;
  modInputQueue="ftebroom_preview_input reset\n";
  if(input->HasMenuFocus())cameraEnd();
  else if(sceneLook){sceneLook=false;cameraCursorPending=true;cvars->SetFloat("ftebroom_camera_captured",0);}
  ImGui::GetIO().ClearInputKeys();
}
static void modSetPlay(bool play)
{
  modReleaseInput();modPlay=play;modLastCamera.clear();
  std::fill(std::begin(moveHeld),std::end(moveHeld),false);
  cvars->SetFloat("ftebroom_forward",0);cvars->SetFloat("ftebroom_right",0);cvars->SetFloat("ftebroom_up",0);
}
static void modQueueKey(int key,int unicode,bool down)
{
  if(modInputQueue.size()>3000){modReleaseInput();return;}
  char line[100];std::snprintf(line,sizeof line,"ftebroom_preview_input key %d %d %d\n",key,unicode,down?1:0);
  modInputQueue+=line;
}
#ifdef _WIN32
static HWND modHost,modChild;
static LONG_PTR modHostStyle;
static BOOL CALLBACK modFindHost(HWND window,LPARAM result)
{
  DWORD pid=0;GetWindowThreadProcessId(window,&pid);
  if(pid==GetCurrentProcessId() && IsWindowVisible(window) && !GetWindow(window,GW_OWNER)){
    *reinterpret_cast<HWND*>(result)=window;return FALSE;
  }return TRUE;
}
#endif
static void modEmbedEnd()
{
  if(!modEmbedded)return;
  modReleaseInput();
  modEmbedded=false;modMenuOpen=false;modLoading=false;modControlCapture=-1;
  cvars->SetFloat("cl_maxfps",modSavedFPS);
  cvars->SetFloat("ftebroom_mod_preview",0);
  std::fill(std::begin(moveHeld),std::end(moveHeld),false);
  cvars->SetFloat("ftebroom_forward",0);cvars->SetFloat("ftebroom_right",0);cvars->SetFloat("ftebroom_up",0);
  if(runActive)runStop();
#ifdef _WIN32
  if(IsWindow(modHost))SetWindowLongPtr(modHost,GWL_STYLE,modHostStyle);
  modChild=modHost=nullptr;
#endif
  cameraEnd();
  if(!modSavedCamera.empty()){cvars->SetString("ftebroom_camera_state",modSavedCamera.c_str());command(("ftebroom_camera_restore "+modSavedCamera+"\n").c_str());}
  buildRestoreLayout();
  command("ftebroom\n");
  buildMessage="Returned to editing. Mod simulation stopped.";
}
static void modEmbedBegin()
{
#ifdef _WIN32
  EnumWindows(modFindHost,reinterpret_cast<LPARAM>(&modHost));
  if(!modHost)throw std::runtime_error("Cannot locate the editor window.");
  modHostStyle=GetWindowLongPtr(modHost,GWL_STYLE);
  SetWindowLongPtr(modHost,GWL_STYLE,modHostStyle|WS_CLIPCHILDREN);
  char state[512]={};cvars->GetString("ftebroom_camera_state",state,sizeof state);modSavedCamera=state;
  cancelGesture();cameraEnd();
  buildWindow=false;runWindow=false;
  if(!buildWasPreview){buildSavedLayout=viewLayout;buildSavedFocus=viewFocus;buildSavedMax=viewMax;
    std::copy(std::begin(viewTypes),std::end(viewTypes),buildSavedTypes);buildWasPreview=true;}
  viewLayout=1;viewFocus=0;viewMax=-1;viewTypes[0]=0;cvars->SetFloat("ftebroom_view_layout",1);viewActivate(0);
  modSavedFPS=cvars->GetFloat("cl_maxfps");if(modSavedFPS<=0 || modSavedFPS>60)cvars->SetFloat("cl_maxfps",60);
  modChild=nullptr;modAwaiting=false;modAckCount=runControlAcks;modLastSend=0;modEmbedded=true;
  modLoading=true;modSentAcks=runControlAcks;
  modLastWindow.clear();modSetPlay(true);modControlsApply();
  cvars->SetFloat("ftebroom_mod_preview",1);
#else
  throw std::runtime_error("Embedded mod preview currently requires Windows.");
#endif
}
static void modEmbedTick(float width,float height)
{
#ifdef _WIN32
  if(!modEmbedded)return;
  if(!runActive){auto error=runError;modEmbedEnd();if(!error.empty())buildMessage="Mod preview stopped: "+error;return;}
  if(!runInput || !runProcess)return;
  if(modLoading && runLog.find("ftebroom_preview_ready")!=std::string::npos){modLoading=false;buildMessage.clear();}
  if(!modChild){
    auto at=runLog.find("refocuswindow ");
    if(at!=std::string::npos){
      auto candidate=reinterpret_cast<HWND>(std::strtoull(runLog.c_str()+at+14,nullptr,16));DWORD pid=0;
      GetWindowThreadProcessId(candidate,&pid);
      if(IsWindow(candidate) && pid==GetProcessId(runProcess)){
        modChild=candidate;
      }
    }
  }
  auto r=viewRects[0];
  bool covered=modLoading || popupOpen || !IsWindowVisible(modHost) || IsIconic(modHost);
  for(auto window:ImGui::GetCurrentContext()->Windows){
    if(!window->Active || window->Hidden || (window->Flags & (ImGuiWindowFlags_DockNodeHost|ImGuiWindowFlags_NoBackground)))continue;
    if(std::string(window->Name)=="##Build viewport status")continue;
    if(window->Pos.x<r.x+r.w && window->Pos.y<r.y+r.h && window->Pos.x+window->Size.x>r.x && window->Pos.y+window->Size.y>r.y)covered=true;
  }
  RECT client{};GetClientRect(modHost,&client);
  float sx=float(client.right)/std::max(1.f,width),sy=float(client.bottom)/std::max(1.f,height);
  int x=int(r.x*sx),y=int(r.y*sy),w=std::max(16,int(r.w*sx)),h=std::max(16,int(r.h*sy));
  if(modCaptured && (covered || GetForegroundWindow()!=modHost))modReleaseInput();
  // Allow a bounded pipeline instead of halving the mouse update rate with stop-and-wait.
  if(runControlAcks!=modAckCount){modAckCount=runControlAcks;modPendingSince=ImGui::GetTime();}
  if(modSentAcks>=runControlAcks+3){if(ImGui::GetTime()-modPendingSince>10)buildMessage="Mod preview is not responding. Back to editing stops the process.";return;}
  if(!modChild || ImGui::GetTime()-modLastSend<.016)return;
  char state[512]={};float values[7];cvars->GetString("ftebroom_camera_state",state,sizeof state);
  if(!cameraParse(state,values))return;
  char line[512];std::string control;
  std::snprintf(line,sizeof line,"vid_recenter %d %d %d %d %llx %d\n",x,y,w,h,(unsigned long long)(uintptr_t)modHost,covered?0:1);
  if(modLastWindow!=line){modLastWindow=line;control+="viewsize 120\n";control+=line;}
  std::snprintf(line,sizeof line,"set ftebroom_preview_play %d\nset r_drawviewmodel %d\nset ftebroom_preview_camera \"%g %g %g %g %g %g %g\"\n",modPlay?1:0,modPlay?1:0,values[0],values[1],values[2],values[3],values[4],values[5],values[6]);
  if(modLastCamera!=line){modLastCamera=line;control+=line;
    if(gameProfile==4)control+=std::string("set chase_active 0\nset pm_thirdPerson 0\nset r_showViewModels ")+(modPlay?"1\n":"0\n");}
  control+=modInputQueue;modInputQueue.clear();
  if(modCaptured && (modMouseX || modMouseY)){
    std::snprintf(line,sizeof line,"ftebroom_preview_input mouse %g %g\n",std::clamp(modMouseX,-8192.f,8192.f),std::clamp(modMouseY,-8192.f,8192.f));control+=line;
  }
  modMouseX=modMouseY=0;
  if(control.empty())return;
  control+="ftebroom_preview_input ack\n";
  DWORD written=0;modAckCount=runControlAcks;
  if(!WriteFile(runInput,control.data(),(DWORD)control.size(),&written,nullptr) || written!=control.size()){modEmbedEnd();buildMessage="Mod preview control connection closed.";return;}
  if(modSentAcks==runControlAcks)modPendingSince=ImGui::GetTime();
  ++modSentAcks;modLastSend=ImGui::GetTime();
#endif
}

#include "ftebroom_mod_controls.h"
