// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef _WIN32
static constexpr int uiDefaultScaleChoice = 1;
#else
static constexpr int uiDefaultScaleChoice = 0;
#endif
static int uiScaleChoice = uiDefaultScaleChoice;
static constexpr int uiDefaultCursorPercent=100;
static int uiCursorPercent=uiDefaultCursorPercent;
static int uiTheme = 0;
static const char* uiThemeNames[]={"Modern Dark","Retro Light","Retro Dark","Modern Light","Half-Life"};
static ImFont* uiFonts[5] = {};
static ImFont* uiItalicFonts[5] = {};
static ImFont* uiBoldFonts[5] = {};
static float uiScale()
{
  return 1.f + .25f * uiScaleChoice;
}
static ImVec4 uiColor(int r, int g, int b)
{
  return {r / 255.f, g / 255.f, b / 255.f, 1};
}
static void uiStyle()
{
  // Match the dark palette's panel, button, text and highlight roles.
  ImGui::StyleColorsDark();
  auto& s = ImGui::GetStyle();
  s = ImGuiStyle();
  ImGui::StyleColorsDark(&s);
  auto& c = s.Colors;
  c[ImGuiCol_Text] = uiColor(207, 207, 207);
  c[ImGuiCol_TextDisabled] = uiColor(128, 128, 128);
  c[ImGuiCol_WindowBg] = uiColor(50, 50, 50);
  c[ImGuiCol_ChildBg] = uiColor(27, 27, 27);
  c[ImGuiCol_PopupBg] = uiColor(35, 35, 35);
  c[ImGuiCol_Border] = uiColor(24, 24, 24);
  c[ImGuiCol_FrameBg] = uiColor(27, 27, 27);
  c[ImGuiCol_FrameBgHovered] = uiColor(55, 55, 55);
  c[ImGuiCol_FrameBgActive] = uiColor(65, 65, 65);
  c[ImGuiCol_TitleBg] = uiColor(40, 40, 40);
  c[ImGuiCol_TitleBgActive] = uiColor(50, 50, 50);
  c[ImGuiCol_MenuBarBg] = uiColor(40, 40, 40);
  c[ImGuiCol_Button] = uiColor(65, 75, 90);
  c[ImGuiCol_ButtonHovered] = uiColor(62, 82, 112);
  c[ImGuiCol_ButtonActive] = uiColor(62, 112, 205);
  c[ImGuiCol_Header] = uiColor(55, 70, 95);
  c[ImGuiCol_HeaderHovered] = uiColor(62, 92, 155);
  c[ImGuiCol_HeaderActive] = uiColor(62, 112, 205);
  c[ImGuiCol_CheckMark] = c[ImGuiCol_SliderGrab] = uiColor(100, 153, 240);
  c[ImGuiCol_SliderGrabActive] = uiColor(140, 180, 250);
  c[ImGuiCol_Tab] = uiColor(35, 35, 35);
  c[ImGuiCol_TabSelected] = uiColor(55, 70, 95);
  c[ImGuiCol_TabHovered] = uiColor(62, 112, 205);
  c[ImGuiCol_TabDimmed] = uiColor(35, 35, 35);
  c[ImGuiCol_TabDimmedSelected] = uiColor(50, 50, 50);
  c[ImGuiCol_TabSelectedOverline] = uiColor(62, 112, 205);
  c[ImGuiCol_Separator] = uiColor(28, 28, 28);
  c[ImGuiCol_SeparatorHovered] = c[ImGuiCol_SeparatorActive] = uiColor(62, 112, 205);
  c[ImGuiCol_DockingEmptyBg] = uiColor(40, 40, 40);
  s.WindowRounding = 3;
  s.ChildRounding = 2;
  s.FrameRounding = 3;
  s.GrabRounding = 3;
  s.PopupRounding = 4;
  s.WindowBorderSize = 1;
  s.FrameBorderSize = 0;
  s.WindowPadding = {8, 8};
  s.FramePadding = {6, 4};
  s.ItemSpacing = {8, 6};
  s.WindowMinSize = {46, 46};
  if (uiTheme == 1) {
    for (auto slot : {ImGuiCol_WindowBg,ImGuiCol_PopupBg,ImGuiCol_MenuBarBg,ImGuiCol_Button,ImGuiCol_ButtonHovered,ImGuiCol_ButtonActive,ImGuiCol_Tab,ImGuiCol_TabSelected,ImGuiCol_TabDimmed,ImGuiCol_TabDimmedSelected,ImGuiCol_ScrollbarBg}) c[slot]=uiColor(192,192,192);
    c[ImGuiCol_Text]=uiColor(0,0,0);c[ImGuiCol_TextDisabled]=uiColor(100,100,100);
    c[ImGuiCol_ChildBg]=uiColor(212,208,200);
    c[ImGuiCol_FrameBg]=c[ImGuiCol_FrameBgHovered]=c[ImGuiCol_FrameBgActive]=uiColor(255,255,255);
    c[ImGuiCol_TitleBg]=uiColor(128,128,128);c[ImGuiCol_TitleBgActive]=uiColor(0,0,128);
    c[ImGuiCol_Header]=uiColor(180,180,192);c[ImGuiCol_HeaderHovered]=uiColor(160,160,192);c[ImGuiCol_HeaderActive]=uiColor(140,140,192);
    c[ImGuiCol_TabHovered]=uiColor(212,208,200);c[ImGuiCol_Border]=uiColor(128,128,128);c[ImGuiCol_BorderShadow]=uiColor(255,255,255);
    c[ImGuiCol_CheckMark]=c[ImGuiCol_SliderGrab]=c[ImGuiCol_SliderGrabActive]=uiColor(0,0,128);
    c[ImGuiCol_Separator]=uiColor(128,128,128);c[ImGuiCol_DockingEmptyBg]=uiColor(128,128,128);
    s.WindowRounding=s.ChildRounding=s.FrameRounding=s.GrabRounding=s.PopupRounding=s.TabRounding=s.ScrollbarRounding=0;
    s.FrameBorderSize=2;s.WindowBorderSize=2;s.ChildBorderSize=2;s.TabBorderSize=1;
  }
  if(uiTheme==2) {
    // Raised retro frames use a dark shadow and a lighter upper edge.
    c[ImGuiCol_Border]=uiColor(16,16,16);c[ImGuiCol_BorderShadow]=uiColor(105,105,105);
    c[ImGuiCol_TitleBgActive]=uiColor(30,55,100);
    s.WindowRounding=s.ChildRounding=s.FrameRounding=s.GrabRounding=s.PopupRounding=s.TabRounding=s.ScrollbarRounding=0;
    s.FrameBorderSize=2;s.WindowBorderSize=2;s.ChildBorderSize=2;s.TabBorderSize=1;
  }
  if(uiTheme==3) {
    ImGui::StyleColorsLight(&s);
    c[ImGuiCol_WindowBg]=uiColor(240,242,245);c[ImGuiCol_ChildBg]=uiColor(250,251,253);
    c[ImGuiCol_PopupBg]=uiColor(248,249,252);c[ImGuiCol_Text]=uiColor(28,32,40);
    c[ImGuiCol_Button]=uiColor(200,214,233);c[ImGuiCol_ButtonHovered]=uiColor(170,199,237);
    c[ImGuiCol_ButtonActive]=uiColor(140,180,232);c[ImGuiCol_FrameBg]=uiColor(255,255,255);
    c[ImGuiCol_TextDisabled]=uiColor(110,115,125);
  }
  if(uiTheme==4) {
    // Original olive/orange palette inspired by classic Half-Life VGUI.
    for(auto slot:{ImGuiCol_WindowBg,ImGuiCol_PopupBg,ImGuiCol_MenuBarBg,ImGuiCol_Tab,ImGuiCol_TabDimmed})c[slot]=uiColor(63,68,52);
    c[ImGuiCol_ChildBg]=c[ImGuiCol_FrameBg]=uiColor(37,42,32);
    c[ImGuiCol_Text]=uiColor(225,211,176);c[ImGuiCol_TextDisabled]=uiColor(151,151,126);
    c[ImGuiCol_Border]=uiColor(116,119,88);c[ImGuiCol_BorderShadow]=uiColor(22,27,21);
    c[ImGuiCol_TitleBg]=uiColor(38,43,32);c[ImGuiCol_TitleBgActive]=uiColor(78,83,61);
    c[ImGuiCol_Button]=uiColor(85,91,65);c[ImGuiCol_ButtonHovered]=uiColor(112,113,68);c[ImGuiCol_ButtonActive]=uiColor(153,113,37);
    c[ImGuiCol_FrameBgHovered]=uiColor(62,70,46);c[ImGuiCol_FrameBgActive]=uiColor(77,81,49);
    c[ImGuiCol_Header]=c[ImGuiCol_TabSelected]=uiColor(91,93,58);c[ImGuiCol_HeaderHovered]=c[ImGuiCol_TabHovered]=uiColor(128,110,45);
    c[ImGuiCol_HeaderActive]=uiColor(151,112,37);c[ImGuiCol_TabDimmedSelected]=uiColor(76,81,57);
    c[ImGuiCol_CheckMark]=c[ImGuiCol_SliderGrab]=c[ImGuiCol_SliderGrabActive]=c[ImGuiCol_TabSelectedOverline]=uiColor(245,164,37);
    c[ImGuiCol_Separator]=uiColor(99,104,74);c[ImGuiCol_SeparatorHovered]=c[ImGuiCol_SeparatorActive]=uiColor(245,164,37);
    s.WindowRounding=s.ChildRounding=s.FrameRounding=s.GrabRounding=s.PopupRounding=s.TabRounding=0;
    s.WindowBorderSize=s.ChildBorderSize=s.FrameBorderSize=1;
  }
  s.ScaleAllSizes(uiScale());
  ImGui::GetIO().FontDefault = uiFonts[uiScaleChoice];
}
static std::string uiClosedDialog;
static bool uiDialog(const char* title, ImVec2 initial = {560, 240})
{
  const auto display = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowSize(
    {std::min(display.x - 24, initial.x * uiScale()),
     std::min(display.y - 64, initial.y * uiScale())},
    ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSizeConstraints({280, 120}, ImGui::GetIO().DisplaySize);
  bool open=true;
  bool visible=ImGui::BeginPopupModal(title, &open, ImGuiWindowFlags_HorizontalScrollbar);
  if(!open)uiClosedDialog=title;
  return visible;
}

// A top pane with its own scrollbar leaves a draggable boundary above the browser.
static void uiInspectorTopBegin(const char* id,float& height) {
  height=std::clamp(height,60.f,std::max(60.f,ImGui::GetContentRegionAvail().y-160));
  ImGui::BeginChild(id,{0,height},ImGuiChildFlags_None);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(5,3));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(5,2));
}
static void uiInspectorTopEnd(const char* id,float& height) {
  ImGui::PopStyleVar(2);ImGui::EndChild();
  ImGui::InvisibleButton(id,{ImGui::GetContentRegionAvail().x,8});
  auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
  ImGui::GetWindowDrawList()->AddLine({a.x,(a.y+b.y)*.5f},{b.x,(a.y+b.y)*.5f},ImGui::GetColorU32(ImGuiCol_SeparatorHovered),2);
  if(ImGui::IsItemHovered()||ImGui::IsItemActive())ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
  if(ImGui::IsItemActive())height+=ImGui::GetIO().MouseDelta.y;
}
