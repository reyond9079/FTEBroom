// SPDX-License-Identifier: GPL-2.0-or-later
static bool uiLoaded = false, uiReset = false, uiLocked = false;
static bool uiHasScene = false;
static ImVec2 uiScenePos, uiSceneSize;
static ImGuiID uiDockId;
static bool uiExplorerDockChecked=false;
static bool uiExplorerMarkerChecked=false;
static float uiAppliedScale = 0;
static void uiScaleDockNode(ImGuiDockNode* node, float ratio)
{
  if (!node)
    return;
  node->SizeRef.x *= ratio;
  node->SizeRef.y *= ratio;
  uiScaleDockNode(node->ChildNodes[0], ratio);
  uiScaleDockNode(node->ChildNodes[1], ratio);
}
static void uiApplyScale(float w, float h)
{
  const float scale = uiScale();
  if (uiAppliedScale != 0 && uiAppliedScale != scale)
  {
    const float ratio = scale / uiAppliedScale;
    uiScaleDockNode(ImGui::DockBuilderGetNode(uiDockId), ratio);
    for (auto window : ImGui::GetCurrentContext()->Windows)
    {
      if (window->DockIsActive || (window->Flags & ImGuiWindowFlags_ChildWindow))
        continue;
      const std::string name = window->Name;
      if (
        name != "Map" && name != "Entity" && name != "Material" && name != "Tools"
        && name != "Tool options" && !(window->Flags & ImGuiWindowFlags_Modal))
        continue;
      const ImVec2 size{
        std::min(w - 24, window->SizeFull.x * ratio),
        std::min(h - 64, window->SizeFull.y * ratio)};
      ImGui::SetWindowSize(window, size, ImGuiCond_Always);
      ImGui::SetWindowPos(
        window,
        {std::clamp(window->Pos.x, 0.f, std::max(0.f, w - size.x)),
         std::clamp(window->Pos.y, 24.f, std::max(24.f, h - size.y))},
        ImGuiCond_Always);
    }
  }
  uiAppliedScale = scale;
  uiStyle();
}
static void uiSaveAppearance()
{
  if (cvars->GetFloat("ftebroom_skip_setup"))
    return;
  std::ofstream out(documentPath("appearance.cfg"));
  out << uiScaleChoice << ' ' << uiLocked << ' ' << uiTheme << ' ' << uiLanguage << ' ' << uiCursorPercent << '\n';
}
static void uiLoad()
{
  if (uiLoaded)
    return;
  uiLoaded = true;
  if (!cvars->GetFloat("ftebroom_skip_setup"))
  {
    int scale = 0, locked = 0;
    std::ifstream appearance(documentPath("appearance.cfg"));
    if (appearance >> scale >> locked)
    {
      uiScaleChoice = std::clamp(scale, 0, 4);
      uiLocked = locked != 0;
      int theme=0;if(appearance>>theme)uiTheme=std::clamp(theme,0,4);
      int language=0;if(appearance>>language)uiLanguage=std::clamp(language,0,6);
      int cursor=uiDefaultCursorPercent;
      uiCursorPercent=(appearance>>cursor)?std::clamp(cursor,50,250):uiDefaultCursorPercent;
    }
    std::ifstream layout(documentPath("workspace.ini"), std::ios::binary);
    std::string data((std::istreambuf_iterator<char>(layout)), {});
    if (!data.empty() && data.size() <= 1024 * 1024)
      ImGui::LoadIniSettingsFromMemory(data.data(), data.size());
  }
}
static void uiSaveLayout()
{
  auto& io = ImGui::GetIO();
  if (!io.WantSaveIniSettings)
    return;
  if (!cvars->GetFloat("ftebroom_skip_setup"))
  {
    size_t size = 0;
    const char* data = ImGui::SaveIniSettingsToMemory(&size);
    std::ofstream out(documentPath("workspace.ini"), std::ios::binary);
    out.write(data, size);
  }
  io.WantSaveIniSettings = false;
}
static void uiWorkspace(float w, float h, float top, float bottom)
{
  ImGui::SetNextWindowPos({0, top});
  ImGui::SetNextWindowSize({w, std::max(1.f, h - top - bottom)});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
  ImGui::Begin(
    "##workspace",
    nullptr,
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
      | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking
      | ImGuiWindowFlags_NoSavedSettings);
  ImGui::PopStyleVar(3);
  uiDockId = ImGui::GetID("WorkspaceDock");
  const auto flags = ImGuiDockNodeFlags_PassthruCentralNode
                     | ImGuiDockNodeFlags_NoDockingOverCentralNode
                     | (uiLocked ? ImGuiDockNodeFlags_NoResize : 0);
  if (uiReset || !ImGui::DockBuilderGetNode(uiDockId))
  {
    ImGui::DockBuilderRemoveNode(uiDockId);
    ImGui::DockBuilderAddNode(uiDockId, ImGuiDockNodeFlags_DockSpace | flags);
    ImGui::DockBuilderSetNodeSize(uiDockId, {w, std::max(1.f, h - top - bottom)});
    ImGuiID center = uiDockId, right, info, explorer;
    ImGui::DockBuilderSplitNode(
      center,
      ImGuiDir_Right,
      std::min(.48f, 370 * uiScale() / std::max(1.f, w - 64 * uiScale())),
      &right,
      &center);
    ImGui::DockBuilderSplitNode(
      center,
      ImGuiDir_Up,
      std::min(.3f, 120 * uiScale() / std::max(1.f, h - top - bottom)),
      &info,
      &center);
    ImGui::DockBuilderSplitNode(center,ImGuiDir_Down,.34f,&explorer,&center);
    ImGui::DockBuilderDockWindow("Mod file manager",explorer);
    uiExplorerDockChecked=true;
    ImGui::DockBuilderDockWindow("Tools", info);
    ImGui::DockBuilderDockWindow("Map", right);
    ImGui::DockBuilderDockWindow("Material", right);
    ImGui::DockBuilderDockWindow("Entity", right);
    ImGui::DockBuilderDockWindow("FTE Engine terrain", right);
    ImGui::DockBuilderFinish(uiDockId);
    uiReset = false;
  }
  if(!uiExplorerDockChecked) {
    uiExplorerDockChecked=true;
    // Upgrade older layouts once; subsequent runs preserve the user's docking choice.
    if(!std::filesystem::exists(documentPath("explorer-layout-v1.cfg"))) {
      if(auto central=ImGui::DockBuilderGetCentralNode(uiDockId)){ImGuiID center=central->ID,explorer;ImGui::DockBuilderSplitNode(center,ImGuiDir_Down,.34f,&explorer,&center);ImGui::DockBuilderDockWindow("Mod file manager",explorer);ImGui::DockBuilderFinish(uiDockId);}
    }
  }
  if(!uiExplorerMarkerChecked) {
    uiExplorerMarkerChecked=true;
    if(!cvars->GetFloat("ftebroom_skip_setup")&&!std::filesystem::exists(documentPath("explorer-layout-v1.cfg")))std::ofstream(documentPath("explorer-layout-v1.cfg"))<<"1\n";
  }
  ImGui::DockSpace(uiDockId, {0, 0}, flags);
  if (auto node = ImGui::DockBuilderGetCentralNode(uiDockId))
  {
    uiScenePos = node->Pos;
    uiSceneSize = node->Size;
    uiHasScene = true;
  }
  ImGui::End();
}
static bool uiBlocksScene(float x, float y, bool acceptEntityDrop = false)
{
  if (!uiHasScene)
    return false;
  ImGuiWindow* hovered = nullptr;
  ImGuiWindow* underMoving = nullptr;
  ImGui::FindHoveredWindowEx({x, y}, false, &hovered, &underMoving);
  // The empty central node is a hole in the dock host. Floating panels,
  // splitters, popup windows and title-bar drags retain their input.
  auto payload = ImGui::GetDragDropPayload();
  bool entityDrop = acceptEntityDrop && payload && (payload->IsDataType("FTE_ENTITY")||payload->IsDataType("FTEBROOM_MOD_FILE")||payload->IsDataType("FTEBROOM_PREFAB"));
  return hovered != nullptr || ImGui::GetCurrentContext()->MovingWindow != nullptr
         || ImGui::GetIO().WantTextInput
         || (ImGui::GetCurrentContext()->ActiveId != 0 && !entityDrop);
}
static ImGuiWindowFlags uiPanelFlags()
{
  return ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar
         | (uiLocked ? ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize : 0);
}
static void uiPreferences()
{
  ImGui::SeparatorText("Appearance and workspace");
  if (
    ImGui::Combo(
      "UI / font scale",
      &uiScaleChoice,
      "100% (17 px)\0 125% (21 px)\0 150% (26 px)\0 175% (30 px)\0 200% (34 px)\0"))
    uiSaveAppearance();
  ImGui::SetNextItemWidth(140*uiScale());
  if(ImGui::SliderInt("Cursor size",&uiCursorPercent,50,250,"%d%%",ImGuiSliderFlags_AlwaysClamp))uiSaveAppearance();
  ImGui::SameLine();
  if(ImGui::Button("Reset cursor size")){uiCursorPercent=uiDefaultCursorPercent;uiSaveAppearance();}
  ImGui::TextWrapped("Cursor size is independent of UI / font scale and applies immediately.");
  ImGui::TextWrapped(
    "Dark panels use the TrenchBroom dark palette. Drag a panel tab to undock it or join "
    "another tab group. Drag dividers to resize.");
  if (ImGui::Checkbox("Lock panel layout", &uiLocked))
    uiSaveAppearance();
  if (ImGui::Button("Reset panel layout"))
    uiReset = true;
  ImGui::SameLine();
  if (ImGui::Button("Reset UI scale"))
  {
    uiScaleChoice = uiDefaultScaleChoice;
    uiSaveAppearance();
  }
  ImGui::TextWrapped(
    "Layout and scale are saved locally. Panels stay inside the editor window; no "
    "separate OS windows are created.");
}
