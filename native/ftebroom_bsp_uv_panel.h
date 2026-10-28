// SPDX-License-Identifier: GPL-2.0-or-later
#include "ftebroom_bsp_uv.h"
#include <future>
struct BspUVJob {std::vector<ftebroom::bspUV::Material> materials;std::string overlay,error;};
static std::future<BspUVJob> bspUVJob;
static std::vector<ftebroom::bspUV::Material> bspUVMaterials;
static bool bspUVOpen=false,bspUVOverlay=true,bspUVFocus=false;
static int bspUVSelected=0,bspUVLoaded=-1;
static qhandle_t bspUVTexture=0,bspUVLines=0;
static std::string bspUVMessage,bspUVSource;
static float bspUVZoom=1;
static void bspUVUnload(){if(bspUVTexture)draw->UnloadImage(bspUVTexture);if(bspUVLines)draw->UnloadImage(bspUVLines);bspUVTexture=bspUVLines=0;bspUVLoaded=-1;}
static void bspUVShow(){
  bspUVFocus=true;
  if(bspUVJob.valid()){bspUVOpen=true;return;}
  try{
    auto bytes=modelResource(modelViewerPath);if(bytes.size()>268435456)throw std::runtime_error("BSP exceeds 256 MiB UV preview limit.");
    auto palette=modelResource("gfx/palette.lmp");bspUVUnload();bspUVMaterials.clear();bspUVSource=modelViewerPath;bspUVSelected=0;bspUVZoom=1;bspUVMessage="Reading BSP UVs...";bspUVOpen=true;
    std::map<std::string,std::string> wals;
    if(bytes.size()>=160&&bytes.substr(0,4)=="IBSP"&&ftebroom::quake1::word(bytes,4)==38){auto off=ftebroom::quake1::word(bytes,48),length=ftebroom::quake1::word(bytes,52);ftebroom::quake1::range(bytes,off,length);if(length%76||length/76>4096)throw std::runtime_error("Invalid Quake II texture directory.");
      for(size_t at=off;at<off+length;at+=76){auto name=ftebroom::quake1::name(bytes,at+40,32);if(!wals.count(name))wals[name]=modelResource("textures/"+name+".wal");}
      try{palette=ftebroom::legacy::pcx(modelResource("pics/colormap.pcx")).palette;}catch(...){}
    }
    bspUVJob=std::async(std::launch::async,[bytes=std::move(bytes),palette=std::move(palette),wals=std::move(wals)](){BspUVJob r;try{r.materials=ftebroom::bspUV::read(bytes,palette,wals);}catch(const std::exception& e){r.error=e.what();}return r;});
  }catch(const std::exception& e){bspUVMessage=e.what();bspUVOpen=true;}
}
static void bspUVPanel(){
  if(bspUVJob.valid()&&bspUVJob.wait_for(std::chrono::seconds(0))==std::future_status::ready){
    auto result=bspUVJob.get();bspUVMessage=result.error;
    if(!result.materials.empty())bspUVMaterials=std::move(result.materials);
    else if(!result.overlay.empty()){
      draw->LoadImageData("ftebroom/bsp-uv-lines.tga","image/tga",result.overlay.data(),result.overlay.size());bspUVLines=draw->LoadImage("ftebroom/bsp-uv-lines.tga");
    }
  }
  if(!bspUVOpen){if(!bspUVJob.valid()){bspUVUnload();bspUVMaterials.clear();}return;}
  ImGui::SetNextWindowSize({760*uiScale(),700*uiScale()},ImGuiCond_FirstUseEver);
  if(bspUVFocus){ImGui::SetNextWindowFocus();bspUVFocus=false;}
  if(ImGui::Begin("BSP UV / texture",&bspUVOpen)){
    ImGui::TextWrapped("%s",bspUVSource.c_str());
    if(!bspUVMaterials.empty()){
      ImGui::BeginDisabled(bspUVJob.valid());
      ImGui::SetNextItemWidth(-1);if(ImGui::BeginCombo("##bsp-uv-material",bspUVMaterials[bspUVSelected].name.c_str())){for(size_t i=0;i<bspUVMaterials.size();i++)if(ImGui::Selectable(bspUVMaterials[i].name.c_str(),bspUVSelected==int(i)))bspUVSelected=int(i);ImGui::EndCombo();}
      ImGui::EndDisabled();ImGui::Checkbox("UV overlay",&bspUVOverlay);ImGui::SameLine();if(ImGui::Button("Fit"))bspUVZoom=1;ImGui::SameLine();ImGui::SetNextItemWidth(150*uiScale());ImGui::SliderFloat("Zoom",&bspUVZoom,.1f,16.f,"%.2fx",ImGuiSliderFlags_Logarithmic);
      auto& m=bspUVMaterials[bspUVSelected];
      if(bspUVLoaded!=bspUVSelected&&!bspUVJob.valid())try{
        bspUVUnload();bspUVLoaded=bspUVSelected;
        if(!m.image.empty()){draw->LoadImageData("ftebroom/bsp-uv-texture.tga","image/tga",m.image.data(),m.image.size());bspUVTexture=draw->LoadImage("ftebroom/bsp-uv-texture.tga");}
        else{
          auto found=materials.find(m.name);std::string name=found!=materials.end()&&!found->second.preview.empty()?found->second.preview:m.name;
          // Read dimensions before the renderer's asynchronous image upload,
          // otherwise non-square Q3 textures would initially use a square canvas.
          for(const char* suffix:{"",".tga",".png",".jpg",".jpeg"}){
            auto source=modelResource(name+suffix);int w=0,h=0,channels=0;
            if(!source.empty()&&source.size()<=64*1024*1024&&stbi_info_from_memory((const stbi_uc*)source.data(),source.size(),&w,&h,&channels)&&w>0&&h>0&&w<=8192&&h<=8192){m.width=w;m.height=h;name+=suffix;break;}
          }
          bspUVTexture=draw->LoadImage(name.c_str());
        }
        float width=0,height=0;if(bspUVTexture)draw->ImageSize(bspUVTexture,&width,&height);
        if(!m.width||!m.height){m.width=unsigned(std::max(0.f,width));m.height=unsigned(std::max(0.f,height));}
        unsigned w=m.width,h=m.height;if(!w||!h){w=h=512;bspUVMessage="Texture unavailable; showing UVs on a neutral background.";}else bspUVMessage="Generating UV overlay...";
        auto mesh=m;mesh.image.clear();bspUVJob=std::async(std::launch::async,[mesh=std::move(mesh),w,h](){BspUVJob r;try{r.overlay=ftebroom::bspUV::raster(mesh,w,h);}catch(const std::exception& e){r.error=e.what();}return r;});
      }catch(const std::exception& e){bspUVMessage=e.what();}
      ImGui::Text("%u x %u | %zu UV edges",m.width,m.height,m.edges.size());
      ImGui::TextDisabled("Wheel: zoom | scrollbars: pan | yellow lines: UV boundaries (repeated tiles wrapped)");
      ImGui::BeginChild("BSP UV canvas",{0,-ImGui::GetTextLineHeightWithSpacing()*2},ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
      auto avail=ImGui::GetWindowSize();avail.x=std::max(1.f,avail.x-32);avail.y=std::max(1.f,avail.y-32);float w=float(m.width?m.width:512),h=float(m.height?m.height:512);float fit=std::min(avail.x/w,avail.y/h);fit=std::max(.01f,fit);ImVec2 size{w*fit*bspUVZoom,h*fit*bspUVZoom},pos=ImGui::GetCursorScreenPos();
      auto* list=ImGui::GetWindowDrawList();list->AddRectFilled(pos,{pos.x+size.x,pos.y+size.y},IM_COL32(60,60,60,255));if(bspUVTexture)list->AddImage((ImTextureID)bspUVTexture,pos,{pos.x+size.x,pos.y+size.y});if(bspUVOverlay&&bspUVLines)list->AddImage((ImTextureID)bspUVLines,pos,{pos.x+size.x,pos.y+size.y});ImGui::InvisibleButton("UV canvas",size);
      if(ImGui::IsWindowHovered()&&ImGui::GetIO().MouseWheel)bspUVZoom=std::clamp(bspUVZoom*std::pow(1.15f,ImGui::GetIO().MouseWheel),.1f,16.f);
      ImGui::EndChild();
    }
    if(bspUVJob.valid())ImGui::TextUnformatted("Working in background...");
    if(!bspUVMessage.empty())ImGui::TextWrapped("%s",bspUVMessage.c_str());
  }ImGui::End();
  if(!bspUVOpen&&!bspUVJob.valid()){bspUVUnload();bspUVMaterials.clear();}
}
