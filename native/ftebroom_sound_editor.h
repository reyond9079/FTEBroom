// SPDX-License-Identifier: GPL-2.0-or-later
#include <future>
#include <chrono>
static bool soundFile(const std::filesystem::path& p){auto e=entityLower(p.extension().u8string());return e==".wav"||e==".ogg"||e==".mp3"||e==".flac";}
static bool soundEditorOpen=false,soundPlaying=false,soundTrim=false,soundStreamQueued=false;
static std::filesystem::path soundEditorPath;
static std::string soundOriginal,soundMessage;
static std::vector<int16_t> soundPCM;
static std::array<ImVec2,1024> soundPeaks;
static float soundVolume=1,soundSpeed=1,soundStart=0,soundEnd=0;
static double soundPlayCursor=0,soundQueueClock=0;
static int soundTool=0;
static ImVec2 soundVolumeMin,soundVolumeMax;
static plugaudiofuncs_t* soundAPI;
struct SoundJobResult {std::string bytes,error;bool save=false;};
static std::future<SoundJobResult> soundJob;
static bool soundBusy(){return soundJob.valid();}
static std::string soundRead(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Cannot read audio conversion output");return std::string(std::istreambuf_iterator<char>(f),{});}
static void soundWrite(const std::filesystem::path& p,const std::string& bytes){std::ofstream f(p,std::ios::binary);f.write(bytes.data(),bytes.size());if(!f)throw std::runtime_error("Cannot write temporary audio file");}
#ifdef _WIN32
static std::wstring soundQuote(const std::wstring& value){std::wstring out=L"\"";int slashes=0;for(auto c:value){if(c==L'\\'){slashes++;continue;}if(c==L'"')out.append(slashes*2+1,L'\\');else out.append(slashes,L'\\');slashes=0;out+=c;}out.append(slashes*2,L'\\');return out+L"\"";}
#endif
static void soundConvert(const std::filesystem::path& exe,const std::vector<std::string>& args){
#ifdef _WIN32
  std::wstring line=soundQuote(exe.wstring());for(auto& a:args)line+=L" "+soundQuote(std::filesystem::u8path(a).wstring());
  STARTUPINFOW si{};si.cb=sizeof si;PROCESS_INFORMATION pi{};
  if(!CreateProcessW(exe.c_str(),line.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi))throw std::runtime_error("Cannot start bundled audio converter");
  CloseHandle(pi.hThread);auto wait=WaitForSingleObject(pi.hProcess,120000);DWORD code=1;
  if(wait!=WAIT_OBJECT_0){TerminateProcess(pi.hProcess,1);WaitForSingleObject(pi.hProcess,5000);}else GetExitCodeProcess(pi.hProcess,&code);
  CloseHandle(pi.hProcess);if(code)throw std::runtime_error("Audio conversion failed or exceeded two minutes");
#else
  std::vector<std::string> values{exe.string()};values.insert(values.end(),args.begin(),args.end());std::vector<char*> argv;for(auto& v:values)argv.push_back(v.data());argv.push_back(nullptr);pid_t pid;
  if(posix_spawn(&pid,exe.c_str(),nullptr,nullptr,argv.data(),environ))throw std::runtime_error("Cannot start audio converter");
  int status=0;auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(120);
  for(;;){auto waited=waitpid(pid,&status,WNOHANG);if(waited==pid)break;if(waited<0&&errno!=EINTR)throw std::runtime_error("Cannot read audio converter exit status. Use Launch-ReyondBroom.sh.");if(std::chrono::steady_clock::now()>deadline){kill(pid,SIGKILL);waitpid(pid,&status,0);throw std::runtime_error("Audio conversion timed out");}std::this_thread::sleep_for(std::chrono::milliseconds(20));}
  if(!WIFEXITED(status)||WEXITSTATUS(status))throw std::runtime_error("Audio conversion failed");
#endif
}
static void soundStop(){soundPlaying=false;if(soundAPI&&soundStreamQueued)soundAPI->RawAudio(1836,nullptr,44100,0,2,2,1);soundStreamQueued=false;}
static double soundDuration(){return double(soundPCM.size()/2)/44100;}
static double soundRangeStart(){return soundTool==1?soundStart:0;}
static double soundRangeEnd(){return soundTool==1?soundEnd:soundDuration();}
static void soundSeek(double seconds,bool play){soundStop();soundPlayCursor=std::clamp(seconds,0.0,soundDuration())*44100;soundQueueClock=ImGui::GetTime();soundPlaying=play&&soundAPI&&soundPlayCursor<soundDuration()*44100;}
static void soundWave(){
  size_t frames=soundPCM.size()/2;
  for(size_t b=0;b<soundPeaks.size();b++){float lo=0,hi=0;size_t first=b*frames/soundPeaks.size(),end=(b+1)*frames/soundPeaks.size();for(size_t f=first;f<end;f++)for(int ch=0;ch<2;ch++){float v=soundPCM[f*2+ch]/32768.f;lo=std::min(lo,v);hi=std::max(hi,v);}soundPeaks[b]={lo,hi};}
}
static void soundLaunch(bool save){
  if(soundBusy())return;soundStop();
  auto exe=documentPath("ftebroom.cfg").parent_path().parent_path()/"tools/audio/ffmpeg.exe";
#ifndef _WIN32
  exe.replace_extension();
#endif
  auto source=soundOriginal,ext=entityLower(soundEditorPath.extension().u8string());
  unsigned rate=44100,channels=2;
  if(ext==".wav"&&source.size()>=12&&source.compare(0,4,"RIFF")==0&&source.compare(8,4,"WAVE")==0) {
    auto u32=[&](size_t at){unsigned value=0;for(int i=0;i<4;i++)value|=unsigned((unsigned char)source[at+i])<<(8*i);return value;};
    for(size_t at=12;at+8<=source.size();) {unsigned length=u32(at+4);if(length>source.size()-at-8)break;
      if(source.compare(at,4,"fmt ")==0&&length>=16){channels=(unsigned char)source[at+10]|unsigned((unsigned char)source[at+11])<<8;rate=u32(at+12);break;}at+=8+length+(length&1);}
    if(rate<8000||rate>192000||channels<1||channels>2){soundMessage="Only mono/stereo WAV files between 8 and 192 kHz can be edited";return;}
  }
  std::string pcm;if(save)pcm.assign((const char*)soundPCM.data(),soundPCM.size()*sizeof(int16_t));
  float start=soundTrim?soundStart:0,end=soundTrim?soundEnd:float(soundPCM.size()/2)/44100,volume=soundVolume,speed=soundSpeed;
  soundMessage=save?"Saving audio...":"Loading audio...";
  soundJob=std::async(std::launch::async,[exe,source=std::move(source),pcm=std::move(pcm),ext,save,start,end,volume,speed,rate,channels](){
    SoundJobResult result;result.save=save;std::filesystem::path folder;
    try {
      folder=std::filesystem::temp_directory_path()/("ReyondBroom-audio-"+std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
      if(!std::filesystem::create_directory(folder))throw std::runtime_error("Cannot create private audio workspace");
      auto input=folder/(save?"input.pcm":"input"+ext),output=folder/(save?"output"+ext:"output.pcm");soundWrite(input,save?pcm:source);
      std::vector<std::string> args={"-nostdin","-hide_banner","-loglevel","error","-y"};
      if(save){for(auto v:{"-f","s16le","-ar","44100","-ac","2"})args.push_back(v);}
      args.insert(args.end(),{"-i",input.u8string(),"-vn"});
      if(save){std::string filter="atrim=start="+std::to_string(start)+":end="+std::to_string(end)+",asetpts=PTS-STARTPTS,asetrate="+std::to_string(44100*speed)+",aresample=44100,volume="+std::to_string(volume);args.insert(args.end(),{"-af",filter,"-c:a",ext==".ogg"?"vorbis":ext==".mp3"?"libmp3lame":ext==".flac"?"flac":"pcm_s16le"});if(ext==".ogg")args.insert(args.end(),{"-strict","-2"});}
      else args.insert(args.end(),{"-f","s16le","-c:a","pcm_s16le","-ar","44100","-ac","2","-fs","268435456"});
      if(save&&ext==".wav")args.insert(args.end(),{"-ar",std::to_string(rate),"-ac",std::to_string(channels)});
      args.push_back(output.u8string());soundConvert(exe,args);
      if(!std::filesystem::exists(output)||std::filesystem::file_size(output)>=268435456)throw std::runtime_error("Decoded audio exceeds 256 MiB");
      result.bytes=soundRead(output);if(result.bytes.empty())throw std::runtime_error("Audio output is empty");
    }catch(const std::exception& e){result.error=e.what();}
    if(!folder.empty()){std::error_code error;std::filesystem::remove_all(folder,error);}return result;
  });
}
static void soundOpen(const std::filesystem::path& path){
  if(soundBusy()){soundMessage="Wait for the current audio operation";return;}
  try {modFileValidate(path);auto bytes=modFileBytes(path);soundStop();soundEditorPath=path;soundOriginal=std::move(bytes);soundPCM.clear();soundEditorOpen=true;soundVolume=soundSpeed=1;soundStart=soundEnd=0;soundTrim=false;soundTool=0;soundPlayCursor=0;soundLaunch(false);}catch(const std::exception& e){modFilesError=e.what();}
}
static void soundFinish(){
  if(!soundBusy()||soundJob.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
  auto result=soundJob.get();if(!result.error.empty()){soundMessage=result.error;return;}
  if(result.save){try {
    modFileValidate(soundEditorPath);if(modFileBytes(soundEditorPath)!=soundOriginal)throw std::runtime_error("Source changed outside the editor; reopen it before saving");
    if(modFileMember(soundEditorPath))ftebroom::pak::replace(soundEditorPath,result.bytes);
    else {auto backup=soundEditorPath;backup+=".ftebroom-"+std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count())+".bak";
      auto temporary=backup;temporary+=".tmp";soundWrite(temporary,result.bytes);
#ifdef _WIN32
      if(!ReplaceFileW(soundEditorPath.c_str(),temporary.c_str(),backup.c_str(),0,nullptr,nullptr)){std::error_code error;std::filesystem::remove(temporary,error);throw std::runtime_error("Could not replace sound; original is unchanged");}
#else
      std::filesystem::copy_file(soundEditorPath,backup);std::filesystem::rename(temporary,soundEditorPath);
#endif
    }
    soundOriginal=std::move(result.bytes);soundVolume=soundSpeed=1;soundTrim=false;modFilesRefresh=true;soundLaunch(false);
  }catch(const std::exception& e){soundMessage=e.what();}}
  else {soundPCM.resize(result.bytes.size()/2);std::memcpy(soundPCM.data(),result.bytes.data(),soundPCM.size()*2);soundStart=0;soundEnd=float(soundPCM.size()/2)/44100;soundPlayCursor=0;soundWave();soundMessage="";}
}
static void soundPanel(){
  soundFinish();if(!soundEditorOpen){soundStop();return;}
  if(!soundAPI)soundAPI=(plugaudiofuncs_t*)core->GetEngineInterface(plugaudiofuncs_name,sizeof(plugaudiofuncs_t));
  ImGui::SetNextWindowSize({780,430},ImGuiCond_FirstUseEver);
  if(ImGui::Begin("Sound editor",&soundEditorOpen)) {
    ImGui::TextWrapped("%s",soundEditorPath.u8string().c_str());
    ImGui::BeginDisabled(soundBusy()||soundPCM.empty());
    if(ImGui::RadioButton("Seek / playback",soundTool==0)){soundStop();soundTool=0;}
    ImGui::SameLine();if(ImGui::RadioButton("Trim",soundTool==1)){soundStop();soundTool=1;soundTrim=true;}
    float duration=float(soundPCM.size()/2)/44100;auto p=ImGui::GetCursorScreenPos();ImVec2 sz={std::max(64.f,ImGui::GetContentRegionAvail().x),160*uiScale()};
    ImGui::InvisibleButton("Waveform",sz);auto d=ImGui::GetWindowDrawList();d->PushClipRect(p,{p.x+sz.x,p.y+sz.y},true);d->AddRectFilled(p,{p.x+sz.x,p.y+sz.y},IM_COL32(25,29,35,255));
    if(duration>0){float left=p.x+sz.x*soundStart/duration,right=p.x+sz.x*soundEnd/duration;if(soundTrim||soundTool==1)d->AddRectFilled({left,p.y},{right,p.y+sz.y},IM_COL32(55,80,110,180));
      for(size_t b=0;b<soundPeaks.size();b++){float x=p.x+sz.x*b/soundPeaks.size();d->AddLine({x,p.y+sz.y*(.5f-.45f*std::clamp(soundPeaks[b].y*soundVolume,-1.f,1.f))},{x,p.y+sz.y*(.5f-.45f*std::clamp(soundPeaks[b].x*soundVolume,-1.f,1.f))},IM_COL32(110,200,225,255));}
      static float anchor=0;if(ImGui::IsItemActivated())anchor=std::clamp((ImGui::GetIO().MousePos.x-p.x)/sz.x,0.f,1.f)*duration;
      if(ImGui::IsItemActive()&&(ImGui::IsItemActivated()||ImGui::GetIO().MouseDelta.x!=0)){float current=std::clamp((ImGui::GetIO().MousePos.x-p.x)/sz.x,0.f,1.f)*duration;if(soundTool==1){soundStart=std::min(anchor,current);soundEnd=std::max(anchor,current);soundTrim=true;soundStop();}else soundSeek(current,true);}
      {float x=p.x+sz.x*float(soundPlayCursor/44100)/duration;d->AddLine({x,p.y},{x,p.y+sz.y},IM_COL32(255,210,75,255),2);}}
    d->PopClipRect();
    ImGui::SliderFloat("Volume",&soundVolume,0,2,"%.2fx");soundVolumeMin=ImGui::GetItemRectMin();soundVolumeMax=ImGui::GetItemRectMax();ImGui::SliderFloat("Speed",&soundSpeed,.25f,4,"%.2fx");
    if(soundTool==1&&ImGui::DragFloatRange2("Selection",&soundStart,&soundEnd,.01f,0,std::max(.01f,duration),"Start %.3f s","End %.3f s"))soundStop();
    if(soundTool==1)ImGui::Checkbox("Apply trim when saving",&soundTrim);ImGui::Text("%.3f s / %.3f s",std::max(0.f,soundEnd-soundStart)/soundSpeed,duration);
    ImGui::BeginDisabled(soundRangeEnd()<=soundRangeStart()||!soundAPI);if(ImGui::Button(soundPlaying?"Pause":"Play")){if(soundPlaying)soundStop();else{double cursor=soundPlayCursor/44100;if(cursor<soundRangeStart()||cursor>=soundRangeEnd())cursor=soundRangeStart();soundSeek(cursor,true);}}ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Stop")){soundStop();soundPlayCursor=soundRangeStart()*44100;}
    ImGui::SameLine();ImGui::BeginDisabled(soundTrim&&soundEnd<=soundStart);if(ImGui::Button("Save with backup"))soundLaunch(true);ImGui::EndDisabled();ImGui::EndDisabled();
    if(!soundMessage.empty())ImGui::TextWrapped("%s",soundMessage.c_str());
    ImGui::TextWrapped("Save replaces the original file. A backup is retained. Speed also changes pitch.");
  }ImGui::End();
  if(!soundEditorOpen)soundStop();
  if(soundPlaying&&soundAPI){double now=ImGui::GetTime();while(soundQueueClock<now+.06&&soundPlaying){std::array<int16_t,2048> samples;int frames=0;while(frames<1024&&soundPlayCursor<std::min(soundRangeEnd()*44100,double(soundPCM.size()/2))){size_t pos=size_t(soundPlayCursor);for(int ch=0;ch<2;ch++)samples[frames*2+ch]=int16_t(std::clamp(soundPCM[pos*2+ch]*soundVolume,-32768.f,32767.f));frames++;soundPlayCursor+=soundSpeed;}if(frames){soundAPI->RawAudio(1836,samples.data(),44100,frames,2,2,1);soundStreamQueued=true;}soundQueueClock+=double(frames)/44100;if(frames<1024)soundPlaying=false;}}
}
