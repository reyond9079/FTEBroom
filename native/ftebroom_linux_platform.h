// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef _WIN32
#include <future>
#include <cerrno>
#include <filesystem>
#include <chrono>
#include <spawn.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <signal.h>
extern char** environ;
static std::string linuxClipboardDirectory;
static std::future<std::string> linuxClipboardRead;
static std::future<std::string> linuxClipboardWrite;
static void (*linuxClipboardCallback)(void*,const char*);
static void* linuxClipboardContext;
static std::string linuxTextClipboard;
static ImGuiID linuxPasteTarget;
static bool linuxPasteReady,linuxPasteReplay,linuxPasteRelease,linuxPasteArm;
static std::array<std::string,ImGuiMouseCursor_COUNT> linuxCursorNames;
static std::array<ImVec2,ImGuiMouseCursor_COUNT> linuxCursorHotspots;
static int linuxCursorShape(int cursor)
{
  switch(cursor){
    case ImGuiMouseCursor_TextInput:return 9;
    case ImGuiMouseCursor_ResizeAll:return 32;
    case ImGuiMouseCursor_ResizeNS:return 27;
    case ImGuiMouseCursor_ResizeEW:return 26;
    case ImGuiMouseCursor_ResizeNESW:return 28;
    case ImGuiMouseCursor_ResizeNWSE:return 29;
    case ImGuiMouseCursor_Hand:return 4;
    case ImGuiMouseCursor_Wait:return 6;
    case ImGuiMouseCursor_Progress:return 5;
    case ImGuiMouseCursor_NotAllowed:return 15;
    default:return 1;
  }
}
static bool linuxTestWindow,linuxTestFocus;
static char linuxTestText[4096]="replace me";
static void linuxPlatformTestPanel(){if(!linuxTestWindow)return;ImGui::SetNextWindowSize({600,260},ImGuiCond_Once);if(linuxTestFocus)ImGui::SetNextWindowFocus();ImGui::Begin("Linux platform regression");if(linuxTestFocus){ImGui::SetKeyboardFocusHere();linuxTestFocus=false;}ImGui::InputTextMultiline("Text",linuxTestText,sizeof linuxTestText);ImGui::End();}
static bool linuxWayland(){const char* value=std::getenv("WAYLAND_DISPLAY");return value&&*value;}
// No shell: clipboard contents are data, never command arguments.
static std::string linuxClipboardProcess(const std::string& executable,const std::string& text,bool copy)
{
  int output[2];if(pipe(output))return {};
  char name[]="/tmp/ftebroom-clipboard-XXXXXX";int source=mkstemp(name);
  if(source<0){close(output[0]);close(output[1]);return {};}
  unlink(name);
  if(copy){size_t done=0;while(done<text.size()){auto n=write(source,text.data()+done,text.size()-done);if(n<=0)break;done+=n;}lseek(source,0,SEEK_SET);}
  posix_spawn_file_actions_t actions;posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_adddup2(&actions,source,STDIN_FILENO);
  posix_spawn_file_actions_adddup2(&actions,output[1],STDOUT_FILENO);
  posix_spawn_file_actions_addclose(&actions,output[0]);posix_spawn_file_actions_addclose(&actions,output[1]);posix_spawn_file_actions_addclose(&actions,source);
  char* copyArgs[]={const_cast<char*>(executable.c_str()),const_cast<char*>("--type"),const_cast<char*>("text/plain;charset=utf-8"),nullptr};
  char* pasteArgs[]={const_cast<char*>(executable.c_str()),const_cast<char*>("--no-newline"),const_cast<char*>("--type"),const_cast<char*>("text"),nullptr};
  char** args=copy?copyArgs:pasteArgs;
  pid_t child;int error=posix_spawn(&child,executable.c_str(),&actions,nullptr,args,environ);
  posix_spawn_file_actions_destroy(&actions);close(source);close(output[1]);
  if(error){close(output[0]);return {};}
  fcntl(output[0],F_SETFL,O_NONBLOCK);
  std::string result;auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(4);
  bool finished=false;int status=0;
  while(std::chrono::steady_clock::now()<deadline){
    char buffer[8192];ssize_t count;
    while((count=read(output[0],buffer,sizeof buffer))>0){result.append(buffer,count);if(result.size()>16*1024*1024)break;}
    if(result.size()>16*1024*1024)break;
    auto waited=waitpid(child,&status,WNOHANG);if(waited==child){finished=true;break;}if(waited<0&&errno!=EINTR)break;
    pollfd fd{output[0],POLLIN,0};poll(&fd,1,10);
  }
  if(!finished){kill(child,SIGKILL);while(waitpid(child,&status,0)<0&&errno==EINTR){}}
  if(finished){char buffer[8192];ssize_t count;while((count=read(output[0],buffer,sizeof buffer))>0&&result.size()<=16*1024*1024)result.append(buffer,count);}
  close(output[0]);
  if(result.size()>16*1024*1024)return {};
  return finished&&WIFEXITED(status)&&WEXITSTATUS(status)==0?result:std::string{};
}
static void linuxClipboardGet(void (*callback)(void*,const char*),void* context)
{
  if(!linuxWayland()){if(input->ClipboardGet)input->ClipboardGet(0,callback,context);return;}
  if(linuxClipboardRead.valid())return;
  linuxClipboardCallback=callback;linuxClipboardContext=context;
  const auto path=linuxClipboardDirectory+"/wl-paste";
  linuxClipboardRead=std::async(std::launch::async,[path](){return linuxClipboardProcess(path,{},false);});
}
static void linuxClipboardSet(const char* text)
{
  if(!linuxWayland()){if(input->ClipboardSet)input->ClipboardSet(0,text);return;}
  if(linuxClipboardWrite.valid()&&linuxClipboardWrite.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
  if(linuxClipboardWrite.valid())linuxClipboardWrite.get();
  const auto path=linuxClipboardDirectory+"/wl-copy";std::string value=text?text:"";
  if(value.size()>16*1024*1024)return;
  linuxClipboardWrite=std::async(std::launch::async,[path,value](){return linuxClipboardProcess(path,value,true);});
}
static void linuxTextReceived(void*,const char* text){linuxTextClipboard=text?text:"";linuxPasteReady=true;}
static void linuxPlatformPump(bool ctrl)
{
  if(linuxClipboardRead.valid()&&linuxClipboardRead.wait_for(std::chrono::seconds(0))==std::future_status::ready){auto text=linuxClipboardRead.get();if(linuxClipboardCallback)linuxClipboardCallback(linuxClipboardContext,text.c_str());}
  auto& io=ImGui::GetIO();
  if(linuxPasteRelease){io.AddKeyEvent(ImGuiKey_V,false);io.AddKeyEvent(ImGuiMod_Ctrl,ctrl);linuxPasteRelease=false;}
  if(linuxPasteTarget!=ImGui::GetActiveID()){linuxPasteReplay=false;linuxPasteArm=false;}
  if(linuxPasteArm){linuxPasteArm=false;linuxPasteReplay=true;io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_V,true);linuxPasteRelease=true;}
  if(linuxPasteReady){linuxPasteReady=false;if(linuxPasteTarget&&ImGui::GetActiveID()==linuxPasteTarget){io.AddKeyEvent(ImGuiKey_V,false);linuxPasteArm=true;}}
}
static const char* linuxImGuiClipboardGet(ImGuiContext*)
{
  if(linuxPasteReplay){linuxPasteReplay=false;return linuxTextClipboard.c_str();}
  linuxPasteTarget=ImGui::GetActiveID();linuxClipboardGet(linuxTextReceived,nullptr);return nullptr;
}
static void linuxImGuiClipboardSet(ImGuiContext*,const char* text){linuxClipboardSet(text);}
static void linuxPlatformInitialize(plugfsfuncs_t* fs,ImFontAtlas* atlas,const unsigned char* pixels,int width,int height)
{
  char path[4096]={};fs->NativePath("default.fmf",(fs_relative)5,path,sizeof path);
  linuxClipboardDirectory=(std::filesystem::u8path(path).parent_path().parent_path()/"tools/clipboard").u8string();
  auto& platform=ImGui::GetPlatformIO();platform.Platform_GetClipboardTextFn=linuxImGuiClipboardGet;platform.Platform_SetClipboardTextFn=linuxImGuiClipboardSet;
  if(!pixels||!atlas)return;
  for(int cursor=0;cursor<ImGuiMouseCursor_COUNT;++cursor){
    ImVec2 hot,size,border[2],fill[2];
    if(!ImFontAtlasGetMouseCursorTexData(atlas,cursor,&hot,&size,border,fill))continue;
    int cw=(int)size.x,ch=(int)size.y;
    int bx=(int)(border[0].x*width+.5f),by=(int)(border[0].y*height+.5f);
    int fx=(int)(fill[0].x*width+.5f),fy=(int)(fill[0].y*height+.5f);
    if(cw<=0||ch<=0)continue;
    std::vector<unsigned char> tga(18+cw*ch*4,0);
    tga[2]=2;tga[12]=(unsigned char)cw;tga[14]=(unsigned char)ch;tga[16]=32;tga[17]=0x28;
    for(int y=0;y<ch;y++)for(int x=0;x<cw;x++){
      auto b=pixels[((by+y)*width+bx+x)*4+3];
      auto f=pixels[((fy+y)*width+fx+x)*4+3];
      auto n=18+(y*cw+x)*4;
      tga[n]=tga[n+1]=tga[n+2]=f;
      tga[n+3]=std::max(b,f);
    }
    auto name="data/ui-cursor-"+std::to_string(cursor)+".tga";
    qhandle_t file;
    if(fs->Open(name.c_str(),&file,2)>=0){
      fs->Write(file,tga.data(),(int)tga.size());
      fs->Close(file);
      linuxCursorNames[cursor]=name;
      linuxCursorHotspots[cursor]=hot;
    }
  }
}
static void linuxCursorUpdate(bool look,float scale)
{
  ImGui::GetIO().MouseDrawCursor=false;
  if(look||!input->HasMenuFocus())return;
  int cursor=ImGui::GetMouseCursor();
  if(cursor<0||cursor>=ImGuiMouseCursor_COUNT)cursor=ImGuiMouseCursor_Arrow;
  const auto& fallback=linuxCursorNames[cursor];
  const auto& hot=linuxCursorHotspots[cursor];
  char name[160];
  std::snprintf(name,sizeof name,"shape:%d:%s",linuxCursorShape(cursor),fallback.c_str());
  input->SetMenuFocus(qtrue,name,hot.x*scale,hot.y*scale,scale);
}
#endif
