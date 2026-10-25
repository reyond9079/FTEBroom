// SPDX-License-Identifier: GPL-2.0-or-later
// Experimental relay client. Networking runs in a separate Python process.
static bool onlineWindow=false,onlineRunning=false,onlineJoined=false,onlineOldLock=false;
static bool onlineOldGeometry,onlineOldVis,onlineOldLight;static int onlineOldBsp;
static char onlineServer[512]="http://127.0.0.1:28777",onlineName[64]="Mapper";
static char onlineMaster[512]="http://127.0.0.1:28776";
static bool onlinePreparePending=false,onlineMountPending=false,onlineJoinAfterResources=true;
#ifdef _WIN32
static char onlinePython[4096]="python";
#else
static char onlinePython[4096]="python3";
#endif
static std::string onlineMessage;
static double onlineLastPoll;
#ifdef _WIN32
static HANDLE onlineProcess=nullptr;
#else
static pid_t onlineProcess=-1;
#endif
static bool onlineProcessAlive(){
#ifdef _WIN32
  if(!onlineProcess)return false;DWORD code=0;if(GetExitCodeProcess(onlineProcess,&code)&&code==STILL_ACTIVE)return true;CloseHandle(onlineProcess);onlineProcess=nullptr;return false;
#else
  if(onlineProcess<=0)return false;if(waitpid(onlineProcess,nullptr,WNOHANG)==0)return true;onlineProcess=-1;return false;
#endif
}
static std::filesystem::path onlineDirectory(){return documentPath("data/online");}
static std::string onlineJson(const std::string& value){std::string out="\"";for(unsigned char c:value){if(c=='"'||c=='\\')out+='\\';if(c<32)throw std::runtime_error("Control characters are not supported.");out+=c;}return out+'"';}
static void onlineWrite(const std::filesystem::path& path,const std::string& text){
  auto temporary=path;temporary+=".native-tmp";std::ofstream out(temporary,std::ios::binary);out<<text;out.close();if(!out)throw std::runtime_error("Cannot write online session data.");
  std::error_code ec;std::filesystem::rename(temporary,path,ec);
  if(ec){std::filesystem::copy_file(temporary,path,std::filesystem::copy_options::overwrite_existing);std::filesystem::remove(temporary);}
}
#include "ftebroom_online_browser.h"
static bool onlineSetupComplete(){return onlineJoined;}
static bool onlinePreparing(){return onlinePreparePending||onlineMountPending||onlineRunning||assetsLoading;}
static void onlinePrepareJoin(){
  if(onlinePreparing())return;
  if(cvars->GetFloat("ftebroom_dirty")){onlineMessage="Save the current map before joining.";return;}
  onlineJoinAfterResources=true;
  gameProfile=2;q1UseLibre=true;setupModChoice=0;q1BspFormat=1;
  *q1Wads=0;*q1Definitions=0;
  onlineSaveBrowser();onlinePreparePending=true;onlineMessage="Preparing LibreQuake resources...";
  if(!libreReady())libreStartDownload();
}
static void onlinePrepareResources(){onlinePrepareJoin();if(onlinePreparePending)onlineJoinAfterResources=false;}
static void onlineDisconnect(){
  if(onlineRunning){try{onlineWrite(onlineDirectory()/"stop","stop");}catch(...){}gameProfileLocked=onlineOldLock||buildRunning;buildGeometry=onlineOldGeometry;buildVis=onlineOldVis;buildLight=onlineOldLight;q1BspFormat=onlineOldBsp;}
  onlineRunning=onlineJoined=false;cvars->SetFloat("ftebroom_online_active",0);command("ftebroom_online_clear\n");
  onlineResourceLock=false;onlineAllowedMaterials.clear();
}
static void onlineConnect(){
  try{
    if(onlineRunning)return;
    if(onlineProcessAlive())throw std::runtime_error("The previous background client is stopping. Retry shortly.");
    if(gameProfile!=2||!q1UseLibre||!libreReady()||assetsLoading)throw std::runtime_error("Load the Quake 1 / LibreQuake profile before connecting.");
    if(cvars->GetFloat("ftebroom_dirty"))throw std::runtime_error("Save the current map before joining. Joining opens your own online MAP.");
    auto folder=onlineDirectory();std::filesystem::create_directories(folder);
    for(const char* file:{"stop","ready.txt","request.json","status.txt","own-bsp.txt"})std::filesystem::remove(folder/file);
    onlineWrite(folder/"connect.json","{\"server\":"+onlineJson(onlineServer)+",\"name\":"+onlineJson(onlineName)+",\"resources\":"+onlineJson(libreMod())+"}");
    auto script=documentPath("online/client.py");
#ifdef _WIN32
    auto args=L"\""+std::filesystem::u8path(onlinePython).wstring()+L"\" \""+script.wstring()+L"\" --directory \""+folder.wstring()+L"\"";
    STARTUPINFOW si{};si.cb=sizeof si;PROCESS_INFORMATION pi{};
    if(!CreateProcessW(nullptr,args.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi))throw std::runtime_error("Cannot launch Python 3. Set its executable path below.");
    CloseHandle(pi.hThread);onlineProcess=pi.hProcess;
#else
    std::vector<std::string> args={onlinePython,script.u8string(),"--directory",folder.u8string()};std::vector<char*> argv;for(auto& arg:args)argv.push_back(arg.data());argv.push_back(nullptr);
    int result=posix_spawnp(&onlineProcess,onlinePython,nullptr,nullptr,argv.data(),environ);
    if(result)throw std::runtime_error("Cannot launch Python 3.");
#endif
    buildLoadSettings();onlineOldGeometry=buildGeometry;onlineOldVis=buildVis;onlineOldLight=buildLight;onlineOldBsp=q1BspFormat;
    onlineRunning=true;onlineJoined=false;onlineOldLock=gameProfileLocked;gameProfileLocked=true;q1BspFormat=1;onlineMessage="Connecting...";
  }catch(const std::exception& e){onlineMessage=e.what();}
}
static void onlineTick(){
  onlineBrowserTick();
  if(onlinePreparePending&&!libreBusy()){
    onlinePreparePending=false;
    if(!libreReady()){onlineMessage=libreError.empty()?"LibreQuake is not ready. Retry joining.":libreError;}
    else if(gameProfileSave()&&mountAssets()){onlineMountPending=true;onlineMessage="Loading LibreQuake resources...";}
    else onlineMessage=assetError;
  }
  if(onlineMountPending&&!assetsLoading){onlineMountPending=false;if(setupNeeded)onlineMessage=assetError;else if(onlineJoinAfterResources)onlineConnect();else onlineMessage="LibreQuake is ready. You can start your relay and refresh the server list.";setupNeeded=false;}
  bool alive=onlineProcessAlive();if(!onlineRunning)return;
  double now=core->GetMilliseconds()/1000.;if(now-onlineLastPoll<.25)return;onlineLastPoll=now;
  try{
    auto folder=onlineDirectory();std::ifstream status(folder/"status.txt");if(status)std::getline(status,onlineMessage);
    if(!alive){onlineDisconnect();return;}
    if(gameProfile!=2||!q1UseLibre){onlineMessage="Profile changed; session disconnected.";onlineDisconnect();return;}
    q1BspFormat=1;setupNeeded=false;gameProfileLocked=true;
    if(!onlineJoined){
      std::ifstream readyFile(folder/"ready.txt");int owner;float bounds[6];
      if(readyFile>>owner>>bounds[0]>>bounds[1]>>bounds[2]>>bounds[3]>>bounds[4]>>bounds[5]){
        std::ifstream materialFile(folder/"materials.txt");std::string material;onlineAllowedMaterials.clear();while(std::getline(materialFile,material))if(!material.empty())onlineAllowedMaterials.insert(material);
        if(onlineAllowedMaterials.empty())throw std::runtime_error("The online material list is empty.");
        onlineResourceLock=true;auto initial=*onlineAllowedMaterials.begin();if(initial.rfind("textures/",0)==0)initial.erase(0,9);cvars->SetString("ca_newbrushtexture",initial.c_str());
        for(int i=0;i<6;i++)cvars->SetFloat(("ftebroom_online_bound"+std::to_string(i)).c_str(),bounds[i]);
        cvars->SetFloat("ftebroom_online_active",1);onlineJoined=true;
        command("mapedit maps/online-own.map\nin 2 ftebroom_online_enable\n");
      }
      return;
    }
    char camera[512]={};cvars->GetString("ftebroom_camera_state",camera,sizeof camera);float pose[7];
    if(cameraParse(camera,pose)){std::ostringstream out;for(int i=0;i<5;i++)out<<pose[i]<<' ';onlineWrite(folder/"pose.txt",out.str());}
    command("ftebroom_online_scene\n");
  }catch(const std::exception& e){onlineMessage=e.what();}
}
static void onlinePanel(){
  if(!onlineWindow)return;
  ImGui::SetNextWindowSize({640*uiScale(),600*uiScale()},ImGuiCond_FirstUseEver);
  if(ImGui::Begin("ReyondBroom online (experimental)",&onlineWindow)){
    ImGui::TextWrapped("Local relay prototype: eight private 2048 x 2048 plots, shared free camera, LibreQuake only, BSP2 only. Python 3 is required. Online compilation retains collision hulls, skips VIS and accepts open maps (-nofill).");
    onlineBrowserControls();
    ImGui::BeginDisabled(onlinePreparing());if(ImGui::Button("Join session"))onlinePrepareJoin();ImGui::EndDisabled();
    if(onlineRunning){
      ImGui::BeginDisabled(buildRunning);if(ImGui::Button("Disconnect"))onlineDisconnect();ImGui::EndDisabled();
      ImGui::BeginDisabled(!onlineJoined);
      if(ImGui::Button("Publish last saved MAP + compiled BSP"))try{
        if(documentFile.empty()||buildResult.empty()||buildResultDocument!=documentFile||buildRunning||runHadError||buildResultRevision!=(int)cvars->GetFloat("ftebroom_revision"))throw std::runtime_error("Save and successfully compile the current map revision before publishing.");
        onlineWrite(onlineDirectory()/"request.json","{\"action\":\"publish\",\"map\":"+onlineJson(documentFile.u8string())+",\"bsp\":"+onlineJson(buildResult.u8string())+"}");
      }catch(const std::exception& e){onlineMessage=e.what();}
      if(ImGui::Button("View last published BSP")){
        std::ifstream file(onlineDirectory()/"own-bsp.txt");std::string path;std::getline(file,path);
        if(path.rfind("maps/ftebroom-build/online-",0)==0 && path.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-/.")==std::string::npos)command(("ftebroom_bsp "+path+"\n").c_str());
        else onlineMessage="Publish a compiled BSP first.";
      }
      ImGui::SameLine();if(ImGui::Button("Back to editable MAP"))buildEdit();
      ImGui::EndDisabled();
    }
    ImGui::TextWrapped("%s",onlineMessage.c_str());
    ImGui::TextDisabled("BSP: 16 MiB, 16K faces, 32K vertices; server never runs uploaded code.");
  }ImGui::End();
}
