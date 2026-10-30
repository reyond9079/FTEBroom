// SPDX-License-Identifier: GPL-2.0-or-later
static bool preferencesSave();
// Action contexts are OR alternatives within each group and AND between groups.
enum ActionContext : uint32_t
{
  CtxAny = 0,
  CtxView3D = 1 << 0,
  CtxView2D = 1 << 1,
  CtxAnyView = 3,
  CtxNoSelection = 1 << 2,
  CtxNodeSelection = 1 << 3,
  CtxFaceSelection = 1 << 4,
  CtxAnySelection = CtxNodeSelection | CtxFaceSelection,
  CtxAnyOrNoSelection = CtxNoSelection | CtxAnySelection,
  CtxNoTool = 1 << 5,
  CtxBrushTool = 1 << 6,
  CtxClipTool = 1 << 7,
  CtxRotateTool = 1 << 8,
  CtxScaleTool = 1 << 9,
  CtxShearTool = 1 << 10,
  CtxVertexTool = 1 << 11,
  CtxEdgeTool = 1 << 12,
  CtxFaceTool = 1 << 13,
  CtxControlPointTool = 1 << 14,
  CtxSweepTool = 1 << 15,
  CtxSculptTool = 1 << 16,
  CtxBendTool = 1 << 17,
  CtxPolygonCutTool = 1 << 18,
  CtxPolygonSpawnTool = 1 << 19,
  CtxSpinTool = 1 << 20,
  CtxAnyHandleTool = CtxVertexTool | CtxEdgeTool | CtxFaceTool | CtxControlPointTool,
  CtxAnyTool = 0x1fffc0,
  CtxAnyOrNoTool = 0x1fffe0
};
struct KeyChord
{
  int key;
  bool ctrl, alt, shift;
};
struct Action
{
  const char* id;
  const char* label;
  uint32_t context;
  std::vector<KeyChord> defaults;
  const char* command;
  bool (*enabled)();
  std::string shortcutText;
  const char* gesture;
};
static std::vector<Action> actions;
static bool actionReference = false, preferencesRequest = false, preferencesAppearanceRequest=false;
static void preferencesApplyActions();
static void initActions()
{
  static int definitionEpoch = -1, usageEpoch = -1;
  static size_t staticCount = 0;
  static std::map<std::string, std::array<std::string, 3>> dynamicText;
  if (!actions.empty())
  {
    if (definitionEpoch == entityDefinitionEpoch && usageEpoch == entitySelectionEpoch)
      return;
    actions.resize(staticCount);
    dynamicText.clear();
  }
  auto add = [](
               const char* id,
               const char* label,
               uint32_t ctx,
               const char* chords,
               const char* cmd,
               const char* gesture) {
    Action a{id, label, ctx, {}, cmd, nullptr, chords, gesture};
    std::string keys = chords;
    size_t start = 0;
    while (start < keys.size())
    {
      auto end = keys.find(';', start);
      auto chord = keys.substr(start, end == std::string::npos ? end : end - start);
      KeyChord k{0, false, false, false};
      for (auto modifier : {"Ctrl+", "Alt+", "Shift+"})
      {
        auto pos = chord.find(modifier);
        if (pos == std::string::npos)
          continue;
        if (modifier[0] == 'C')
          k.ctrl = true;
        else if (modifier[0] == 'A')
          k.alt = true;
        else
          k.shift = true;
        chord.erase(pos, std::char_traits<char>::length(modifier));
      }
      k.key = input->GetKeyCode(chord.c_str(), nullptr);
      a.defaults.push_back(k);
      if (end == std::string::npos)
        break;
      start = end + 1;
    }
    actions.push_back(std::move(a));
  };
  if (actions.empty())
  {
#include "ftebroom_actions.inc"
    staticCount = actions.size();
  }
  definitionEpoch = entityDefinitionEpoch;
  usageEpoch = entitySelectionEpoch;
  std::set<std::string> classes;
  for (const auto& item : entityDefinitions)
    classes.insert(item.first);
  for (const auto& item : entityUsage)
    classes.insert(item.first);
  for (const auto& name : classes)
  {
    if (name == "worldspawn")
      continue;
    auto id = "View Filter/Entity class/" + name;
    auto& text = dynamicText[id];
    text = {id, "Show/hide " + name, "ftebroom_filter_class " + organizationQuote(name)};
    add(text[0].c_str(), text[1].c_str(), CtxAny, "", text[2].c_str(), "");
    auto def = entityDefinitions.find(name);
    if (def == entityDefinitions.end() || def->second.brush)
      continue;
    id = "Create Entity/" + name;
    auto& create = dynamicText[id];
    create = {
      id, "Create " + name, "ftebroom_entity_at_cursor " + organizationQuote(name)};
    add(
      create[0].c_str(),
      create[1].c_str(),
      CtxAnyView | CtxNoTool,
      "",
      create[2].c_str(),
      "");
  }
  preferencesApplyActions();
}
static uint32_t currentActionContext()
{
  uint32_t ctx = cvars->GetFloat("ftebroom_ctx_view") == 0 ? CtxView3D : CtxView2D;
  int selection = (int)cvars->GetFloat("ftebroom_ctx_selection");
  ctx |= selection == 2   ? CtxFaceSelection
         : selection == 1 ? CtxNodeSelection
                          : CtxNoSelection;
  static const uint32_t contexts[] = {
    CtxNoTool,
    CtxBrushTool,
    CtxClipTool,
    CtxRotateTool,
    CtxSweepTool,
    CtxScaleTool,
    CtxShearTool,
    CtxVertexTool,
    CtxEdgeTool,
    CtxFaceTool,
    CtxControlPointTool, CtxSculptTool, CtxBendTool, CtxPolygonCutTool, CtxPolygonSpawnTool, CtxSpinTool, CtxRotateTool};
  int tool = std::clamp((int)cvars->GetFloat("ftebroom_tool"), 0, 16);
  ctx |= contexts[tool];
  return ctx;
}
static bool actionMatches(uint32_t required, uint32_t current)
{
  for (uint32_t group :
       {uint32_t(CtxAnyView), uint32_t(CtxAnyOrNoSelection), uint32_t(CtxAnyOrNoTool)})
    if ((required & group) && !(required & current & group))
      return false;
  return true;
}
static bool actionAvailable(const Action& a)
{
  if(gameClassic() && std::string(a.id)=="Menu/Tools/Control Point")return false;
  if((int)cvars->GetFloat("ftebroom_tool")==4)
  {
    const std::string id=a.id;
    if(id.rfind("Menu/Textures/",0)==0 ||
       (id.rfind("Menu/Edit/",0)==0 && id!="Menu/Edit/Undo" && id!="Menu/Edit/Redo"))return false;
  }
  return a.command && actionMatches(a.context, currentActionContext())
         && (!a.enabled || a.enabled());
}
static void executeAction(const Action& a, bool down = true)
{
  if (!a.command)
  {
    if (down)
      core->Print(
        (std::string("ReyondBroom: ") + a.label + " is not implemented yet\n").c_str());
    return;
  }
  std::string cmd = a.command;
  if (cmd.rfind("ftebroom_hold ", 0) == 0)
  {
    const char* keys = "wsadqx";
    const char* found = std::strchr(keys, cmd.back());
    if (found)
      moveHeld[found - keys] = down;
    return;
  }
  if (!down)
    return;
  if(modEmbedded && cmd!="ftebroom_ui exit"){if(cmd=="ftebroom_ui cancel"){if(modMenuOpen)modMenuOpen=false;else modOpenControls();}return;}
  if(generatorWindow&&(cmd=="ftebroom_undo"||cmd=="ftebroom_redo")){generatorHistoryStep(cmd=="ftebroom_redo");return;}
  if (cmd.rfind("ftebroom_ui ", 0) == 0)
  {
    auto name = cmd.substr(12);
    if (name == "cancel")
    {
      if (popupOpen || ImGui::GetIO().WantTextInput)
        return;
      if (sceneLook)
      {
        cameraEnd();
        return;
      }
      if(generatorWindow){generatorClose();return;}
      sceneDrag = false;
      command("ftebroom_cancel\n");
    }
    else if (name == "texturelock" || name == "uvlock")
    {
      const char* var =
        name == "texturelock" ? "ftebroom_texture_lock" : "ftebroom_uv_lock";
      cvars->SetFloat(var, !cvars->GetFloat(var));
    }
    else if (name == "cycle" || name == "maximize")
      viewAction(name);
    else if (
      name == "copy" || name == "cut" || name == "paste" || name == "pasteoriginal")
      clipboardAction(name);
    else if (name == "replacematerial")
      replaceMaterialRequest = true;
    else if (name == "moveobjects")
      moveObjectsRequest = true;
    else if (name == "selectline")
      selectLineRequest = true;
    else if (name == "preferences")
      preferencesRequest = true;
    else if (name == "issues")
      issuesWindow = true;
    else if (name == "build")buildWindow=true;
    else if (name == "buildnow")buildBegin();
    else if (name == "bsptoggle") {if(cvars->GetFloat("ftebroom_bsp_preview"))buildEdit();else buildShow();}
    else if (name == "compile" || name == "launch" || name == "launchfte" || name == "rerun")
      runAction(name);
    else if (name == "clip")
    {
      cancelGesture();
      command("ftebroom_tool_toggle clip\n");
      selectMapTab = true;
    }
    else if (name == "new" || name == "close")
      documentRequest("__new");
    else if (name == "revert" && !documentFile.empty())
      documentRequest("@"+documentFile.u8string());
    else if (name == "exit")
      documentRequest("exit");
    else if (name == "save" && !documentUntitled)
      documentSave(documentName);
    else if (name == "save" || name == "saveas")
    {
      saveRequest = true;
      inspector = true;
    }
    else if (name == "open")
    {
      openRequest = true;
      inspector = true;
    }
    else if (name == "cameraposition")
      cameraPositionRequest = true;
    else if (name == "resources")
    {
      cancelGesture();
      setupNeeded = true;
    }
    else if (name == "grid")
      cvars->SetFloat("ftebroom_grid_visible", !cvars->GetFloat("ftebroom_grid_visible"));
    else if (name == "map")
      selectMapTab = true;
    else if (name == "exportmap")
      exportRequest = true;
    else if (name == "exportbspmodel")
      bspModelShow();
    else if (name == "exportobj")
      objRequest = true;
    else if (name == "loadpoints" || name == "loadportals")
      traceRequest = name == "loadpoints" ? 1 : 2;
    else if (name == "reloadpoints" || name == "reloadportals")
    {
      int kind = name == "reloadpoints" ? 1 : 2;
      if (!tracePaths[kind-1].empty()) traceChoose(kind, tracePaths[kind-1]);
    }
    else if (name == "group" || name == "renamegroups")
    {
      organizationNameRequest = true;
      organizationRename = name == "renamegroups";
    }
    else if (name == "entitylist")
      entityListWindow=true;
    else if (name == "prefabs")
      prefabShow();
    else if (name == "unity")
      unityShow();
    else if (name == "spawnmaster")
      spawnMasterShow();
    else if (name == "entity")
      selectEntityTab = true;
    else if (name == "material")
      selectMaterialTab = true;
    else if (name == "actions")
      actionReference = true;
    else if (name == "inspector")
      inspector = true;
    return;
  }
  if (cmd.rfind("ftebroom_tool_", 0) == 0)
  {
    cameraEnd();
    sceneDrag = sceneLook = false;
    selectMapTab = true;
  }
  if (cmd == "ftebroom_undo" || cmd == "ftebroom_redo" || cmd == "ftebroom_delete")
    cancelGesture();
  if(traceAction(cmd))return;
  command((cmd + "\n").c_str());
}
static bool dispatchAction(int key, bool down, bool inScene)
{
  initActions();
  for(int f=1;f<=12;f++){auto name="f"+std::to_string(f);if(key==input->GetKeyCode(name.c_str(),nullptr))return false;}
  // Key release always clears held actions, even after modifiers or focus changed.
  if (!down)
  {
    for (auto& a : actions)
      if (a.command && std::string(a.command).rfind("ftebroom_hold ", 0) == 0)
        for (auto& k : a.defaults)
          if (k.key == key)
            executeAction(a, false);
    return false;
  }
  if (entityPropertyFocused && !inScene)
    for (const char* name :
         {"del",
          "tab",
          "enter",
          "leftarrow",
          "rightarrow",
          "uparrow",
          "downarrow",
          "backspace",
          "a"})
      if (key == input->GetKeyCode(name, nullptr))
        return false;
  const Action* chosen = nullptr;
  // Flight modifiers change speed instead of disabling held movement shortcuts.
  if (
    !ctrlHeld && inScene && !popupOpen && !ImGui::GetIO().WantTextInput && !sceneDrag
    && cvars->GetFloat("ca_brush_view") == 0)
  {
    const char* flight = "wsadqx";
    for (int i = 0; i < 6; i++)
    {
      char name[2] = {flight[i], 0};
      if (key == input->GetKeyCode(name, nullptr))
      {
        moveHeld[i] = true;
        return true;
      }
    }
  }
  for (auto& a : actions)
  {
    if (!actionMatches(a.context, currentActionContext()))
      continue;
    if (a.enabled && !a.enabled())
      continue;
    for (auto& k : a.defaults)
    {
      if (k.key != key || k.ctrl != ctrlHeld || k.alt != altHeld || k.shift != shiftHeld)
        continue;
      bool cancel = std::string(a.id) == "Menu/Tools/Cancel";
      if ((popupOpen || ImGui::GetIO().WantTextInput) && !cancel)
        continue;
      if ((a.context & CtxAnyView) && !inScene)
        continue;
      if (
        a.command && std::string(a.command).rfind("ftebroom_hold ", 0) == 0 && sceneDrag)
        continue;
      if (chosen)
        core->Print("ReyondBroom: ambiguous shortcut\n");
      else
        chosen = &a;
    }
  }
  if (chosen)
    executeAction(*chosen);
  return chosen != nullptr;
}
static std::string historyActionLabel(const Action& a);
static void actionButton(const char* id)
{
  initActions();
  for (auto& a : actions)
    if (std::string(a.id) == id)
    {
      ImGui::BeginDisabled(!actionAvailable(a));
      if (ImGui::Button(historyActionLabel(a).c_str()))
        executeAction(a);
      ImGui::EndDisabled();
      return;
    }
}
static std::string historyActionLabel(const Action& a)
{
  const std::string id = a.id;
  if (id != "Menu/Edit/Undo" && id != "Menu/Edit/Redo")
    return a.label;
  if(generatorWindow)return std::string(a.label)+" settings";
  char name[160] = {};
  cvars->GetString(
    id == "Menu/Edit/Undo" ? "ftebroom_undo_name" : "ftebroom_redo_name",
    name,
    sizeof name);
  return std::string(a.label) + (*name ? " " + std::string(name) : "");
}
static void actionMenuGroup(const char* group)
{
  std::string prefix = std::string("Menu/") + group + "/";
  for (auto& a : actions)
    if (
      std::string(a.id).rfind(prefix, 0) == 0
      && std::string(a.id).find('/', prefix.size()) == std::string::npos)
    {
      if(gameClassic() && std::string(a.id)=="Menu/Tools/Control Point")continue;
      bool held = a.command && std::string(a.command).rfind("ftebroom_hold ", 0) == 0;
      if (
        ImGui::MenuItem(
          historyActionLabel(a).c_str(),
          a.shortcutText.c_str(),
          false,
          actionAvailable(a) && !held))
        executeAction(a);
    }
}
static bool symmetryWindow=false;
static void symmetryPanel(){
  if(!symmetryWindow)return;
  ImGui::SetNextWindowSize(ImVec2(480,420),ImGuiCond_FirstUseEver);
  if(ImGui::Begin("Symmetry settings",&symmetryWindow)){
          int angle=(int)cvars->GetFloat("ftebroom_symmetry_angle");
          if(ImGui::SliderInt("Plane angle",&angle,-180,180,"%d degrees")){cvars->SetFloat("ftebroom_symmetry_angle",angle);preferencesSave();}
          bool custom=cvars->GetFloat("ftebroom_symmetry_custom_center")!=0;
          if(ImGui::Checkbox("Custom symmetry center",&custom)){cvars->SetFloat("ftebroom_symmetry_custom_center",custom);preferencesSave();}
          if(custom){float center[3]={cvars->GetFloat("ftebroom_symmetry_center_x"),cvars->GetFloat("ftebroom_symmetry_center_y"),cvars->GetFloat("ftebroom_symmetry_center_z")};
            if(ImGui::InputFloat3("Center XYZ",center,"%.1f")){cvars->SetFloat("ftebroom_symmetry_center_x",center[0]);cvars->SetFloat("ftebroom_symmetry_center_y",center[1]);cvars->SetFloat("ftebroom_symmetry_center_z",center[2]);preferencesSave();}}
          bool relink=cvars->GetFloat("ftebroom_symmetry_relink")!=0;
          if(ImGui::Checkbox("Relink matching existing groups",&relink)){cvars->SetFloat("ftebroom_symmetry_relink",relink);preferencesSave();}
          bool guides=cvars->GetFloat("ftebroom_symmetry_guides")!=0;
          if(ImGui::Checkbox("Show symmetry guides",&guides)){cvars->SetFloat("ftebroom_symmetry_guides",guides);preferencesSave();}
          ImGui::TextDisabled("Choose a mode below to apply these settings.");

    ImGui::Separator();actionMenuGroup("Edit/Symmetry");
    ImGui::TextWrapped("Existing groups are relinked only when every side matches. Otherwise new copies are created. Later linked edits synchronize geometry and unprotected entity properties.");
  }ImGui::End();
}
static void actionMenus()
{
  initActions();
  for (auto group : {"File", "Edit", "View", "Run", "Help"})
    if (ImGui::BeginMenu(group))
    {
      actionMenuGroup(group);
      if(std::string(group)=="Edit") {
        if(ImGui::BeginMenu("Symmetry")){
          actionMenuGroup("Edit/Symmetry");ImGui::Separator();
          if(ImGui::MenuItem("Settings..."))symmetryWindow=true;
          ImGui::TextDisabled("Open a side's group to edit all sides.");ImGui::EndMenu();
        }
        if(ImGui::MenuItem("Terrain generator...")){command("ftebroom_generator cancel\n");generatorWindow=1;generatorPreview=false;}
        if(ImGui::MenuItem("Subdivision...")){command("ftebroom_generator cancel\n");generatorWindow=6;generatorPreview=false;}
        if(ImGui::MenuItem("Rock...")){command("ftebroom_generator cancel\n");generatorWindow=8;generatorPreview=false;}
        if(ImGui::MenuItem("Spin...")){command("ftebroom_tool_toggle spin\n");generatorWindow=0;generatorPreview=false;}
        if(ImGui::MenuItem("Settle...")){command("ftebroom_generator cancel\n");generatorWindow=0;settleWindow=true;}
        if(ImGui::MenuItem("Inset...")){command("ftebroom_generator cancel\n");generatorWindow=5;generatorPreview=false;}
        if(ImGui::MenuItem("Array..."))arrayOpen=true;
        if(ImGui::MenuItem("Perforate...")){command("ftebroom_generator cancel\n");generatorWindow=4;generatorPreview=false;}
        if(ImGui::MenuItem("Fracture...")){command("ftebroom_generator cancel\n");generatorWindow=3;generatorPreview=false;}
        if(ImGui::MenuItem("Bevel...")){command("ftebroom_generator cancel\n");generatorWindow=2;generatorPreview=false;}
      }
      if(std::string(group)=="Edit" && gameProfile==1)ImGui::MenuItem("Create FTE Engine terrain...",nullptr,&terrainCreateWindow);
      if(std::string(group)=="Help" && ImGui::BeginMenu("Updates")){updatePanel();ImGui::EndMenu();}
      if(std::string(group)=="View") {
        if(ImGui::MenuItem("Preferences...")){preferencesRequest=true;preferencesAppearanceRequest=true;}
        bool outlines=cvars->GetFloat("ftebroom_show_edges")!=0;
        if(ImGui::MenuItem("Brush outlines",nullptr,outlines))cvars->SetFloat("ftebroom_show_edges",!outlines);
        if(ImGui::MenuItem("Resolution...")){uiResolutionRead();uiResolutionWindow=true;}
        if(ImGui::MenuItem("Fullscreen",nullptr,uiFullscreen))uiFullscreenSet(!uiFullscreen);
        if(!uiFullscreenError.empty())ImGui::TextWrapped("%s",uiFullscreenError.c_str());
        ImGui::Separator();
      }
      if(std::string(group)=="View"&&ImGui::BeginMenu("Language")) {
        for(int language=0;language<7;language++)if(ImGui::MenuItem(uiLanguageNames[language],nullptr,uiLanguage==language)){uiLanguage=language;uiSaveAppearance();}
        ImGui::EndMenu();
      }
      if(std::string(group)=="View" && ImGui::BeginMenu("Interface theme")) {
        for(int t=0;t<5;t++)if(ImGui::MenuItem(uiThemeNames[t],nullptr,uiTheme==t)){uiTheme=t;uiSaveAppearance();}
        ImGui::EndMenu();
      }
      if (std::string(group) == "View" && ImGui::BeginMenu("Grid"))
      {
        actionMenuGroup("View/Grid");
        int angle=(int)std::round(cvars->GetFloat("ftebroom_grid_rotation"));
        if(ImGui::SliderInt("Grid rotation",&angle,-180,180,"%d degrees")){cvars->SetFloat("ftebroom_grid_rotation",angle);preferencesSave();}
        if(ImGui::MenuItem("Reset grid rotation")){command("ftebroom_grid reset\n");}
        ImGui::EndMenu();
      }
      if (std::string(group) == "Edit")
        for (auto nested : {"Tools", "Transform", "Textures"})
          if (ImGui::BeginMenu(nested))
          {
            actionMenuGroup(nested);
            ImGui::EndMenu();
          }
      if (std::string(group) == "View" && ImGui::BeginMenu("Camera"))
      {
        actionMenuGroup("Camera");
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
}
static void actionReferenceWindow()
{
  if (actionReference)
  {
    ImGui::OpenPopup("Action reference");
    actionReference = false;
  }
  ImGui::SetNextWindowSize({760, 500}, ImGuiCond_FirstUseEver);
  if (uiDialog("Action reference",{760,540}))
  {
    if (ImGui::Button("Close") || ImGui::IsKeyPressed(ImGuiKey_Escape))
      ImGui::CloseCurrentPopup();
    ImGui::TextWrapped(
      "Reserved actions are not implemented. Mouse gestures are listed as roadmap "
      "references.");
    if (
      ImGui::BeginTable(
        "actions",
        3,
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg,
        {0, 400}))
    {
      ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 4);
      ImGui::TableSetupColumn(
        "Shortcut / gesture", ImGuiTableColumnFlags_WidthStretch, 3);
      ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 2);
      ImGui::TableHeadersRow();
      for (auto& a : actions)
      {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", a.label);
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip("%s", a.id);
        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", *a.gesture ? a.gesture : a.shortcutText.c_str());
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(
          *a.gesture  ? "Gesture reference"
          : a.command ? "Implemented"
                      : "Reserved");
      }
      ImGui::EndTable();
    }
    ImGui::EndPopup();
  }
}
static void dumpActions()
{
  initActions();
  core->Print("FTEBROOM_ACTIONS_BEGIN\n");
  for (auto& a : actions)
  {
    std::string codes;
    for (auto& k : a.defaults)
    {
      if (!codes.empty())
        codes += ";";
      codes += std::to_string(k.key) + ":" + std::to_string(k.ctrl)
               + std::to_string(k.alt) + std::to_string(k.shift);
    }
    core->Print((std::string("FTEBROOM_ACTION\t") + a.id + "\t" + a.label + "\t"
                 + std::to_string(a.context) + "\t" + a.shortcutText + "\t" + codes + "\t"
                 + (a.command ? "implemented" : "reserved") + "\t" + a.gesture + "\n")
                  .c_str());
  }
  core->Print("FTEBROOM_ACTIONS_END\n");
}
