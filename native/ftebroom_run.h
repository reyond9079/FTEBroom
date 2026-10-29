// SPDX-License-Identifier: GPL-2.0-or-later
#include <thread>
#ifndef _WIN32
#include <spawn.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif
struct RunTask {int kind=0;std::string source,destination;bool enabled=true;};
struct RunProfile {std::string name="New profile",work="${MAP_DIR_PATH}";bool launch=false;std::vector<RunTask> tasks;};
static std::vector<RunProfile> runProfiles;
static std::map<std::string,std::string> runVariables;
static RunProfile runningProfile,lastRunProfile;
static std::map<std::string,std::string> activeVariables;
static bool runExclusive=false;
static bool runWindow,runActive,runExportWaiting,runHadError,runHaveLast;
static int runSelected,runStep;
static std::string runLog,runError;
static uint64_t runControlAcks;
static void (*runExportHook)(const std::filesystem::path&)=nullptr;
static void (*runStageHook)()=nullptr;
#ifdef _WIN32
static HANDLE runProcess,runPipe,runJob,runInput;
#else
static pid_t runProcess=-1;static int runPipe=-1;
#endif
static void (*runPreviewSettings)(const std::string&)=nullptr;
static void runAppend(const std::string& text)
{
  auto previous=runLog.size();runLog+=text;
  if(runPreviewSettings){
    size_t at=previous>512?previous-512:0;
    while((at=runLog.find("reyond_preview_settings ",at))!=std::string::npos){
      auto end=runLog.find('\n',at);if(end==std::string::npos)break;
      if(end>=previous&&end-at<512)runPreviewSettings(runLog.substr(at+24,end-at-24));at=end+1;
    }
  }
  for(size_t at=previous>32?previous-32:0;(at=runLog.find("ftebroom_camera_ack",at))!=std::string::npos;at+=18)
    if(at+18>previous)++runControlAcks;
  if(runLog.size()>4*1024*1024)runLog.erase(0,runLog.size()-4*1024*1024);
}
static std::filesystem::path runBundledEngine()
{
  auto root=documentPath("ftebroom.cfg").parent_path().parent_path();
#ifdef _WIN32
  auto engine=root/"bin/fteqw64.exe";
  return std::filesystem::is_regular_file(engine)?engine:root/"fteqw64.exe";
#else
  auto preview=root/"tools/runtimes/fteqw-preview64";
  if(std::filesystem::is_regular_file(preview))return preview;
  return root/"fteqw-gl64";
#endif
}
static void runLoad()
{
  static bool loaded=false;if(loaded)return;loaded=true;
  std::ifstream stream(documentPath("ftebroom-run-profiles.txt"));std::string line;
  while(std::getline(stream,line))
  {
    std::istringstream row(line);std::string type;row>>type;
    if(type=="profile") {RunProfile profile;if(row>>std::quoted(profile.name)>>profile.launch>>std::quoted(profile.work))runProfiles.push_back(profile);}
    else if(type=="task"&&!runProfiles.empty()){RunTask task;if(row>>task.kind>>std::quoted(task.source)>>std::quoted(task.destination) && task.kind>=0&&task.kind<=4){int enabled=1;if(row>>enabled)task.enabled=enabled!=0;runProfiles.back().tasks.push_back(task);}}
    else if(type=="variable"){std::string name,value;if(row>>std::quoted(name)>>std::quoted(value))runVariables[name]=value;}
  }
  if(runProfiles.empty())
  {
    runProfiles.push_back({"Q3: Export + q3map2 -meta","${MAP_DIR_PATH}/_ftebroom-build",false,{{0,"${WORK_DIR_PATH}/${MAP_BASE_NAME}.map",""},{1,"${q3map2}","-game quake3 -fs_basepath \"${GAME_DIR_PATH}/..\" -meta \"${WORK_DIR_PATH}/${MAP_BASE_NAME}.map\""}}});
#ifdef _WIN32
    const char* launchEngine="${APP_DIR_PATH}/fteqw64.exe";
#else
    const char* launchEngine="${APP_DIR_PATH}/fteqw-gl64";
#endif
    runProfiles.push_back({"Launch in FTE","${APP_DIR_PATH}",true,{{1,launchEngine,"-basedir \"${GAME_DIR_PATH}/..\" -game baseq3 +map ${MAP_BASE_NAME}"}}});
    runVariables["q3map2"]="";runVariables["bspc"]="";
  }
  auto preset=[&](RunProfile profile){if(std::none_of(runProfiles.begin(),runProfiles.end(),[&](const RunProfile& p){return p.name==profile.name;}))runProfiles.push_back(std::move(profile));};
  const std::string map="\"${WORK_DIR_PATH}/${MAP_BASE_NAME}.map\"",bsp="\"${WORK_DIR_PATH}/${MAP_BASE_NAME}.bsp\"";
  const std::string q3="-game quake3 -fs_basepath \"${GAME_DIR_PATH}/..\" ";
  preset({"Q3: BSP + fast VIS + LIGHT","${MAP_DIR_PATH}/_ftebroom-build",false,{
    {0,"${WORK_DIR_PATH}/${MAP_BASE_NAME}.map",""},{1,"${q3map2}",q3+"-meta "+map},
    {1,"${q3map2}",q3+"-vis -fast "+bsp},{1,"${q3map2}",q3+"-light -fast "+bsp}}});
  preset({"Q3: BSP + VIS + final LIGHT","${MAP_DIR_PATH}/_ftebroom-build",false,{
    {0,"${WORK_DIR_PATH}/${MAP_BASE_NAME}.map",""},{1,"${q3map2}",q3+"-meta "+map},
    {1,"${q3map2}",q3+"-vis "+bsp},{1,"${q3map2}",q3+"-light -samples 2 "+bsp}}});
  preset({"Q1: BSP + VIS + LIGHT","${MAP_DIR_PATH}/_ftebroom-build",false,{
    {0,"${WORK_DIR_PATH}/${MAP_BASE_NAME}.map",""},{1,"${qbsp}",map},{1,"${vis}",bsp},{1,"${light}","-extra "+bsp}}});
  const std::string q2Paths="-basedir \"${BASE_GAME_DIR_PATH}\" -gamedir \"${GAME_DIR_PATH}\" -path \"${WORK_DIR_PATH}\" ";
  preset({"Q2: BSP + VIS + LIGHT","${MAP_DIR_PATH}/_ftebroom-build",false,{{0,"${WORK_DIR_PATH}/${MAP_BASE_NAME}.map",""},{1,"${q2bsp}","-q2bsp "+q2Paths+map},{1,"${q2vis}",bsp},{1,"${q2light}",q2Paths+bsp}}});
  preset({"HL: CSG + BSP + VIS + RAD","${MAP_DIR_PATH}/_ftebroom-build",false,{{0,"${WORK_DIR_PATH}/${MAP_BASE_NAME}.map",""},{1,"${hlcsg}","-nowadtextures "+map},{1,"${hlbsp}",map},{1,"${hlvis}",map},{1,"${hlrad}",map}}});
  auto root=documentPath("ftebroom.cfg").parent_path().parent_path();
  for(const auto& pair:std::vector<std::pair<std::string,std::string>>{{"q2bsp","tools/ericw-q2/qbsp"},{"q2vis","tools/ericw-q2/vis"},{"q2light","tools/ericw-q2/light"},{"hlcsg","tools/sdhlt/sdHLCSG"},{"hlbsp","tools/sdhlt/sdHLBSP"},{"hlvis","tools/sdhlt/sdHLVIS"},{"hlrad","tools/sdhlt/sdHLRAD"}}){
    auto path=root/pair.second;
#ifdef _WIN32
    path+=".exe";
#endif
    if(runVariables[pair.first].empty()&&std::filesystem::is_regular_file(path))runVariables[pair.first]=std::filesystem::absolute(path).u8string();
  }
  std::error_code compilerError;
  if(std::none_of(runProfiles.begin(),runProfiles.end(),[](const RunProfile& p){return p.name=="Q1: Export + qbsp";}))
    runProfiles.push_back({"Q1: Export + qbsp","${MAP_DIR_PATH}/_ftebroom-build",false,{{0,"${WORK_DIR_PATH}/${MAP_BASE_NAME}.map",""},{1,"${qbsp}","\"${WORK_DIR_PATH}/${MAP_BASE_NAME}.map\""}}});
  if(runVariables["q3map2"].empty() || !std::filesystem::is_regular_file(std::filesystem::u8path(runVariables["q3map2"]),compilerError)) {
    auto bundled=documentPath("ftebroom.cfg").parent_path().parent_path()/"tools/q3map2/q3map2.exe";
#ifndef _WIN32
    bundled.replace_extension();
#endif
    if(std::filesystem::exists(bundled))runVariables["q3map2"]=std::filesystem::absolute(bundled).lexically_normal().u8string();
  }
  for(const char* name:{"qbsp","vis","light"})if(runVariables[name].empty()) {
    auto bundled=documentPath("ftebroom.cfg").parent_path().parent_path()/"tools/ericw"/(std::string(name)+".exe");
#ifndef _WIN32
    bundled.replace_extension();
#endif
    if(std::filesystem::exists(bundled))runVariables[name]=std::filesystem::absolute(bundled).lexically_normal().u8string();
  }

}
static bool runSaveProfiles()
{
  auto path=documentPath("ftebroom-run-profiles.txt"),temporary=path;temporary+=".tmp";
  std::ofstream stream(temporary);
  for(const auto& profile:runProfiles){stream<<"profile "<<std::quoted(profile.name)<<' '<<profile.launch<<' '<<std::quoted(profile.work)<<'\n';
    for(const auto& task:profile.tasks)stream<<"task "<<task.kind<<' '<<std::quoted(task.source)<<' '<<std::quoted(task.destination)<<' '<<task.enabled<<'\n';}
  for(const auto& variable:runVariables)stream<<"variable "<<std::quoted(variable.first)<<' '<<std::quoted(variable.second)<<'\n';
  stream.close();if(!stream){runError="Cannot write run profiles.";return false;}
#ifdef _WIN32
  if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){runError="Cannot replace run profiles.";return false;}
#else
  std::error_code error;std::filesystem::rename(temporary,path,error);if(error){runError=error.message();return false;}
#endif
  return true;
}
static std::string runExpand(const std::string& text)
{
  std::string out;size_t cursor=0;
  while(cursor<text.size())
  {
    auto begin=text.find("${",cursor);if(begin==std::string::npos){out+=text.substr(cursor);break;}
    out+=text.substr(cursor,begin-cursor);auto end=text.find('}',begin+2);
    if(end==std::string::npos)throw std::runtime_error("Unclosed profile variable.");
    auto name=text.substr(begin+2,end-begin-2);auto found=activeVariables.find(name);
    if(found==activeVariables.end())throw std::runtime_error("Unknown profile variable: "+name);
    out+=found->second;cursor=end+1;
  }
  // Preserve saved launch profiles from distributions with the engine at root.
#ifdef _WIN32
  auto legacy=documentPath("ftebroom.cfg").parent_path().parent_path()/"fteqw64.exe";
  if(std::filesystem::u8path(out).lexically_normal()==legacy.lexically_normal())
    return runBundledEngine().u8string();
#endif
  return out;
}
static void runStop()
{
#ifdef _WIN32
  if(runInput){CloseHandle(runInput);runInput=nullptr;}
  if(runJob){TerminateJobObject(runJob,1);CloseHandle(runJob);runJob=nullptr;}
  else if(runProcess)TerminateProcess(runProcess,1);
  if(runProcess){CloseHandle(runProcess);runProcess=nullptr;}
  if(runPipe){CloseHandle(runPipe);runPipe=nullptr;}
#else
  if(runProcess>0){kill(-runProcess,SIGKILL);waitpid(runProcess,nullptr,0);runProcess=-1;}
  if(runPipe>=0){close(runPipe);runPipe=-1;}
#endif
  runActive=runExportWaiting=false;runAppend("\nStopped.\n");
}
#ifdef _WIN32
static std::wstring runWide(const std::string& text)
{
  int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),(int)text.size(),nullptr,0);
  if(!count&&!text.empty())throw std::runtime_error("Invalid UTF-8 process argument.");
  std::wstring out(count,0);MultiByteToWideChar(CP_UTF8,0,text.data(),(int)text.size(),out.data(),count);return out;
}
#endif
static void runStartProcess(std::string executable,const std::string& arguments,const std::filesystem::path& working)
{
  if(executable.empty())throw std::runtime_error("Set the executable path in the profile's tool variables.");
  auto configuredExecutable=std::filesystem::u8path(executable);
  if(configuredExecutable.is_relative()&&configuredExecutable.has_parent_path())executable=std::filesystem::absolute(configuredExecutable).lexically_normal().u8string();
#ifdef _WIN32
  SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};HANDLE writer=nullptr;
  if(!CreatePipe(&runPipe,&writer,&security,0))throw std::runtime_error("Cannot create compiler output pipe.");
  SetHandleInformation(runPipe,HANDLE_FLAG_INHERIT,0);
  auto exe=runWide(executable),line=L"\""+exe+L"\" "+runWide(arguments);
  STARTUPINFOW startup{};startup.cb=sizeof startup;startup.dwFlags=STARTF_USESTDHANDLES;
  HANDLE reader=nullptr;
  if(arguments.find("-ftebroom-preview")!=std::string::npos){
    if(!CreatePipe(&reader,&runInput,&security,65536)){CloseHandle(writer);CloseHandle(runPipe);runPipe=nullptr;throw std::runtime_error("Cannot create preview control pipe.");}
    SetHandleInformation(runInput,HANDLE_FLAG_INHERIT,0);
  }
  startup.hStdOutput=startup.hStdError=writer;startup.hStdInput=reader?reader:GetStdHandle(STD_INPUT_HANDLE);
  PROCESS_INFORMATION process{};
  BOOL started=CreateProcessW(exe.c_str(),line.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|CREATE_SUSPENDED,nullptr,working.c_str(),&startup,&process);
  CloseHandle(writer);if(reader)CloseHandle(reader);
  if(!started && runInput){CloseHandle(runInput);runInput=nullptr;}
  if(!started){CloseHandle(runPipe);runPipe=nullptr;throw std::runtime_error("Cannot launch tool (Windows error "+std::to_string(GetLastError())+").");}
  runProcess=process.hProcess;runJob=CreateJobObjectW(nullptr,nullptr);
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if(!runJob || !SetInformationJobObject(runJob,JobObjectExtendedLimitInformation,&limits,sizeof limits) || !AssignProcessToJobObject(runJob,runProcess))
  {
    TerminateProcess(runProcess,1);CloseHandle(process.hThread);CloseHandle(runProcess);runProcess=nullptr;
    if(runJob){CloseHandle(runJob);runJob=nullptr;}CloseHandle(runPipe);runPipe=nullptr;
    if(runInput){CloseHandle(runInput);runInput=nullptr;}
    throw std::runtime_error("Cannot create a stoppable process group.");
  }
  ResumeThread(process.hThread);CloseHandle(process.hThread);
#else
  int descriptors[2];if(pipe(descriptors))throw std::runtime_error("Cannot create compiler output pipe.");
  std::vector<std::string> words{executable};std::istringstream args(arguments);std::string word;
  while(args>>std::quoted(word))words.push_back(word);std::vector<char*> argv;
  for(auto& value:words)argv.push_back(value.data());argv.push_back(nullptr);
  posix_spawn_file_actions_t fileActions;posix_spawn_file_actions_init(&fileActions);
  posix_spawn_file_actions_adddup2(&fileActions,descriptors[1],STDOUT_FILENO);posix_spawn_file_actions_adddup2(&fileActions,descriptors[1],STDERR_FILENO);
  posix_spawn_file_actions_addclose(&fileActions,descriptors[0]);posix_spawn_file_actions_addclose(&fileActions,descriptors[1]);
  posix_spawn_file_actions_addchdir_np(&fileActions,working.c_str());
  posix_spawnattr_t attributes;posix_spawnattr_init(&attributes);posix_spawnattr_setflags(&attributes,POSIX_SPAWN_SETPGROUP);posix_spawnattr_setpgroup(&attributes,0);
  int result=posix_spawnp(&runProcess,executable.c_str(),&fileActions,&attributes,argv.data(),environ);
  posix_spawnattr_destroy(&attributes);posix_spawn_file_actions_destroy(&fileActions);close(descriptors[1]);
  if(result){close(descriptors[0]);runProcess=-1;throw std::runtime_error("Cannot launch tool: "+std::to_string(result));}
  runPipe=descriptors[0];fcntl(runPipe,F_SETFL,O_NONBLOCK);
#endif
}
static void runBegin(const RunProfile& profile)
{
  if(runActive||runExclusive)return;
  runWindow=true;runError.clear();runHadError=false;runLog.clear();runStep=0;
  if(gameClassic() && std::any_of(profile.tasks.begin(),profile.tasks.end(),[](const RunTask& task){return task.kind==1 && task.source.find("q3map2")!=std::string::npos;})){
    runError="q3map2 cannot compile this game format. Use Compile / Setup compile for the active game profile.";runHadError=true;return;
  }
  if(documentFile.empty() && !profile.launch && profile.name!="Export selected BSP model"){runError="Save the map before running a profile.";return;}
  try
  {
    activeVariables=runVariables;
    activeVariables["MAP_BASE_NAME"]=documentFile.stem().u8string();activeVariables["MAP_FULL_NAME"]=documentFile.filename().u8string();
    activeVariables["MAP_DIR_PATH"]=ftebroom::pak::diskFolder(documentFile.parent_path()).u8string();activeVariables["GAME_DIR_PATH"]=baseq3;
    auto base=std::filesystem::u8path(baseq3);if(gameProfile==3){auto sibling=base.parent_path()/"baseq2";if(std::filesystem::is_directory(sibling))base=sibling;}activeVariables["BASE_GAME_DIR_PATH"]=base.u8string();
    activeVariables["GAME_DIR"]=baseq3;activeVariables["MODS"]="baseq3";
    activeVariables["CPU_COUNT"]=std::to_string(std::max(1u,std::thread::hardware_concurrency()));
    activeVariables["APP_DIR_PATH"]=documentPath("ftebroom.cfg").parent_path().parent_path().u8string();
    activeVariables["APP_DIR"]=activeVariables["APP_DIR_PATH"];
    activeVariables["WORK_DIR_PATH"]=ftebroom::pak::diskFolder(documentFile.parent_path()).u8string();activeVariables["WORK_DIR"]=activeVariables["WORK_DIR_PATH"];
    auto working=runExpand(profile.work);activeVariables["WORK_DIR_PATH"]=working;activeVariables["WORK_DIR"]=working;
    std::filesystem::create_directories(std::filesystem::u8path(working));
    if(!std::filesystem::is_directory(std::filesystem::u8path(working)))throw std::runtime_error("Working directory does not exist.");
    runningProfile=lastRunProfile=profile;runHaveLast=true;runActive=true;runAppend("Running "+profile.name+"\n");
  }
  catch(const std::exception& error){runError=error.what();runHadError=true;}
}
static std::filesystem::path runFilePath(const std::string& text)
{
  auto path=std::filesystem::u8path(text);
  if(path.is_relative())path=std::filesystem::u8path(activeVariables.at("WORK_DIR_PATH"))/path;
  return std::filesystem::weakly_canonical(path);
}
static bool runIsWorkingMap(const std::filesystem::path& path)
{
  std::error_code error;
  if(std::filesystem::equivalent(path,documentFile,error)&&!error)return true;
  auto left=std::filesystem::weakly_canonical(path).u8string();
  auto right=std::filesystem::weakly_canonical(documentFile).u8string();
#ifdef _WIN32
  return CompareStringOrdinal(runWide(left).c_str(),-1,runWide(right).c_str(),-1,TRUE)==CSTR_EQUAL;
#else
  return left==right;
#endif
}
static bool runGlobMatch(const std::string& pattern,const std::string& name)
{
  size_t p=0,n=0,star=std::string::npos,retry=0;
  while(n<name.size())
  {
    if(p<pattern.size()&&(pattern[p]=='?'||pattern[p]==name[n])){++p;++n;}
    else if(p<pattern.size()&&pattern[p]=='*'){star=p++;retry=n;}
    else if(star!=std::string::npos){p=star+1;n=++retry;}
    else return false;
  }
  while(p<pattern.size()&&pattern[p]=='*')++p;
  return p==pattern.size();
}
static void runFileTask(int kind,const std::string& source,const std::string& destination)
{
  auto from=runFilePath(source),to=runFilePath(destination);
  const auto pattern=from.filename().u8string();
  const bool wildcard=pattern.find_first_of("*?")!=std::string::npos;
  if(wildcard&&kind==3)throw std::runtime_error("Rename File requires one exact source path.");
  std::vector<std::filesystem::path> inputs;
  if(wildcard)
  {
    for(const auto& entry:std::filesystem::directory_iterator(from.parent_path()))
      if(entry.is_regular_file()&&runGlobMatch(pattern,entry.path().filename().u8string()))inputs.push_back(entry.path());
    std::sort(inputs.begin(),inputs.end());
    if(inputs.empty())throw std::runtime_error("No files match: "+source);
  }
  else inputs.push_back(from);
  // Validate the complete batch before any file is changed.
  std::vector<std::filesystem::path> outputs;
  for(const auto& path:inputs)
  {
    auto output=(wildcard||std::filesystem::is_directory(to))?to/path.filename():to;
    if(!std::filesystem::is_regular_file(path))throw std::runtime_error("File tasks accept regular files only.");
    if(runIsWorkingMap(path)||(kind!=4&&runIsWorkingMap(output)))throw std::runtime_error("File tasks must not replace or delete the working map.");
    outputs.push_back(output);
  }
  for(size_t i=0;i<inputs.size();++i)
  {
    if(kind==4)std::filesystem::remove(inputs[i]);
    else
    {
      std::filesystem::create_directories(outputs[i].parent_path());
      if(kind==2)std::filesystem::copy_file(inputs[i],outputs[i],std::filesystem::copy_options::overwrite_existing);
      else std::filesystem::rename(inputs[i],outputs[i]);
    }
  }
}
static void runTick()
{
  if(!runActive)return;
  try
  {
#ifdef _WIN32
    if(runProcess)
    {
      DWORD available=0;char buffer[4096];int batches=0;
      while(batches++<64 && PeekNamedPipe(runPipe,nullptr,0,nullptr,&available,nullptr)&&available){DWORD received=0;
        if(!ReadFile(runPipe,buffer,std::min<DWORD>(available,sizeof buffer),&received,nullptr)||!received)break;runAppend(std::string(buffer,received));}
      if(WaitForSingleObject(runProcess,0)==WAIT_TIMEOUT)return;
      if(PeekNamedPipe(runPipe,nullptr,0,nullptr,&available,nullptr)&&available)return;
      DWORD code=1;GetExitCodeProcess(runProcess,&code);CloseHandle(runProcess);runProcess=nullptr;
      CloseHandle(runPipe);runPipe=nullptr;if(runInput){CloseHandle(runInput);runInput=nullptr;}if(runJob){CloseHandle(runJob);runJob=nullptr;}
      if(code)throw std::runtime_error("Tool exited with code "+std::to_string(code));
      if(runStageHook)runStageHook();
      ++runStep;
    }
#else
    if(runProcess>0){char buffer[4096];ssize_t count;while((count=read(runPipe,buffer,sizeof buffer))>0)runAppend(std::string(buffer,count));
      int status=0;auto waited=waitpid(runProcess,&status,WNOHANG);if(waited==0 || (waited<0&&errno==EINTR))return;runProcess=-1;close(runPipe);runPipe=-1;
      if(waited<0)throw std::runtime_error("Cannot read tool exit status. Start with Launch-ReyondBroom.sh so the editor owns compiler processes.");
      if(!WIFEXITED(status)||WEXITSTATUS(status))throw std::runtime_error("Tool failed.");if(runStageHook)runStageHook();++runStep;}
#endif
    if(runExportWaiting)
    {
      int status=(int)cvars->GetFloat("ftebroom_export_status");if(!status)return;
      if(status!=1)throw std::runtime_error("Map export failed.");
      auto target=runFilePath(runExpand(runningProfile.tasks[runStep].source));
      if(runIsWorkingMap(target))throw std::runtime_error("Export Map must use a build copy, not the working map.");
      std::filesystem::create_directories(target.parent_path());
      std::filesystem::copy_file(documentPath("data/export-map.map"),target,std::filesystem::copy_options::overwrite_existing);
      if(gameWad()){auto converted=ftebroom::quake1::map(q1Read(target),false,q1CompilerWads(target.parent_path(),q1Read(target)),false,gameProfile==4);
        for(const auto& texture:ftebroom::quake1::mapTextures(converted))if(!materials.count("textures/"+texture))throw std::runtime_error("Missing Quake texture '"+texture+"'. Add its WAD in Game / resources.");
        std::ofstream output(target,std::ios::binary);output<<converted;output.close();if(!output)throw std::runtime_error("Cannot write Quake build snapshot.");}
      if(gameProfile==3){auto texture=target.parent_path()/"textures/fb_grey.wal";std::filesystem::create_directories(texture.parent_path());auto bytes=modelResource("textures/fb_grey.wal");std::ofstream f(texture,std::ios::binary);f.write(bytes.data(),bytes.size());if(bytes.empty()||!f)throw std::runtime_error("Cannot stage default Quake II material.");}
      if(runExportHook)runExportHook(target);
      runExportWaiting=false;++runStep;
    }
    if(runStep>=(int)runningProfile.tasks.size()){runActive=false;runAppend("Completed successfully.\n");return;}
    if(!runningProfile.tasks[runStep].enabled){runAppend("Skipped disabled step "+std::to_string(++runStep)+"\n");return;}
    const auto& task=runningProfile.tasks[runStep];auto source=runExpand(task.source),destination=runExpand(task.destination);
    runAppend("Step "+std::to_string(runStep+1)+": "+source+" "+destination+"\n");
    if(task.kind==0){cvars->SetFloat("ftebroom_export_status",0);command(task.destination=="@selected-brushes"?"ftebroom_export_selected_bsp\n":"ftebroom_export_map\n");runExportWaiting=true;return;}
    if(task.kind==1){runStartProcess(source,destination,std::filesystem::u8path(activeVariables["WORK_DIR_PATH"]));return;}
    runFileTask(task.kind,source,destination);
    ++runStep;
  }
  catch(const std::exception& error){runError=error.what();runHadError=true;runActive=runExportWaiting=false;runAppend("ERROR: "+runError+"\n");}
}
static bool runText(const char* label,std::string& value)
{
  std::vector<char> buffer(std::max<size_t>(4096,value.size()+256));std::copy(value.begin(),value.end(),buffer.begin());
  if(ImGui::InputText(label,buffer.data(),buffer.size())){value=buffer.data();return true;}return false;
}
static void runPanel()
{
  if(!runWindow)return;runLoad();
  ImGui::SetNextWindowSize({800,650},ImGuiCond_FirstUseEver);
  if(ImGui::Begin("Compile / Launch",&runWindow,uiPanelFlags()))
  {
    ImGui::BeginDisabled(runActive);
    static int previousGame=-1;
    if(previousGame!=gameProfile){previousGame=gameProfile;for(int i=0;i<(int)runProfiles.size();++i)if(runProfiles[i].name.rfind(gameProfile==4?"HL:":gameProfile==3?"Q2:":gameProfile==2?"Q1:":"Q3:",0)==0){runSelected=i;break;}}
    runSelected=std::clamp(runSelected,0,(int)runProfiles.size()-1);
    if(ImGui::BeginCombo("Profile",runProfiles[runSelected].name.c_str())){for(int i=0;i<(int)runProfiles.size();++i)if(ImGui::Selectable(runProfiles[i].name.c_str(),i==runSelected))runSelected=i;ImGui::EndCombo();}
    if(ImGui::Button("New profile")){runProfiles.push_back(RunProfile{});runSelected=(int)runProfiles.size()-1;}
    ImGui::SameLine();if(ImGui::Button("Save profiles"))runSaveProfiles();
    auto& profile=runProfiles[runSelected];runText("Name",profile.name);runText("Working directory",profile.work);ImGui::Checkbox("Engine launch profile",&profile.launch);
    const char* kinds[]={"Export Map","Run Tool","Copy Files","Rename File","Delete Files"};
    if(ImGui::BeginChild("Profile tasks",{0,200},ImGuiChildFlags_Borders))
    {
      for(size_t i=0;i<profile.tasks.size();++i){ImGui::PushID((int)i);auto& task=profile.tasks[i];
        ImGui::Checkbox("Enabled",&task.enabled);
        ImGui::Combo("Task",&task.kind,kinds,5);runText(task.kind==1?"Executable":"Source / export path",task.source);
        if(task.kind==2||task.kind==4)ImGui::TextDisabled("Filename patterns: * and ? (current folder only)");
        if(task.kind!=0&&task.kind!=4)runText(task.kind==1?"Arguments":"Destination",task.destination);
        if(ImGui::Button("Remove")){profile.tasks.erase(profile.tasks.begin()+i);ImGui::PopID();break;}
        ImGui::SameLine();if(i&&ImGui::Button("Move up"))std::swap(profile.tasks[i],profile.tasks[i-1]);ImGui::Separator();ImGui::PopID();}
      if(ImGui::Button("Add task"))profile.tasks.push_back(RunTask{});
    }
    ImGui::EndChild();
    if(ImGui::CollapsingHeader("Tool paths and variables")){
      for(auto& variable:runVariables)runText(variable.first.c_str(),variable.second);
      static char name[128]={};ImGui::InputText("New variable",name,sizeof name);ImGui::SameLine();if(ImGui::Button("Add variable")&&*name){runVariables[name]="";name[0]=0;}
      ImGui::TextWrapped("Variables: ${MAP_BASE_NAME}, ${MAP_FULL_NAME}, ${MAP_DIR_PATH}, ${GAME_DIR_PATH}, ${MODS}, ${CPU_COUNT}, ${WORK_DIR_PATH}, ${APP_DIR_PATH}. Quote path variables in arguments.");}
    if(ImGui::Button("Run profile")&&runSaveProfiles())runBegin(profile);ImGui::EndDisabled();
    ImGui::SameLine();ImGui::BeginDisabled(!runActive);if(ImGui::Button("Stop"))runStop();ImGui::EndDisabled();
    ImGui::SameLine();if(ImGui::Button("Copy log"))ImGui::SetClipboardText(runLog.c_str());
    if(!runError.empty())ImGui::TextColored({1,.3f,.25f,1},"%s",runError.c_str());
    if(ImGui::BeginChild("Tool output",{0,0},ImGuiChildFlags_Borders)){std::istringstream lines(runLog);std::string line;
      while(std::getline(lines,line)){bool error=line.find("ERROR")!=std::string::npos||line.find("error")!=std::string::npos||line.find("Error")!=std::string::npos;
        if(error)ImGui::TextColored({1,.3f,.25f,1},"%s",line.c_str());else ImGui::TextUnformatted(line.c_str());}}
    ImGui::EndChild();
  }
  ImGui::End();
}
static void runAction(const std::string& operation)
{
  runLoad();runWindow=true;
  if(operation=="rerun"){if(runHaveLast)runBegin(lastRunProfile);return;}
  if(operation=="launchfte"){for(const auto& profile:runProfiles)if(profile.name=="Launch in FTE"){runBegin(profile);return;}}
  for(int i=0;i<(int)runProfiles.size();++i)if(runProfiles[i].launch==(operation=="launch")){runSelected=i;break;}
}
static void testRunProfiles()
{
  if(!cvars->GetFloat("ftebroom_skip_setup"))return;
  char argument[16]={};commands->Argv(1,argument,sizeof argument);int stage=std::atoi(argument);
  auto expect=[](bool ok,const char* name){core->Print((std::string(ok?"FTEBROOM_RUN_PASS ":"FTEBROOM_RUN_FAIL ")+name+"\n").c_str());};
  auto root=documentPath("maps/run-test");std::filesystem::create_directories(root);
#ifdef _WIN32
  wchar_t system[MAX_PATH]={};GetSystemDirectoryW(system,MAX_PATH);auto shell=(std::filesystem::path(system)/"cmd.exe").u8string();
#else
  std::string shell="/bin/sh";
#endif
  if(stage==0)
  {
    std::ofstream(root/"source.txt")<<"pipeline fixture";
    RunProfile profile{"Pipeline test",root.u8string(),false,{{0,"${WORK_DIR_PATH}/build.map",""},
#ifdef _WIN32
      {1,shell,"/d /c echo FTEBROOM_TOOL_OUTPUT"},
#else
      {1,"/bin/echo","FTEBROOM_TOOL_OUTPUT"},
#endif
      {2,"source.txt","copy.txt"},
      {3,"copy.txt","renamed.txt"},
      {4,"renamed.txt",""},{1,"missing-disabled-tool","",false}}};
    runLoad();runBegin(profile);
  }
  else if(stage==1)
  {
    expect(!runActive&&!runHadError&&runStep==6 && runLog.find("Skipped disabled step")!=std::string::npos,"all_five_tasks");
    expect(runLog.find("FTEBROOM_TOOL_OUTPUT")!=std::string::npos,"streamed_output");
    expect(std::filesystem::exists(root/"build.map")&&!std::filesystem::exists(root/"copy.txt")&&!std::filesystem::exists(root/"renamed.txt"),"export_copy_rename_delete");
    expect(activeVariables["MAP_FULL_NAME"]==documentFile.filename().u8string()&&!activeVariables["CPU_COUNT"].empty(),"variables");
    expect(runIsWorkingMap(documentFile.parent_path()/"."/documentFile.filename()),"working_map_path_alias_protected");
    expect(runFilePath("../run-test/source.txt")==std::filesystem::weakly_canonical(root/"source.txt"),"relative_paths_use_profile_working_directory");
    runFileTask(2,"*.txt","batch");
    expect(std::filesystem::exists(root/"batch/source.txt"),"copy_file_pattern");
    runFileTask(4,"batch/*.txt","");
    expect(!std::filesystem::exists(root/"batch/source.txt"),"delete_file_pattern");
  }
  else if(stage==2)
  {
#ifdef _WIN32
    runBegin({"Failing tool",root.u8string(),false,{{1,shell,"/d /c exit /b 7"}}});
#else
    runBegin({"Failing tool",root.u8string(),false,{{1,"/bin/false",""}}});
#endif
  }
  else if(stage==3)expect(!runActive&&runHadError,"nonzero_exit_stops_pipeline");
  else if(stage==4)
  {
#ifdef _WIN32
    runBegin({"Stop test",root.u8string(),false,{{1,shell,"/d /c ping -n 30 127.0.0.1"}}});
#else
    runBegin({"Stop test",root.u8string(),false,{{1,"/bin/sleep","30"}}});
#endif
  }
  else if(stage==5){runStop();expect(!runActive,"stop_process_tree");}
  else if(stage==6)
  {
    char compiler[4096]={};cvars->GetString("ftebroom_test_compiler",compiler,sizeof compiler);
    runBegin({"Actual q3map2 test",root.u8string(),false,{{0,"build.map",""},
      {1,compiler,"-game quake3 -fs_basepath \"${WORK_DIR_PATH}\" -meta \"${WORK_DIR_PATH}/build.map\""}}});
  }
  else if(stage==7)
  {
    expect(!runActive&&!runHadError&&runStep==2,"real_q3map2_profile");
    expect(std::filesystem::exists(root/"build.bsp")&&std::filesystem::file_size(root/"build.bsp")>100,"real_q3map2_bsp");
  }
}
