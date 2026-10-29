// SPDX-License-Identifier: GPL-2.0-or-later
// Explicit cursor images prevent game assets from supplying a tiny editor cursor.
static std::array<std::string,ImGuiMouseCursor_COUNT> uiCursorNames;
static std::array<ImVec2,ImGuiMouseCursor_COUNT> uiCursorHotspots;
static int uiCursorShape(int cursor)
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
static void uiCursorInitialize(plugfsfuncs_t* fs,ImFontAtlas* atlas,const unsigned char* pixels,int width,int height)
{
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
      uiCursorNames[cursor]=name;
      uiCursorHotspots[cursor]=hot;
    }
  }
}
static void uiCursorUpdate(bool look,float scale)
{
  ImGui::GetIO().MouseDrawCursor=false;
  scale=std::max(1.f,scale)*1.75f;
  if(look||!input->HasMenuFocus())return;
  int cursor=ImGui::GetMouseCursor();
  if(cursor<0||cursor>=ImGuiMouseCursor_COUNT)cursor=ImGuiMouseCursor_Arrow;
  const auto& fallback=uiCursorNames[cursor];
  const auto& hot=uiCursorHotspots[cursor];
  char name[160];
  std::snprintf(name,sizeof name,"shape:%d:%s",uiCursorShape(cursor),fallback.c_str());
  input->SetMenuFocus(qtrue,name,hot.x*scale,hot.y*scale,scale);
}
