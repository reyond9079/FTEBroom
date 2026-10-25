// SPDX-License-Identifier: GPL-2.0-or-later
// Directory requests run in a short-lived Python helper, never on the render thread.
struct OnlineServerRow{std::string url,name;int active=0,free=0;};
static std::vector<OnlineServerRow> onlineServers;
static std::string onlineDirectoryMessage;
static bool onlineBrowserLoaded=false,onlineDiscoverBusy=false;
#ifdef _WIN32
static HANDLE onlineDiscoverProcess=nullptr;
#else
static pid_t onlineDiscoverProcess=-1;
#endif
static void onlineSaveBrowser(){
  try{std::filesystem::create_directories(onlineDirectory());onlineWrite(onlineDirectory()/"browser-settings.txt",std::string(onlineMaster)+"\n"+onlineServer+"\n"+onlineName+"\n"+onlinePython+"\n");}
  catch(const std::exception& e){onlineDirectoryMessage=e.what();}
}
static void onlineLoadBrowser(){
  if(onlineBrowserLoaded)return;onlineBrowserLoaded=true;
  std::ifstream file(onlineDirectory()/"browser-settings.txt");std::string value;
  if(std::getline(file,value))std::snprintf(onlineMaster,sizeof onlineMaster,"%s",value.c_str());
  if(std::getline(file,value))std::snprintf(onlineServer,sizeof onlineServer,"%s",value.c_str());
  if(std::getline(file,value))std::snprintf(onlineName,sizeof onlineName,"%s",value.c_str());
  if(std::getline(file,value))std::snprintf(onlinePython,sizeof onlinePython,"%s",value.c_str());
}
static void onlineDiscover(){
  if(onlineDiscoverBusy)return;
  try{
    onlineSaveBrowser();onlineServers.clear();std::filesystem::create_directories(onlineDirectory());
    auto output=onlineDirectory()/"directory.txt";std::filesystem::remove(output);
    auto script=documentPath("online/discover.py");
#ifdef _WIN32
    std::wstring args=L"\""+std::filesystem::u8path(onlinePython).wstring()+L"\" \""+script.wstring()+L"\" --master \""+std::filesystem::u8path(onlineMaster).wstring()+L"\" --output \""+output.wstring()+L"\"";
    if(std::string(onlineMaster).find('"')!=std::string::npos)throw std::runtime_error("Invalid master address.");
    STARTUPINFOW si{};si.cb=sizeof si;PROCESS_INFORMATION pi{};
    if(!CreateProcessW(nullptr,args.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi))throw std::runtime_error("Cannot start Python 3. Set its executable path.");
    CloseHandle(pi.hThread);onlineDiscoverProcess=pi.hProcess;
#else
    std::vector<std::string> args={onlinePython,script.u8string(),"--master",onlineMaster,"--output",output.u8string()};std::vector<char*> argv;for(auto& a:args)argv.push_back(a.data());argv.push_back(nullptr);
    if(posix_spawnp(&onlineDiscoverProcess,onlinePython,nullptr,nullptr,argv.data(),environ))throw std::runtime_error("Cannot start Python 3.");
#endif
    onlineDiscoverBusy=true;onlineDirectoryMessage="Refreshing servers...";
  }catch(const std::exception& e){onlineDirectoryMessage=e.what();}
}
static void onlineBrowserTick(){
  if(!onlineDiscoverBusy)return;
#ifdef _WIN32
  DWORD code=0;if(GetExitCodeProcess(onlineDiscoverProcess,&code)&&code==STILL_ACTIVE)return;CloseHandle(onlineDiscoverProcess);onlineDiscoverProcess=nullptr;
#else
  if(waitpid(onlineDiscoverProcess,nullptr,WNOHANG)==0)return;onlineDiscoverProcess=-1;
#endif
  onlineDiscoverBusy=false;
  std::ifstream file(onlineDirectory()/"directory.txt");std::string line;
  if(!std::getline(file,line)){onlineDirectoryMessage="No directory response. Check Python and the master address.";return;}
  if(line!="OK"){std::getline(file,onlineDirectoryMessage);return;}
  while(onlineServers.size()<256&&std::getline(file,line)){
    OnlineServerRow row;std::istringstream fields(line);std::string counts;
    if(std::getline(fields,row.url,'\t')&&std::getline(fields,row.name,'\t')&&(fields>>row.active>>row.free))onlineServers.push_back(row);
  }
  onlineDirectoryMessage=onlineServers.empty()?"No active servers listed. Start a relay or use a direct address.":"Select a server below.";
}
static void onlineBrowserControls(){
  onlineLoadBrowser();
  static bool requested=false;if(!requested){requested=true;onlineDiscover();}
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);
  auto bg=ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
  ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(std::min(1.f,bg.x+.08f),std::min(1.f,bg.y+.08f),std::min(1.f,bg.z+.08f),1));
  ImGui::TextWrapped("LibreQuake resources and BSP2 are fixed in this online build. Missing resources download automatically when you join.");
  ImGui::BeginDisabled(onlinePreparing());
  ImGui::BeginDisabled(onlineDiscoverBusy);if(ImGui::InputText("Master server",onlineMaster,sizeof onlineMaster))onlineServers.clear();ImGui::EndDisabled();
  ImGui::SameLine();ImGui::BeginDisabled(onlineDiscoverBusy);if(ImGui::Button("Refresh servers"))onlineDiscover();ImGui::EndDisabled();
  ImGui::TextWrapped("%s",onlineDirectoryMessage.c_str());
  ImGui::BeginChild("Online server list",{0,130*uiScale()},ImGuiChildFlags_Borders);
  for(auto& row:onlineServers){ImGui::PushID(row.url.c_str());
    std::string label=row.name+"  |  "+std::to_string(row.active)+" online, "+std::to_string(row.free)+" free plots";
    if(ImGui::Selectable(label.c_str(),row.url==onlineServer)){std::snprintf(onlineServer,sizeof onlineServer,"%s",row.url.c_str());onlineSaveBrowser();}
    if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",row.url.c_str());ImGui::PopID();
  }ImGui::EndChild();
  ImGui::InputText("Direct server / selected",onlineServer,sizeof onlineServer);
  ImGui::InputText("Name",onlineName,sizeof onlineName);ImGui::InputText("Python executable",onlinePython,sizeof onlinePython);
  if(ImGui::CollapsingHeader("Hosting / directory setup")){
    ImGui::TextWrapped("Start online/master.py on one LAN/VPN computer, then start online/server.py with --master pointing to it. The default directory address is local, not a public service.");
    if(ImGui::Button("Prepare LibreQuake for hosting"))onlinePrepareResources();
    ImGui::TextDisabled(libreReady()?"LibreQuake is installed.":"Resources will download automatically.");
  }
  ImGui::EndDisabled();
  if(libreBusy()){unsigned long long done=0,total=0;std::string phase;std::ifstream(libreInstall()/"progress.txt")>>done>>total>>phase;ImGui::ProgressBar(total?float(double(done)/total):0,{-1,0},"Downloading LibreQuake...");}
  ImGui::TextWrapped("%s",onlineMessage.c_str());
  ImGui::TextDisabled("No public directory is bundled. Run a master on your LAN/VPN or enter its address.");
  ImGui::PopStyleColor();ImGui::PopStyleVar();
}
