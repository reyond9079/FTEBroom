// SPDX-License-Identifier: GPL-2.0-or-later
#include <iomanip>
static const char* preferenceVariables[] = {
  "ftebroom_grid_fade_start","ftebroom_grid_fade_end","ftebroom_grid_aa","gl_texture_anisotropy","gl_texturemode","vid_multisample",
  "ftebroom_autosave_interval","ftebroom_autosave_count","ftebroom_drag_mode",
  "ftebroom_wheel_grid","ftebroom_wheel_invert","ftebroom_hover_highlight",
  "ftebroom_selection_dimensions","ftebroom_selection_spikes","ftebroom_view_layout",
  "ftebroom_look_speed","ftebroom_pan_speed","ftebroom_move_speed","ftebroom_fly_speed",
  "ftebroom_fov","ftebroom_invert_horizontal","ftebroom_invert_vertical",
  "ftebroom_move_to_cursor","ftebroom_alt_move","ftebroom_grid_alpha",
  "ftebroom_color_selected","ftebroom_color_locked","ftebroom_color_hover",
  "ftebroom_show_entity_names","ftebroom_show_entity_bounds","ftebroom_show_group_bounds",
  "ftebroom_show_brush_entity_bounds","ftebroom_show_edges","ftebroom_grid_visible",
  "ftebroom_texture_lock","ftebroom_uv_lock","ftebroom_auto_integer_snap","ftebroom_sky_overlays","ftebroom_bounds_visible",
  "ftebroom_new_scale_x","ftebroom_new_scale_y","ftebroom_new_angle",
  "ftebroom_new_offset_x","ftebroom_new_offset_y","ftebroom_extrude_handle_radius","ftebroom_grid_rotation",
  "ftebroom_symmetry_angle","ftebroom_symmetry_custom_center","ftebroom_symmetry_center_x","ftebroom_symmetry_center_y","ftebroom_symmetry_center_z","ftebroom_symmetry_guides","ftebroom_symmetry_relink"
};
static std::map<std::string,std::string> shortcutOverrides;
static std::string captureShortcut;
static std::string preferenceError;
static std::string shortcutVariable(const char* id)
{
  static const char hex[]="0123456789abcdef";
  std::string result="ftebroom_key_";
  for(unsigned char c : std::string(id)){result+=hex[c>>4];result+=hex[c&15];}
  return result;
}
static void shortcutApply(Action& a,const std::string& text)
{
  a.shortcutText=text;a.defaults.clear();
  std::istringstream parts(text);std::string chord;
  while(std::getline(parts,chord,';'))
  {
    KeyChord k{};
    for(auto modifier : {"Ctrl+","Alt+","Shift+"})
    {
      auto pos=chord.find(modifier);if(pos==std::string::npos)continue;
      if(modifier[0]=='C')k.ctrl=true;else if(modifier[0]=='A')k.alt=true;else k.shift=true;
      chord.erase(pos,std::strlen(modifier));
    }
    k.key=input->GetKeyCode(chord.c_str(),nullptr);
    bool functionKey=false;for(int f=1;f<=12;f++)if(k.key==input->GetKeyCode(("f"+std::to_string(f)).c_str(),nullptr))functionKey=true;
    if(k.key>0 && !functionKey)a.defaults.push_back(k);
  }
}
static void preferencesApplyActions()
{
  static bool loaded=false;
  if(!loaded)
  {
    loaded=true;std::ifstream stream(documentPath("ftebroom-preferences.cfg"));std::string line;
    while(std::getline(stream,line))
    {
      std::istringstream row(line);std::string commandName,name,value;
      if(!(row>>commandName>>name>>std::quoted(value)) || commandName!="seta")continue;
      if(name.rfind("ftebroom_key_",0)==0)shortcutOverrides[name]=value;
      else for(auto setting : preferenceVariables)if(name==setting)cvars->SetString(setting,value.c_str());
    }
    float angle=cvars->GetFloat("ftebroom_grid_rotation");
    cvars->SetFloat("ftebroom_grid_rotation",std::isfinite(angle)?std::clamp(std::round(angle),-180.f,180.f):0);
  }
  for(auto& a : actions){auto found=shortcutOverrides.find(shortcutVariable(a.id));if(found!=shortcutOverrides.end())shortcutApply(a,found->second);}
}
static bool shortcutOverlap(uint32_t a,uint32_t b)
{
  for(uint32_t group : {uint32_t(CtxAnyView),uint32_t(CtxAnyOrNoSelection),uint32_t(CtxAnyOrNoTool)})
    if((a&group)&&(b&group)&&!(a&b&group))return false;
  return true;
}
static std::set<std::string> shortcutConflicts()
{
  std::set<std::string> result;
  for(size_t i=0;i<actions.size();++i)for(size_t j=i+1;j<actions.size();++j)
  {
    auto& a=actions[i];auto& b=actions[j];
    if(!a.command||!b.command||std::strcmp(a.command,b.command)==0||!shortcutOverlap(a.context,b.context))continue;
    for(auto x:a.defaults)for(auto y:b.defaults)
      if(x.key==y.key&&x.ctrl==y.ctrl&&x.alt==y.alt&&x.shift==y.shift){result.insert(a.id);result.insert(b.id);}
  }
  return result;
}
static bool preferencesSave()
{
  auto path=documentPath("ftebroom-preferences.cfg"),temp=path;temp+=".tmp";
  std::ofstream stream(temp);
  for(auto setting:preferenceVariables){char value[256]={};cvars->GetString(setting,value,sizeof value);stream<<"seta "<<setting<<' '<<std::quoted(value)<<'\n';}
  for(auto& item:shortcutOverrides)stream<<"seta "<<item.first<<' '<<std::quoted(item.second)<<'\n';
  stream.close();if(!stream){preferenceError="Cannot write preferences.";return false;}
#ifdef _WIN32
  if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){preferenceError="Cannot replace preferences.";return false;}
#else
  std::error_code error;std::filesystem::rename(temp,path,error);if(error){preferenceError=error.message();return false;}
#endif
  preferenceError.clear();return true;
}
static bool preferencesCaptureKey(int key,bool down)
{
  if(captureShortcut.empty())return false;
  if(!down)return true;
  for(int f=1;f<=12;f++)if(key==input->GetKeyCode(("f"+std::to_string(f)).c_str(),nullptr)){preferenceError="F1-F12 are reserved and cannot be assigned.";return true;}
  for(auto modifier:{"shift","ctrl","alt"})if(key==input->GetKeyCode(modifier,nullptr))return true;
  if(key==input->GetKeyCode("escape",nullptr)){captureShortcut.clear();return true;}
  if(std::find(std::begin(mouseKeys),std::end(mouseKeys),key)!=std::end(mouseKeys))return false;
  const char* name=input->GetKeyName(key,0);if(!name||!*name)return true;
  std::string text=(ctrlHeld?"Ctrl+":"");text+=(altHeld?"Alt+":"");text+=(shiftHeld?"Shift+":"");text+=name;
  for(auto& a:actions)if(captureShortcut==a.id){shortcutApply(a,text);shortcutOverrides[shortcutVariable(a.id)]=text;break;}
  captureShortcut.clear();return true;
}
static void preferencesKeyboard()
{
  auto conflicts=shortcutConflicts();
  if(!conflicts.empty())ImGui::TextColored({1,.35f,.3f,1},"Resolve overlapping shortcuts before closing Preferences.");
  if(!captureShortcut.empty())ImGui::TextWrapped("Press a key combination (Escape cancels): %s",captureShortcut.c_str());
  static char search[128]={};ImGui::InputText("Find action",search,sizeof search);
  if(ImGui::BeginTable("Shortcuts",3,ImGuiTableFlags_ScrollY|ImGuiTableFlags_Borders|ImGuiTableFlags_RowBg,{0,300}))
  {
    ImGui::TableSetupColumn("Action");ImGui::TableSetupColumn("Shortcut");ImGui::TableSetupColumn("Context");ImGui::TableHeadersRow();
    for(auto& a:actions)
    {
      if(!a.command||*a.gesture||(*search&&!std::strstr(a.id,search)))continue;
      ImGui::PushID(a.id);ImGui::TableNextRow();ImGui::TableNextColumn();
      bool conflict=conflicts.count(a.id);if(conflict)ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1,.35f,.3f,1));
      ImGui::TextWrapped("%s",a.label);ImGui::TableNextColumn();
      if(ImGui::Selectable(a.shortcutText.empty()?"Click to assign":a.shortcutText.c_str(),captureShortcut==a.id))captureShortcut=a.id;
      if(ImGui::BeginPopupContextItem()){if(ImGui::MenuItem("Clear shortcut")){shortcutApply(a,"");shortcutOverrides[shortcutVariable(a.id)]="";}ImGui::EndPopup();}
      ImGui::TableNextColumn();ImGui::Text("0x%04x",a.context);if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",a.id);
      if(conflict)ImGui::PopStyleColor();ImGui::PopID();
    }
    ImGui::EndTable();
  }
}
static void preferencesBackups()
{
  float minutes=cvars->GetFloat("ftebroom_autosave_interval");int count=std::clamp((int)cvars->GetFloat("ftebroom_autosave_count"),1,200);
  if(ImGui::SliderFloat("Autosave interval (minutes, 0 disables)",&minutes,0,60,"%.1f"))cvars->SetFloat("ftebroom_autosave_interval",minutes);
  if(ImGui::SliderInt("Retained backups per map",&count,1,200))cvars->SetFloat("ftebroom_autosave_count",count);
  ImGui::TextWrapped("Backups are stored in the editor's autosave folder. Recovery opens an unsaved copy and leaves the source map untouched.");
}
static void testPreferences()
{
  if(!cvars->GetFloat("ftebroom_skip_setup"))return;
  initActions();auto expect=[](bool ok,const char* name){core->Print((std::string(ok?"FTEBROOM_PREFS_PASS ":"FTEBROOM_PREFS_FAIL ")+name+"\n").c_str());};
  expect(shortcutConflicts().empty(),"default_shortcuts_unambiguous");
  for(auto& a:actions)if(std::string(a.id)=="Menu/Tools/Vertex")shortcutApply(a,"c");
  auto conflicts=shortcutConflicts();expect(conflicts.count("Menu/Tools/Vertex")&&conflicts.count("Menu/Tools/Clip"),"overlap_detected");
  for(auto& a:actions)if(std::string(a.id)=="Menu/Tools/Clip")shortcutApply(a,"Shift+c");
  expect(shortcutConflicts().empty(),"reassignment_resolves_conflict");
  for(auto& a:actions)if(std::string(a.id)=="Menu/Tools/Clip"||std::string(a.id)=="Menu/Tools/Vertex")shortcutOverrides[shortcutVariable(a.id)]=a.shortcutText;
  cvars->SetFloat("ftebroom_autosave_count",7);expect(preferencesSave(),"preferences_saved");
  std::ifstream saved(documentPath("ftebroom-preferences.cfg"));std::string text((std::istreambuf_iterator<char>(saved)),{});
  expect(text.find("seta ftebroom_autosave_count \"7\"")!=std::string::npos && text.find("Shift+c")!=std::string::npos,"preferences_file_contains_values");
  preferencesRequest=true;
}
static void testPreferencesReload()
{
  if(!cvars->GetFloat("ftebroom_skip_setup"))return;initActions();bool clip=false,vertex=false;
  for(const auto& a:actions){if(std::string(a.id)=="Menu/Tools/Clip")clip=a.shortcutText=="Shift+c";if(std::string(a.id)=="Menu/Tools/Vertex")vertex=a.shortcutText=="c";}
  core->Print(clip&&vertex&&cvars->GetFloat("ftebroom_autosave_count")==7?"FTEBROOM_PREFS_PASS restart_restores_preferences\n":"FTEBROOM_PREFS_FAIL restart_restores_preferences\n");
}
