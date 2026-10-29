// SPDX-License-Identifier: GPL-2.0-or-later

static void testDesktop(){

 if(!cvars->GetFloat("ftebroom_skip_setup")||commands->IsInsecure())return;

 auto expect=[](bool ok,const char* name){core->Print((std::string(ok?"FTEBROOM_DESKTOP_PASS ":"FTEBROOM_DESKTOP_FAIL ")+name+"\n").c_str());};

 char mode[64]={},root[4096]={};commands->Argv(1,mode,sizeof mode);cvars->GetString("ftebroom_test_import_root",root,sizeof root);auto frame=[](){event(0,0,0,950,550,1280,800);};

 try {
#ifndef _WIN32
  if(std::string(mode)=="rtl100protocol"){
    modLightingRaw=false;runPreviewSettings=modPreviewReadSettings;
    runAppend("reyond_preview_set");
    runAppend("tings 1 512 1.3 0.35 0.2 0.7 4 1 119 115 97 100 32 49 50 51 114 99\n");
    expect(modLightingRTL&&modLightingDistance==512&&modLighting.baked==.7f&&modSensitivity==4&&modInvertMouse,"rtl100_fragmented_settings_received");
    modLighting=PreviewLighting{};modLightingLoaded=false;modLightingLoad();
    expect(modLightingRTL&&modLightingDistance==512&&modLighting.baked==.7f,"rtl100_child_settings_persisted");
    modLightingRaw=true;modLightingLoad();
    expect(modLighting.baked==0&&modLighting.energy==.8f,"rtl100_raw_has_independent_soft_defaults");
    runPreviewSettings=nullptr;return;
  }
#endif
  static int rtl99Wait=0;
  if(std::string(mode)=="rtl99log"){core->Print(("RTL99_LOG\n"+runLog+"\n").c_str());return;}
  if(std::string(mode)=="rtl99launch"){
    if(buildRunning){command("in 1 ftebroom_test_desktop rtl99launch\n");return;}
    expect(!buildResult.empty()&&buildValidBsp(buildResult),"rtl99_fixture_compiled");
    modLightingLoaded=true;modLightingRTL=false;modLightingDistance=4096;modLaunch(buildResult,false);
    expect(runActive,"rtl99_compiled_preview_started");command("in 5 ftebroom_test_desktop rtl99menu\n");
  }
  if(std::string(mode).rfind("rtl99",0)==0 && std::string(mode)!="rtl99launch"){
    std::string needed="ftebroom_preview_ready";
    if(std::string(mode)=="rtl99on")needed="ftebroom_preview_lighting_applied rtl=1 imported=1";
    if(std::string(mode)=="rtl99near")needed="distance=128 inrange=0 culled=1";
    if(std::string(mode)=="rtl99unlimited")needed="distance=0 inrange=1 culled=0";
    if(std::string(mode)=="rtl99off")needed="ftebroom_preview_lighting_applied rtl=0";
    if(std::string(mode)=="rtl99raw")needed="ftebroom_preview_lighting_applied rtl=1 imported=1";
    if(!runActive||runLog.find(needed)==std::string::npos){
      if(++rtl99Wait<90){modInputQueue+="ftebroom_preview_lighting\n";command(("in 1 ftebroom_test_desktop "+std::string(mode)+"\n").c_str());}
      else {expect(false,"rtl99_runtime_timeout");core->Print((buildMessage+"\n"+runLog).c_str());}return;
    }
    rtl99Wait=0;
    if(std::string(mode)=="rtl99menu"){
      modOpenControls();frame();frame();expect(modMenuDrawn,"rtl99_escape_menu_has_lighting");
      auto p=modLightingToggleCenter;event(0,0,0,p.x,p.y,1280,800);event(1,mouseKeys[0],0,p.x,p.y,1280,800);event(0,0,0,p.x,p.y,1280,800);event(2,mouseKeys[0],0,p.x,p.y,1280,800);frame();
      expect(modLightingRTL,"rtl99_checkbox_enabled");modMenuOpen=false;frame();command("in 5 ftebroom_test_desktop rtl99on\n");
    }else if(std::string(mode)=="rtl99on"){
      expect(runLog.find("lightmaps=0.7 fullbright=0")!=std::string::npos,"rtl100_ordinary_light_imported_with_baked_fill");
      modPlay=false;modLastCamera.clear();cvars->SetString("ftebroom_camera_state","0 -768 64 0 90 0 90");
      modLightingDistance=128;modLightingApply();modInputQueue+="in 1 ftebroom_preview_lighting\n";command("in 4 ftebroom_test_desktop rtl99near\n");
    }else if(std::string(mode)=="rtl99near"){
      expect(true,"rtl99_far_light_culled");modLightingDistance=0;modLightingApply();modInputQueue+="in 1 ftebroom_preview_lighting\n";command("in 4 ftebroom_test_desktop rtl99unlimited\n");
    }else if(std::string(mode)=="rtl99unlimited"){
      expect(true,"rtl99_unlimited_light_restored");modLightingRTL=false;modLightingApply();command("in 4 ftebroom_test_desktop rtl99off\n");
    }else if(std::string(mode)=="rtl99off"){
      expect(runLog.find("lightmaps=1 fullbright=0")!=std::string::npos,"rtl99_lightmap_restored");
      modEmbedEnd();if(runActive)runStop();modLightingRTL=true;modLightingDistance=4096;modPreviewChoose(2);command("in 20 ftebroom_test_desktop rtl99raw\n");
    }else if(std::string(mode)=="rtl99raw"){
      modLightingApply();
      expect(runLog.find("+map \"maps/room.map\"")!=std::string::npos,"rtl99_raw_map_realtime_lighting");
      auto home=std::filesystem::u8path(runningProfile.work)/"home"/std::filesystem::u8path(gameRuntime()).filename();
      auto shader=q1Read(home/"scripts/reyondbroom-raw-preview.shader");
      shader.erase(std::remove(shader.begin(),shader.end(),'\r'),shader.end());
      expect(shader.find("clip\n{\nsurfaceparm nodraw\nsurfaceparm noshadows")!=std::string::npos,"rtl99_raw_clip_hidden_no_shadows");
      expect(q1Read(home/"maps/room.map").find("CLIP")!=std::string::npos,"rtl99_clip_geometry_retained");
      auto enabled=modLightingRTL;auto distance=modLightingDistance;modLightingLoaded=false;modLightingLoad();expect(modLightingRTL==enabled&&modLightingDistance==distance,"rtl99_preferences_persisted");
      modInputQueue+="ftebroom_preview_lighting trace 0 0 32 0 -400 32\n";
    }else if(std::string(mode)=="rtl99collision"){
      auto at=runLog.rfind("ftebroom_preview_collision ");float fraction=1,x=0,y=0,z=0;
      bool position=at!=std::string::npos&&std::sscanf(runLog.c_str()+at,"ftebroom_preview_collision fraction=%f end=%f,%f,%f",&fraction,&x,&y,&z)==4;
      expect(position&&fraction>0&&fraction<1&&std::abs(x)<16&&y< -200&&y> -280,"rtl99_invisible_clip_blocks_player_trace");
      core->Print(("RTL99_COLLISION "+std::to_string(x)+" "+std::to_string(y)+" "+std::to_string(z)+"\n").c_str());
      modEmbedEnd();if(runActive)runStop();command("in 2 quit force\n");
    }
  }
  static std::filesystem::path preview98Bsp;static int preview98Revision,preview98Wait;
  if(std::string(mode)=="preview98edit"){
    if(buildRunning){command("in 1 ftebroom_test_desktop preview98edit\n");return;}
    command("in 3 ftebroom_test_desktop preview98last\nin 35 ftebroom_test_desktop preview98lastcheck\n");
    expect(modCompiledCurrent(),"preview98_fresh_build_current");preview98Bsp=buildResult;preview98Revision=buildResultRevision;
    core->Print(("RB98_FRESH "+std::to_string(buildResultContentRevision)+"/"+std::to_string(buildCurrentContentRevision())+" gen="+std::to_string(buildResultGeneration)+"/"+std::to_string(documentGeneration)+"\n").c_str());
    auto source=buildResultSourceMap;auto content=buildResultContentRevision;auto generation=buildResultGeneration;buildResult.clear();buildRestoreHistory();expect(modHasCompiledMap()&&!modCompiledCurrent(),"preview98_previous_session_build_restored");
    buildResultRevision=preview98Revision;buildResultSourceMap=source;buildResultContentRevision=content;buildResultGeneration=generation;buildEdit();entityProperty("set","message","preview98_unsaved");
  }
  if(std::string(mode)=="preview98last"){
    expect(!modCompiledCurrent(),"preview98_unsaved_change_detected");modPreview();expect(modChoiceOpen,"preview98_choice_dialog");modPreviewChoose(1);
    expect(runActive&&!modChoiceOpen,"preview98_last_bsp_started");preview98Wait=0;
  }
  if(std::string(mode)=="preview98lastcheck"||std::string(mode)=="preview98rawcheck"||std::string(mode)=="preview98compiledcheck"){
    if(!runActive||runLog.find("ftebroom_preview_ready")==std::string::npos){
      if(++preview98Wait<150)command(("in 1 ftebroom_test_desktop "+std::string(mode)+"\n").c_str());
      else {expect(false,"preview98_runtime_timeout");core->Print((buildMessage+"\n"+runLog).c_str());}return;
    }
    preview98Wait=0;
    if(std::string(mode)=="preview98lastcheck"){
      expect(runLog.find("+map room ")!=std::string::npos&&buildResult==preview98Bsp&&buildResultRevision==preview98Revision,"preview98_last_bsp_kept_current_edits");
      modEmbedEnd();if(runActive)runStop();modPreview();modPreviewChoose(2);expect(modRawExporting,"preview98_current_map_exporting");command("in 25 ftebroom_test_desktop preview98rawcheck\n");
    }else if(std::string(mode)=="preview98rawcheck"){
      auto root=std::filesystem::u8path(runningProfile.work)/"home"/std::filesystem::u8path(gameRuntime()).filename();
      auto map=root/"maps/room.map";
      expect(runLog.find("+map \"maps/room.map\"")!=std::string::npos&&std::filesystem::is_regular_file(map)&&!std::filesystem::exists(root/"maps/room.bsp"),"preview98_raw_map_without_bsp");
      expect(q1Read(map).find("preview98_unsaved")!=std::string::npos&&buildResult==preview98Bsp&&buildResultRevision==preview98Revision,"preview98_raw_uses_unsaved_contents");
      expect(q1Read(map).find("\"classname\" \"func_group\"")==std::string::npos,"preview98_raw_editor_groups_flattened");
      expect(std::filesystem::is_regular_file(root/"scripts/reyondbroom-raw-preview.shader"),"preview98_raw_wad_materials");
      modEmbedEnd();if(runActive)runStop();modPreview();modPreviewChoose(0);expect(buildRunning&&modCompilePending,"preview98_compile_and_play_requested");command("in 45 ftebroom_test_desktop preview98compiledcheck\n");
    }else {
      expect(buildResult!=preview98Bsp&&modCompiledCurrent()&&!modCompilePending,"preview98_compile_then_play_latest");
      modEmbedEnd();if(runActive)runStop();command("in 2 quit force\n");
    }
  }
  if(std::string(mode)=="cursor98small"||std::string(mode)=="cursor98large"||std::string(mode)=="cursor98font"){
    cameraEnd();modelEditorCameraEnd(true);uiCursorPercent=std::string(mode)=="cursor98large"?200:100;
    uiScaleChoice=std::string(mode)=="cursor98font"?4:0;
    event(0,0,0,200,400,1280,800);event(0,0,0,200,400,1280,800);
#ifdef _WIN32
    RECT client{};GetClientRect(desktopHost,&client);auto r=viewRects[0];auto size=ImGui::GetIO().DisplaySize;
    POINT point{LONG((r.x+r.w*.5f)*client.right/size.x),LONG((r.y+r.h*.5f)*client.bottom/size.y)};ClientToScreen(desktopHost,&point);SetForegroundWindow(desktopHost);SetCursorPos(point.x,point.y);
#endif
  }
  if(std::string(mode).rfind("cursor98check",0)==0){
    static int small=0;
#ifdef _WIN32
    desktopProc(desktopHost,WM_SETCURSOR,(WPARAM)desktopHost,HTCLIENT);
    ICONINFO info{};BITMAP bitmap{};int height=0;
    CURSORINFO current{sizeof(CURSORINFO)};GetCursorInfo(&current);
    if(GetIconInfo(current.hCursor,&info)){if(GetObject(info.hbmColor?info.hbmColor:info.hbmMask,sizeof bitmap,&bitmap))height=info.hbmColor?bitmap.bmHeight:bitmap.bmHeight/2;if(info.hbmColor)DeleteObject(info.hbmColor);if(info.hbmMask)DeleteObject(info.hbmMask);}
    if(std::string(mode)=="cursor98checksmall"){small=height;expect(height>=16&&height<=32,"cursor98_smaller_default");}
    else if(std::string(mode)=="cursor98checklarge")expect(height>small,"cursor98_size_changes_hardware_cursor");
    else expect(height==small,"cursor98_font_scale_independent");
    core->Print(("RB98_CURSOR_HEIGHT "+std::to_string(height)+"\n").c_str());
#else
    expect(!uiCursorNames[ImGuiMouseCursor_Arrow].empty()&&!ImGui::GetIO().MouseDrawCursor,"cursor98_single_scaled_image");
#endif
  }
  if(std::string(mode)=="cursor98preferences"){
    uiCursorPercent=175;cvars->SetFloat("ftebroom_skip_setup",0);uiSaveAppearance();
    uiCursorPercent=100;uiLoaded=false;uiLoad();expect(uiCursorPercent==175,"cursor98_size_persisted");
    std::ofstream(documentPath("appearance.cfg"))<<"0 0 0 0\n";uiCursorPercent=175;uiLoaded=false;uiLoad();expect(uiCursorPercent==100,"cursor98_legacy_settings_default");
    cvars->SetFloat("ftebroom_skip_setup",1);uiScaleChoice=0;preferencesRequest=true;preferencesAppearanceRequest=true;frame();frame();
    auto window=ImGui::FindWindowByName("Preferences");expect(window&&window->Active&&!preferencesAppearanceRequest,"cursor98_appearance_page_open");
  }
  if(std::string(mode)=="cursor97"){
    cameraEnd();modelEditorCameraEnd(true);ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);uiCursorUpdate(false,1.f);
    expect(!uiCursorNames[ImGuiMouseCursor_Arrow].empty()&&!ImGui::GetIO().MouseDrawCursor,"cursor97_single_explicit_cursor");
  }
  if(std::string(mode)=="cursor97check"){
#ifdef _WIN32
    ICONINFO info{};BITMAP bitmap{};int height=0;
    if(GetIconInfo(GetCursor(),&info)){if(GetObject(info.hbmColor?info.hbmColor:info.hbmMask,sizeof bitmap,&bitmap))height=info.hbmColor?bitmap.bmHeight:bitmap.bmHeight/2;if(info.hbmColor)DeleteObject(info.hbmColor);if(info.hbmMask)DeleteObject(info.hbmMask);}
    expect(height>=16,"cursor97_visible_hardware_size");core->Print(("RB97_CURSOR_HEIGHT "+std::to_string(height)+"\n").c_str());
#else
    expect(uiCursorShape(ImGuiMouseCursor_Arrow)==1&&!uiCursorNames[ImGuiMouseCursor_Arrow].empty(),"cursor97_native_shape_and_scaled_fallback");
#endif
  }
  if(std::string(mode)=="play97check"){
    expect(runActive&&runLog.find("ftebroom_preview_player ")!=std::string::npos,"play97_player_camera_default");
    expect(runLog.find("+set ftebroom_preview_play 1")!=std::string::npos&&runLog.find("+set pm_thirdPerson 0 +set r_showViewModels 1")!=std::string::npos,"play97_half_life_first_person");
#ifdef _WIN32
    expect(modEmbedded&&modPlay,"play97_embedded_play_selected");
    modSetPlay(false);expect(!modPlay&&!modCaptured,"play97_free_camera_available");modSetPlay(true);
#endif
  }
  if(std::string(mode)=="setup96q2fields"||std::string(mode)=="setup96hlfields"){
    recentWindow=false;setupInitialized=true;setupNeeded=false;gameProfileLocked=false;firstSetupStep=4;gameProfile=std::string(mode)=="setup96hlfields"?4:3;frame();frame();
  }
  if(std::string(mode)=="setup96q2"||std::string(mode)=="setup96hl"){
    bool hl=std::string(mode)=="setup96hl";
    recentWindow=false;setupInitialized=true;setupNeeded=false;gameProfileLocked=false;firstSetupStep=3;
    auto click=[&](ImVec2 p){event(0,0,0,p.x,p.y,1280,800);event(1,mouseKeys[0],0,p.x,p.y,1280,800);event(0,0,0,p.x,p.y,1280,800);event(2,mouseKeys[0],0,p.x,p.y,1280,800);frame();};
    frame();frame();click(setupGameCenters[hl?3:1]);expect(gameProfile==(hl?4:3),"setup96_game_row");
    firstSetupStep=4;frame();frame();auto window=ImGui::FindWindowByName("First-time setup");expect(window&&window->Active,"setup96_resource_step");
    char path[4096]{};cvars->GetString(hl?"ftebroom_test_mod":"ftebroom_test_import_root",path,sizeof path);
    setupModBrowseStart(gameProfile);std::snprintf(setupModBrowsePath,sizeof setupModBrowsePath,"%s",path);frame();frame();
    auto picker=ImGui::FindWindowByName("Choose mod directory");expect(picker&&picker->Active,"setup96_folder_browser");
    click(setupModBrowseUseCenter);expect(!setupModBrowseOpen&&std::string(hl?hlMod:q2Mod)==path,"setup96_folder_browser_target");
    frame();frame();click(setupLoadCenter);expect(firstSetupStep==5&&assetsLoading,"setup96_load_from_wizard");
  }
  if(std::string(mode)=="setup96ready"){
    expect(!assetsLoading&&!setupNeeded&&assetError.empty(),"setup96_resources_ready");
    expect(entityDefinitions.count(gameProfile==4?"monster_scientist":"monster_soldier")&&materials.count("textures/fb_grey"),"setup96_game_resources");
    expect(cvars->GetFloat("ftebroom_classic_map")&&!cvars->GetFloat("ftebroom_allow_engine_terrain"),"setup96_classic_tools");
    std::ifstream saved(settingsPath().parent_path()/"reyondbroom-legacy-games.txt");std::string q2,hl;saved>>std::quoted(q2)>>std::quoted(hl);
    auto actual=std::filesystem::u8path(gameProfile==4?hlMod:q2Mod);expect((gameProfile==4?hl:q2)==actual.generic_u8string()&&actual.filename()==(gameProfile==4?"valve":"baseq2"),"setup96_normalized_path_saved");
  }
  if(std::string(mode)=="hl95retail"){
    gameProfile=4;char mod[4096]={};cvars->GetString("ftebroom_test_mod",mod,sizeof mod);std::snprintf(hlMod,sizeof hlMod,"%s",mod);hlRuntime=0;gamePreviewAfterBuild=false;gameProfileSave();expect(mountAssets(),"hl95_retail_mount");documentRequest("@"+(std::filesystem::u8path(root)/"hl/maps/room.map").u8string());
  }
  if(std::string(mode)=="hl95ready"){
    static int attempts=0;if(!pendingDocumentFile.empty()||documentUntitled||documentFile.filename()!="room.map"){
      if(++attempts<90)command("in 1 ftebroom_test_desktop hl95ready\n");else expect(false,"hl95_map_open_timeout");
    }else {attempts=0;command("ftebroom_test_desktop legacy94ready\n");}
  }
  if(std::string(mode)=="hl95light"){
    expect(!buildRunning&&!runHadError&&!buildResult.empty()&&buildValidBsp(buildResult),"hl95_real_resources_compile");
    if(!buildResult.empty()){auto b=q1Read(buildResult);auto o=ftebroom::quake1::word(b,4+8*8),n=ftebroom::quake1::word(b,8+8*8);expect(n>1&&o+n<=b.size(),"hl95_bsp_has_lightmaps");}
    core->Print(("HL95_BUILD_LOG\n"+runLog+"\n").c_str());buildShow();
  }
  if(std::string(mode)=="hl95preview"){
    modelViewerOpen=false;modPreview();expect(runActive,"hl95_preview_started");
  }
  if(std::string(mode)=="hl95previewcheck"){
    expect(runActive&&runLog.find("ftebroom_preview_ready")!=std::string::npos,"hl95_preview_world_ready");
    core->Print(("HL95_PREVIEW_LOG\n"+runLog+"\n").c_str());modEmbedEnd();
  }
  if(std::string(mode)=="hl95model")modelEditorShow("models/scientist.mdl");
  if(std::string(mode)=="hl95export"){
    modelEditLoad();expect(modelEditLoaded&&modelEditNodes.size()==2&&cvars->GetFloat("ftebroom_modeleditor_drawn")>0,"hl95_studio_editable_and_drawn");
    if(modelEditNodes.size()==2){modelEditNodes[0].total=1.5;modelEditNodes[0].rotation={10,20,30};modelEditNodes[0].position={4,5,6};modelEditChanged=true;modelEditPublish();modelEditExport();auto path=std::filesystem::u8path(root)/"studio95-export.mdl";std::snprintf(modelConverterDestination,sizeof modelConverterDestination,"%s",path.u8string().c_str());modelConverterFormat=6;modelConverterSave({});expect(std::filesystem::is_regular_file(path),"hl95_native_studio_export");auto cached=documentPath("models/qa/studio95-export.mdl");std::filesystem::create_directories(cached.parent_path());std::filesystem::copy_file(path,cached,std::filesystem::copy_options::overwrite_existing);modelConverterOpen=false;modelViewerOpen=false;command("fs_flush\nin 0.2 ftebroom_test_desktop hl95reopen\n");}
  }
  if(std::string(mode)=="hl95reopen"){modelEditorShow("models/qa/studio95-export.mdl");modelViewerPlay=true;modelViewerBones=true;}
  if(std::string(mode)=="hl95reopencheck")expect(cvars->GetFloat("ftebroom_modeleditor_drawn")>0&&cvars->GetFloat("ftebroom_modelviewer_count")>1&&cvars->GetFloat("ftebroom_modelviewer_bones_drawn")>0,"hl95_export_reopened_textured_animated_bones");
  if(std::string(mode)=="md295model"){modelConverterOpen=false;modelEditorShow("models/qa/native95.md2");}
  if(std::string(mode)=="md295export"){
    modelEditLoad();expect(modelEditLoaded&&modelEditNodes.size()==2&&cvars->GetFloat("ftebroom_modeleditor_drawn")>0,"md295_editable_textured_model");
    if(modelEditNodes.size()==2){modelEditNodes[0].total=1.25;modelEditNodes[0].position={2,3,4};modelEditChanged=true;modelEditPublish();modelEditExport();auto path=std::filesystem::u8path(root)/"md295-export.md2";std::snprintf(modelConverterDestination,sizeof modelConverterDestination,"%s",path.u8string().c_str());modelConverterFormat=5;modelConverterSave({});expect(ftebroom::quake1::word(q1Read(path),40)==3,"md295_native_export_all_frames");std::filesystem::copy_file(path,documentPath("models/qa/md295-export.md2"),std::filesystem::copy_options::overwrite_existing);auto skin=path.parent_path()/"md295-export-skins/0.pcx";auto dest=documentPath("md295-export-skins/0.pcx");std::filesystem::create_directories(dest.parent_path());std::filesystem::copy_file(skin,dest,std::filesystem::copy_options::overwrite_existing);modelConverterOpen=false;modelViewerOpen=false;command("fs_flush\nin 0.2 ftebroom_test_desktop md295reopen\n");}
  }
  if(std::string(mode)=="md295reopen"){modelEditorShow("models/qa/md295-export.md2");modelViewerPlay=true;}
  if(std::string(mode)=="md295reopencheck")expect(cvars->GetFloat("ftebroom_modelviewer_count")==3&&cvars->GetFloat("ftebroom_modeleditor_drawn")>0,"md295_export_reopened_animated");
  if(std::string(mode)=="legacy94q2"||std::string(mode)=="legacy94hl"){
    bool hl=std::string(mode)=="legacy94hl";gameProfile=hl?4:3;auto folder=std::filesystem::u8path(root)/(hl?"hl":"q2");
    std::snprintf(hl?hlMod:q2Mod,1024,"%s",folder.u8string().c_str());gameProfileSave();expect(mountAssets(),"legacy94_mount_requested");documentRequest("@"+(folder/"maps/room.map").u8string());
  }
  if(std::string(mode)=="legacy94ready"){
    expect(!assetsLoading&&!setupNeeded&&assetError.empty(),"legacy94_resources_ready");
    expect(!documentUntitled&&documentFile.filename()=="room.map"&&documentError.empty(),"legacy94_map_open");
    expect(materials.count("textures/fb_grey")&&entityDefinitions.count(gameProfile==4?"monster_scientist":"monster_soldier"),"legacy94_materials_and_entities");
    if(!assetError.empty())core->Print(("LEGACY94_ASSET_ERROR "+assetError+"\n").c_str());
    expect(cvars->GetFloat("ftebroom_classic_map")&&!cvars->GetFloat("ftebroom_allow_engine_terrain"),"legacy94_classic_tools_guard");
    buildGeometry=buildVis=buildLight=true;buildBegin();expect(buildRunning,"legacy94_build_started");
  }
  if(std::string(mode)=="legacy94built"){
    expect(!buildRunning&&!runHadError&&!buildResult.empty()&&buildValidBsp(buildResult),"legacy94_compiled_valid_bsp");
    core->Print(("LEGACY94_BUILD "+buildMessage+"\n"+runLog+"\n").c_str());buildEdit();
  }
  if(std::string(mode)=="legacy94runtimes"){
    auto mod=std::filesystem::u8path(hlMod);hlRuntime=1;halfLifeInstallRuntime();expect(std::filesystem::exists(mod/"reyondbroom-freecs-94.pk3")&&!std::filesystem::exists(mod/"reyondbroom-freehl-94.pk3"),"legacy94_freecs_exclusive");
    hlRuntime=0;halfLifeInstallRuntime();expect(std::filesystem::exists(mod/"reyondbroom-freehl-94.pk3")&&!std::filesystem::exists(mod/"reyondbroom-freecs-94.pk3"),"legacy94_freehl_exclusive");
  }
  if(std::string(mode)=="model94hl"){modelEditorShow("models/qa/studio94.mdl");modelViewerBones=true;modelViewerPlay=true;}
  if(std::string(mode)=="model94hlcheck"){
    expect(cvars->GetFloat("ftebroom_modelviewer_count")==1&&cvars->GetFloat("ftebroom_modelviewer_bones_drawn")==2,"model94_goldsrc_animation_and_bones");
    ftebroom::EntityDefinition d;d.model="models/qa/studio94.mdl";d.name="test94_studio";entityDefinitions[d.name]=d;cvars->SetFloat("ftebroom_test_model_previews",1);cvars->SetString("ftebroom_test_entity_filter","test94_studio");selectEntityTab=true;frame();frame();expect(entityPreviews.count(d.model+":")&&entityPreviews[d.model+":"].image,"model94_goldsrc_textured_thumbnail");modelViewerOpen=false;
  }
  if(std::string(mode)=="model94q2"){modelEditorShow("models/qa/test94.md2");modelViewerBones=false;}
  if(std::string(mode)=="model94q2check"){
    expect(cvars->GetFloat("ftebroom_modelviewer_count")==1,"model94_md2_loaded");
    ftebroom::EntityDefinition d;d.model="models/qa/test94.md2";d.name="test94_md2";entityDefinitions[d.name]=d;cvars->SetFloat("ftebroom_test_model_previews",1);cvars->SetString("ftebroom_test_entity_filter","test94_md2");selectEntityTab=true;frame();frame();expect(entityPreviews.count(d.model+":")&&entityPreviews[d.model+":"].image,"model94_md2_textured_thumbnail");modelViewerOpen=false;
  }
  if(std::string(mode)=="bones94open"){modelEditorShow("models/qa/bones94.md5mesh");modelViewerBones=true;}
  if(std::string(mode)=="bones94check"){
    expect(cvars->GetFloat("ftebroom_modelviewer_bones_drawn")==2,"bones94_visible_skeletal_joints");modelViewerOpen=false;
  }
  if(std::string(mode)=="sound94play"){modelViewerOpen=false;soundSeek(.2,true);frame();}
  if(std::string(mode)=="sound94gain"){
    auto p=soundVolumeMin,b=soundVolumeMax;float x=p.x+(b.x-p.x)*.3f,y=(p.y+b.y)*.5f;auto before=soundVolume;
    event(0,0,0,x,y,1280,800);event(1,mouseKeys[0],0,x,y,1280,800);event(0,0,0,x,y,1280,800);event(2,mouseKeys[0],0,x,y,1280,800);event(0,0,0,x,y,1280,800);
    expect(soundVolume!=before&&soundPlaying,"sound94_volume_changes_without_stopping");soundStop();soundEditorOpen=false;
  }

  if(std::string(mode)=="uv93open"){
    modelEditorShow(gameProfile==2?"models/selected92.bsp":"qa5/selected90.bsp");bspUVShow();expect(bspUVOpen&&bspUVJob.valid(),"uv93_background_read_started");
  }
  if(std::string(mode)=="uv93check"){
    expect(bspUVOpen&&!bspUVJob.valid()&&!bspUVMaterials.empty(),"uv93_materials_loaded");expect(bspUVTexture&&bspUVLines,"uv93_texture_and_overlay_ready");
  }
  static std::string sound93PreviousMod;static bool sound93PreviousLibre;
  if(std::string(mode)=="sound93open"){
    sound93PreviousMod=q1Mod;sound93PreviousLibre=q1UseLibre;q1UseLibre=false;std::snprintf(q1Mod,sizeof q1Mod,"%s",root);
    auto path=std::filesystem::u8path(root)/"seek93.wav";std::string wav(44+44100*2*3,0);auto put=[&](size_t at,unsigned v,int n){for(int i=0;i<n;i++)wav[at+i]=char(v>>(i*8));};
    wav.replace(0,4,"RIFF");put(4,wav.size()-8,4);wav.replace(8,8,"WAVEfmt ");put(16,16,4);put(20,1,2);put(22,1,2);put(24,44100,4);put(28,88200,4);put(32,2,2);put(34,16,2);wav.replace(36,4,"data");put(40,wav.size()-44,4);
    for(int i=0;i<44100*3;i++)put(44+i*2,unsigned(int(8000*std::sin(i*440*6.2831853/44100))),2);soundWrite(path,wav);soundOpen(path);
    expect(soundEditorOpen&&soundTool==0&&soundBusy(),"sound93_default_seek_tool");
  }
  if(std::string(mode)=="sound93seek"){
    expect(!soundBusy()&&std::abs(soundDuration()-3)<.001,"sound93_decode_duration");auto start=soundStart,end=soundEnd;soundSeek(1.5,true);
    expect(std::abs(soundPlayCursor/44100-1.5)<.001&&soundPlaying&&soundStart==start&&soundEnd==end&&!soundTrim,"sound93_seek_plays_without_trim");soundStop();
    soundTool=1;soundTrim=true;soundStart=.5;soundEnd=2;expect(soundRangeStart()==.5&&soundRangeEnd()==2,"sound93_trim_tool_range");
    soundTool=0;expect(soundRangeStart()==0&&soundRangeEnd()==3,"sound93_seek_ignores_trim_range");soundTool=1;soundVolume=.75;soundSpeed=1.25;soundLaunch(true);
  }
  if(std::string(mode)=="sound93check"){
    expect(!soundBusy()&&std::abs(soundDuration()-1.2)<.005,"sound93_trim_volume_speed_save");bool backup=false;for(auto& entry:std::filesystem::directory_iterator(std::filesystem::u8path(root)))if(entry.path().filename().u8string().rfind("seek93.wav.ftebroom-",0)==0&&entry.path().extension()==".bak")backup=true;
    expect(backup,"sound93_original_backup_retained");std::snprintf(q1Mod,sizeof q1Mod,"%s",sound93PreviousMod.c_str());q1UseLibre=sound93PreviousLibre;
  }
  if(std::string(mode)=="bsp92viewer"){
    auto path=gameProfile==2?"models/selected92.bsp":"qa5/selected90.bsp";expect(modelViewableFile(path)&&!modelFile(path),"bsp92_view_without_converter_route");
    modelEditorShow(path);expect(modelViewerOpen&&modelEditorActive&&modelViewerIsBsp(),"bsp92_model_editor_open");
  }
  if(std::string(mode)=="bsp92viewercheck"){
    auto low=modelEditRead("ftebroom_modeleditor_low"),high=modelEditRead("ftebroom_modeleditor_high");
    expect(cvars->GetFloat("ftebroom_modelviewer_count")>0&&high[0]>low[0]&&high[1]>low[1]&&high[2]>low[2],"bsp92_model_editor_bounds_ready");
    expect(!modelEditDirty&&modelEditNodes.empty(),"bsp92_model_editor_read_only");
  }
  if(std::string(mode)=="assets92"){
    auto archive=std::filesystem::u8path(root)/"pictures92.pak";
    auto palette=modFileBytes(archive/"gfx/palette.lmp");
    auto image=fileImageDecode(archive/"gfx/conback.lmp",true,palette);
    expect(image.error.empty()&&image.bytes.size()>18&&image.bytes[16]==32,"lmp92_pak_picture_decode");
    auto font=fileImageDecode(archive/"gfx/conchars.lmp",false,palette);expect(font.error.empty()&&!font.bytes.empty(),"lmp92_pak_font_thumbnail");
    lmpShow(archive/"gfx/palette.lmp");expect(paletteOpen&&paletteError.empty()&&paletteBytes.size()==768,"lmp92_palette_dispatch");paletteOpen=false;
    lmpShow(archive/"gfx/conback.lmp");expect(imageViewerOpen&&imageViewerPath==archive/"gfx/conback.lmp","lmp92_image_dispatch");
    frame();frame();
  }
  if(std::string(mode)=="bsp92export"){
    auto target=std::filesystem::u8path(root)/"selected92";
    auto entered="\""+target.u8string()+"\"";std::snprintf(bspModelDestination,sizeof bspModelDestination,"%s",entered.c_str());bspModelAtlas=true;bspModelShow();bspModelBegin();
    expect(bspModelBusy&&bspModelTarget==target.replace_extension(".bsp"),"bsp92_quoted_extensionless_destination");
  }
  if(std::string(mode)=="bsp92check"){
    auto target=std::filesystem::u8path(root)/"selected92.bsp";
    expect(!bspModelBusy&&!runHadError&&std::filesystem::exists(target),"bsp92_compiled");
    expect(std::filesystem::exists(std::filesystem::u8path(root)/"selected92-atlas/atlas0.tga")&&std::filesystem::exists(std::filesystem::u8path(root)/"selected92-atlas/uv0.tga"),"bsp92_atlas_and_painting_template");
    core->Print(("BSP92_RESULT "+bspModelMessage+"\n").c_str());
    bool rejected=false;try{bspModelBegin();}catch(const std::exception& e){rejected=std::string(e.what()).find("existing")!=std::string::npos;}expect(rejected,"bsp92_existing_file_protected");
  }
  if(std::string(mode)=="bsp90export"){
    auto target=std::filesystem::u8path(root)/"selected90.bsp";std::snprintf(bspModelDestination,sizeof bspModelDestination,"%s",target.u8string().c_str());bspModelAtlas=true;bspModelShow();bspModelBegin();expect(bspModelBusy,"bsp90_async_started");
  }
  if(std::string(mode)=="bsp90check"){
    expect(!bspModelBusy&&!runHadError&&std::filesystem::exists(std::filesystem::u8path(root)/"selected90.bsp"),"bsp90_compiled");
    expect(std::filesystem::exists(std::filesystem::u8path(root)/"selected90-atlas/atlas0.tga")&&std::filesystem::exists(std::filesystem::u8path(root)/"selected90-atlas/uv0.tga"),"bsp90_painting_files");
    core->Print(("BSP90_RESULT "+bspModelMessage+"\n").c_str());
    newWadShow(std::filesystem::u8path(root));std::snprintf(newWadName,sizeof newWadName,"new90.wad");newWadCreate();auto bytes=q1Read(std::filesystem::u8path(root)/"new90.wad");expect(bytes.size()==12&&ftebroom::quake1::wad(bytes).empty(),"wad90_valid_empty_archive");
    bool rejected=false;try{newWadCreate();}catch(...){rejected=true;}expect(rejected&&q1Read(std::filesystem::u8path(root)/"new90.wad")==bytes,"wad90_no_overwrite");newWadOpen=false;
    paletteShow(std::filesystem::u8path(root)/"palette90.pak/gfx/palette.lmp");expect(paletteError.empty()&&paletteBytes.size()==768,"palette90_pak_read");paletteOpen=false;
  }
  if(std::string(mode)=="map75check"){
    expect(gameProfile==2&&!assetsLoading&&!setupNeeded&&assetError.empty(),"map75_resources_ready");
    expect(!documentFile.empty()&&documentError.empty()&&!documentUntitled,"map75_external_document_open");
    expect(materials.count("textures/q1test")!=0,"map75_valid_wad_survives_missing_archives");
    expect(!q1MissingWads.empty(),"map75_missing_wads_reported");
  }
  if(std::string(mode)=="plugin72open"){
    pluginsTick();expect(editorPlugins.size()==1&&editorPlugins[0].api&&editorPlugins[0].module,"plugin72_external_module_loaded");
    if(!editorPlugins.empty()){auto& p=editorPlugins[0];p.open=true;frame();if(auto window=ImGui::FindWindowByName(p.name.c_str())){ImGui::SetWindowPos(window,{10,40},ImGuiCond_Always);ImGui::SetWindowSize(window,{650,730},ImGuiCond_Always);}frame();if(auto window=ImGui::FindWindowByName(p.name.c_str()))ImGui::SetScrollY(window,window->ScrollMax.y);frame();}
  }
  if(std::string(mode)=="plugin72apply"){
    auto it=pluginButtonPositions.find("Apply");expect(it!=pluginButtonPositions.end(),"plugin72_sdk_button_drawn");
    if(it!=pluginButtonPositions.end()){auto p=it->second;event(0,0,0,p.x,p.y,1280,800);event(1,mouseKeys[0],0,p.x,p.y,1280,800);event(0,0,0,p.x,p.y,1280,800);event(2,mouseKeys[0],0,p.x,p.y,1280,800);event(0,0,0,p.x,p.y,1280,800);}
  }
  if(std::string(mode)=="plugin72done"){
    expect(!editorPlugins.empty()&&editorPlugins[0].api&&!pluginBusy(editorPlugins[0])&&cvars->GetFloat("ftebroom_settle_applied"),"plugin72_async_workflow_complete");
    if(!editorPlugins.empty()){pluginUnload(editorPlugins[0]);expect(!editorPlugins[0].api&&!editorPlugins[0].module,"plugin72_unload");pluginLoad(editorPlugins[0]);expect(editorPlugins[0].api!=nullptr,"plugin72_reload");}
  }
  if(std::string(mode)=="medit72camera"){modelEditDirty=false;modelEditorShow("models/qa/test.md3");frame();}
  if(std::string(mode)=="medit72cameraStart"){
    auto p=modelEditViewMin,s=modelEditViewSize;float x=p.x+s.x*.5f,y=p.y+s.y*.5f;
    event(0,0,0,x,y,1280,800);modelEditCameraCursor={x,y};event(1,mouseKeys[1],0,x,y,1280,800);event(3,0,0,40,20,1280,800);event(1,input->GetKeyCode("w",nullptr),0,x,y,1280,800);
    modelEditSet("ftebroom_test_modelcamera_start",modelEditCameraEye);modelEditSet("ftebroom_test_mapcamera_start",modelEditRead("ftebroom_camera_offset"));
    expect(modelEditCameraCaptured&&modelEditCameraKeys[0]&&modelEditCameraFree,"medit72_rmb_fly_capture");
  }
  if(std::string(mode)=="medit72cameraEnd"){
    frame();expect(modelEditCameraEye!=modelEditRead("ftebroom_test_modelcamera_start"),"medit72_wasd_camera_moves");
    expect(modelEditRead("ftebroom_camera_offset")==modelEditRead("ftebroom_test_mapcamera_start"),"medit72_map_camera_unchanged");
    event(2,input->GetKeyCode("w",nullptr),0,0,0,1280,800);event(2,mouseKeys[1],0,0,0,1280,800);
    expect(!modelEditCameraCaptured&&!modelEditCameraKeys[0],"medit72_cursor_release");
    expect(cvars->GetFloat("ftebroom_modeleditor_grid_lines")>0&&cvars->GetFloat("ftebroom_modeleditor_grid_lines")<=260&&modelEditOriginAxes,"medit72_bounded_floor_grid_and_axes");
    expect(cvars->GetFloat("ftebroom_sky_overlays")==0,"medit72_sky_overlays_default_off");
  }
  if(std::string(mode)=="medit72hoverStart"){
    auto p=modelEditViewMin,s=modelEditViewSize;float x=p.x+s.x*.25f,y=p.y+s.y*.4f;
    event(0,0,0,x,y,1280,800);event(1,input->GetKeyCode("w",nullptr),0,x,y,1280,800);modelEditSet("ftebroom_test_modelcamera_start",modelEditCameraEye);
    expect(modelEditCameraKeys[0]&&!modelEditCameraCaptured,"medit72_hover_wasd_without_capture");
  }
  if(std::string(mode)=="medit72hoverEnd"){
    expect(modelEditCameraEye!=modelEditRead("ftebroom_test_modelcamera_start"),"medit72_hover_camera_moves");
    event(2,input->GetKeyCode("w",nullptr),0,0,0,1280,800);expect(!modelEditDirty,"medit72_camera_does_not_edit_model");
  }
  if(std::string(mode)=="medit71open"){
    firstSetupStep=-1;setupNeeded=false;recentWindow=false;modelEditorShow("models/qa/test.md3");frame();
    if(auto window=ImGui::FindWindowByName("Model editor")){ImGui::SetWindowPos(window,{10,40},ImGuiCond_Always);ImGui::SetWindowSize(window,{1250,740},ImGuiCond_Always);}frame();
    expect(modelEditorActive&&!modelEditPlayer,"medit71_separate_editor");
  }
  if(std::string(mode)=="medit71check"){
    expect(modelEditLoaded&&modelEditNodes.size()==3,"medit71_mesh_hierarchy");
    if(modelEditNodes.size()==3){modelEditNodes[0].position={12,-8,6};modelEditNodes[0].rotation={10,20,30};modelEditNodes[0].scale={1.25f,.75f,1.5f};modelEditNodes[2].parent=1;modelEditNodes[1].position={2,3,4};modelEditChanged=true;modelEditPublish();
      auto expected=modelEditWorld(2);auto actual=modelEditRead("ftebroom_modeleditor_transform_2_origin");expect(std::abs(actual[0]-expected.m[0][3])<.001&&std::abs(actual[1]-expected.m[1][3])<.001,"medit71_parent_propagates");}
    modelViewerPlay=true;modelEditDisplay=0;frame();
  }
  if(std::string(mode)=="medit71wire"){expect(modelEditLoaded,"medit71_preview_ready");modelEditDisplay=2;frame();}
  if(std::string(mode)=="medit71pixels"){
    frame();auto path=documentPath("data/model-editor-rect.txt");std::filesystem::create_directories(path.parent_path());std::ofstream(path)<<modelEditViewMin.x<<' '<<modelEditViewMin.y<<' '<<modelEditViewSize.x<<' '<<modelEditViewSize.y;
    expect(cvars->GetFloat("ftebroom_modeleditor_drawn")==2,"medit71_actual_surfaces_submitted");
  }
  if(std::string(mode)=="medit71solid"){modelEditDisplay=1;frame();}
  if(std::string(mode)=="medit71export"){
    modelEditDisplay=0;modelEditExport();modelConverterFormat=0;std::snprintf(modelConverterDestination,sizeof modelConverterDestination,"%s/models/editor-out.md3",root);
    cvars->SetString("ftebroom_convert_source",modelViewerPath.c_str());cvars->SetFloat("ftebroom_convert_sequence",-1);cvars->SetFloat("ftebroom_convert_frames",2);cvars->SetFloat("ftebroom_convert_fps",10);cvars->SetFloat("ftebroom_convert_status",0);modelConverterPending=true;command("ftebroom_model_extract\n");
  }
  if(std::string(mode)=="medit71exports"){
    expect(!modelConverterPending&&std::filesystem::exists(std::filesystem::u8path(root)/"models/editor-out.md3"),"medit71_md3_export");
    auto source=ftebroom::modelExport::parse(q1Read(documentPath("data/model-convert.mesh")));
    for(int format:{1,2,3,4}){modelConverterFormat=format;std::snprintf(modelConverterDestination,sizeof modelConverterDestination,"%s/models/editor-out%d%s",root,format,modelConverterExtension());modelConverterSave(source);}
    expect(std::filesystem::exists(std::filesystem::u8path(root)/"models/editor-out2.mdl")&&std::filesystem::exists(std::filesystem::u8path(root)/"models/editor-out3.iqm")&&std::filesystem::exists(std::filesystem::u8path(root)/"models/editor-out4.md5mesh"),"medit71_all_export_files");modelConverterOpen=false;
    modelEditDirty=false;modelEditorShow("models/qa/bones.iqm");modelViewerBones=true;modelViewerPlay=true;
  }
  if(std::string(mode)=="medit71bones"){
    expect(cvars->GetFloat("ftebroom_modelviewer_bones")==2&&modelViewerBones,"medit71_bones_overlay");modelEditPlayer=false;modelEditDirty=false;modelEditorShow("models/qa/v_test.md3");
  }
  if(std::string(mode)=="medit71player"){
    expect(modelEditPlayer&&cvars->GetFloat("ftebroom_modeleditor_player"),"medit71_viewmodel_auto_camera");auto eye=modelEditRead("ftebroom_modeleditor_eye");expect(eye==ModelEditV{0,0,0},"medit71_player_camera_origin");modelEditPlayer=false;modelEditTool=0;frame();
  }
  if(std::string(mode)=="medit71drag"){
    auto a=modelEditGizmoStart[0],b=modelEditGizmoEnd[0];float x=a.x+(b.x-a.x)*.7f,y=a.y+(b.y-a.y)*.7f;auto before=modelEditNodes[0].position;
    event(0,0,0,x,y,1280,800);event(1,mouseKeys[0],0,x,y,1280,800);event(0,0,0,x,y,1280,800);event(0,0,0,x+45,y+15,1280,800);event(2,mouseKeys[0],0,x+45,y+15,1280,800);frame();
    expect(modelEditNodes[0].position!=before,"medit71_mouse_axis_move");
  }
  if(std::string(mode)=="medit71mdl"){modelEditDirty=false;modelEditorShow("models/editor-out2.mdl");frame();}
  if(std::string(mode)=="medit71mdlwrite"){
    modelEditNodes[0].position={3,-2,5};modelEditNodes[0].scale={.5f,1.2f,2};modelEditChanged=true;modelEditPublish();modelEditExport();
    modelConverterFormat=2;std::snprintf(modelConverterDestination,sizeof modelConverterDestination,"%s/models/editor-retained.mdl",root);
    cvars->SetString("ftebroom_convert_source",modelViewerPath.c_str());cvars->SetFloat("ftebroom_convert_sequence",-1);cvars->SetFloat("ftebroom_convert_frames",2);cvars->SetFloat("ftebroom_convert_fps",10);cvars->SetFloat("ftebroom_convert_status",0);modelConverterPending=true;command("ftebroom_model_extract\n");
  }
  if(std::string(mode)=="medit71mdlcheck"){
    auto before=q1Read(std::filesystem::u8path(root)/"models/editor-out2.mdl"),after=q1Read(std::filesystem::u8path(root)/"models/editor-retained.mdl");
    size_t count=4+ftebroom::quake1::word(before,52)*ftebroom::quake1::word(before,56)+ftebroom::quake1::word(before,60)*12+ftebroom::quake1::word(before,64)*16;
    expect(before.substr(84,count)==after.substr(84,count)&&before.size()==after.size()&&ftebroom::quake1::word(before,68)==ftebroom::quake1::word(after,68),"medit71_mdl_original_skins_uv_frames_retained");modelConverterOpen=false;
  }
  if(std::string(mode)=="medit71reloadobj"){modelEditDirty=false;modelEditorShow("models/editor-out1.obj");}
  if(std::string(mode)=="medit71reloadiqm"){modelEditDirty=false;modelEditorShow("models/editor-out3.iqm");}
  if(std::string(mode)=="medit71reloadmd5"){modelEditDirty=false;modelEditorShow("models/editor-out4.md5mesh");}
  if(std::string(mode)=="medit71reloadcheck"){expect(modelEditLoaded&&modelEditNodes.size()==3&&cvars->GetFloat("ftebroom_modeleditor_drawn")==2,"medit71_export_reopened_in_fte");}
  if(std::string(mode)=="renderer65before") {
    cvars->SetFloat("ftebroom_test_restart_revision",cvars->GetFloat("ftebroom_revision"));
    cvars->SetFloat("vid_multisample",4);command("ftebroom_renderer_restart\n");
  }
  if(std::string(mode)=="renderer65after") {
    frame();frame();float w=0,h=0;
    expect(ready&&draw->ImageSize(atlas,&w,&h)>0&&w>0&&h>0,"renderer65_font_restored");
    expect(ImGui::GetDrawData()&&ImGui::GetDrawData()->TotalVtxCount>0,"renderer65_ui_rendered");
    expect(cvars->GetFloat("ftebroom_revision")==cvars->GetFloat("ftebroom_test_restart_revision"),"renderer65_document_preserved");
    for(auto window:ImGui::GetCurrentContext()->Windows)
      if(std::string(window->Name).find("Material properties")!=std::string::npos)ImGui::SetScrollY(window,270*uiScale());
  }
  if(std::string(mode)=="resolution70set") {
    uiResolutionWidth=1024;uiResolutionHeight=768;
    cvars->SetFloat("ftebroom_test_display_revision",cvars->GetFloat("ftebroom_revision"));
    expect(uiResolutionApply(),"resolution70_apply");
  }
  if(std::string(mode)=="resolution70check") {
    frame();frame();float logical[2];unsigned pixels[2]={0,0};draw->GetVideoSize(logical,pixels);expect(ready&&pixels[0]==1024&&pixels[1]==768,"resolution70_framebuffer");
    expect(cvars->GetFloat("ftebroom_revision")==cvars->GetFloat("ftebroom_test_display_revision"),"resolution70_document_preserved");
    expect(ImGui::GetDrawData()&&ImGui::GetDrawData()->TotalVtxCount>0,"resolution70_imgui_restored");
  }
  if(std::string(mode)=="resolution70show"){uiResolutionRead();uiResolutionWindow=true;frame();}
  if(std::string(mode)=="external69load") {
    gameProfileLoad();gameProfile=2;q1UseLibre=false;firstSetupStep=-1;recentWindow=false;
    std::snprintf(q1Mod,sizeof q1Mod,"%s",root);*q1Wads=0;*q1Definitions=0;
    expect(mountAssets(),"external69_mount");
  }
  if(std::string(mode)=="external69check") {
    expect(!assetsLoading&&assetError.empty(),"external69_ready");
    char expected[128]={};cvars->GetString("ftebroom_test_expected",expected,sizeof expected);
    size_t n=0;auto bytes=(char*)files->LoadFile("external69-pak.txt",&n);
    expect(bytes&&std::string(bytes,n)==expected,"external69_pak_and_mod_priority");if(bytes)core->Free(bytes);
    bytes=(char*)files->LoadFile("external69-loose.txt",&n);
    expect(bytes&&std::string(bytes,n)=="loose-external","external69_loose");if(bytes)core->Free(bytes);
    bytes=(char*)files->LoadFile("gfx/palette.lmp",&n);
    expect(bytes&&n==768&&static_cast<unsigned char>(bytes[0])==17,"external69_palette");if(bytes)core->Free(bytes);
    bytes=(char*)files->LoadFile("progs/player.mdl",&n);
    expect(bytes&&n>84&&std::string(bytes,4)=="IDPO","external69_packed_model");if(bytes)core->Free(bytes);
    expect(materials.count("textures/q1test")!=0,"external69_nested_wad");
    expect(std::filesystem::u8path(q1Mod).is_absolute()&&std::string(q1Mod)==baseq3,"external69_saved_absolute_mod");
  }
  if(std::string(mode)=="external69empty") {
    expect(!assetsLoading&&assetError.empty()&&!materials.empty(),"external69_empty_mod_accepted");
    size_t n=0;auto bytes=files->LoadFile("external69-pak.txt",&n);
    expect(!bytes,"external69_old_mount_removed");if(bytes)core->Free(bytes);
  }
  if(std::string(mode)=="mods63model") {
    expect(cvars->GetFloat("ftebroom_modelviewer_count")>0&&cvars->GetFloat("ftebroom_modelviewer_count")==cvars->GetFloat("ftebroom_test_model_expected"),"mods63_model_rendered");
  }
  if(std::string(mode)=="mods63load") {
    gameProfileLoad();gameProfile=2;q1UseLibre=false;firstSetupStep=-1;recentWindow=false;
    std::snprintf(q1Mod,sizeof q1Mod,"%s",root);*q1Wads=0;*q1Definitions=0;
    expect(gameProfileSave()&&mountAssets(),"mods63_mount_requested");
  }
  if(std::string(mode)=="mods63check") {
    core->Print(("ReyondBroom resource status: "+assetError+"\n").c_str());
    expect(!assetsLoading&&assetError.empty(),"mods63_resources_ready");
    expect(q1Palette().size()==768&&!materials.empty(),"mods63_palette_and_materials");
    expect(!entityDefinitions.empty(),"mods63_definitions");
    modelEditorShow("progs/player.mdl");modelViewerPlay=true;
    auto model=modelResource("progs/player.mdl");expect(model.substr(0,4)=="IDPO","mods63_player_resource");
    cvars->SetFloat("ftebroom_test_model_expected",ftebroom::quake1::word(model,68));
    command("mod_memlist\n");
  }
  if(std::string(mode)=="blender62") {
    recentWindow=false;setupNeeded=false;firstSetupStep=-1;gameProfile=1;
    std::snprintf(baseq3,sizeof baseq3,"%s",root);std::snprintf(gameMod,sizeof gameMod,"%s",root);
    modelConverterShow("fixture");modelImportExternal=std::filesystem::u8path(root)/"fixtures/animated.glb";
    modelConverterSequence=1;modelConverterFrames=2;modelConverterAcceptLoss=true;
    frame();if(auto window=ImGui::FindWindowByName("Convert model")){ImGui::SetWindowPos(window,{20,60},ImGuiCond_Always);ImGui::SetWindowSize(window,{1120,700},ImGuiCond_Always);}frame();frame();auto p=modelConverterBakeCenter;
    event(0,0,0,p.x,p.y,1280,800);event(1,mouseKeys[0],0,p.x,p.y,1280,800);event(0,0,0,p.x,p.y,1280,800);event(2,mouseKeys[0],0,p.x,p.y,1280,800);frame();
    expect(modelConverterPending&&modelImportJob.valid(),"blender62_conversion_started");
  }
  if(std::string(mode)=="blender62check") {
    if(modelConverterPending){command("in 2 ftebroom_test_desktop blender62check\n");return;}
    expect(modelImportRuntimeReady(),"blender62_managed_runtime_ready");
    expect(modelConverterMessage.rfind("Saved ",0)==0,"blender62_automatic_conversion_saved");
    core->Print((modelConverterMessage+"\n").c_str());command("screenshot blender62-finished.png\nin 1 quit force\n");
  }
  if(std::string(mode)=="tools64horizontal") {
    modelViewerOpen=false;recentWindow=false;setupNeeded=false;firstSetupStep=-1;frame();
    if(auto window=ImGui::FindWindowByName("Tools")){
      ImGui::SetWindowDock(window,0,ImGuiCond_Always);ImGui::SetWindowPos(window,{80,180},ImGuiCond_Always);ImGui::SetWindowSize(window,{1060,110},ImGuiCond_Always);frame();frame();
    }
  }
  if(std::string(mode)=="model64") {
    recentWindow=false;setupNeeded=false;firstSetupStep=-1;gameProfile=1;
    std::snprintf(baseq3,sizeof baseq3,"%s",root);std::snprintf(gameMod,sizeof gameMod,"%s",root);
    auto fixture=std::filesystem::u8path(root)/"fixtures";
    for(const auto* ext:{"blend","fbx","glb"}){
      modelConverterShow("fixture");modelImportStage=fixture/(std::string("result-")+ext);modelImportExternal=fixture/(std::string("animated.")+ext);
      auto mesh=ftebroom::modelExport::parse(q1Read(modelImportStage/"model.mesh"));
      modelConverterFormat=2;auto path=std::filesystem::u8path(root)/"models"/(std::string("result-")+ext+".mdl");
      std::snprintf(modelConverterDestination,sizeof modelConverterDestination,"%s",path.u8string().c_str());modelConverterSave(mesh);
      auto bytes=q1Read(path);auto mdl=ftebroom::readMdl(bytes);float minx=1e30f,maxx=-1e30f;for(auto t:mdl.triangles)for(auto v:t){minx=std::min(minx,v.point[0]);maxx=std::max(maxx,v.point[0]);}
      expect(std::abs(minx-24)<.17f&&std::abs(maxx-56)<.17f&&ftebroom::quake1::word(bytes,68)==2,"model64_blender_mdl_bounds_frames");
      expect(std::any_of(mdl.skin.pixels.begin(),mdl.skin.pixels.end(),[](unsigned char c){return c!=0;}),"model64_embedded_texture");
      modelConverterFormat=0;path.replace_extension(".md3");std::snprintf(modelConverterDestination,sizeof modelConverterDestination,"%s",path.u8string().c_str());modelConverterSave(mesh);
      auto md3=ftebroom::readMd3(q1Read(path));expect(md3.size()==1&&std::filesystem::exists(std::filesystem::u8path(root)/md3[0].shader),"model64_md3_texture_portable");
    }
    modelConverterShow("models/qa/halflife.mdl");modelImportExternal=std::filesystem::u8path(root)/"models/qa/halflife.mdl";modelConverterFrames=2;
    cvars->SetString("ftebroom_convert_source","models/qa/halflife.mdl");cvars->SetFloat("ftebroom_convert_sequence",0);cvars->SetFloat("ftebroom_convert_frames",2);cvars->SetFloat("ftebroom_convert_fps",24);cvars->SetFloat("ftebroom_convert_status",0);modelConverterPending=true;command("ftebroom_model_extract\n");
  }
  if(std::string(mode)=="model64check") {
    expect(!modelConverterPending&&modelConverterMessage.rfind("Saved ",0)==0,"model64_halflife_converted");
    if(modelConverterMessage.rfind("Saved ",0)!=0)core->Print((modelConverterMessage+"\n").c_str());
    modelConverterOpen=false;auto source=std::filesystem::u8path(root)/"models/result-blend.mdl";auto bytes=q1Read(source);auto skinSize=4+ftebroom::quake1::word(bytes,52)*ftebroom::quake1::word(bytes,56);
    bytes[48]=2;auto skin=bytes.substr(84,skinSize);for(size_t i=4;i<skin.size();i++)skin[i]=char(2);bytes.insert(84+skinSize,skin);soundWrite(std::filesystem::u8path(root)/"models/two-skins.mdl",bytes);
    command("fs_flush\nin 0.2 ftebroom_test_desktop model64viewer\n");
  }
  if(std::string(mode)=="model64viewer") {modelEditorShow("models/two-skins.mdl");frame();}
  if(std::string(mode)=="model64skins") {
    expect(cvars->GetFloat("ftebroom_modelviewer_skins")==2,"model64_two_skins_detected");
    auto clickSkin=[&](ImVec2 p){event(0,0,0,p.x,p.y,1280,800);event(1,mouseKeys[0],0,p.x,p.y,1280,800);event(0,0,0,p.x,p.y,1280,800);event(2,mouseKeys[0],0,p.x,p.y,1280,800);frame();};
    frame();frame();clickSkin(modelViewerSkinCombo);frame();frame();clickSkin(modelViewerSkinChoices[1]);frame();
    expect(cvars->GetFloat("ftebroom_modelviewer_skin")==1,"model64_skin_selection");
    clickSkin(modelViewerSkinCombo);frame();
  }
  if(std::string(mode)=="setup63download") {
    setupInitialized=true;setupNeeded=false;gameProfileLocked=false;firstSetupStep=4;gameProfile=2;setupModChoice=0;q1UseLibre=false;
    recentWindow=true;frame();frame();expect(!recentWindow,"setup63_no_recent_during_first_run");
    cvars->SetFloat("ftebroom_skip_setup",0);
    expect(!projectLoadRequested(true)&&resourceDownloadPending&&libreBusy(),"setup63_start_download");
    frame();
  }
  if(std::string(mode)=="setup63ready") {
    expect(libreReady()&&!libreBusy()&&!resourceDownloadPending,"setup63_download_finished");
    expect(q1UseLibre&&firstSetupStep==5&&!recentWindow,"setup63_automatic_start_without_recent");
    expect(q1MaterialWad.size()>100,"setup63_libre_mapping_textures");
    try {
      auto wad=std::filesystem::u8path(libreMod())/"texture-wads/lq_dev.wad";
      auto entries=modFileList(wad);expect(!entries.empty(),"setup63_wad_folder");
      if(!entries.empty())expect(modFileBytes(entries[0].second).size()>18,"setup63_wad_texture_image");
      auto staged=q1CompilerWads(settingsPath().parent_path()/"test-build");expect(!staged.empty(),"setup63_compiler_wads");
      modFilesFolder=wad;modFilesRoot=modFileRoot();modFilesRefresh=true;frame();frame();
    }catch(const std::exception& e){expect(false,e.what());}

    cvars->SetFloat("ftebroom_skip_setup",1);
  }
  if(std::string(mode)=="setup62scale") {
    recentWindow=false;setupInitialized=true;setupNeeded=false;gameProfileLocked=false;firstSetupStep=1;frame();frame();
    for(int i=0;i<2;i++){
      frame();frame(); // Settle font and window size before the next click.
      auto p=setupScaleCenters[i];event(0,0,0,p.x,p.y,1280,800);event(1,mouseKeys[0],0,p.x,p.y,1280,800);event(0,0,0,p.x,p.y,1280,800);event(2,mouseKeys[0],0,p.x,p.y,1280,800);frame();
      expect(uiScaleChoice==i,"setup62_scale_click");
    }
    cvars->SetFloat("ftebroom_skip_setup",0);uiSaveAppearance();uiScaleChoice=0;uiLoaded=false;uiLoad();cvars->SetFloat("ftebroom_skip_setup",1);
    expect(uiScaleChoice==1,"setup62_scale_saved");frame();frame();
  }
  if(std::string(mode)=="setup61") {
    recentWindow=false;setupInitialized=true;setupNeeded=false;gameProfileLocked=false;firstSetupStep=3;
    auto click=[&](ImVec2 p){event(0,0,0,p.x,p.y,1280,800);event(1,mouseKeys[0],0,p.x,p.y,1280,800);event(0,0,0,p.x,p.y,1280,800);event(2,mouseKeys[0],0,p.x,p.y,1280,800);frame();};
    frame();frame();const int profiles[]={2,3,0};
    for(int i=0;i<3;i++){click(setupGameCenters[i]);expect(gameProfile==profiles[i],"setup61_game_choice");}
    click(setupGameCenters[0]);firstSetupStep=4;frame();frame();
    click(setupModCenters[0]);expect(setupModChoice==0,"setup61_libre_choice");
    click(setupModCenters[1]);expect(setupModChoice==1&&!q1UseLibre,"setup61_own_choice");
    auto window=ImGui::FindWindowByName("First-time setup");
    click({window->Pos.x+window->Size.x-ImGui::GetFrameHeight()*.5f,window->Pos.y+ImGui::GetFrameHeight()*.5f});
    frame();frame();expect(firstSetupStep==-1&&!ImGui::FindWindowByName("First-time setup")->Active,"setup61_wizard_close");
    setupNeeded=true;frame();frame();window=ImGui::FindWindowByName("Game / resources");
    expect(window&&window->Active,"setup61_resources_open");
    click({window->Pos.x+window->Size.x-ImGui::GetFrameHeight()*.5f,window->Pos.y+ImGui::GetFrameHeight()*.5f});
    frame();frame();expect(!setupNeeded&&!ImGui::FindWindowByName("Game / resources")->Active,"setup61_resources_stays_closed");
    firstSetupStep=3;frame();frame();
  }
  if(std::string(mode)=="setup61mods"){firstSetupStep=4;gameProfile=2;setupModChoice=0;frame();frame();}
  if(std::string(mode)=="offset60") {
    int profile=gameProfile;float value=cvars->GetFloat("r_glsl_offsetmapping");
    gameProfile=0;gameProfileRendering();cvars->SetFloat("r_glsl_offsetmapping",0);
    gameProfile=1;gameProfileRendering();expect(cvars->GetFloat("r_glsl_offsetmapping")==1,"offset60_custom_enabled");
    gameProfile=0;gameProfileRendering();expect(cvars->GetFloat("r_glsl_offsetmapping")==0,"offset60_q3_restored");
    gameProfile=2;gameProfileRendering();expect(cvars->GetFloat("r_glsl_offsetmapping")==0,"offset60_q1_unchanged");
    cvars->SetFloat("r_glsl_offsetmapping",1);gameProfile=1;gameProfileRendering();gameProfile=2;gameProfileRendering();expect(cvars->GetFloat("r_glsl_offsetmapping")==1,"offset60_user_value_preserved");
    gameProfile=profile;cvars->SetFloat("r_glsl_offsetmapping",value);
  }
  if(std::string(mode)=="setup68discord") {
    recentWindow=false;setupNeeded=false;setupInitialized=true;gameProfileLocked=false;assetsLoading=false;firstSetupStep=5;
    frame();frame();auto window=ImGui::FindWindowByName("First-time setup");
    expect(window&&window->Active&&firstSetupStep==5,"setup68_discord_step_visible");
  }
  if(std::string(mode)=="setup68finish") {
    auto p=setupFinishCenter;
    event(0,0,0,p.x,p.y,1280,800);event(1,mouseKeys[0],0,p.x,p.y,1280,800);
    event(0,0,0,p.x,p.y,1280,800);event(2,mouseKeys[0],0,p.x,p.y,1280,800);frame();
    expect(firstSetupStep==-1&&!recentWindow,"setup68_finish_without_discord");
  }
  if(std::string(mode)=="setup60") {
    recentWindow=false;setupNeeded=true;setupInitialized=true;gameProfileLocked=false;firstSetupStep=0;
    ImVec2 size{},position{};
    for(int step=0;step<6;step++) {
      firstSetupStep=step;frame();frame();auto window=ImGui::FindWindowByName("First-time setup");
      expect(window&&window->Active,"setup60_step_visible");
      if(window){if(step==0){size=window->Size;position=window->Pos;}else expect(window->Size.x==size.x&&window->Size.y==size.y&&window->Pos.x==position.x&&window->Pos.y==position.y,"setup60_stable_geometry");}
    }
    firstSetupStep=2;frame();frame();
    for(int theme=0;theme<4;theme++) {
      auto point=setupThemeCenters[theme];
      event(0,0,0,point.x,point.y,1280,800);
      event(1,mouseKeys[0],0,point.x,point.y,1280,800);event(0,0,0,point.x,point.y,1280,800);
      event(2,mouseKeys[0],0,point.x,point.y,1280,800);event(0,0,0,point.x,point.y,1280,800);
      expect(uiTheme==theme,"setup60_theme_click");
    }
    cvars->SetFloat("ftebroom_skip_setup",0);uiSaveAppearance();uiTheme=0;uiLoaded=false;uiLoad();cvars->SetFloat("ftebroom_skip_setup",1);
    expect(uiTheme==3,"setup60_theme_persisted");uiTheme=0;frame();frame();
  }
  if(std::string(mode)=="setup60resources"){firstSetupStep=4;frame();frame();}
  if(std::string(mode)=="fullscreen60"){
    firstSetupStep=-1;setupNeeded=false;ImGui::ClosePopupToLevel(0,true);
    float revision=cvars->GetFloat("ftebroom_revision"),dirty=cvars->GetFloat("ftebroom_dirty");
    uiFullscreenSet(true);expect(uiFullscreen,"fullscreen60_enabled");
#ifdef _WIN32
    RECT client;GetClientRect(desktopHost,&client);MONITORINFO monitor={sizeof monitor};GetMonitorInfo(MonitorFromWindow(desktopHost,MONITOR_DEFAULTTONEAREST),&monitor);
    expect(!(GetWindowLongPtr(desktopHost,GWL_STYLE)&WS_CAPTION)&&client.right==monitor.rcMonitor.right-monitor.rcMonitor.left&&client.bottom==monitor.rcMonitor.bottom-monitor.rcMonitor.top,"fullscreen60_window_bounds");
#endif
    int saved=0;std::ifstream(documentPath("display-mode.txt"))>>saved;expect(saved==1,"fullscreen60_saved");
    uiFullscreenSet(false);expect(!uiFullscreen,"fullscreen60_restored");
    std::ifstream(documentPath("display-mode.txt"))>>saved;expect(saved==0,"fullscreen60_windowed_saved");
    expect(cvars->GetFloat("ftebroom_revision")==revision&&cvars->GetFloat("ftebroom_dirty")==dirty,"fullscreen60_document_unchanged");
  }
  static int reload60Materials=-1,reload60Definitions=-1;
  if(std::string(mode)=="reload60capture"){
    reload60Materials=int(cvars->GetFloat("ftebroom_material_tag_epoch"));reload60Definitions=entityDefinitionEpoch;
    expect(reload60Materials>0&&reload60Definitions>0,"reload60_initialized");
  }
  if(std::string(mode)=="reload60apply"){
    gameProfile=1;gameWallMode=gameWallMode==3?0:3;
    expect(gameProfileSave()&&mountAssets(),"reload60_apply");
    // Keep large real asset collections out of the diagnostic console dump.
    cvars->SetFloat("ftebroom_skip_setup",0);
  }
  if(std::string(mode)=="reload60check"){
    expect(!assetsLoading,"reload60_finished");
    expect(int(cvars->GetFloat("ftebroom_material_tag_epoch"))==reload60Materials,"reload60_material_cache_preserved");
    expect(entityDefinitionEpoch==reload60Definitions,"reload60_entity_cache_preserved");
    expect(cvars->GetFloat("ftebroom_wall_override")==float(gameWallMode!=0),"reload60_shader_switch");
  }
  if(std::string(mode)=="wall59"){
    recentWindow=false;firstSetupStep=-1;gameProfileLocked=false;gameProfile=1;gameWallMode=4;setupNeeded=true;
    auto folder=documentPath(u8"shader-test/Шейдеры с пробелами");std::filesystem::create_directories(folder);
    size_t size=0;auto data=(char*)files->LoadFile("glsl/ftebroom_studio_wall.glsl",&size);std::string source;if(data){source.assign(data,size);core->Free(data);}
    auto path=folder/"custom.GLSL";{std::ofstream f(path,std::ios::binary);f<<source;}
    gameWallChoose(path);expect(!source.empty()&&std::string(gameWallFile)==std::filesystem::absolute(path).lexically_normal().u8string(),"wall59_unicode_selection");
    expect(gameProfileSave(),"wall59_save");prepareEditorWall();expect(cvars->GetFloat("ftebroom_wall_override")==1,"wall59_override");
    auto shader=draw->LoadImageShader("ftebroom/custom-wall-test","{ program ftebroom_selected_wall { map textures/ftebroom/grid.tga } }");expect(shader!=0,"wall59_program");if(shader)draw->UnloadImage(shader);
    expect(gameWallReadFile(path)==source,"wall59_source_unchanged");
    bool bad=false;try{gameWallReadFile(folder/"missing.glsl");}catch(...){bad=true;}expect(bad,"wall59_missing");
    {std::ofstream f(folder/"empty.glsl");}bad=false;try{gameWallReadFile(folder/"empty.glsl");}catch(...){bad=true;}expect(bad,"wall59_empty");
    gameProfile=0;prepareEditorWall();expect(cvars->GetFloat("ftebroom_wall_override")==0,"wall59_q3_isolated");gameProfile=2;prepareEditorWall();expect(cvars->GetFloat("ftebroom_wall_override")==0,"wall59_q1_isolated");gameProfile=1;prepareEditorWall();
  }
  if(std::string(mode)=="wall59browse")gameWallBrowse();
  if(std::string(mode)=="wall59reload"){
    gameProfileLoad();expect(gameWallMode==4&&std::filesystem::u8path(gameWallFile).filename()=="custom.GLSL","wall59_persisted_restart");
    gameProfile=1;prepareEditorWall();expect(cvars->GetFloat("ftebroom_wall_override")==1,"wall59_restart_apply");
  }
  if(std::string(mode)=="entity57"){
    prefabWindow=false;entityListWindow=true;entityListLoad();bool group=false,door=false;
    for(const auto& e:entityListEntries){group|=e.kind==2&&e.name=="Test Group";door|=e.classname=="func_door";}
    expect(group&&door,"entity57_list_objects");std::snprintf(entityListFilter,sizeof entityListFilter,"func_door");entityListLoad();expect(entityListMatches.size()==1,"entity57_class_filter");entityListFilter[0]=0;entityListLoad();
  }
  if(std::string(mode)=="entity57door"||std::string(mode)=="entity57group"){
    entityListLoad();for(const auto& e:entityListEntries)if(std::string(mode)=="entity57door"?e.classname=="func_door":e.kind==2&&e.name=="Test Group"){organize("focus",e.id);break;}
  }
  if(std::string(mode)=="entity57doorcheck")expect(cvars->GetFloat("ftebroom_selection_count")==1,"entity57_brush_entity_focus");
  if(std::string(mode)=="entity57groupcheck")expect(cvars->GetFloat("ftebroom_selection_count")==2&&cvars->GetFloat("ftebroom_entity_selection_count")==1,"entity57_group_focus");
  if(std::string(mode)=="prefab56"){
    recentWindow=false;setupNeeded=false;firstSetupStep=-1;prefabShow();prefabCollections.clear();
    prefabAdd(std::filesystem::u8path(root)/"collection.map");prefabSave();expect(prefabCollections.size()==1&&prefabCollections[0].data.groups.size()==1,"prefab56_collection");
  }
  if(std::string(mode)=="prefab56insert"){auto r=viewRects[viewFocus];prefabInsert(0,0,r.x+r.w*.5f,r.y+r.h*.5f);}
  if(std::string(mode)=="prefab56one"||std::string(mode)=="prefab56two"){
    organizationLoad();int matches=0;bool first=false,second=false;
    for(const auto& n:organizationNodes)if(n.kind==2){if(n.name=="Test Group"){first=true;matches++;}if(n.name=="Test Group_1"){second=true;matches++;}}
    expect(first&&matches==(std::string(mode)=="prefab56one"?1:2)&&(std::string(mode)=="prefab56one"||second),std::string(mode)=="prefab56one"?"prefab56_one_instance":"prefab56_two_unique_instances");
  }
  if(std::string(mode)=="layout55setup"){recentWindow=false;setupNeeded=false;firstSetupStep=-1;}
  if(std::string(mode)=="layout55clip")expect(cvars->GetFloat("ftebroom_tool")==2,"layout55_clip_with_russian_text");
  if(std::string(mode)=="layout55off")expect(cvars->GetFloat("ftebroom_tool")==0,"layout55_clip_toggle_off");
  if(std::string(mode)=="symmetry54"){symmetryWindow=true;recentWindow=false;setupNeeded=false;firstSetupStep=-1;}
  if(std::string(mode)=="folders52") {
    recentWindow=false;setupNeeded=false;firstSetupStep=-1;gameProfile=2;q1UseLibre=false;
    auto root=documentPath("folder-test"),source=root/std::filesystem::u8path(u8"Изображения с пробелами");std::filesystem::create_directories(source/"nested");std::filesystem::create_directories(root/"gfx");
    std::string palette(768,0);for(int i=0;i<256;i++)for(int c=0;c<3;c++)palette[i*3+c]=char(i);
    auto write=[](const std::filesystem::path& path,const std::string& data){std::ofstream out(path,std::ios::binary);out.write(data.data(),data.size());};
    write(root/"gfx/palette.lmp",palette);auto image=unityResource("textures/ftebroom/qa_red.tga");write(source/"red.tga",image);write(source/"nested/long-texture-name-example.tga",image);
    write(source/"odd.png",std::string("\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52\x00\x00\x00\x1f\x00\x00\x00\x13\x08\x02\x00\x00\x00\xe7\x0e\x41\x15\x00\x00\x00\x2c\x49\x44\x41\x54\x78\x9c\x63\x8c\x9d\x7c\x92\x81\x66\x80\x89\x76\x46\x33\x8c\x9a\x8e\x1b\x8c\x86\x3b\x2e\x30\x1a\x32\xb8\xc0\x68\xc8\xe0\x02\xa3\x21\x83\x0b\x0c\xe5\x90\x01\x00\x8d\x08\x01\xdf\x2b\x13\x7d\x5c\x00\x00\x00\x00\x49\x45\x4e\x44\xae\x42\x60\x82",101));
    write(source/"odd-jpg.jpg",std::string("\xff\xd8\xff\xe0\x00\x10\x4a\x46\x49\x46\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00\xff\xdb\x00\x43\x00\x08\x06\x06\x07\x06\x05\x08\x07\x07\x07\x09\x09\x08\x0a\x0c\x14\x0d\x0c\x0b\x0b\x0c\x19\x12\x13\x0f\x14\x1d\x1a\x1f\x1e\x1d\x1a\x1c\x1c\x20\x24\x2e\x27\x20\x22\x2c\x23\x1c\x1c\x28\x37\x29\x2c\x30\x31\x34\x34\x34\x1f\x27\x39\x3d\x38\x32\x3c\x2e\x33\x34\x32\xff\xdb\x00\x43\x01\x09\x09\x09\x0c\x0b\x0c\x18\x0d\x0d\x18\x32\x21\x1c\x21\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\x32\xff\xc0\x00\x11\x08\x00\x13\x00\x1f\x03\x01\x22\x00\x02\x11\x01\x03\x11\x01\xff\xc4\x00\x1f\x00\x00\x01\x05\x01\x01\x01\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\xff\xc4\x00\xb5\x10\x00\x02\x01\x03\x03\x02\x04\x03\x05\x05\x04\x04\x00\x00\x01\x7d\x01\x02\x03\x00\x04\x11\x05\x12\x21\x31\x41\x06\x13\x51\x61\x07\x22\x71\x14\x32\x81\x91\xa1\x08\x23\x42\xb1\xc1\x15\x52\xd1\xf0\x24\x33\x62\x72\x82\x09\x0a\x16\x17\x18\x19\x1a\x25\x26\x27\x28\x29\x2a\x34\x35\x36\x37\x38\x39\x3a\x43\x44\x45\x46\x47\x48\x49\x4a\x53\x54\x55\x56\x57\x58\x59\x5a\x63\x64\x65\x66\x67\x68\x69\x6a\x73\x74\x75\x76\x77\x78\x79\x7a\x83\x84\x85\x86\x87\x88\x89\x8a\x92\x93\x94\x95\x96\x97\x98\x99\x9a\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xff\xc4\x00\x1f\x01\x00\x03\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00\x00\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\xff\xc4\x00\xb5\x11\x00\x02\x01\x02\x04\x04\x03\x04\x07\x05\x04\x04\x00\x01\x02\x77\x00\x01\x02\x03\x11\x04\x05\x21\x31\x06\x12\x41\x51\x07\x61\x71\x13\x22\x32\x81\x08\x14\x42\x91\xa1\xb1\xc1\x09\x23\x33\x52\xf0\x15\x62\x72\xd1\x0a\x16\x24\x34\xe1\x25\xf1\x17\x18\x19\x1a\x26\x27\x28\x29\x2a\x35\x36\x37\x38\x39\x3a\x43\x44\x45\x46\x47\x48\x49\x4a\x53\x54\x55\x56\x57\x58\x59\x5a\x63\x64\x65\x66\x67\x68\x69\x6a\x73\x74\x75\x76\x77\x78\x79\x7a\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x92\x93\x94\x95\x96\x97\x98\x99\x9a\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xff\xda\x00\x0c\x03\x01\x00\x02\x11\x03\x11\x00\x3f\x00\xb3\x45\x14\x57\xd0\x1e\x18\x51\x45\x14\x00\x51\x45\x14\x00\x51\x45\x14\x01\xff\xd9",644));
    ftebroom::quake1::Texture hi{"hires",1024,16,std::vector<unsigned char>(1024*16,250)};write(source/"hires.tga",ftebroom::quake1::tga(hi,palette));
    hi.name="{mask";for(unsigned y=0;y<16;y++)for(unsigned x=0;x<512;x++)hi.pixels[y*1024+x]=255;write(source/"{mask.tga",ftebroom::quake1::tga(hi,palette));
    std::string bmp(1078,0);bmp[0]='B';bmp[1]='M';bmp[10]=54;bmp[11]=4;bmp[14]=40;bmp[28]=8;for(int i=0;i<256;i++)for(int c=0;c<3;c++)bmp[54+i*4+c]=palette[i*3+2-c];write(root/"palette.bmp",bmp);
    expect(textureFolderLoadPalette(root/"palette.bmp")==palette,"folders52_bmp_palette");
    std::string pcx(128,0);pcx[0]=10;pcx[3]=8;pcx[65]=1;pcx+=char(12);pcx+=palette;write(root/"palette.pcx",pcx);
    expect(textureFolderLoadPalette(root/"palette.pcx")==palette&&textureFolderLoadPalette(root/"gfx/palette.lmp")==palette,"folders52_palette_formats");
    bool rejected=false;write(root/"bad.lmp","bad");try{textureFolderLoadPalette(root/"bad.lmp");}catch(...){rejected=true;}expect(rejected,"folders52_invalid_palette");
    std::snprintf(q1Mod,sizeof q1Mod,"%s",root.u8string().c_str());std::snprintf(textureFolderSource,sizeof textureFolderSource,"%s",source.u8string().c_str());std::snprintf(textureFolderPalette,sizeof textureFolderPalette,"%s",(root/"gfx/palette.lmp").u8string().c_str());textureFolderShow();textureFolderStart();
  }
  if(std::string(mode)=="folders52check"||std::string(mode)=="folders52recheck") {
    textureFolderFinish();auto target=documentPath("data/texture-folder-wads")/(unityId(textureFolderPending.source)+".wad");
    expect(!textureFolderJob.valid()&&std::filesystem::exists(target),"folders52_async_wad");auto textures=ftebroom::quake1::wad(q1Read(target));expect(textures.size()==6,"folders52_recursive_images");bool resized=false;for(const auto& texture:textures)if(texture.name=="odd")resized=texture.width==32&&texture.height==32;expect(resized,"folders52_png_resize");
    bool high=false,mask=false;for(const auto& texture:textures){if(texture.name=="hires")high=texture.width==1024&&texture.height==16&&texture.pixels[0]==250;if(texture.name=="{mask")mask=texture.pixels[0]==255&&texture.pixels[1023]==250;}expect(high&&mask,"folders52_full_palette_mask");
    expect(textures[0].name.size()<=15&&textures[1].name.size()<=15&&textures[0].name!=textures[1].name,"folders52_unique_names");
    expect(std::string(q1Wads).find(target.u8string())!=std::string::npos||std::string(q1Wads).find(target.generic_u8string())!=std::string::npos,"folders52_registered_wad");
    auto build=documentPath("folder-test/compiler");std::filesystem::create_directories(build);auto staged=q1CompilerWads(build);expect(staged=="textures.wad","folders52_compiler_staging");
    if(std::string(mode)=="folders52check"){textureFolderQueue=textureFolders;textureFolderNext();}else{
      expect(std::filesystem::exists(target.u8string()+".previous"),"folders52_backup");textureFolderCancel=true;bool cancelled=false;try{textureFolderCompile(std::filesystem::u8path(textureFolderSource),textureFolderColors);}catch(...){cancelled=true;}expect(cancelled,"folders52_cancel");textureFolderCancel=false;
    }
  }
  if(std::string(mode)=="unity51") {
    recentWindow=false;setupNeeded=false;firstSetupStep=-1;
    auto project=documentPath("unity-test");std::filesystem::create_directories(project/"Assets");
    std::snprintf(unityFolder,sizeof unityFolder,"%s",project.u8string().c_str());unitySeparate=true;unityTextures=true;unityColliders=true;unityScale=.0254f;unityOpen=true;unityStart();
  }
  if(std::string(mode)=="unity51check"||std::string(mode)=="unity51recheck") {
    unityFinish();expect(!unityWaiting&&std::filesystem::exists(unityOutput/"map.ftebroom.json"),"unity51_manifest");
    auto obj=unityRead(unityOutput/"map.obj"),manifest=unityRead(unityOutput/"map.ftebroom.json");
    expect(obj.find("usemtl material_")!=std::string::npos&&obj.find("vt ")!=std::string::npos,"unity51_geometry_uv");
    expect(std::filesystem::exists(unityAssets/"ReyondBroomEditor/Editor/ReyondBroomSync.cs"),"unity51_importer");
    expect(std::filesystem::is_directory(unityOutput/"Textures")&&!std::filesystem::is_empty(unityOutput/"Textures"),"unity51_texture_copy");
    expect(!std::filesystem::exists(unityOutput/".ftebroom-lock"),"unity51_lock_removed");
    if(std::string(mode)=="unity51check"){
      std::filesystem::path assets,out;auto unknown=unityAssets/"unowned";std::filesystem::create_directories(unknown/("ReyondBroom_"+documentName));std::ofstream(unknown/("ReyondBroom_"+documentName)/"keep.txt")<<"keep";
      bool rejected=false;try{unityResolve(unknown,assets,out);}catch(...){rejected=true;}expect(rejected,"unity51_unowned_guard");
      unitySeparate=false;unityStart();
    }else expect(manifest.find("\"separate\":false")!=std::string::npos,"unity51_resync");
  }
  if(std::string(mode)=="settle44") {recentWindow=false;setupNeeded=false;firstSetupStep=-1;settleWindow=true;settleOptions.kick=0;settleOptions.duration=3;settleStart();}
  if(std::string(mode)=="settle44check") {expect(!settleJob.valid()&&!settleWaiting&&settleError.empty()&&cvars->GetFloat("ftebroom_settle_valid"),"settle44_async_preview");}
  if(std::string(mode)=="generator50") {
    recentWindow=false;setupNeeded=false;firstSetupStep=-1;
    for(int window=1;window<=8;window++){
      generatorWindow=window;auto& h=generatorHistories[window];h=GeneratorHistory{};generatorRecord(h);
      h.state.ints[0]=37;h.state.material[0]='x';generatorRecord(h);
      expect(generatorHistoryStep(false)&&h.state.ints[0]==10&&h.state.material[0]==0,"generator50_undo");
      expect(generatorHistoryStep(true)&&h.state.ints[0]==37&&h.state.material[0]=='x',"generator50_redo");
      generatorHistoryStep(false);h.state.ints[0]=41;generatorRecord(h);expect(!generatorHistoryStep(true),"generator50_branch");
      for(int i=0;i<140;i++){h.state.ints[0]=i;generatorRecord(h);}expect(h.steps.size()==128,"generator50_history_budget");
      h.state=GeneratorSettings{};generatorRecord(h);expect(h.state.ints[0]==10&&h.state.material[0]==0,"generator50_defaults");
    }
    generatorWindow=6;
  }
  if(std::string(mode)=="spawn49") {spawnMasterShow();recentWindow=false;setupNeeded=false;firstSetupStep=-1;}
  if(std::string(mode)=="array48") {arrayOpen=true;recentWindow=false;setupNeeded=false;firstSetupStep=-1;}
  if(std::string(mode)=="tools44") {
    char which[16]={};commands->Argv(2,which,sizeof which);generatorWindow=std::clamp(std::atoi(which),0,8);generatorPreview=false;recentWindow=false;setupNeeded=false;firstSetupStep=-1;
  }
  if(std::string(mode)=="convert42") {
    modelConverterShow("progs/convert.mdl");
    cvars->SetString("ftebroom_convert_source","progs/convert.mdl");cvars->SetFloat("ftebroom_convert_sequence",-1);cvars->SetFloat("ftebroom_convert_frames",2);cvars->SetFloat("ftebroom_convert_fps",10);cvars->SetFloat("ftebroom_convert_status",0);
    modelConverterPending=true;command("ftebroom_model_extract\n");
  }
  if(std::string(mode)=="convert42check") {
    auto target=std::filesystem::u8path(modelConverterDestination);
    expect(!modelConverterPending&&std::filesystem::exists(target),"model_conversion_file_created");
    if(std::filesystem::exists(target)){auto bytes=modFileBytes(target);auto surfaces=ftebroom::readMd3(bytes);expect(surfaces.size()==1&&surfaces[0].triangles.size()==2&&ftebroom::quake1::word(bytes,76)==2,"model_conversion_md3_roundtrip");}
  }
  if(std::string(mode)=="images42") {
    gameProfile=0;std::snprintf(baseq3,sizeof baseq3,"%s",root);
    recentWindow=false;setupNeeded=false;firstSetupStep=-1;
    auto path=std::filesystem::u8path(root)/"images/test.tga";
    imageViewerShow(path);frame();frame();
    modFilesOpen=true;modFilesGrid=true;modFilesFolder=std::filesystem::u8path(root)/"images";modFilesRoot=std::filesystem::u8path(root);modFilesRefresh=true;
  }
  if(std::string(mode)=="images42check") {
    auto path=std::filesystem::u8path(root)/"images/test.tga";auto& image=fileImage(path,true);float width=0,height=0;
    expect(image.error.empty()&&image.handle&&draw->ImageSize(image.handle,&width,&height)>0&&width==64&&height==32,"image_decoded_dimensions");
    expect(ImGui::FindWindowByName("Image viewer")!=nullptr,"image_viewer_window");auto before=fileImages.size();fileImage(path,true);expect(fileImages.size()==before,"image_cache_reuses_file");
    imageViewerShow(std::filesystem::u8path(root)/"images.pak/images/test.tga");
  }
  if(std::string(mode)=="images42pakcheck") {
    auto& packed=fileImage(std::filesystem::u8path(root)/"images.pak/images/test.tga",true);expect(packed.error.empty()&&packed.handle,"image_from_pak");
    expect(fileImages.size()<=256,"image_cache_bounded");clearFileImages();expect(fileImages.empty(),"image_cache_released");
  }
  if(std::string(mode)=="files43") {
    auto folder=std::filesystem::u8path(root);auto archive=folder/"browser.pk3";
    modFilesEdit(archive/"texts/readme.txt");std::snprintf(modFilesEditBuffer.data(),modFilesEditBuffer.size(),"edited in PK3");modFilesSaveText();expect(ftebroom::pak::read(archive/"texts/readme.txt")=="edited in PK3","pk3_text_editor_save");
    std::snprintf(modFilesEditBuffer.data(),modFilesEditBuffer.size(),"unsaved");modFilesEdit(folder/"note.txt");expect(modFilesEditConfirm&&modFilesEditPath==archive/"texts/readme.txt","text_editor_dirty_switch_guard");modFilesEditConfirm=false;modFilesEditPending.clear();modFilesEditOpen=false;
    modFilesDelete(archive/"delete.txt");expect(!ftebroom::pak::exists(archive/"delete.txt"),"pk3_delete_direct");
    modFilesDelete(folder/"note.txt");expect(!std::filesystem::exists(folder/"note.txt")&&!std::filesystem::exists(folder/".ftebroom-trash"),"loose_delete_direct");
    imageViewerOpen=false;modelConverterOpen=false;modFilesFolder=folder/"many-images";modFilesRefresh=true;frame();frame();
    ImGuiWindow* contents=nullptr;for(auto window:GImGui->Windows)if(std::strstr(window->Name,"Folder contents"))contents=window;
    if(contents){ImGui::SetScrollY(contents,contents->ScrollMax.y);frame();frame();float height=contents->ScrollMax.y,scroll=contents->Scroll.y;for(int i=0;i<8;i++)frame();expect(std::abs(contents->ScrollMax.y-height)<.1f&&std::abs(contents->Scroll.y-scroll)<.1f,"file_tiles_stable_at_bottom");}
    else expect(false,"file_tiles_stable_at_bottom");
    auto mounted=folder.parent_path()/"ftebroom/pak9.pk3";ftebroom::pak::replace(mounted/"mounted43.txt","updated mounted");expect(ftebroom::pak::read(mounted/"mounted43.txt")=="updated mounted","mounted_pk3_replace");
    gameProfile=1;gameWallMode=3;prepareEditorWall();expect(cvars->GetFloat("ftebroom_wall_override")==1,"custom_wall_staged");command("fs_restart fresh\n");
  }
  if(std::string(mode)=="mounted43check") {
    size_t length=0;auto bytes=(char*)files->LoadFile("mounted43.txt",&length);expect(bytes&&std::string(bytes,length)=="updated mounted","mounted_pk3_reload");if(bytes)core->Free(bytes);
    auto shader=draw->LoadImageShader("ftebroom/studio-test","{ program defaultwall { map textures/ftebroom/grid.tga } }");expect(shader!=0,"studio_wall_shader_loaded");if(shader)draw->UnloadImage(shader);
  }
  if(std::string(mode)=="import42") {
    gameProfile=0;std::snprintf(baseq3,sizeof baseq3,"%s",root);
    modFileImportMap(std::filesystem::u8path(root)/"maps/other.map",640,400);
  }
  if(std::string(mode)=="linux38") {
    uiLanguage=1;recentWindow=false;setupNeeded=true;setupInitialized=true;firstSetupStep=0;frame();frame();
    bool glyphs=true;for(auto font:uiFonts)for(int c=0x410;c<=0x44f;++c)glyphs &= font&&font->FindGlyphNoFallback((ImWchar)c)!=nullptr;
    expect(glyphs,"cyrillic_all_font_scales");
#ifndef _WIN32
    expect(!ImGui::GetIO().MouseDrawCursor,"linux_native_cursor_owner");
    expect(!linuxCursorNames[ImGuiMouseCursor_Arrow].empty(),"linux_cursor_fallback_generated");
    expect(linuxCursorShape(ImGuiMouseCursor_TextInput)==9,"linux_cursor_shape_text");
    expect(linuxCursorShape(ImGuiMouseCursor_ResizeEW)==26,"linux_cursor_shape_ew");
    sceneLook=true;frame();expect(!ImGui::GetIO().MouseDrawCursor,"linux_cursor_hidden_for_camera");
    sceneLook=false;frame();expect(!ImGui::GetIO().MouseDrawCursor,"linux_native_cursor_after_camera");
#endif
  }

#ifndef _WIN32
  if(std::string(mode)=="linux40") {
    expect(uiScaleChoice==0,"linux_default_scale_100");
    expect(gameProfile==2,"quake1_profile");
    size_t n=0;auto data=(char*)files->LoadFile("profile-priority.txt",&n);
    expect(data && std::string(data,n)=="mod","single_mod_resources");if(data)core->Free(data);
    data=(char*)files->LoadFile("gfx/palette.lmp",&n);
    expect(data && n==768,"palette_from_mod_pak");if(data)core->Free(data);
    expect(materials.count("textures/q1test")!=0,"mod_wad_loaded");
  }
  if(std::string(mode)=="linux40drop") {
    expect(std::string(q1Wads).find("external")!=std::string::npos,"wayland_external_wad_imported");
    expect(materials.count("textures/droptest")!=0,"dropped_wad_material_indexed");
  }
  if(std::string(mode)=="linux39") {
    firstSetupStep=-1;setupNeeded=false;recentWindow=false;if(GImGui->OpenPopupStack.Size)ImGui::ClosePopupToLevel(0,true);
    linuxTestWindow=linuxTestFocus=true;frame();frame();
    auto state=ImGui::GetInputTextState(ImGui::GetActiveID());expect(state!=nullptr,"linux_text_field_focused");if(state)state->SelectAll();
    expect(!ImGui::GetIO().MouseDrawCursor,"linux_native_cursor_owner");
    bool shapes=true;for(int c=0;c<ImGuiMouseCursor_COUNT;++c){ImVec2 hot,size,border[2],fill[2];shapes &= ImFontAtlasGetMouseCursorTexData(ImGui::GetIO().Fonts,c,&hot,&size,border,fill)&&size.x>0&&size.y>0;}expect(shapes,"imgui_cursor_shapes_available");
    expect(linuxCursorShape(ImGuiMouseCursor_TextInput)==9,"linux_cursor_shape_text");
    expect(linuxCursorShape(ImGuiMouseCursor_ResizeEW)==26,"linux_cursor_shape_ew");
    expect(!linuxCursorNames[ImGuiMouseCursor_ResizeEW].empty(),"linux_cursor_fallback_generated");

  }
  if(std::string(mode)=="linux39paste") {
    auto& io=ImGui::GetIO();io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_V,true);
  }
  if(std::string(mode)=="linux39release") {
    auto& io=ImGui::GetIO();io.AddKeyEvent(ImGuiKey_V,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);
  }
  if(std::string(mode)=="linux39check") {
    expect(std::string(linuxTestText)=="Привет Arch\nsecond line","wayland_external_paste_utf8_multiline");
    ImGui::SetClipboardText("Ответ из ReyondBroom");
    expect(!ImGui::GetIO().MouseDrawCursor,"linux_native_cursor_after_paste");
  }
#endif
  if(std::string(mode)=="setup37") {
    recentWindow=false;setupNeeded=true;setupInitialized=true;firstSetupStep=0;frame();frame();
    auto wizard=ImGui::FindWindowByName("First-time setup");expect(wizard&&wizard->Active,"language_setup_first");
  }
  if(std::string(mode)=="setup37profile") {firstSetupStep=3;frame();frame();}
  if(std::string(mode)=="setup37resources") {firstSetupStep=4;ImGui::ClosePopupToLevel(0,true);frame();frame();}
  if(std::string(mode)=="roots37") {
    if(!*root)throw std::runtime_error("Missing test root");
    firstSetupStep=-1;setupNeeded=false;ImGui::ClosePopupToLevel(0,true);
    auto fixture=std::filesystem::u8path(root);auto mod=fixture/"mod",sources=fixture/"sources";
    std::filesystem::create_directories(mod/"models");std::filesystem::create_directories(sources/"maps");
    gameProfile=1;std::snprintf(gameMod,sizeof gameMod,"%s",mod.u8string().c_str());std::snprintf(baseq3,sizeof baseq3,"%s",sources.u8string().c_str());
    expect(modFileInside(mod/"models/a.md3")&&modFileInside(sources/"maps/a.map")&&!modFileInside(fixture/"outside.txt"),"configured_roots_only");
    expect(modFileModelPath(sources/"models/test.md3")=="models/test.md3","authoring_model_relative_path");
    modFilesSources=true;modFilesOpen=true;frame();frame();expect(modFilesFolder==sources&&gameRuntime()==mod.u8string(),"authoring_navigation_keeps_runtime");
    modFilesSources=false;frame();frame();expect(modFilesFolder==mod,"return_to_mod_root");
  }
#ifdef _WIN32
  if(std::string(mode)=="window37") {
    desktopAttach();auto style=GetWindowLongPtr(desktopHost,GWL_STYLE);
    expect(cvars->GetFloat("vid_fullscreen")==0 && (style&WS_CAPTION),"decorated_window_default");
  }
  if(std::string(mode)=="seamless36") {
    desktopAttach();RECT client{};GetClientRect(desktopHost,&client);MONITORINFO monitor{};monitor.cbSize=sizeof monitor;GetMonitorInfo(MonitorFromWindow(desktopHost,MONITOR_DEFAULTTONEAREST),&monitor);
    auto style=GetWindowLongPtr(desktopHost,GWL_STYLE);
    expect(cvars->GetFloat("vid_fullscreen")==2&&!(style&WS_CAPTION)&&client.right==monitor.rcMonitor.right-monitor.rcMonitor.left&&client.bottom==monitor.rcMonitor.bottom-monitor.rcMonitor.top,"default_borderless_desktop_window");
  }
#endif
  if(std::string(mode)=="audio36open") {
    gameProfile=0;std::snprintf(baseq3,sizeof baseq3,"%s",root);
    soundOpen(std::filesystem::u8path(root)/"sound/test.wav");
    expect(soundBusy()&&soundEditorOpen,"audio_background_decode_started");
  }
  if(std::string(mode)=="audio36save") {
    soundFinish();expect(!soundBusy()&&soundPCM.size()==176400,"audio_decoded_two_seconds_stereo");
    soundVolume=.5f;soundSpeed=2;soundStart=.5f;soundEnd=1.5f;soundTrim=true;soundLaunch(true);
  }
  if(std::string(mode)=="audio36check") {
    soundFinish();expect(!soundBusy()&&soundPCM.size()>43000&&soundPCM.size()<45000,"audio_trim_speed_saved_in_original");
    bool backup=false;for(auto& entry:std::filesystem::directory_iterator(soundEditorPath.parent_path()))if(entry.path().extension()==".bak")backup=true;
    expect(backup,"audio_original_backup_retained");soundEditorOpen=false;
  }
  if(std::string(mode)=="text84") {
    const int languageBefore=uiLanguage,scaleBefore=uiScaleChoice;
    const char* labels[]={"Compile","Setup compile","Show BSP","Preview mod","Back to editing","Shape settings","Replace selection","Apply"};
    bool widths=true,vertices=true;
    for(int scale=0;scale<3;scale++)for(int language=0;language<7;language++){
      uiScaleChoice=scale;uiLanguage=language;uiStyle();
      ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1200,750});ImGui::Begin("Text sizing regression");
      for(auto label:labels){
        const char* translated=label;const char* end=nullptr;FteBroomTranslate(translated,end);
        float actual=ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(),FLT_MAX,0,translated,end).x;
        widths&=std::abs(ImGui::CalcTextSize(label,nullptr,true).x-std::floor(actual+.99999f))<1.1f;
        auto list=ImGui::GetWindowDrawList();int first=list->VtxBuffer.Size;compileButton(label,0);auto low=ImGui::GetItemRectMin(),high=ImGui::GetItemRectMax();
        widths&=high.x-low.x>=actual+50*uiScale();
        for(int v=first;v<list->VtxBuffer.Size;v++)vertices&=list->VtxBuffer[v].pos.x>=low.x-1&&list->VtxBuffer[v].pos.x<=high.x+1;
      }
      ImGui::End();ImGui::EndFrame();
    }
    expect(widths,"text84_translated_widths_seven_languages_three_scales");expect(vertices,"text84_button_geometry_stays_inside_bounds");
    uiLanguage=languageBefore;uiScaleChoice=scaleBefore;uiStyle();
  }
  if(std::string(mode)=="ui36") {
    recentWindow=false;uiReset=true;uiTheme=2;uiLanguage=1;frame();frame();
    expect(ImGui::GetStyle().Colors[ImGuiCol_WindowBg].x<.3f&&ImGui::GetStyle().FrameRounding==0,"retro_dark_palette");
    const char* label="File";const char* end=nullptr;FteBroomTranslate(label,end);expect(std::string(label,end)=="Файл","localized_label");
    bool glyphs=true;for(auto& row:uiTranslations)for(int language:{4,5}){const char* p=row[language];while(*p){unsigned int code;int n=ImTextCharFromUtf8(&code,p,nullptr);if(n<=0)break;glyphs&=uiFonts[1]->FindGlyphNoFallback((ImWchar)code)!=nullptr;p+=n;}}
    expect(glyphs,"complete_cjk_catalog_glyphs");
    auto explorer=ImGui::FindWindowByName("Mod file manager");
    expect(explorer&&explorer->Pos.x>=uiScenePos.x-1&&explorer->Pos.x+explorer->Size.x<=uiScenePos.x+uiSceneSize.x+1,"explorer_below_center_only");
  }
  if(std::string(mode)=="light36"){uiTheme=3;uiLanguage=4;frame();expect(ImGui::GetStyle().Colors[ImGuiCol_WindowBg].x>.7f,"modern_light_palette");}
  if(std::string(mode)=="maps36") {
    auto destination=std::filesystem::u8path(root)/"other.map";
    std::ofstream(destination)<<"// Game: Quake 3\n// Format: Quake3 (Valve)\n{\n\"classname\" \"worldspawn\"\n}\n";
    auto previous=documentFile;auto name=documentName;float dirty=cvars->GetFloat("ftebroom_dirty");cvars->SetFloat("ftebroom_dirty",1);
    modFileOpenMap(destination);
    expect(confirmDocument&&documentNext=="@"+destination.u8string()&&documentFile==previous&&documentName==name,"explorer_map_waits_for_unsaved_changes_decision");
    confirmDocument=false;uiClosedDialog="Save changes?";frame();
    expect(documentNext.empty()&&documentFile==previous,"cancel_map_open_preserves_current_document");cvars->SetFloat("ftebroom_dirty",dirty);
  }

  if(std::string(mode)=="ui"){

   uiTheme=1;cvars->SetFloat("ftebroom_skip_setup",0);uiSaveAppearance();uiTheme=0;uiLoaded=false;uiLoad();cvars->SetFloat("ftebroom_skip_setup",1);expect(uiTheme==1,"theme_persisted");selectMaterialTab=true;frame();frame();

   expect(ImGui::GetStyle().FrameRounding==0&&ImGui::GetStyle().Colors[ImGuiCol_WindowBg].x>.7,"retro_style");

#ifdef _WIN32

   desktopAttach();ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);expect(desktopProc(desktopHost,WM_SETCURSOR,(WPARAM)desktopHost,HTCLIENT)==TRUE&&!uiCursorNames[ImGuiMouseCursor_ResizeEW].empty(),"horizontal_resize_cursor");

   ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);expect(desktopProc(desktopHost,WM_SETCURSOR,(WPARAM)desktopHost,HTCLIENT)==TRUE&&!uiCursorNames[ImGuiMouseCursor_ResizeNWSE].empty(),"diagonal_resize_cursor");

#endif

  }

#ifdef _WIN32

  if(std::string(mode)=="drop"){

   selectMaterialTab=true;frame();frame();auto path=(std::filesystem::u8path(root)/"import.png").wstring();SIZE_T size=sizeof(DROPFILES)+(path.size()+2)*sizeof(wchar_t);auto memory=GlobalAlloc(GHND,size);auto drop=(DROPFILES*)GlobalLock(memory);drop->pFiles=sizeof(DROPFILES);drop->fWide=TRUE;RECT rect;GetClientRect(desktopHost,&rect);drop->pt.x=int((materialDropMin.x+20)*rect.right/ImGui::GetIO().DisplaySize.x);drop->pt.y=int((materialDropMin.y+20)*rect.bottom/ImGui::GetIO().DisplaySize.y);std::memcpy((char*)drop+sizeof(DROPFILES),path.c_str(),(path.size()+1)*sizeof(wchar_t));GlobalUnlock(memory);SendMessageW(desktopHost,WM_DROPFILES,(WPARAM)memory,0);expect(desktopDrops.size()==1,"explorer_drop_routed");frame();expect(importImages.size()==1,"import_destination_dialog");

   std::snprintf(importWad,sizeof importWad,"%s",(std::filesystem::u8path(root)/"target.wad").u8string().c_str());std::snprintf(importName,sizeof importName,"imported");importSaveImage();frame();auto textures=ftebroom::quake1::wad(q1Read(std::filesystem::u8path(importWad)));expect(textures.size()==2&&textures.back().width==32&&textures.back().height==16,"image_resized_and_appended");

  }

#endif

#ifdef _WIN32
  if(std::string(mode)=="crash31"){crashContextTick();RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr);}
#endif
  if(std::string(mode)=="wad32load") {importAddWads({std::filesystem::u8path(root)/"concrete_sh.wad"});expect(true,"wad_selected_from_map_directory");}
  if(std::string(mode)=="wad32paint") {
    auto textures=ftebroom::quake1::wad(q1Read(std::filesystem::u8path(root)/"concrete_sh.wad"));
    command("ftebroom_select all\n");chooseMaterial("textures/"+textures.front().name);expect(true,"wad_material_applied");
  }
  if(std::string(mode)=="wad32build") {gamePreviewAfterBuild=false;runLoad();buildLoadSettings();buildGeometry=true;buildVis=false;buildLight=false;buildBegin();expect(buildRunning,"real_background_build_started");}
  if(std::string(mode)=="wad32result") {
    std::ofstream(documentPath("wad32-build.log"))<<runLog<<"\n"<<buildMessage;
    expect(!buildRunning&&!runHadError&&!buildResult.empty(),"real_background_build_finished");
    if(!buildResult.empty())std::ofstream(documentPath("wad32-result.txt"))<<buildResult.u8string();
  }
  if(std::string(mode)=="recent33") {recentRemember(std::filesystem::u8path(root)/"id1/maps/wad-external.map",2);recentMaps.front().favorite=true;recentSave();recentMaps.clear();recentLoaded=false;recentLoad();expect(!recentMaps.empty()&&recentMaps.front().favorite&&recentMaps.front().profile==2,"recent_profile_favorite_persisted");recentWindow=true;}
  if(std::string(mode)=="viewer33") {recentWindow=false;modelEditorShow("models/qa/test.md3");frame();}
  if(std::string(mode)=="viewer33check") {expect(cvars->GetFloat("ftebroom_modelviewer_count")>0,"model_viewer_frames");expect(cvars->GetFloat("ftebroom_modelviewer_poses")>0,"model_viewer_poses");}
  if(std::string(mode)=="viewer33iqm") {modelEditorShow("models/qa/skeleton.iqm");modelViewerBones=true;modelViewerPlay=true;}
  if(std::string(mode)=="viewer33iqmcheck") {expect(cvars->GetFloat("ftebroom_modelviewer_bones")==2,"viewer_skeletal_bones");expect(cvars->GetFloat("ftebroom_modelviewer_poses")==2,"viewer_animation_poses");}
  if(std::string(mode)=="material33") {modelViewerOpen=false;selectMaterialTab=true;materialAdvanced=true;materialWidths[0]=materialWidths[1]=64;materialHeights[0]=materialHeights[1]=64;}
  if(std::string(mode)=="material33check") {bool found=false;for(auto& d:materialDimensions)if(d.second.x==64&&d.second.y==64)found=true;expect(found,"material_dimensions_loaded");}
  if(std::string(mode)=="profile33") {modelViewerOpen=false;documentRequest("__profile:0");}
  if(std::string(mode)=="profile33check") {expect(gameProfile==0 && documentUntitled && documentFile.empty(),"profile_switch_starter_document");}
  if(std::string(mode)=="thumbnail32") {
    modFilesOpen=false;
    draw->LoadImageShader("textures/qa/deform","{ deformVertexes wave 1 sin 0 200 0 1 { map textures/qa/tile.tga } }");
    materials["textures/qa/deform"]={"textures/qa/deform","textures/qa/tile.tga",true,0};
    cvars->SetString("ftebroom_reveal_material","qa/deform");selectMaterialTab=true;frame();frame();
    expect(thumbnail("textures/qa/deform")!=0,"isolated_thumbnail_shader");
  }
  if(std::string(mode)=="new32") {modFilesOpen=false;documentNext="__new";documentProceed();frame();expect(fileBrowserNew,"new_map_profile_dialog");}
  if(std::string(mode)=="new32result") {expect(!documentFile.empty()&&documentFile.filename()=="created32.map"&&gameProfile==2,"new_map_created_with_profile");}
  if(std::string(mode)=="files32") {
    auto root=modFileRoot()/"manager32";std::filesystem::create_directories(root/"one");std::filesystem::create_directories(root/"two");std::ofstream(root/"one/a.txt")<<"preserve";
    modFilesRoot=modFileRoot();modFilesFolder=root/"two";modFilesClipboard=root/"one/a.txt";modFilesCut=false;modFilesPaste();expect(q1Read(root/"two/a.txt")=="preserve"&&std::filesystem::exists(root/"one/a.txt"),"manager_copy_paste");
    modFilesClipboard=root/"one/a.txt";modFilesCut=true;modFilesFolder=root;modFilesPaste();expect(std::filesystem::exists(root/"a.txt")&&!std::filesystem::exists(root/"one/a.txt"),"manager_cut_paste");
    modFilesOpen=true;modFilesGrid=true;modFilesRefresh=true;frame();
  }
  if(std::string(mode)=="windows31"){modFilesOpen=true;selectMaterialTab=true;frame();}
  if(std::string(mode)=="assets31") {

#ifdef _WIN32

   auto folder=std::filesystem::u8path(root);auto external=folder/std::filesystem::u8path(u8"Рабочий стол");

   importAddWads({external/"concrete_sh.wad"});auto original=ftebroom::quake1::wad(q1Read(external/"concrete_sh.wad"));expect(!original.empty(),"unicode_wad_loaded");

   for(auto extension:{"jpg","png"}){importImages={external/(std::string("images.")+extension)};std::snprintf(importWad,sizeof importWad,"%s",(folder/"converted.wad").u8string().c_str());std::snprintf(importName,sizeof importName,"image%s",extension);importSaveImage();}

   auto converted=ftebroom::quake1::wad(q1Read(folder/"converted.wad"));expect(converted.size()==2&&converted[0].width==512&&converted[0].height==512,"real_jpg_png_converted");

   auto build=folder/"compiler";std::filesystem::create_directories(build);auto wads=q1CompilerWads(build);expect(wads.find(u8"Рабочий")==std::string::npos&&wads=="textures.wad","compiler_ascii_wads");

   auto map=ftebroom::quake1::map(q1Read(documentPath("maps/build-fixture.map")),false,wads);

   size_t at=0;int face=0;while((at=map.find("q1test",at))!=std::string::npos){auto name=face++%3==0?original.front().name:face%2?"imagejpg":"imagepng";map.replace(at,6,name);at+=name.size();}

   std::ofstream output(build/"unicode.map",std::ios::binary);output<<map;output.close();

   std::ofstream expected(build/"expected.txt");expected<<original.front().name<<"\nimagejpg\nimagepng\n";

   auto oldProfile=gameProfile;auto oldFormat=q1MapFormat;

   for(int profile=0;profile<3;profile++){gameProfile=profile;documentTerrain=false;auto bytes=documentMetadata("{\n}\n");expect(bytes.rfind("// ReyondBroom profile=",0)==0,"map_profile_marker");}

   gameProfile=oldProfile;q1MapFormat=oldFormat;

   auto destination=modFileRoot()/"import-qa";std::filesystem::create_directories(destination);auto copied=modFileCopy(external/"images.png",destination);auto copiedAgain=modFileCopy(external/"images.png",destination);expect(copied!=copiedAgain&&q1Read(copied)==q1Read(copiedAgain),"manager_copy_preserves_collision");

   auto sub=destination/"sub";std::filesystem::create_directory(sub);modFileMove(copied,sub);expect(std::filesystem::exists(sub/copied.filename())&&!std::filesystem::exists(copied),"manager_move");

   bool rejected=false;try{modFileValidate(folder.parent_path());}catch(...){rejected=true;}expect(rejected,"manager_rejects_outside_mod");

   removedCollections.insert("test collection");collectionsSave();collectionScope.clear();collectionsLoad();expect(removedCollections.count("test collection"),"removed_collection_persists");

#endif

  }

  if(std::string(mode)=="hidefiles34") {recentWindow=false;modFilesOpen=false;frame();frame();}
  if(std::string(mode)=="viewer34") {modFilesOpen=true;modFilesRoot=modFileRoot();modFilesFolder=modFileRoot()/"models/qa";modFilesRefresh=true;modelEditorShow("models/qa/skeleton.iqm");modelViewerPlay=true;modelViewerBones=true;}
  if(std::string(mode)=="viewer34check") {expect(modelViewerPlay&&cvars->GetFloat("ftebroom_modelviewer_poses")==2,"animation_with_large_thumbnail_survives");}
  if(std::string(mode)=="pak34") {
   recentWindow=false;
   modelViewerOpen=false;modFilesOpen=true;modFilesGrid=true;modFilesRoot=modFileRoot();modFilesFolder=modFileRoot()/"explorer.pak";modFilesRefresh=true;
   auto entries=modFileList(modFilesFolder);expect(entries.size()==2&&entries.front().first,"pak_root_folders");
   auto filesInPak=modFileList(modFilesFolder/"models/qa");expect(filesInPak.size()==1&&!filesInPak.front().first,"pak_nested_files");
   expect(modFileModelPath(filesInPak.front().second)=="models/qa/skeleton.iqm","pak_model_relative_path");
   auto archiveBytes=[](){std::ifstream in(modFileRoot()/"explorer.pak",std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});};auto before=archiveBytes();auto target=modFileCopy(filesInPak.front().second,modFileRoot());expect(q1Read(target)==q1Read(filesInPak.front().second),"pak_copy_extracts_exact_bytes");
   expect(archiveBytes()==before,"pak_browse_preserves_archive");
   auto imported=modFileCopy(target,modFilesFolder/"new/models");expect(q1Read(imported)==q1Read(target),"pak_import_file");
   auto originalSound=q1Read(modFilesFolder/"sound/ambient/test.wav");
   ftebroom::pak::rename(imported,modFilesFolder/"new/renamed.iqm");expect(!ftebroom::pak::exists(imported)&&q1Read(modFilesFolder/"new/renamed.iqm")==q1Read(target),"pak_rename_file");
   ftebroom::pak::replace(modFilesFolder/"config/editor.cfg","original\n");modFilesEdit(modFilesFolder/"config/editor.cfg");std::snprintf(modFilesEditBuffer.data(),modFilesEditBuffer.size(),"edited\n");modFilesSaveText();modFilesEditOpen=false;
   expect(q1Read(modFilesFolder/"config/editor.cfg")=="edited\n"&&q1Read(modFilesFolder/"sound/ambient/test.wav")==originalSound,"pak_text_edit_preserves_other_members");
   bool backupFound=false;for(const auto& e:std::filesystem::directory_iterator(modFileRoot()))if(e.path().filename().u8string().rfind("explorer.pak.ftebroom-",0)==0&&e.path().extension()==".bak")backupFound=true;expect(backupFound,"pak_edits_create_backups");
  }
  if(std::string(mode)=="modelindices34") {modFilesOpen=false;frame();frame();
   ftebroom::EntityDefinition d;d.name="stress_model";d.model="models/qa/test.md3";d.brush=false;entityDefinitions[d.name]=d;
   cvars->SetFloat("ftebroom_test_model_previews",1);cvars->SetString("ftebroom_test_entity_filter","stress_model");selectEntityTab=true;frame();frame();
   auto data=ImGui::GetDrawData();bool split=false,large=false,valid=true;
   for(int n=0;n<data->CmdListsCount;++n){auto list=data->CmdLists[n];large|=list->VtxBuffer.Size>65535;
    for(const auto& cmd:list->CmdBuffer){split|=cmd.VtxOffset>0;for(unsigned int i=0;i<cmd.ElemCount;++i){auto offset=cmd.IdxOffset+i;if(offset>=unsigned(list->IdxBuffer.Size)){valid=false;break;}if(list->IdxBuffer[offset]+cmd.VtxOffset>=unsigned(list->VtxBuffer.Size))valid=false;}}
   }
   expect(large&&split&&valid,"model_thumbnail_72k_vertices_valid_offsets");
  }

  if(std::string(mode)=="models"){

   ftebroom::EntityDefinition d;d.name="test_md3";d.model="models/qa/test.md3";d.brush=false;entityDefinitions[d.name]=d;d.name="test_mdl";d.model="models/qa/test.mdl";entityDefinitions[d.name]=d;cvars->SetFloat("ftebroom_test_model_previews",1);cvars->SetString("ftebroom_test_entity_filter","test_");selectEntityTab=true;frame();frame();

   expect(entityPreviews.count("models/qa/test.md3:")&&entityPreviews["models/qa/test.md3:"].parts.size()==2&&entityPreviews["models/qa/test.md3:"].image,"md3_textured_surfaces");expect(entityPreviews.count("models/qa/test.mdl:")&&entityPreviews["models/qa/test.mdl:"].image,"mdl_uploaded_skin");

  }

 }catch(const std::exception& e){core->Print((std::string("FTEBROOM_DESKTOP_FAIL ")+e.what()+"\n").c_str());}

}
