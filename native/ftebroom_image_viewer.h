// SPDX-License-Identifier: GPL-2.0-or-later
#include <future>
#include "ftebroom_lmp.h"
#include "ftebroom_legacy_images.h"
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 16384
#include "third_party/stb_image.h"
// Decode loose files and archive members, then upload bounded images to FTE.
struct FileImage { qhandle_t handle=0; int slot=0,frame=-1; unsigned long long used=0; std::string error; };
static std::map<std::pair<std::filesystem::path,bool>,FileImage> fileImages;
static unsigned long long fileImageClock=0;
static std::filesystem::path imageViewerPath;
static bool imageViewerOpen=false, imageViewerFit=true;
static float imageViewerZoom=1;
static bool imageFile(const std::filesystem::path& path) {
  auto ext=entityLower(path.extension().u8string());
  return ext==".wal"||ext==".pcx"||ext==".lmp"||ext==".png"||ext==".jpg"||ext==".jpeg"||ext==".tga"||ext==".bmp"||ext==".gif"||ext==".psd";
}
struct FileImagePixels {std::string bytes,error;};
static std::future<FileImagePixels> fileImageJob;
static std::pair<std::filesystem::path,bool> fileImageJobKey;
static unsigned fileImageGeneration=0,fileImageJobGeneration=0;
static FileImagePixels fileImageDecode(const std::filesystem::path& path,bool full,const std::string& palette={}){
  FileImagePixels result;
  try {
    auto bytes=modFileBytes(path);if(bytes.empty()||bytes.size()>32*1024*1024)throw std::runtime_error("Image must be between 1 byte and 32 MiB.");
    auto ext=entityLower(path.extension().u8string());
    if(ext==".wal"||ext==".pcx"){auto texture=ext==".wal"?ftebroom::legacy::wal(bytes,palette):ftebroom::legacy::pcx(bytes);
      if(!full&&(texture.width>256||texture.height>256)){auto small=texture;float scale=std::min(256.f/texture.width,256.f/texture.height);small.width=std::max(1u,unsigned(texture.width*scale));small.height=std::max(1u,unsigned(texture.height*scale));small.pixels.resize(size_t(small.width)*small.height);for(unsigned y=0;y<small.height;y++)for(unsigned x=0;x<small.width;x++)small.pixels[size_t(y)*small.width+x]=texture.pixels[size_t(y*texture.height/small.height)*texture.width+x*texture.width/small.width];texture=std::move(small);}
      result.bytes=ftebroom::quake1::tga(texture,palette);return result;}
    int width=0,height=0,channels=0;
    if(entityLower(path.extension().u8string())==".lmp"){
      auto image=ftebroom::lmp::decode(bytes,palette,entityLower(path.filename().u8string()));
      width=image.width;height=image.height;float scale=full?1.f:std::min(1.f,std::min(256.f/width,256.f/height));int tw=std::max(1,int(width*scale)),th=std::max(1,int(height*scale));
      std::string raster(18+tw*th*4,0);raster[2]=2;raster[12]=tw&255;raster[13]=tw>>8;raster[14]=th&255;raster[15]=th>>8;raster[16]=32;raster[17]=40;
      for(int y=0;y<th;y++)for(int x=0;x<tw;x++){auto from=(const unsigned char*)image.rgba.data()+((y*height/th)*width+x*width/tw)*4;auto to=&raster[18+(y*tw+x)*4];to[0]=from[2];to[1]=from[1];to[2]=from[0];to[3]=from[3];}result.bytes=std::move(raster);return result;
    }
    if(!stbi_info_from_memory((const stbi_uc*)bytes.data(),bytes.size(),&width,&height,&channels)||width<=0||height<=0||size_t(width)*height>16777216)throw std::runtime_error("Unsupported image or more than 16 million pixels.");
    auto pixels=stbi_load_from_memory((const stbi_uc*)bytes.data(),bytes.size(),&width,&height,&channels,4);
    if(!pixels)throw std::runtime_error("Cannot decode image.");
    struct FreePixels{stbi_uc* p;~FreePixels(){stbi_image_free(p);}} cleanup{pixels};
    float scale=full?1.f:std::min(1.f,std::min(256.f/width,256.f/height));int tw=std::max(1,int(width*scale)),th=std::max(1,int(height*scale));
    std::string raster(18+tw*th*4,0);raster[2]=2;raster[12]=tw&255;raster[13]=tw>>8;raster[14]=th&255;raster[15]=th>>8;raster[16]=32;raster[17]=40;
    for(int y=0;y<th;y++)for(int x=0;x<tw;x++){auto from=pixels+((y*height/th)*width+x*width/tw)*4;auto to=&raster[18+(y*tw+x)*4];to[0]=from[2];to[1]=from[1];to[2]=from[0];to[3]=from[3];}
    result.bytes=std::move(raster);
  }catch(const std::exception& e){result.error=e.what();}return result;
}
static void clearFileImages() {
  ++fileImageGeneration;
  for(auto& item:fileImages)if(item.second.handle)draw->UnloadImage(item.second.handle);
  fileImages.clear();
}
// Called before drawing any panels: uploads and texture-slot reuse stay on the render thread.
static void fileImagePump(){
  if(!fileImageJob.valid()||fileImageJob.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
  auto result=fileImageJob.get();if(fileImageJobGeneration!=fileImageGeneration)return;
  if(fileImageJobKey.second)for(auto it=fileImages.begin();it!=fileImages.end();){if(it->first.second){if(it->second.handle)draw->UnloadImage(it->second.handle);it=fileImages.erase(it);}else ++it;}
  if(fileImages.size()>=256){auto oldest=std::min_element(fileImages.begin(),fileImages.end(),[](const auto& a,const auto& b){return a.second.used<b.second.used;});if(oldest->second.handle)draw->UnloadImage(oldest->second.handle);fileImages.erase(oldest);}
  FileImage entry;entry.error=result.error;entry.used=++fileImageClock;
  for(;;entry.slot++){bool occupied=false;for(const auto& cached:fileImages)occupied|=cached.second.slot==entry.slot;if(!occupied)break;}
  if(entry.error.empty()){auto name="ftebroom/file-image-slot-"+std::to_string(entry.slot)+".tga";draw->LoadImageData(name.c_str(),"image/tga",result.bytes.data(),result.bytes.size());entry.handle=draw->LoadImage(name.c_str());if(!entry.handle)entry.error="Cannot upload image.";}
  fileImages.emplace(fileImageJobKey,std::move(entry));
}
static FileImage& fileImage(const std::filesystem::path& path,bool full=false) {
  auto key=std::make_pair(path,full);auto found=fileImages.find(key);
  if(found!=fileImages.end()){found->second.used=++fileImageClock;found->second.frame=ImGui::GetFrameCount();return found->second;}
  static FileImage pending;
  if(!fileImageJob.valid()){
    try{modFileValidate(path);}catch(const std::exception& e){FileImage bad;bad.error=e.what();return fileImages.emplace(key,std::move(bad)).first->second;}
    std::string palette;
    if(entityLower(path.extension().u8string())==".lmp"||entityLower(path.extension().u8string())==".wal"){
      try{std::filesystem::path archive;std::string member;if(ftebroom::pak::split(path,archive,member))palette=modFileBytes(archive/"gfx/palette.lmp");}catch(...){}
      if(palette.size()!=768)try{palette=modelResource("gfx/palette.lmp");}catch(...){}
    }
    if(entityLower(path.extension().u8string())==".wal")try{auto pcx=modelResource("pics/colormap.pcx");palette=ftebroom::legacy::pcx(pcx).palette;}catch(...){}
    fileImageJobKey=key;fileImageJobGeneration=fileImageGeneration;fileImageJob=std::async(std::launch::async,[path,full,palette](){return fileImageDecode(path,full,palette);});
  }
  return pending;
}
static bool fileImageTile(const std::filesystem::path& path,ImVec2 p,float size) {
  auto& image=fileImage(path);float w=0,h=0;
  if(!image.handle||draw->ImageSize(image.handle,&w,&h)<=0||w<=0||h<=0)return false;
  float scale=std::min(size/w,size/h);ImVec2 a(p.x+(size-w*scale)*.5f,p.y+(size-h*scale)*.5f);
  auto list=ImGui::GetWindowDrawList();list->PushClipRect(p,{p.x+size,p.y+size},true);
  list->AddImage((ImTextureID)image.handle,a,{a.x+w*scale,a.y+h*scale});list->PopClipRect();return true;
}
static void imageViewerShow(const std::filesystem::path& path){imageViewerPath=path;imageViewerOpen=true;imageViewerFit=true;imageViewerZoom=1;}
static void imageViewerPanel() {
  if(!imageViewerOpen)return;
  ImGui::SetNextWindowSize({760,580},ImGuiCond_FirstUseEver);
  if(ImGui::Begin("Image viewer",&imageViewerOpen)) {
    ImGui::TextWrapped("%s",imageViewerPath.u8string().c_str());
    ImGui::Checkbox("Fit image",&imageViewerFit);ImGui::SameLine();
    if(ImGui::Button("100%")){imageViewerFit=false;imageViewerZoom=1;}
    ImGui::SameLine();ImGui::SetNextItemWidth(180);
    if(ImGui::SliderFloat("Zoom",&imageViewerZoom,.05f,8.f,"%.2fx"))imageViewerFit=false;
    auto& image=fileImage(imageViewerPath,true);float w=0,h=0;
    if(!image.error.empty())ImGui::TextWrapped("%s",image.error.c_str());
    else if(image.handle&&draw->ImageSize(image.handle,&w,&h)>0&&w>0&&h>0) {
      ImGui::Text("%.0f x %.0f",w,h);
      ImGui::BeginChild("Image canvas",{0,0},ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar);
      auto avail=ImGui::GetContentRegionAvail();float zoom=imageViewerFit?std::min(avail.x/w,avail.y/h):imageViewerZoom;
      ImGui::Image((ImTextureID)image.handle,{std::max(1.f,w*zoom),std::max(1.f,h*zoom)});
      ImGui::EndChild();
    }else ImGui::TextUnformatted("Loading image...");
  }ImGui::End();
}

static void lmpShow(const std::filesystem::path& path){
  try{auto bytes=modFileBytes(path);if(bytes.size()==768){auto image=ftebroom::lmp::decode(bytes,"",path.filename().u8string());if(image.width==16&&image.height==16){paletteShow(path);return;}}}catch(...){}
  imageViewerShow(path);
}
