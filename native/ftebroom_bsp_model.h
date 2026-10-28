// SPDX-License-Identifier: GPL-2.0-or-later
#include "ftebroom_bsp_atlas.h"
static bool bspModelWindow=false,bspModelBusy=false,bspModelAtlas=true,bspModelChooseFolder=false;
static char bspModelDestination[4096]={};
static std::filesystem::path bspModelTarget,bspModelWork,bspModelFolder;
static std::string bspModelMessage;
static int bspModelSteps=0,bspModelProfile=0;
static std::future<ftebroom::bspAtlas::Result> bspModelAtlasJob;
static void bspModelShow(){
  if(!*bspModelDestination){auto target=documentDefaultFolder()/"selected-model.bsp";std::snprintf(bspModelDestination,sizeof bspModelDestination,"%s",target.u8string().c_str());}
  bspModelWindow=true;
}
static void bspModelPublish(ftebroom::bspAtlas::Result result){
  auto assets=bspModelTarget.parent_path()/(bspModelTarget.stem().u8string()+"-atlas");auto staging=bspModelWork/"output";std::filesystem::create_directory(staging);
  for(size_t i=0;i<result.images.size();i++){
    soundWrite(staging/("atlas"+std::to_string(i)+".tga"),ftebroom::bspAtlas::tga(result.images[i]));
    soundWrite(staging/("uv"+std::to_string(i)+".tga"),ftebroom::bspAtlas::tga(result.outlines[i]));
  }
  if(!result.images.empty()){
    std::string notes="Each atlasN.tga is paired with uvN.tga (transparent UV island boundaries). Keep UV islands and image dimensions unchanged when painting.\n";
    if(bspModelProfile==2){
      auto off=ftebroom::quake1::word(result.bsp,20);auto textures=result.bsp.substr(off,ftebroom::quake1::word(result.bsp,24));unsigned n=ftebroom::quake1::word(textures,0);
      for(unsigned i=0;i<result.images.size();i++){auto name=ftebroom::quake1::name(textures,ftebroom::quake1::word(textures,4+(n-result.images.size()+i)*4),16);
        notes+="atlas"+std::to_string(i)+".tga -> embedded texture "+name+". FTE replacement: textures/"+name+".tga under the game/mod root.\n";}
      notes+="Quake BSP stores its textures internally. For engines without external replacements, re-embed painted images with a BSP texture tool. The original collision and UV mapping must be retained.\n";
    }else notes+="Quake III references atlasN images in this folder. Keep this directory beside the BSP at its original game-relative path. Painted TGA changes are visible after reloading resources.\n";
    soundWrite(staging/"PAINTING.txt",notes);
  }
  bool ownAssets=false;
  try{
    if(std::filesystem::exists(bspModelTarget))throw std::runtime_error("Destination appeared during compilation; choose a new filename.");
    if(!result.images.empty()){if(std::filesystem::exists(assets))throw std::runtime_error("Atlas directory already exists; choose a new model name.");ownAssets=std::filesystem::create_directory(assets);if(!ownAssets)throw std::runtime_error("Cannot create atlas directory.");for(const auto& file:std::filesystem::directory_iterator(staging))soundWrite(assets/file.path().filename(),q1Read(file.path()));}
    // Copy to a sibling before renaming, so cross-volume destinations work too.
    auto pending=bspModelTarget;pending+=".reyond-export.tmp";if(std::filesystem::exists(pending))throw std::runtime_error("Export temporary file already exists.");
    try{soundWrite(pending,result.bsp);std::filesystem::rename(pending,bspModelTarget);}catch(...){std::error_code ec;std::filesystem::remove(pending,ec);throw;}
    bspModelMessage="Exported "+bspModelTarget.u8string()+". Collision and original world coordinates are retained.";
    if(ownAssets){bspModelMessage+=" Atlas images and transparent UV outlines are in "+assets.u8string()+". Paint atlasN.tga with uvN.tga as an overlay; see PAINTING.txt.";if(bspModelProfile==2)bspModelMessage+=" Quake BSP embeds its palette texture; editing a TGA does not rewrite the embedded texture.";}
  }catch(...){if(ownAssets){std::error_code ec;std::filesystem::remove_all(assets,ec);}throw;}
}
static void bspModelBegin(){
  if(runActive||runExclusive||buildRunning||assetsLoading)throw std::runtime_error("Wait for the current resource or compilation job.");
  runLoad();auto target=std::filesystem::absolute(std::filesystem::u8path(bspModelDestination)).lexically_normal();
  if(entityLower(target.extension().u8string())!=".bsp"||!std::filesystem::is_directory(target.parent_path())||modFilePacked(target))throw std::runtime_error("Choose a loose .bsp filename in an existing directory.");
  if(std::filesystem::exists(target))throw std::runtime_error("Choose a new filename; the source and existing models are never overwritten.");
  if(!*baseq3)throw std::runtime_error("Configure game resources first.");
  auto shaderFolder=target.parent_path()/(target.stem().u8string()+"-atlas");
  if(bspModelAtlas&&std::filesystem::exists(shaderFolder))throw std::runtime_error("The atlas directory already exists. Choose a new model name.");
  if(bspModelAtlas&&gameProfile!=2&&modFileModelPath(shaderFolder).empty())throw std::runtime_error("Quake III atlases need a destination inside the configured game resource directories.");
  auto stamp=std::chrono::high_resolution_clock::now().time_since_epoch().count();
  auto work=std::filesystem::absolute(documentPath("data/bsp-model-export"))/std::to_string(stamp);std::filesystem::create_directories(work);
  RunProfile profile{"Export selected BSP model",work.u8string(),false,{{0,"${WORK_DIR_PATH}/selection.map","@selected-brushes"}}};
  if(gameProfile==2){if(runVariables["qbsp"].empty())throw std::runtime_error("Configure the ericw qbsp compiler in Setup compile.");
    profile.tasks.push_back({1,"${qbsp}",std::string("-nofill ")+(q1BspFormat==1?"-bsp2 ":"")+"\"${WORK_DIR_PATH}/selection.map\""});
  }else{
    if(runVariables["q3map2"].empty())throw std::runtime_error("Configure q3map2 in Setup compile.");
    auto game=std::filesystem::absolute(std::filesystem::u8path(baseq3)).lexically_normal();
    // The selected-only snapshot contains an exterior flood seed for open geometry.
    std::string args="-game quake3 -fs_basepath "+modQuote(game.parent_path().generic_u8string())+" -fs_basegame "+modQuote(game.filename().u8string())+" -fs_game "+modQuote(game.filename().u8string())+" -threads ${CPU_COUNT} -meta \"${WORK_DIR_PATH}/selection.map\"";
    profile.tasks.push_back({1,"${q3map2}",args});
  }
  runExportHook=nullptr;runStageHook=nullptr;runBegin(profile);if(!runActive)throw std::runtime_error(runError.empty()?"Cannot start export.":runError);
  bspModelTarget=target;bspModelWork=work;bspModelProfile=gameProfile;bspModelSteps=profile.tasks.size();bspModelBusy=runExclusive=gameProfileLocked=true;bspModelMessage="Compiling only the selected brushes. No VIS or LIGHT stage.";
}
static void bspModelTick(){
  if(!bspModelBusy)return;
  try{
    if(bspModelAtlasJob.valid()){
      if(bspModelAtlasJob.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
      bspModelPublish(bspModelAtlasJob.get());bspModelBusy=runExclusive=gameProfileLocked=false;return;
    }
    if(runActive)return;
    soundWrite(bspModelWork/"compile.log",runLog);
    if(runHadError||runStep!=bspModelSteps)throw std::runtime_error(runHadError?runError:"BSP model export cancelled.");
    auto bytes=q1Read(bspModelWork/"selection.bsp");if(!ftebroom::quake1::bsp(bytes,bspModelProfile==2))throw std::runtime_error("Compiler produced an invalid BSP.");
    // Model exports carry no gameplay entities, including the Q3 flood seed.
    while(bytes.size()%4)bytes+='\0';size_t entities=bytes.size();bytes+="{\n\"classname\" \"worldspawn\"\n}\n";bytes+='\0';
    size_t header=bspModelProfile==2?4:8;ftebroom::bspAtlas::put(bytes,header,entities);ftebroom::bspAtlas::put(bytes,header+4,bytes.size()-entities);
    if(!bspModelAtlas){ftebroom::bspAtlas::Result result;result.bsp=std::move(bytes);bspModelPublish(std::move(result));bspModelBusy=runExclusive=gameProfileLocked=false;return;}
    uint64_t imagePixels=0;std::map<std::string,ftebroom::bspAtlas::Image> images;
    std::string palette=bspModelProfile==2?modelResource("gfx/palette.lmp"):std::string{};
    std::string prefix="rb";uint32_t hash=2166136261u;for(unsigned char c:bspModelTarget.u8string())hash=(hash^c)*16777619u;prefix+=std::to_string(hash%100000000)+"_";
    if(bspModelProfile!=2){
      auto& b=bytes;auto off=ftebroom::quake1::word(b,16),length=ftebroom::quake1::word(b,20);ftebroom::quake1::range(b,off,length);if(length%72||length/72>4096)throw std::runtime_error("Invalid BSP shader directory.");
      auto faceOffset=ftebroom::quake1::word(b,112),faceLength=ftebroom::quake1::word(b,116);ftebroom::quake1::range(b,faceOffset,faceLength);if(faceLength%104||faceLength/104>4096)throw std::runtime_error("Invalid BSP surfaces.");
      std::set<unsigned> used;for(unsigned f=0;f<faceLength;f+=104)used.insert(ftebroom::quake1::word(b,faceOffset+f));
      for(unsigned index:used){unsigned i=index*72;if(index>=length/72)throw std::runtime_error("Invalid BSP material index.");auto name=ftebroom::quake1::name(b,off+i,64);if(images.count(name))continue;auto material=materials.find(name);if(material!=materials.end()&&(material->second.tags&(64|32|512)))throw std::runtime_error("Atlas requires static opaque materials: "+name);auto imageName=material!=materials.end()&&!material->second.preview.empty()?material->second.preview:name;std::string sourceImage;for(const char* suffix:{"",".tga",".png",".jpg",".jpeg"}){sourceImage=modelResource(imageName+suffix);if(!sourceImage.empty())break;}if(sourceImage.empty())throw std::runtime_error("Missing atlas material image: "+imageName);auto skin=modelImportDecode(sourceImage);imagePixels+=uint64_t(skin.width)*skin.height;if(imagePixels>33554432)throw std::runtime_error("Atlas source images exceed 32 million pixels; export a smaller selection.");ftebroom::bspAtlas::Image image;image.width=skin.width;image.height=skin.height;image.rgba=std::move(skin.rgba);images[name]=std::move(image);}
      prefix=modFileModelPath(bspModelTarget.parent_path()/(bspModelTarget.stem().u8string()+"-atlas"))+"/atlas";
    }
    bspModelMessage="Packing UV islands and baking atlas images...";
    bspModelAtlasJob=std::async(std::launch::async,[bytes=std::move(bytes),palette=std::move(palette),prefix,images=std::move(images)](){return ftebroom::bspAtlas::create(bytes,palette,prefix,[&](const std::string& name){auto it=images.find(name);if(it==images.end())throw std::runtime_error("Missing atlas source: "+name);return it->second;});});
  }catch(const std::exception& e){bspModelMessage=e.what();bspModelBusy=runExclusive=gameProfileLocked=false;}
}
static void bspModelPanel(){
  bspModelTick();if(!bspModelWindow)return;
  ImGui::SetNextWindowSize({650*uiScale(),360*uiScale()},ImGuiCond_FirstUseEver);
  if(ImGui::Begin("Export selected brushes as BSP model",&bspModelWindow,ImGuiWindowFlags_NoCollapse)){
    ImGui::TextWrapped("Select whole brushes first. Only their geometry is exported; point entities and unselected objects are ignored. Select brushes without patches. Coordinates and collision are preserved. Open geometry is allowed.");
    ImGui::BeginDisabled(bspModelBusy);ImGui::InputText("Destination BSP",bspModelDestination,sizeof bspModelDestination);
    if(ImGui::Button("Choose folder...")){bspModelFolder=std::filesystem::u8path(bspModelDestination).parent_path();bspModelChooseFolder=true;}
    if(bspModelChooseFolder)ImGui::OpenPopup("BSP model folder");
    if(ImGui::BeginPopupModal("BSP model folder",&bspModelChooseFolder,ImGuiWindowFlags_AlwaysAutoResize)){
      ImGui::TextWrapped("%s",bspModelFolder.u8string().c_str());if(ImGui::Button("Up")&&bspModelFolder.has_parent_path())bspModelFolder=bspModelFolder.parent_path();
      ImGui::BeginChild("Directories",{460*uiScale(),240*uiScale()},ImGuiChildFlags_Borders);
      try{std::vector<std::filesystem::path> dirs;for(const auto& e:std::filesystem::directory_iterator(bspModelFolder,std::filesystem::directory_options::skip_permission_denied))if(e.is_directory()&&dirs.size()<1024)dirs.push_back(e.path());std::sort(dirs.begin(),dirs.end());for(const auto& p:dirs)if(ImGui::Selectable(p.filename().u8string().c_str())){bspModelFolder=p;break;}}catch(const std::exception& e){ImGui::TextWrapped("%s",e.what());}ImGui::EndChild();
      if(ImGui::Button("Use this folder")){auto target=bspModelFolder/std::filesystem::u8path(bspModelDestination).filename();std::snprintf(bspModelDestination,sizeof bspModelDestination,"%s",target.u8string().c_str());bspModelChooseFolder=false;ImGui::CloseCurrentPopup();}ImGui::EndPopup();
    }
    ImGui::Checkbox("Create texture atlas and UV painting template",&bspModelAtlas);
    if(bspModelAtlas)ImGui::TextWrapped("Static opaque diffuse colors only. Shader animation, sky, liquids and transparency cannot be baked. Up to 16 pages of 1024 pixels; oversized UV islands stop export instead of reducing resolution. UV outlines are separate transparent TGA images.");
    if(ImGui::Button("Compile selected brushes"))try{bspModelBegin();}catch(const std::exception& e){bspModelMessage=e.what();}ImGui::EndDisabled();
    if(bspModelBusy){ImGui::SameLine();if(ImGui::Button("Cancel compilation")&&runActive)runStop();ImGui::ProgressBar(-float(ImGui::GetTime()),{-1,0},"Exporting BSP model");}
    if(!bspModelMessage.empty())ImGui::TextWrapped("%s",bspModelMessage.c_str());
  }ImGui::End();
}
