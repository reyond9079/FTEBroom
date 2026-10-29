// SPDX-License-Identifier: GPL-2.0-or-later
static bool updateStarted=false;
#ifdef _WIN32
static HANDLE updateProcess=nullptr;
#else
static pid_t updateProcess=-1;
#endif
static std::string updateMessage;
static void updateStart(){
#ifdef _WIN32
 if(updateProcess){DWORD code;GetExitCodeProcess(updateProcess,&code);if(code==STILL_ACTIVE)return;CloseHandle(updateProcess);updateProcess=nullptr;}
 try {auto root=settingsPath().parent_path();auto script=documentPath("update-ftebroom.ps1");if(!std::filesystem::is_regular_file(script))throw std::runtime_error("Updater script is missing.");
  auto args=L"powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File \""+script.wstring()+L"\" -Root \""+root.wstring()+L"\" -CurrentVersion 0.5.0-alpha.97 -ParentId "+std::to_wstring(GetCurrentProcessId());STARTUPINFOW si{};si.cb=sizeof si;PROCESS_INFORMATION pi{};
  if(!CreateProcessW(nullptr,args.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi))throw std::runtime_error("Cannot start update manager.");CloseHandle(pi.hThread);updateProcess=pi.hProcess;updateMessage="Checking GitHub releases...";
 }catch(const std::exception& e){updateMessage=e.what();}
#else
 if(updateProcess>0 && waitpid(updateProcess,nullptr,WNOHANG)==0)return;
 try {
  auto root=settingsPath().parent_path();auto script=documentPath("update-ftebroom.py");
  if(!std::filesystem::is_regular_file(script))throw std::runtime_error("Updater script is missing.");
  std::filesystem::create_directories(root/"updates");
  std::vector<std::string> args={"python3",script.u8string(),"--root",root.u8string(),"--current-version","0.5.0-alpha.97","--parent-id",std::to_string(getpid())};
  std::vector<char*> argv;for(auto& arg:args)argv.push_back(arg.data());argv.push_back(nullptr);
  posix_spawn_file_actions_t actions;posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_addopen(&actions,STDIN_FILENO,"/dev/null",O_RDONLY,0);
  auto log=(root/"updates/updater.log").u8string();posix_spawn_file_actions_addopen(&actions,STDOUT_FILENO,log.c_str(),O_CREAT|O_WRONLY|O_APPEND,0600);
  posix_spawn_file_actions_adddup2(&actions,STDOUT_FILENO,STDERR_FILENO);
  int result=posix_spawnp(&updateProcess,"python3",&actions,nullptr,argv.data(),environ);posix_spawn_file_actions_destroy(&actions);
  if(result){updateProcess=-1;throw std::runtime_error("Cannot start update manager. Install Python 3.");}
  updateMessage="Checking GitHub releases...";
 }catch(const std::exception& e){updateMessage=e.what();}
#endif
}
static void updateTick(){
 if(!ready||cvars->GetFloat("ftebroom_skip_setup"))return;
 if(!updateStarted){updateStarted=true;updateStart();}
 static quintptr_t last=0;auto now=core->GetMilliseconds();if(now-last<1000)return;last=now;
 std::ifstream input(settingsPath().parent_path()/"updates/status.txt");if(input){std::string value((std::istreambuf_iterator<char>(input)),{});if(value.size()<4096)updateMessage=value;}
}
static void updatePanel(){ImGui::TextWrapped("%s",updateMessage.empty()?"Updates are checked automatically at startup.":updateMessage.c_str());ImGui::TextWrapped("New releases for your platform download in the background and install after the editor closes. Settings and maps are retained; rollback files stay in updates/.");if(ImGui::Button("Check for updates"))updateStart();}
