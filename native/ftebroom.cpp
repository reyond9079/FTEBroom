// SPDX-License-Identifier: GPL-2.0-or-later
// ReyondBroom: Dear ImGui frontend for FTE's native csaddon brush editor.
#include <cstdint>
#ifdef _WIN32
#define VARGS __cdecl
#else
#define VARGS
#endif
typedef int uploadfmt_t;
typedef int clipboardtype_t;
typedef struct qdownload_s qdownload_t;
typedef struct downloadlist_s downloadlist_t;
enum dlfailreason_e : int;
enum com_tokentype_e : int;
enum fs_relative : int;
#include "imgui.h"
#include "imgui_internal.h"
#include "plugin.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include "ftebroom_tool_icons.h"

static plugcorefuncs_t* core;
static plugcmdfuncs_t* commands;
static plugcvarfuncs_t* cvars;
static plug2dfuncs_t* draw;
static pluginputfuncs_t* input;
static qhandle_t atlas;
static quintptr_t lastTime;
static int mouseKeys[5], wheelUp, wheelDown;
static bool ready;
static bool modEmbedded;
#include "ftebroom_linux_platform.h"
static std::string crashCommands;
static void command(const char* text)
{
  if(modEmbedded && std::strncmp(text,"ftebroom_camera_",16)!=0 && std::strncmp(text,"quit",4)!=0)return;
  if(cvars && cvars->GetFloat("ftebroom_bsp_preview") &&
    (!std::strncmp(text,"mod_terrain_save",16)||!std::strncmp(text,"mapedit ",8)))
    commands->AddText("ftebroom_bsp edit\n",qfalse);
  crashCommands+=text;if(crashCommands.size()>32768)crashCommands.erase(0,crashCommands.size()-32768);
  commands->AddText(text, qfalse);
}

struct Vertex
{
  float x, y, u, v, r, g, b, a;
};
static Vertex lerp(const Vertex& a, const Vertex& b, float t)
{
  return {
    a.x + (b.x - a.x) * t,
    a.y + (b.y - a.y) * t,
    a.u + (b.u - a.u) * t,
    a.v + (b.v - a.v) * t,
    a.r + (b.r - a.r) * t,
    a.g + (b.g - a.g) * t,
    a.b + (b.b - a.b) * t,
    a.a + (b.a - a.a) * t};
}
// Clip triangles before submitting them as degenerate quads to FTE's batched 2D
// API.
static void triangle(std::array<Vertex, 8> polygon, const ImVec4& clip, qhandle_t image)
{
  // A triangle clipped against a rectangle has at most seven vertices.
  // Keep clipping on the stack and bypass it for fully contained triangles.
  size_t count = 3;
  unsigned outside = 15, anyOutside = 0;
  for (size_t i = 0; i < count; ++i) {
    const auto& v = polygon[i];
    unsigned code = (v.x < clip.x ? 1 : 0) | (v.x > clip.z ? 2 : 0)
                  | (v.y < clip.y ? 4 : 0) | (v.y > clip.w ? 8 : 0);
    outside &= code; anyOutside |= code;
  }
  if (outside) return;
  for (int edge = 0; anyOutside && edge < 4 && count; ++edge)
  {
    auto distance = [&](const Vertex& p) {
      return edge == 0 ? p.x - clip.x : edge == 1 ? clip.z - p.x
           : edge == 2 ? p.y - clip.y : clip.w - p.y;
    };
    std::array<Vertex, 8> result{};
    size_t nextCount = 0;
    Vertex previous = polygon[count - 1];
    float pd = distance(previous);
    for (size_t i = 0; i < count; ++i) {
      Vertex current = polygon[i];
      float cd = distance(current);
      if ((cd >= 0) != (pd >= 0))
        result[nextCount++] = lerp(previous, current, pd / (pd - cd));
      if (cd >= 0) result[nextCount++] = current;
      previous = current; pd = cd;
    }
    polygon = result; count = nextCount;
  }
  for (size_t i = 1; i + 1 < count; ++i)
  {
    Vertex p[4] = {polygon[0], polygon[i], polygon[i + 1], polygon[i + 1]};
    // FTE's image shader culls the reverse winding. ImGui's antialias fringes
    // and custom draw lists can contain either winding, so normalize every
    // clipped triangle before encoding it as a degenerate quad.
    const float area = (p[1].x-p[0].x)*(p[2].y-p[0].y)
                     - (p[1].y-p[0].y)*(p[2].x-p[0].x);
    if (area < 0) std::swap(p[1], p[2]);
    p[3] = p[2];
    vec2_t xy[4], uv[4];
    vec4_t color[4];
    for (int j = 0; j < 4; ++j)
    {
      xy[j][0] = p[j].x;
      xy[j][1] = p[j].y;
      uv[j][0] = p[j].u;
      uv[j][1] = p[j].v;
      color[j][0] = p[j].r;
      color[j][1] = p[j].g;
      color[j][2] = p[j].b;
      color[j][3] = p[j].a;
    }
    draw->Image2dQuad(xy, uv, color, image);
  }
}
static void render()
{
  ImGui::Render();
  auto data = ImGui::GetDrawData();
  draw->Colour4f(1, 1, 1, 1);
  for (int n = 0; n < data->CmdListsCount; ++n)
  {
    auto list = data->CmdLists[n];
    for (auto& cmd : list->CmdBuffer)
    {
      if (cmd.UserCallback)
      {
        if (cmd.UserCallback != ImDrawCallback_ResetRenderState)
          cmd.UserCallback(list, &cmd);
        continue;
      }
      for (unsigned int i = 0; i + 2 < cmd.ElemCount; i += 3)
      {
        std::array<Vertex, 8> p{};
        for (int j = 0; j < 3; ++j)
        {
          auto& v =
            list->VtxBuffer[list->IdxBuffer[cmd.IdxOffset + i + j] + cmd.VtxOffset];
          auto c = ImGui::ColorConvertU32ToFloat4(v.col);
          p[j] = {v.pos.x, v.pos.y, v.uv.x, v.uv.y, c.x, c.y, c.z, c.w};
        }
        triangle(p, cmd.ClipRect, (qhandle_t)cmd.GetTexID());
      }
    }
  }
}
#include "ftebroom_localization.h"
#include "ftebroom_appearance.h"
static void uploadFontAtlas()
{
  auto& io=ImGui::GetIO();
  unsigned char* pixels;
  int w, h;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
  std::vector<unsigned char> tga(18 + w * h * 4, 0);
  tga[2] = 2;
  tga[12] = w & 255;
  tga[13] = w >> 8;
  tga[14] = h & 255;
  tga[15] = h >> 8;
  tga[16] = 32;
  tga[17] = 0x28;
  for (int i = 0; i < w * h; ++i)
  {
    tga[18 + i * 4] = pixels[i * 4 + 2];
    tga[19 + i * 4] = pixels[i * 4 + 1];
    tga[20 + i * 4] = pixels[i * 4];
    tga[21 + i * 4] = pixels[i * 4 + 3];
  }
  atlas = draw->LoadImageData("ftebroom/font.tga", "image/tga", tga.data(), tga.size());
  atlas = draw->LoadImage("ftebroom/font.tga");
  io.Fonts->SetTexID((ImTextureID)atlas);
}
static void initialize()
{
  if (ready)
    return;
  IMGUI_CHECKVERSION();
  cvars->SetFloat("ftebroom_mod_preview",0);
  ImGui::CreateContext();
  auto& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;
  // render() applies ImDrawCmd::VtxOffset, allowing 16-bit index batches beyond 64K vertices.
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigWindowsMoveFromTitleBarOnly = true;
  io.IniSavingRate = .5f;
  io.BackendRendererName = "FTE native 2D";
  io.BackendPlatformName = "FTE MenuEvent";
#ifdef _WIN32
  const char* windows = std::getenv("WINDIR");
  std::string font = std::string(windows ? windows : "C:/Windows") + "/Fonts/segoeui.ttf";
#endif
  uiBuildGlyphRanges();
  char cjkPath[4096]={};
  auto fontFiles=(plugfsfuncs_t*)core->GetEngineInterface(plugfsfuncs_name,sizeof(plugfsfuncs_t));
  fontFiles->NativePath("fonts/FTEBroom-CJK.otf",(fs_relative)5,cjkPath,sizeof cjkPath);
  // Rasterize each supported scale, instead of magnifying a small bitmap font.
  for (int i = 0; i < 5; ++i)
  {
    ImFontConfig config;
    config.SizePixels = 17 * (1.f + .25f * i);
#ifdef _WIN32
    if (std::ifstream(font).good())
      uiFonts[i] = io.Fonts->AddFontFromFileTTF(
        font.c_str(), config.SizePixels, &config, io.Fonts->GetGlyphRangesCyrillic());
#endif
    if (!uiFonts[i] && std::ifstream(cjkPath).good())
      uiFonts[i] = io.Fonts->AddFontFromFileTTF(cjkPath, config.SizePixels, &config, uiCJKRanges.Data);
    if (!uiFonts[i])
      uiFonts[i] = io.Fonts->AddFontDefault(&config);
    if(std::ifstream(cjkPath).good()) {
      ImFontConfig merged;merged.MergeMode=true;merged.DstFont=uiFonts[i];
      io.Fonts->AddFontFromFileTTF(cjkPath,config.SizePixels,&merged,uiCJKRanges.Data);
    }
#ifdef _WIN32
    std::string italic =
      std::string(windows ? windows : "C:/Windows") + "/Fonts/segoeuii.ttf";
    if (std::ifstream(italic).good())
      uiItalicFonts[i] = io.Fonts->AddFontFromFileTTF(
        italic.c_str(), config.SizePixels, &config, io.Fonts->GetGlyphRangesCyrillic());
    std::string bold =
      std::string(windows ? windows : "C:/Windows") + "/Fonts/segoeuib.ttf";
    if (std::ifstream(bold).good())
      uiBoldFonts[i] = io.Fonts->AddFontFromFileTTF(
        bold.c_str(), config.SizePixels, &config, io.Fonts->GetGlyphRangesCyrillic());
#endif
    for(auto extra : {uiItalicFonts[i],uiBoldFonts[i]})if(extra&&std::ifstream(cjkPath).good()){ImFontConfig merged;merged.MergeMode=true;merged.DstFont=extra;io.Fonts->AddFontFromFileTTF(cjkPath,config.SizePixels,&merged,uiCJKRanges.Data);}
  }
  uiStyle();
  uploadFontAtlas();
  unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
#ifndef _WIN32
  linuxPlatformInitialize(fontFiles,io.Fonts,pixels,w,h);
#endif
  ready = true;
  char status[96];
  std::snprintf(
    status,
    sizeof status,
    "ReyondBroom: Dear ImGui renderer initialized (atlas %i).\n",
    (int)atlas);
  core->Print(status);
}
static bool safeName(const char* name)
{
  if (!*name)
    return false;
  for (auto p = name; *p; ++p)
    if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9')
          || *p == '_' || *p == '-'))
      return false;
  return true;
}

static bool inspector = true, sceneDrag = false, sceneLook = false, altHeld = false;
static bool selectMapTab = false, selectMaterialTab = false;
static bool ctrlHeld = false, shiftHeld = false;
static bool popupOpen = false, saveRequest = false, openRequest = false;
static float lastX = 0, lastY = 0;
static char mapName[64] = "my_map";
static bool moveHeld[6] = {};
static int cameraButton = 0;
static const char* modalNames[] = {
  "Default (Move)",
  "Brush",
  "Clip",
  "Rotate",
  "Sweep",
  "Scale",
  "Shear",
  "Vertex",
  "Edge",
  "Face",
  "Control Point", "Sculpt", "Bezier Bend", "Polygon Cut", "Polygon Spawn", "Spin", "Rotate Grid"};
static int pointerView = 0;
static quintptr_t selectionClickTime;
static float selectionClickX, selectionClickY, selectionDownX, selectionDownY;
static int selectionClickView = -1;
static bool selectionClickCtrl, selectionClickShift, selectionClickAlt;
static void pointer(const char* phase, float x, float y)
{
  char b[160];
  std::snprintf(
    b,
    sizeof b,
    "ftebroom_pointer %s %g %g %i %i %i\n",
    phase,
    x,
    y,
    altHeld ? 1 : 0,
    pointerView,
    (ctrlHeld ? 1 : 0) | (shiftHeld ? 2 : 0));
  command(b);
}
static void cancelGesture()
{
  pointer("cancel", lastX, lastY);
  sceneDrag = false;
}
static void action(const char* cmd)
{
  cancelGesture();
  command(cmd);
}
#include "ftebroom_assets.h"
#include "ftebroom_document.h"
#include "ftebroom_wall_browser.h"
// Camera persistence depends on document paths and platform file helpers.
#include "ftebroom_camera.h"
#include "ftebroom_clipboard.h"
#include "ftebroom_transform.h"
#include "ftebroom_workspace.h"
// View projection and picking use the central docking node's rectangle.
#include "ftebroom_views.h"
// Actions refer to the resource dialog state declared above.
#include "ftebroom_organization.h"
// Entity panels share organization command quoting.
#include "ftebroom_entities.h"
// View filters use the entity definitions and mounted material catalogue.
#include "ftebroom_view_options.h"
static bool terrainCreateWindow;
// Context menus share entity and organization panels.
#include "ftebroom_context.h"
#include "ftebroom_issues.h"
#include "ftebroom_run.h"
#include "ftebroom_build.h"
#include "ftebroom_terrain.h"
#include "ftebroom_model_viewer.h"
#include "ftebroom_mod_files.h"
#include "ftebroom_desktop.h"
#include "ftebroom_update.h"
// Actions may focus the entity inspector.
#include "ftebroom_generators.h"
#include "ftebroom_settle.h"
#include "ftebroom_array.h"
#include "ftebroom_spawn_master.h"
#include "ftebroom_unity.h"
#include "ftebroom_plugins.h"
#include "ftebroom_texture_folders.h"
#include "ftebroom_prefabs.h"
#include "ftebroom_entity_list.h"
#include "ftebroom_fullscreen.h"
#include "ftebroom_resolution.h"
#include "ftebroom_actions.h"
#include "ftebroom_preferences.h"
#include "ftebroom_shapes.h"
static void gridControl(const char* label)
{
  const char* values[] = {
    "0.125", "0.25", "0.5", "1", "2", "4", "8", "16", "32", "64", "128", "256"};
  float value = cvars->GetFloat("ca_grid");
  int selected = 0;
  while (selected < 11 && std::ldexp(1.f, selected - 3) < value)
    ++selected;
  ImGui::SetNextItemWidth(76 * uiScale());
  if (ImGui::Combo(label, &selected, values, 12))
    cvars->SetFloat("ca_grid", std::ldexp(1.f, selected - 3));
}
static void panel(float w, float h)
{
  auto& io = ImGui::GetIO();
  io.DisplaySize = {w, h};
  auto now = core->GetMilliseconds();
  io.DeltaTime = lastTime ? std::max(.001f, (now - lastTime) / 1000.f) : 1.f / 60;
  lastTime = now;
  uiLoad();
  uiApplyScale(w, h);
#ifndef _WIN32
  linuxPlatformPump(ctrlHeld);
  io.MouseDrawCursor = false;
#endif
  ImGui::NewFrame();
  const float menuHeight = ImGui::GetFrameHeight();
  const float toolbarHeight = ImGui::GetFrameHeight() + 18 * uiScale();
  const float statusHeight = ImGui::GetTextLineHeight() + 16 * uiScale();
  uiWorkspace(w, h, menuHeight + toolbarHeight, statusHeight);
  projectSetup();
  viewCompute(w, h);
  viewOverlay();
  if ((int)cvars->GetFloat("ftebroom_tool") == 1)
  {
    auto overlay = ImGui::GetForegroundDrawList();
    auto rect = viewRects[viewFocus];
    overlay->PushClipRect({rect.x, rect.y}, {rect.x + rect.w, rect.y + rect.h});
    int count = std::min(256, (int)cvars->GetFloat("ftebroom_shape_points"));
    for (int i = 0; i < count; ++i)
    {
      char name[64], data[128];
      float x, y;
      std::snprintf(name, sizeof name, "ftebroom_shape_point_%i", i);
      cvars->GetString(name, data, sizeof data);
      if (std::sscanf(data, "%f %f", &x, &y) == 2)
        overlay->AddCircleFilled({x, y}, 4, IM_COL32(255, 225, 55, 255));
    }
    char hover[128];
    float x, y;
    cvars->GetString("ftebroom_shape_hover", hover, sizeof hover);
    if (std::sscanf(hover, "%f %f", &x, &y) == 2)
      overlay->AddCircle({x, y}, 7, IM_COL32(255, 225, 55, 200), 0, 2);
    overlay->PopClipRect();
  }
  if (
    (int)cvars->GetFloat("ftebroom_tool") == 5
    || (int)cvars->GetFloat("ftebroom_tool") == 6)
  {
    auto overlay = ImGui::GetForegroundDrawList();
    auto rect = viewRects[viewFocus];
    overlay->PushClipRect({rect.x, rect.y}, {rect.x + rect.w, rect.y + rect.h});
    int count = std::min(26, (int)cvars->GetFloat("ftebroom_scale_handles"));
    for (int i = 0; i < count; ++i)
    {
      char name[64], data[128];
      float x, y;
      std::snprintf(name, sizeof name, "ftebroom_scale_handle_%i", i);
      cvars->GetString(name, data, sizeof data);
      if (std::sscanf(data, "%f %f", &x, &y) == 2)
        overlay->AddRectFilled(
          {x - 4, y - 4}, {x + 4, y + 4}, IM_COL32(255, 225, 55, 255));
    }
    overlay->PopClipRect();
  }
  if (
    (int)cvars->GetFloat("ftebroom_tool") >= 7
    && (int)cvars->GetFloat("ftebroom_tool") <= 10)
  {
    auto overlay = ImGui::GetForegroundDrawList();
    auto rect = viewRects[viewFocus];
    overlay->PushClipRect({rect.x, rect.y}, {rect.x + rect.w, rect.y + rect.h});
    int count = std::min(2048, (int)cvars->GetFloat("ftebroom_vertex_handles"));
    for (int i = 0; i < count; ++i)
    {
      char name[64], data[128];
      float x, y;
      int selected, hovered, visible;
      std::snprintf(name, sizeof name, "ftebroom_vertex_handle_%i", i);
      cvars->GetString(name, data, sizeof data);
      if (
        std::sscanf(data, "%f %f %i %i %i", &x, &y, &selected, &hovered, &visible) != 5
        || !visible)
        continue;
      overlay->AddCircleFilled(
        {x, y}, 4, selected ? IM_COL32(255, 65, 65, 255) : IM_COL32(255, 225, 55, 255));
      if (hovered)
      {
        overlay->AddCircle({x, y}, 8, IM_COL32(255, 65, 65, 255), 0, 2);
        char coordinates[128]{};
        cvars->GetString("ftebroom_vertex_coordinates", coordinates, sizeof coordinates);
        overlay->AddText(
          {x + 12, y - ImGui::GetTextLineHeight()},
          IM_COL32(255, 225, 55, 255),
          coordinates);
      }
    }
    char lasso[128];
    char added[128];
    float newX, newY;
    cvars->GetString("ftebroom_vertex_new", added, sizeof added);
    if (std::sscanf(added, "%f %f", &newX, &newY) == 2)
      overlay->AddCircle({newX, newY}, 6, IM_COL32(255, 225, 55, 180), 0, 2);
    float x1, y1, x2, y2;
    cvars->GetString("ftebroom_vertex_lasso", lasso, sizeof lasso);
    if (std::sscanf(lasso, "%f %f %f %f", &x1, &y1, &x2, &y2) == 4)
      overlay->AddRect(
        {std::min(x1, x2), std::min(y1, y2)},
        {std::max(x1, x2), std::max(y1, y2)},
        IM_COL32(255, 225, 55, 255));
    overlay->PopClipRect();
  }
  if ((int)cvars->GetFloat("ftebroom_tool") == 2)
  {
    auto overlay = ImGui::GetForegroundDrawList();
    auto clipRect = viewRects[viewFocus];
    overlay->PushClipRect(
      {clipRect.x, clipRect.y}, {clipRect.x + clipRect.w, clipRect.y + clipRect.h});
    ImVec2 previousPoint;
    for (int i = 0; i < (int)cvars->GetFloat("ftebroom_clip_points"); ++i)
    {
      char name[64], position[128];
      float x = 0, y = 0;
      std::snprintf(name, sizeof name, "ftebroom_clip_point_%i", i);
      cvars->GetString(name, position, sizeof position);
      if (std::sscanf(position, "%f %f", &x, &y) == 2)
      {
        overlay->AddCircleFilled({x, y}, 5, IM_COL32(255, 140, 30, 255));
        overlay->AddText(
          {x + 8, y - 12}, IM_COL32(255, 190, 70, 255), std::to_string(i + 1).c_str());
        if (i)
          overlay->AddLine(previousPoint, {x, y}, IM_COL32(255, 140, 30, 255), 2);
        previousPoint = {x, y};
      }
    }
    if (cvars->GetFloat("ftebroom_clip_hover_valid"))
    {
      char hover[128];
      float x = 0, y = 0;
      cvars->GetString("ftebroom_clip_hover", hover, sizeof hover);
      if (std::sscanf(hover, "%f %f", &x, &y) == 2)
        overlay->AddCircle({x, y}, 7, IM_COL32(255, 140, 30, 255), 0, 2);
    }
    overlay->PopClipRect();
  }
  if (io.WantTextInput || popupOpen || sceneDrag || ctrlHeld || (modEmbedded && modPlay))
    std::fill(std::begin(moveHeld), std::end(moveHeld), false);
  cvars->SetFloat("ftebroom_forward", float(moveHeld[0]) - float(moveHeld[1]));
  cvars->SetFloat("ftebroom_right", float(moveHeld[3]) - float(moveHeld[2]));
  cvars->SetFloat("ftebroom_up", float(moveHeld[4]) - float(moveHeld[5]));
  cameraTick();
  ImGui::BeginDisabled(modEmbedded);
  if (ImGui::BeginMainMenuBar())
  {
    actionMenus();pluginsMenu();
    if (ImGui::BeginMenu("Workspace"))
    {
      ImGui::MenuItem("Mod file manager",nullptr,&modFilesOpen);
      if (ImGui::MenuItem("Appearance / font scale..."))
        preferencesRequest = true;
      if (ImGui::MenuItem("Lock panels", nullptr, &uiLocked))
        uiSaveAppearance();
      if (ImGui::MenuItem("Reset panel layout"))
        uiReset = true;
      ImGui::EndMenu();
    }
    ImGui::TextDisabled("   ReyondBroom 0.5 alpha 73");
    ImGui::EndMainMenuBar();
  }
  const auto fixed = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
                     | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
  ImGui::SetNextWindowPos({0, menuHeight});
  ImGui::SetNextWindowSize({w, toolbarHeight});
  ImGui::Begin("##toolbar", nullptr, fixed | ImGuiWindowFlags_HorizontalScrollbar);
  actionButton("Menu/Edit/Undo");
  ImGui::SameLine();
  actionButton("Menu/Edit/Redo");
  ImGui::SameLine();
  actionButton("Menu/Edit/Delete");
  ImGui::SameLine();
  ImGui::TextUnformatted(" | ");
  ImGui::SameLine();
  gridControl("Grid");
  ImGui::SameLine();
  int view = (int)cvars->GetFloat("ca_brush_view");
  ImGui::SetNextItemWidth(135 * uiScale());
  if (ImGui::Combo("View", &view, "Perspective\0YZ (side)\0XZ (front)\0XY (top)\0"))
  {
    cancelGesture();
    viewTypes[viewFocus] = view;
    viewActivate(viewFocus);
  }
  ImGui::SameLine();
  int layoutChoice = viewLayout - 1;
  ImGui::SetNextItemWidth(90 * uiScale());
  if (ImGui::Combo("Layout", &layoutChoice, "1 view\0 2 views\0 3 views\0 4 views\0"))
    viewSetLayout(layoutChoice + 1);
  ImGui::SameLine();
  ImGui::TextDisabled(
    view ? "  View plane / hold Shift during drag: axis" : "  XY plane / Alt: Z");
  ImGui::SameLine();
  ImGui::End();
  ImGui::Begin("Tools", nullptr, uiPanelFlags());
  const auto toolsArea=ImGui::GetContentRegionAvail();
  const bool horizontalTools=toolsArea.x>toolsArea.y;
  const float toolsRight=ImGui::GetCursorScreenPos().x+toolsArea.x;
  const float toolHeight=34*uiScale();
  const ImVec2 toolPadding{ImGui::GetStyle().FramePadding.x,std::max(0.f,(toolHeight-ImGui::GetFontSize())*.5f)};
  bool toolsFirst=true;
  auto toolFlow=[&](float width){
    if(horizontalTools&&!toolsFirst&&ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+width<=toolsRight)
      ImGui::SameLine();
    toolsFirst=false;
  };
  const int toolIds[] = {0, 1, 2, 7, 8, 9, 10, 3, 4, 5, 6, 11, 12, 13, 14, 15, 16};
  const char* toolNames[] = {
    "Default (Move)",
    "Brush",
    "Clip",
    "Vertex",
    "Edge",
    "Face",
    "Control Point",
    "Rotate",
    "Sweep",
    "Scale",
    "Shear", "Sculpt", "Bezier Bend", "Polygon Cut", "Polygon Spawn", "Spin", "Rotate Grid"};
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,toolPadding);
  for (int i = 0; i < 17; i++)
  {
    if(i==11) {
      toolFlow(toolHeight);
      const auto direction=horizontalTools
        ? (extraToolsExpanded?ImGuiDir_Left:ImGuiDir_Right)
        : (extraToolsExpanded?ImGuiDir_Up:ImGuiDir_Down);
      if(ImGui::ArrowButton("##extra-tools",direction))extraToolsExpanded=!extraToolsExpanded;
      if(ImGui::IsItemHovered())ImGui::SetTooltip("%s Sculpt, Bezier Bend, Polygon Cut, Polygon Spawn, Spin, Rotate Grid",extraToolsExpanded?"Hide":"Show");
    }
    if(i>=11&&!extraToolsExpanded)continue;
    int tool = toolIds[i];
    if(gameProfile==2 && tool==10)continue;
    bool active = (int)cvars->GetFloat("ftebroom_tool") == tool;
    const Action* entry = nullptr;
    for (auto& a : actions)
      if (std::string(a.id) == std::string("Menu/Tools/") + toolNames[i])
        entry = &a;
    if (active)
      ImGui::PushStyleColor(ImGuiCol_Button, uiColor(62, 112, 205));
    ImGui::PushID(i);
    toolFlow(32*uiScale());
    if (FteIcons::button("##tool",i>=15?7:i>=11?i+2:i,{32 * uiScale(), 34 * uiScale()}))
    {
      sceneDrag = sceneLook = false;
      if (entry)
        executeAction(*entry);
      else
        command("ftebroom_tool_deactivate\n");
    }
    if (active)
      ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip(
        "%s %s%s",
        toolNames[i],
        entry ? entry->shortcutText.c_str() : "",
        "");
    ImGui::PopID();
  }
  if(horizontalTools){
    toolFlow(1);auto p=ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(p,{p.x,p.y+34*uiScale()},ImGui::GetColorU32(ImGuiCol_Separator));
    ImGui::Dummy({1,34*uiScale()});
  }else ImGui::Separator();
  bool snapInteger=cvars->GetFloat("ftebroom_auto_integer_snap")!=0;
  toolFlow(ImGui::GetFrameHeight()+ImGui::GetStyle().ItemInnerSpacing.x+ImGui::CalcTextSize("Int").x);
  if(ImGui::Checkbox("Int",&snapInteger))cvars->SetFloat("ftebroom_auto_integer_snap",snapInteger);
  if(ImGui::IsItemHovered())ImGui::SetTooltip("Automatically snap edited brush vertices to integer coordinates");
  for (auto name : {"ftebroom_texture_lock", "ftebroom_uv_lock"})
  {
    bool uv = std::string(name) == "ftebroom_uv_lock";
    bool on = cvars->GetFloat(name) != 0;
    if (on)
      ImGui::PushStyleColor(ImGuiCol_Button, uiColor(62, 112, 205));
    toolFlow(32*uiScale());
    if (FteIcons::button(uv ? "##UV" : "##TL",uv?12:11,{32 * uiScale(), toolHeight}))
      cvars->SetFloat(name, !on);
    if (on)
      ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip(
        "%s",
        uv ? "UV Lock (stored preference; advanced transforms pending)"
           : "Texture Lock: keep texture attached while moving");
  }
  ImGui::PopStyleVar();
  ImGui::EndDisabled();
  buildToolbar(horizontalTools);
  ImGui::BeginDisabled(modEmbedded);
  int currentTool = (int)cvars->GetFloat("ftebroom_tool");
  if(horizontalTools)toolFlow(190*uiScale());else ImGui::Separator();
  if (currentTool == 2)
  {
    ImGui::TextWrapped("Clip");
    if(horizontalTools)ImGui::SameLine();
    int keep = (int)cvars->GetFloat("ftebroom_clip_keep");
    ImGui::SetNextItemWidth(horizontalTools?150 * uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    if (ImGui::Combo(toolOptionLabel(horizontalTools,"Keep"), &keep, "Front\0Both sides\0Back\0"))
      cvars->SetFloat("ftebroom_clip_keep", keep);
    if(horizontalTools)ImGui::SameLine();
    if (ImGui::Button("Apply (Enter)"))
      command("ftebroom_tool_key apply\n");
    if(horizontalTools)ImGui::SameLine();
    ImGui::Text("Points: %g / 3", cvars->GetFloat("ftebroom_clip_points"));
  }
  else if (currentTool == 3 || currentTool == 16)
  {
    if(ImGui::Button("Pick center"))action("ftebroom_rotate_center pick\n");
    if(cvars->GetFloat("ftebroom_rotate_pick_center"))ImGui::TextWrapped("Click a point in the viewport");
    static float center[3] = {};
    ImGui::SetNextItemWidth(horizontalTools?200:std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    ImGui::InputFloat3(toolOptionLabel(horizontalTools,"Center"), center, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
      char text[160];
      std::snprintf(
        text,
        sizeof text,
        "ftebroom_rotate_center %.9g %.9g %.9g\n",
        center[0],
        center[1],
        center[2]);
      action(text);
    }
    else if (!ImGui::IsItemActive())
    {
      center[0] = cvars->GetFloat("ftebroom_rotate_center_x");
      center[1] = cvars->GetFloat("ftebroom_rotate_center_y");
      center[2] = cvars->GetFloat("ftebroom_rotate_center_z");
    }
    if(horizontalTools)ImGui::SameLine();
    if (ImGui::Button(currentTool==16?"Reset grid":"Reset center"))
      action(currentTool==16?"ftebroom_grid_rotate reset\n":"ftebroom_rotate_center reset\n");
    if(horizontalTools)ImGui::SameLine();
    float step = cvars->GetFloat(currentTool==16?"ftebroom_grid_rotate_step":"ftebroom_rotate_step");
    ImGui::SetNextItemWidth(horizontalTools?65:std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    if(currentTool==16) {
      int degrees=std::clamp((int)std::round(step),1,180);
      if(ImGui::InputInt(toolOptionLabel(horizontalTools,"Step (degrees)"),&degrees,0,0))cvars->SetFloat("ftebroom_grid_rotate_step",std::clamp(degrees,1,180));
      step=std::clamp(degrees,1,180);
    } else if(ImGui::InputFloat(toolOptionLabel(horizontalTools,"Angle"), &step, 0, 0, "%.2f") && std::isfinite(step) && step>0 && step<=360)
      cvars->SetFloat("ftebroom_rotate_step",step);
    if(horizontalTools)ImGui::SameLine();
    static int axis = 2;
    ImGui::SetNextItemWidth(horizontalTools?55:std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    ImGui::Combo(toolOptionLabel(horizontalTools,"Axis"), &axis, "X\0Y\0Z\0");
    if(horizontalTools)ImGui::SameLine();
    if (ImGui::Button("Apply"))
    {
      char text[200];
      std::snprintf(
        text,
        sizeof text,
        "ftebroom_rotate %d %d %d %.9g %.9g %.9g %.9g\n",
        axis == 0,
        axis == 1,
        axis == 2,
        step,
        center[0],
        center[1],
        center[2]);
      if(currentTool==16)std::snprintf(text,sizeof text,"ftebroom_grid_rotate %d %d\n",axis,(int)std::round(step));
      action(text);
    }
    if(horizontalTools)ImGui::SameLine();
    bool properties = cvars->GetFloat("ftebroom_rotate_entity_properties") != 0;
    if (currentTool!=16 && ImGui::Checkbox("Entity angles", &properties))
      cvars->SetFloat("ftebroom_rotate_entity_properties", properties);
  }
  else if (currentTool == 0)
  {
    static bool shapeExpanded=false;
    const auto direction=horizontalTools?(shapeExpanded?ImGuiDir_Left:ImGuiDir_Right):(shapeExpanded?ImGuiDir_Up:ImGuiDir_Down);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,toolPadding);
    if(ImGui::ArrowButton("##shape-options",direction))shapeExpanded=!shapeExpanded;
    ImGui::SameLine();
    if(ImGui::Button("Shape / move"))shapeExpanded=!shapeExpanded;
    ImGui::PopStyleVar();
    if(shapeExpanded) {
    if(horizontalTools)toolOptionFlow(190*uiScale());
    shapeOptions(horizontalTools);
    if(horizontalTools)toolOptionFlow(ImGui::CalcTextSize("Move objects by:").x+240*uiScale());
    static float offset[3] = {0, 0, 0};
    ImGui::TextWrapped("Move objects by:");
    if(horizontalTools)ImGui::SameLine();
    ImGui::SetNextItemWidth(horizontalTools?220*uiScale():ImGui::GetContentRegionAvail().x);
    ImGui::InputFloat3(toolOptionLabel(horizontalTools,"##move-offset"), offset, "%.2f");
    if(horizontalTools)toolOptionFlow(ImGui::CalcTextSize("Apply").x+2*ImGui::GetStyle().FramePadding.x);
    ImGui::BeginDisabled(cvars->GetFloat("ftebroom_ctx_selection") != 1);
    if (ImGui::Button("Apply"))
    {
      char cmd[160];
      std::snprintf(
        cmd, sizeof cmd, "ftebroom_move %g %g %g\n", offset[0], offset[1], offset[2]);
      action(cmd);
    }
    ImGui::EndDisabled();
    }
  }
  else if (currentTool == 1)
  {
    ImGui::Text("Brush | %g points", cvars->GetFloat("ftebroom_shape_points"));
    if(horizontalTools)ImGui::SameLine();
    if (ImGui::Button("Create brush (Enter)"))
      command("ftebroom_tool_key apply\n");
    if(horizontalTools)ImGui::SameLine();
    ImGui::TextWrapped(
      "3D faces: click / rectangle / double-click | Shift: extrude polygon");
  }
  else if (currentTool == 5)
  {
    static int mode = 0;
    static float scale[3] = {1, 1, 1};
    ImGui::SetNextItemWidth(horizontalTools?110:std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    if (ImGui::Combo(toolOptionLabel(horizontalTools,"##scale-mode"), &mode, "By factors\0To size\0"))
      scale[0] = scale[1] = scale[2] = mode ? 128.f : 1.f;
    if(horizontalTools)ImGui::SameLine();
    ImGui::SetNextItemWidth(horizontalTools?220:std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    ImGui::InputFloat3(toolOptionLabel(horizontalTools,"##scale-values"), scale, "%.3f");
    if(horizontalTools)ImGui::SameLine();
    if (ImGui::Button("Apply"))
    {
      char text[180];
      std::snprintf(
        text,
        sizeof text,
        "ftebroom_scale %s %.9g %.9g %.9g\n",
        mode ? "size" : "factors",
        scale[0],
        scale[1],
        scale[2]);
      action(text);
    }
    if(horizontalTools)ImGui::SameLine();
    ImGui::TextWrapped("Shift: proportional | Alt: from center");
  }
  else if(currentTool==13||currentTool==14)
  {
    bool spawn=currentTool==14,changed=false;
    if(!spawn){int depth=(int)cvars->GetFloat("ftebroom_polygon_depth");bool keep=cvars->GetFloat("ftebroom_polygon_keep")!=0;
      ImGui::SetNextItemWidth(horizontalTools?110*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));if(ImGui::InputInt(toolOptionLabel(horizontalTools,"Depth (grid steps; 0 = through)"),&depth)){cvars->SetFloat("ftebroom_polygon_depth",std::clamp(depth,0,4096));changed=true;}
      if(horizontalTools)ImGui::SameLine();if(ImGui::Checkbox("Keep cutout",&keep)){cvars->SetFloat("ftebroom_polygon_keep",keep);changed=true;}
    }
    ImGui::TextWrapped(spawn?"Click a planar outline; close it by clicking the first point. Height = grid size.":"Click a planar cut outline; close it by clicking the first point. Drag handles to edit.");
    if(ImGui::Button("Close contour"))command("ftebroom_polygon close\n");if(horizontalTools)ImGui::SameLine();
    if(ImGui::Button(spawn?"Spawn / Enter":"Cut / Enter"))command("ftebroom_polygon apply\n");if(horizontalTools)ImGui::SameLine();
    if(ImGui::Button("Backspace"))command("ftebroom_polygon back\n");if(horizontalTools)ImGui::SameLine();if(ImGui::Button("Reset"))command("ftebroom_polygon reset\n");
    if(changed)command("ftebroom_polygon preview\n");
  }
  else if(currentTool==15)
  {
    int segments=(int)cvars->GetFloat("ftebroom_spin_segments");float flex=cvars->GetFloat("ftebroom_spin_flex"),scale=cvars->GetFloat("ftebroom_spin_scale");bool settings=false;
    ImGui::SetNextItemWidth(horizontalTools?100*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));settings|=ImGui::InputInt(toolOptionLabel(horizontalTools,"Segments"),&segments);
    if(horizontalTools)ImGui::SameLine();ImGui::SetNextItemWidth(horizontalTools?90*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));settings|=ImGui::DragFloat(toolOptionLabel(horizontalTools,"Flex"),&flex,.01f,-1,1,"%.2f");
    if(horizontalTools)ImGui::SameLine();ImGui::SetNextItemWidth(horizontalTools?90*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));settings|=ImGui::DragFloat(toolOptionLabel(horizontalTools,"End scale"),&scale,.01f,.05f,8,"%.3f");
    if(settings){char cmd[128];std::snprintf(cmd,sizeof cmd,"ftebroom_spin settings %d %.9g %.9g\n",std::clamp(segments,1,64),std::clamp(flex,-1.f,1.f),std::clamp(scale,.05f,8.f));command(cmd);}
    int mode=(int)cvars->GetFloat("ftebroom_spin_mode"),component=(int)cvars->GetFloat("ftebroom_spin_component");
    ImGui::SetNextItemWidth(horizontalTools?95*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));if(ImGui::Combo(toolOptionLabel(horizontalTools,"Drag"),&mode,"Move\0Scale\0Rotate\0"))cvars->SetFloat("ftebroom_spin_mode",mode);
    if(horizontalTools)ImGui::SameLine();ImGui::SetNextItemWidth(horizontalTools?75*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));if(ImGui::Combo(toolOptionLabel(horizontalTools,"Constraint"),&component,"All\0X\0Y\0Z\0"))cvars->SetFloat("ftebroom_spin_component",component);
    static float values[2][3]{};const char* labels[]={"Endpoint offset","End rotation"};const char* ops[]={"point","rotation"};const char* names[2][3]={{"ftebroom_spin_x","ftebroom_spin_y","ftebroom_spin_z"},{"ftebroom_spin_rx","ftebroom_spin_ry","ftebroom_spin_rz"}};
    for(int i=0;i<2;i++){
      ImGui::SetNextItemWidth(horizontalTools?260*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));ImGui::InputFloat3(toolOptionLabel(horizontalTools,labels[i]),values[i],"%.3f");
      if(ImGui::IsItemDeactivatedAfterEdit()){char cmd[160];std::snprintf(cmd,sizeof cmd,"ftebroom_spin %s %.9g %.9g %.9g\n",ops[i],values[i][0],values[i][1],values[i][2]);command(cmd);}
      else if(!ImGui::IsItemActive())for(int a=0;a<3;a++)values[i][a]=cvars->GetFloat(names[i][a]);
    }
    ImGui::BeginDisabled(!cvars->GetFloat("ftebroom_spin_valid"));if(ImGui::Button("Apply Spin"))command("ftebroom_spin apply\n");ImGui::EndDisabled();
    if(horizontalTools)ImGui::SameLine();if(ImGui::Button("Default"))command("ftebroom_spin reset\n");if(horizontalTools)ImGui::SameLine();if(ImGui::Button("Cancel Spin"))command("ftebroom_spin cancel\n");
    ImGui::TextWrapped("Drag the endpoint | Alt: vertical | Ctrl: grid | Ctrl+Z/Y: settings history");
  }
  else if(currentTool==12)
  {
    int axis=(int)cvars->GetFloat("ftebroom_bend_axis"),segments=(int)cvars->GetFloat("ftebroom_bend_segments"),subdivisions=(int)cvars->GetFloat("ftebroom_bend_subdivisions");
    bool joints=cvars->GetFloat("ftebroom_bend_joints")!=0,settings=false;
    ImGui::SetNextItemWidth(horizontalTools?65*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));settings|=ImGui::Combo(toolOptionLabel(horizontalTools,"Axis"),&axis,"X\0Y\0Z\0");
    if(horizontalTools)ImGui::SameLine();ImGui::SetNextItemWidth(horizontalTools?90*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));settings|=ImGui::InputInt(toolOptionLabel(horizontalTools,"Path segments"),&segments);
    if(horizontalTools)ImGui::SameLine();ImGui::SetNextItemWidth(horizontalTools?90*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));settings|=ImGui::InputInt(toolOptionLabel(horizontalTools,"Subdivisions"),&subdivisions);
    settings|=ImGui::Checkbox("Keep joints",&joints);
    if(settings){char cmd[160];std::snprintf(cmd,sizeof cmd,"ftebroom_bend settings %d %d %d %d\n",axis,std::clamp(segments,1,32),std::clamp(subdivisions,0,6),int(joints));command(cmd);}
    if(horizontalTools)ImGui::SameLine();int mode=(int)cvars->GetFloat("ftebroom_bend_mode"),component=(int)cvars->GetFloat("ftebroom_bend_component");
    ImGui::SetNextItemWidth(horizontalTools?95*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));if(ImGui::Combo(toolOptionLabel(horizontalTools,"Drag"),&mode,"Move\0Scale\0Rotate\0"))cvars->SetFloat("ftebroom_bend_mode",mode);
    if(horizontalTools)ImGui::SameLine();ImGui::SetNextItemWidth(horizontalTools?75*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));if(ImGui::Combo(toolOptionLabel(horizontalTools,"Constraint"),&component,"All\0X\0Y\0Z\0"))cvars->SetFloat("ftebroom_bend_component",component);
    int handle=(int)cvars->GetFloat("ftebroom_bend_handle");ImGui::SetNextItemWidth(horizontalTools?90*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    if(ImGui::InputInt(toolOptionLabel(horizontalTools,"Control point"),&handle))command(("ftebroom_bend handle "+std::to_string(std::clamp(handle,0,segments))+"\n").c_str());
    static float values[3][3]{};const char* keys[]={"point","scale","rotation"};const char* labels[]={"Position","Scale","Rotation (degrees)"};
    for(int i=0;i<3;++i){ImGui::SetNextItemWidth(horizontalTools?260*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));ImGui::InputFloat3(toolOptionLabel(horizontalTools,labels[i]),values[i],"%.3f");
      if(ImGui::IsItemDeactivatedAfterEdit()){char cmd[160];std::snprintf(cmd,sizeof cmd,"ftebroom_bend %s %.9g %.9g %.9g\n",keys[i],values[i][0],values[i][1],values[i][2]);command(cmd);}
      else if(!ImGui::IsItemActive()){char value[160];cvars->GetString((std::string("ftebroom_bend_")+keys[i]).c_str(),value,sizeof value);std::sscanf(value,"%f %f %f",&values[i][0],&values[i][1],&values[i][2]);}
    }
    ImGui::BeginDisabled(!cvars->GetFloat("ftebroom_bend_valid"));if(ImGui::Button("Apply Bend"))command("ftebroom_bend apply\n");ImGui::EndDisabled();
    if(horizontalTools)ImGui::SameLine();if(ImGui::Button("Reset path"))command("ftebroom_bend reset\n");if(horizontalTools)ImGui::SameLine();if(ImGui::Button("Cancel Bend"))command("ftebroom_bend cancel\n");
  }
  else if(currentTool==11)
  {
    int mode=(int)cvars->GetFloat("ftebroom_sculpt_mode");
    ImGui::SetNextItemWidth(horizontalTools?110*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    if(ImGui::Combo(toolOptionLabel(horizontalTools,"Mode"),&mode,"Raise\0Lower\0Smooth\0"))cvars->SetFloat("ftebroom_sculpt_mode",mode);
    float radius=cvars->GetFloat("ftebroom_sculpt_radius"),intensity=cvars->GetFloat("ftebroom_sculpt_intensity");
    if(horizontalTools)ImGui::SameLine();ImGui::SetNextItemWidth(horizontalTools?95*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    if(ImGui::DragFloat(toolOptionLabel(horizontalTools,"Radius"),&radius,1,1,4096,"%.1f"))cvars->SetFloat("ftebroom_sculpt_radius",std::clamp(radius,1.f,4096.f));
    if(horizontalTools)ImGui::SameLine();ImGui::SetNextItemWidth(horizontalTools?95*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    if(ImGui::DragFloat(toolOptionLabel(horizontalTools,"Intensity"),&intensity,.1f,.1f,1024,"%.1f"))cvars->SetFloat("ftebroom_sculpt_intensity",std::clamp(intensity,.1f,1024.f));
    bool protect=cvars->GetFloat("ftebroom_sculpt_prevent_collapse")!=0;
    if(ImGui::Checkbox("Prevent collapse",&protect))cvars->SetFloat("ftebroom_sculpt_prevent_collapse",protect);
    if(horizontalTools)ImGui::SameLine();ImGui::TextWrapped("Drag over brush surfaces | Esc: cancel stroke");
  }
  else if(currentTool==4)
  {
    bool rebuild=false;
    int segments=(int)cvars->GetFloat("ftebroom_sweep_segments"),iterations=(int)cvars->GetFloat("ftebroom_sweep_iterations");
    ImGui::SetNextItemWidth(horizontalTools?90*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    if(ImGui::InputInt(toolOptionLabel(horizontalTools,"Segments"),&segments)){cvars->SetFloat("ftebroom_sweep_segments",std::clamp(segments,1,64));rebuild=true;}
    if(horizontalTools)ImGui::SameLine();ImGui::SetNextItemWidth(horizontalTools?90*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    if(ImGui::InputInt(toolOptionLabel(horizontalTools,"Iterations"),&iterations)){cvars->SetFloat("ftebroom_sweep_iterations",std::clamp(iterations,1,32));rebuild=true;}
    char path[32];cvars->GetString("ftebroom_sweep_path",path,sizeof path);
    int mode=std::string(path)=="arc"?1:(std::string(path)=="sbend"?2:0);
    if(horizontalTools)ImGui::SameLine();ImGui::SetNextItemWidth(horizontalTools?95*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    if(ImGui::Combo(toolOptionLabel(horizontalTools,"Path"),&mode,"Straight\0Arc\0S-bend\0")){cvars->SetString("ftebroom_sweep_path",mode==1?"arc":(mode==2?"sbend":"straight"));rebuild=true;}
    bool integer=cvars->GetFloat("ftebroom_sweep_integer")!=0;
    if(ImGui::Checkbox("Snap to integer grid",&integer)){cvars->SetFloat("ftebroom_sweep_integer",integer);rebuild=true;}
    if(horizontalTools)ImGui::SameLine();if(ImGui::Button("Reset"))command("ftebroom_sweep reset\n");
    if(horizontalTools)ImGui::SameLine();ImGui::BeginDisabled(cvars->GetFloat("ftebroom_sweep_valid")==0);
    if(ImGui::Button("Perform Sweep (Enter)"))command("ftebroom_sweep apply\n");ImGui::EndDisabled();
    static float offset[3]={0,0,0},scale=1;
    ImGui::SetNextItemWidth(horizontalTools?220*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));
    ImGui::InputFloat3(toolOptionLabel(horizontalTools,"Cap offset"),offset,"%.2f");
    if(ImGui::IsItemDeactivatedAfterEdit())
    {char cmd[180];std::snprintf(cmd,sizeof cmd,"ftebroom_sweep move %.9g %.9g %.9g\n",offset[0],offset[1],offset[2]);command(cmd);}
    else if(!ImGui::IsItemActive()){char value[128];cvars->GetString("ftebroom_sweep_offset",value,sizeof value);std::sscanf(value,"%f %f %f",&offset[0],&offset[1],&offset[2]);}
    if(horizontalTools)ImGui::SameLine();ImGui::SetNextItemWidth(horizontalTools?70*uiScale():std::max(40.f,ImGui::GetContentRegionAvail().x*.58f));ImGui::InputFloat(toolOptionLabel(horizontalTools,"Scale"),&scale,0,0,"%.3f");
    if(ImGui::IsItemDeactivatedAfterEdit()){char cmd[100];std::snprintf(cmd,sizeof cmd,"ftebroom_sweep scale %.9g\n",scale);command(cmd);}
    else if(!ImGui::IsItemActive())scale=cvars->GetFloat("ftebroom_sweep_scale");
    if(rebuild)command("ftebroom_sweep rebuild\n");
  }
  else if (currentTool == 6)
  {
    ImGui::TextWrapped("Shear | Drag a side handle | Alt: vertical | Esc: cancel");
  }
  else if(currentTool==10)
    {
      static int rows=3,columns=3;
      const int currentRows=(int)cvars->GetFloat("ftebroom_patch_rows"),currentColumns=(int)cvars->GetFloat("ftebroom_patch_columns");
      ImGui::SeparatorText("Patch control grid");
      ImGui::BeginDisabled(currentRows<3 || currentColumns<3);
      ImGui::InputInt(toolOptionLabel(horizontalTools,"Rows"),&rows,2,2);
      if(ImGui::IsItemDeactivatedAfterEdit())
        command(("ftebroom_patch_resize 1 "+std::to_string(std::clamp(rows|1,3,31))+"\n").c_str());
      else if(!ImGui::IsItemActive()) rows=currentRows;
      ImGui::InputInt(toolOptionLabel(horizontalTools,"Columns"),&columns,2,2);
      if(ImGui::IsItemDeactivatedAfterEdit())
        command(("ftebroom_patch_resize 0 "+std::to_string(std::clamp(columns|1,3,31))+"\n").c_str());
      else if(!ImGui::IsItemActive()) columns=currentColumns;
      ImGui::EndDisabled();
      ImGui::TextWrapped("Increase preserves the surface. Decrease approximates it. Delete does not remove control points.");
    }
  else if (currentTool >= 7 && currentTool <= 9)
  {
    ImGui::Text(
      "%s | %g selected",
      modalNames[currentTool],
      cvars->GetFloat("ftebroom_handle_count"));
    if(horizontalTools)ImGui::SameLine();
    ImGui::TextWrapped(
      "Drag handles | Ctrl: toggle | Alt: vertical | Delete: remove | Esc: cancel");
  }
  else
  {
    ImGui::Text("%s | framework only", modalNames[std::clamp(currentTool, 0, 16)]);
    if(horizontalTools)ImGui::SameLine();
    if (currentTool == 1)
      ImGui::Text(
        "Points: %g | Esc clears points", cvars->GetFloat("ftebroom_shape_points"));
    else
      ImGui::TextWrapped("Geometry editing is scheduled in a later task");
  }
  ImGui::End();
  if (selectMapTab)
    ImGui::SetNextWindowFocus();
  if (ImGui::Begin("Map", nullptr, uiPanelFlags()))
  {
    if (cvars->GetFloat("ftebroom_ctx_selection") == 2)
      ImGui::Text(
        "Face selection: %i faces", (int)cvars->GetFloat("ftebroom_face_count"));
    ImGui::Text(
      "Selection: %i brush(es), %i entities",
      (int)cvars->GetFloat("ftebroom_selection_count"),
      (int)cvars->GetFloat("ftebroom_entity_selection_count"));
    char openedGroup[256] = {};
    cvars->GetString("ftebroom_open_group", openedGroup, sizeof openedGroup);
    if (openedGroup[0])
      ImGui::Text("Open group: %s", openedGroup);
    char selectedEntity[160] = {};
    cvars->GetString("ftebroom_selected_entity", selectedEntity, sizeof selectedEntity);
    if (*selectedEntity)
      ImGui::Text("Entity: %s", selectedEntity);
    if ((int)cvars->GetFloat("ftebroom_tool") == 2)
    {
      ImGui::SeparatorText("Clip / Cut");
      int keep = (int)cvars->GetFloat("ftebroom_clip_keep");
      if (ImGui::Combo("Keep", &keep, "Front\0Both sides\0Back\0"))
        cvars->SetFloat("ftebroom_clip_keep", float(keep));
      ImGui::Text("Cut points: %i / 3", (int)cvars->GetFloat("ftebroom_clip_points"));
      if (ImGui::Button("Apply cut (Enter)"))
        command("ftebroom_clip apply\n");
      ImGui::SameLine();
      if (ImGui::Button("Reset"))
        command("ftebroom_clip reset\n");
      ImGui::TextWrapped(
        "Orange transparent volume is discarded. Ctrl+Enter changes sides. "
        "Select brushes first. Click or drag cut points on brush faces. Drag a point to "
        "adjust it; "
        "double-click a face to use its plane. Two points use the face/view normal; "
        "three define "
        "an arbitrary plane. Escape removes the last point.");
    }
    organizationPanel();
    viewOptionsPanel();
    ImGui::SeparatorText("Grid");
    gridControl("Spacing");
    bool visible = cvars->GetFloat("ftebroom_grid_visible") != 0;
    if (ImGui::Checkbox("Show grid", &visible))
      cvars->SetFloat("ftebroom_grid_visible", visible);
    bool snap = cvars->GetFloat("ftebroom_grid_snap") != 0;
    if (ImGui::Checkbox("Snap to grid", &snap))
      cvars->SetFloat("ftebroom_grid_snap", snap);
    float alpha = cvars->GetFloat("ftebroom_grid_alpha");
    if (ImGui::SliderFloat("Grid opacity", &alpha, 0.f, 1.f))
      cvars->SetFloat("ftebroom_grid_alpha", alpha);
    ImGui::SeparatorText("Document");
    static bool boundsWasOpen = false;
    bool boundsOpen = ImGui::CollapsingHeader("Soft map bounds");
    if (boundsOpen)
    {
      static float low[3] = {-65536, -65536, -65536};
      static float high[3] = {65536, 65536, 65536};
      if (!boundsWasOpen)
      {
        char current[256] = {};
        if (cvars->GetString("ftebroom_map_bounds", current, sizeof current))
          std::sscanf(
            current,
            "%f %f %f %f %f %f",
            &low[0],
            &low[1],
            &low[2],
            &high[0],
            &high[1],
            &high[2]);
      }
      ImGui::InputFloat3("Minimum", low);
      ImGui::InputFloat3("Maximum", high);
      bool valid = true;
      for (int a = 0; a < 3; ++a)
        valid &= std::isfinite(low[a]) && std::isfinite(high[a]) && low[a] < high[a]
                 && std::abs(low[a]) <= 65536 && std::abs(high[a]) <= 65536;
      ImGui::BeginDisabled(!valid);
      if (ImGui::Button("Apply bounds"))
      {
        char value[256];
        std::snprintf(
          value,
          sizeof value,
          "%g %g %g %g %g %g",
          low[0],
          low[1],
          low[2],
          high[0],
          high[1],
          high[2]);
        cvars->SetString("ftebroom_map_bounds", value);
      }
      ImGui::EndDisabled();
    }
    boundsWasOpen = boundsOpen;
    ImGui::Text(
      "%s.map%s", documentName.c_str(), cvars->GetFloat("ftebroom_dirty") ? "*" : "");
    ImGui::TextWrapped("%s", documentFile.u8string().c_str());
    if (ImGui::Button("Save .map"))
    {
      if (documentUntitled)
        saveRequest = true;
      else
        documentSave(documentName);
    }
    ImGui::SameLine();
    if (ImGui::Button("Open .map"))
      openRequest = true;
    ImGui::TextWrapped(
      "Drag in the scene while this inspector stays open. "
      "Esc cancels the current gesture.");
  }
  ImGui::End();
  if (
    selectMaterialTab
    || (cvars->GetFloat("ftebroom_test_material_tab") && cvars->GetFloat("ftebroom_skip_setup")))
    ImGui::SetNextWindowFocus();
#ifdef _WIN32
  materialDropVisible=false;
#endif
  if (ImGui::Begin("Material", nullptr, uiPanelFlags()))
    materialBrowser();
  ImGui::End();
  selectMapTab = selectMaterialTab = false;
  ImGui::SetNextWindowPos({0, h - statusHeight});
  ImGui::SetNextWindowSize({w, statusHeight});
  ImGui::Begin("##status", nullptr, fixed);
  char selectionStatus[192] = {};
  cvars->GetString("ftebroom_selection_status", selectionStatus, sizeof selectionStatus);
  if (!selectionStatus[0])
    ImGui::Text(
      "%s | Grid %g | Selected: %g | %s",
      modalNames[std::clamp((int)cvars->GetFloat("ftebroom_tool"), 0, 16)],
      cvars->GetFloat("ca_grid"),
      cvars->GetFloat("ftebroom_selection_count"),
      sceneDrag ? (altHeld ? "Z constraint | Esc cancels" : "XY constraint | Esc cancels")
                : (currentTool == 2
                     ? "Place 2 or 3 points | Enter applies | Ctrl+Enter changes side | "
                       "Esc removes last point"
                     : "Click selects | Drag creates / moves | Shift+Esc exits tool"));
  else
    ImGui::TextColored({1, .65f, .15f, 1}, "%s", selectionStatus);
  ImGui::SameLine();
  if(ImGui::SmallButton(modFilesOpen ? "Hide files" : "Show files"))modFilesOpen=!modFilesOpen;
  ImGui::End();
  entityPanel();
  issuesPanel();
  runPanel();
  buildPanel();
  ImGui::EndDisabled();
  buildOverlay();
  modControlsPanel();
  ImGui::BeginDisabled(modEmbedded);
  sceneContextPanel();
  documentFileBrowser();
  documentDialogs();
  recentPanel();
  cameraPositionDialog();
  selectLineDialog();
  replaceMaterialDialog();
  moveObjectsDialog();
  actionReferenceWindow();
  static int chosenProfile=-1;
  if (preferencesRequest)
  {
    chosenProfile=gameProfile;uiResolutionRead();
    ImGui::OpenPopup("Preferences");
    preferencesRequest = false;
  }
  if (uiDialog("Preferences", {640, 620}))
  {
    if(ImGui::BeginTabBar("Preference pages"))
    {
    if(ImGui::BeginTabItem("View"))
    {
    uiPreferences();
    if(ImGui::CollapsingHeader("Resolution"))uiResolutionControls();
    bool legacy = cvars->GetFloat("ftebroom_drag_mode") != 0;
    if (ImGui::Checkbox("Drag unselected brushes to move (legacy)", &legacy))
      cvars->SetFloat("ftebroom_drag_mode", legacy);
    bool wheelGrid = cvars->GetFloat("ftebroom_wheel_grid") != 0;
    if (ImGui::Checkbox("Plain wheel changes grid (legacy)", &wheelGrid))
      cvars->SetFloat("ftebroom_wheel_grid", wheelGrid);
    bool wheelInvert = cvars->GetFloat("ftebroom_wheel_invert") != 0;
    if (ImGui::Checkbox("Invert wheel direction", &wheelInvert))
      cvars->SetFloat("ftebroom_wheel_invert", wheelInvert);
    bool hover = cvars->GetFloat("ftebroom_hover_highlight") != 0;
    if (ImGui::Checkbox("Highlight hovered objects", &hover))
      cvars->SetFloat("ftebroom_hover_highlight", hover);
    bool dimensions = cvars->GetFloat("ftebroom_selection_dimensions") != 0;
    if (ImGui::Checkbox("Show selection dimensions", &dimensions))
      cvars->SetFloat("ftebroom_selection_dimensions", dimensions);
    bool spikes = cvars->GetFloat("ftebroom_selection_spikes") != 0;
    if (ImGui::Checkbox("Show alignment guides on hover", &spikes))
      cvars->SetFloat("ftebroom_selection_spikes", spikes);
    viewPreferences();
    float gridOpacity=cvars->GetFloat("ftebroom_grid_alpha");
    if(ImGui::SliderFloat("Grid brightness",&gridOpacity,0,1))cvars->SetFloat("ftebroom_grid_alpha",gridOpacity);
    ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Mouse"))
    {
    cameraPreferences();
    ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Colors"))
    {
      for(auto setting:{std::pair{"Selected geometry","ftebroom_color_selected"},std::pair{"Locked geometry","ftebroom_color_locked"},std::pair{"Hovered geometry","ftebroom_color_hover"}})
      {
        char text[128]={};float color[3]={};cvars->GetString(setting.second,text,sizeof text);std::sscanf(text,"%f %f %f",&color[0],&color[1],&color[2]);
        if(ImGui::ColorEdit3(setting.first,color)){std::snprintf(text,sizeof text,"%.6g %.6g %.6g",color[0],color[1],color[2]);cvars->SetString(setting.second,text);}
      }
      ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Game profile"))
    {
      int& chosen=chosenProfile;if(chosen<0)chosen=gameProfile;
      ImGui::Text("Active: %s",gameProfileName());
      ImGui::Combo("Profile",&chosen,"Quake III Arena\0FTE Custom game\0Quake 1\0");
      ImGui::TextWrapped("Switching closes the current map and opens the selected game's starter scene. Unsaved changes can be saved first.");
      ImGui::BeginDisabled(chosen==gameProfile||assetsLoading||gameProfileLocked);
      if(ImGui::Button("Switch profile") && preferencesSave()){documentRequest("__profile:"+std::to_string(chosen));ImGui::CloseCurrentPopup();}
      ImGui::EndDisabled();
      ImGui::TextWrapped("Authoring directory: %s",*baseq3?baseq3:"Not configured");
      if(ImGui::Button("Game / resources...") && shortcutConflicts().empty() && preferencesSave()){setupNeeded=true;ImGui::CloseCurrentPopup();}
      ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Backups"))
    {
    preferencesBackups();
    ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Keyboard")){preferencesKeyboard();ImGui::EndTabItem();}
    if(ImGui::BeginTabItem("Update"))
    {
      ImGui::TextUnformatted("ReyondBroom 0.5 alpha 73");
      ImGui::TextLinkOpenURL("Check published releases","https://github.com/reyond9079/ReyondBroom/releases");
      updatePanel();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
    }
    if(!preferenceError.empty())ImGui::TextWrapped("%s",preferenceError.c_str());
    bool canClosePreferences=shortcutConflicts().empty() && captureShortcut.empty();
    ImGui::BeginDisabled(!canClosePreferences);
    if ((ImGui::Button("Close") || ImGui::IsKeyPressed(ImGuiKey_Escape)) && canClosePreferences && preferencesSave())
      ImGui::CloseCurrentPopup();
    ImGui::EndDisabled();
    ImGui::EndPopup();
  }
  ImGui::EndDisabled();
  popupOpen = ImGui::IsPopupOpen(
    nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
  terrainPanel();
  modFilesPanel();
  modelViewerPanel();
  pluginsPanel();
  symmetryPanel();generatorPanel();arrayPanel();settlePanel();spawnMasterPanel();unityPanel();textureFolderPanel();prefabPanel();entityListPanel();
  importDialog();
  desktopAttach();
  uiFullscreenTick();
  uiResolutionDialog();
#ifndef _WIN32
  linuxPlatformTestPanel();
#endif
#ifndef _WIN32
  linuxCursorUpdate(sceneLook||modelEditCameraCaptured,uiScale());
#endif
  render();
  modEmbedTick(w,h);
  uiSaveLayout();
}
static qboolean QDECL
event(int type, int key, int unicode, float x, float y, float w, float h)
{
  initialize();
  auto& io = ImGui::GetIO();
  if(modelEditorInput(type,key,x,y)){
    if(type==0){io.AddMousePosEvent(modelEditCameraCursor.x,modelEditCameraCursor.y);panel(w,h);}
    return qtrue;
  }
  // Restore absolute mode only after our existing menu regains focus.
  if (cameraCursorPending && input->HasMenuFocus())
  {
    input->SetMenuFocus(qtrue, "", 0, 0, 1);
    cameraCursorPending = false;
    io.AddMousePosEvent(lastX, lastY);
  }
  // Engine patch delivers unbounded relative deltas as event type 3.
  if (type == 3)
  {
    if(modEmbedded && modCaptured){modMouseX+=x;modMouseY+=y;return qtrue;}
    if (sceneLook)
    {
      if (cameraButton == 1)
      {
        sceneContextDX += x;
        sceneContextDY += y;
        if (sceneContextDX * sceneContextDX + sceneContextDY * sceneContextDY >= 16)
          sceneContextCandidate = false;
      }
      cameraInput(cameraButton == 2 ? "pan" : "look", x, y);
    }
    return qtrue;
  }
  if (sceneLook)
  {
    x = lastX;
    y = lastY;
  }
  io.AddMousePosEvent(x, y);
  cvars->SetFloat("ftebroom_mouse_x", x);
  cvars->SetFloat("ftebroom_mouse_y", y);
  viewCompute(w, h);
  if (viewSplitDrag && type == 0)
  {
    if (viewSplitDrag == 1)
      viewSplitX = std::clamp((x - viewArea.x) / std::max(16.f, viewArea.w), .2f, .8f);
    else
      viewSplitY = std::clamp((y - viewArea.y) / std::max(16.f, viewArea.h), .2f, .8f);
    viewCompute(w, h);
  }
  if (type == 2 && key == mouseKeys[0] && viewSplitDrag)
  {
    viewSplitDrag = 0;
    viewSave();
    io.AddMouseButtonEvent(0, false);
    return qtrue;
  }
  if (
    type == 1 && key == mouseKeys[0] && !popupOpen && !sceneDrag && !sceneLook
    && !uiBlocksScene(x, y))
  {
    viewSplitDrag = viewDivider(x, y, w, h);
    if (viewSplitDrag)
      return qtrue;
  }
  int hit = viewHit(x, y);
  bool inScene = hit >= 0 && !popupOpen && !viewSplitDrag
                 && !uiBlocksScene(x, y, type == 2 && key == mouseKeys[0]);
  if(type==1 && key==input->GetKeyCode("escape",nullptr)
     && cvars->GetFloat("ftebroom_bsp_preview") && !popupOpen && !io.WantTextInput && captureShortcut.empty()) {
    cameraEnd();buildEdit();return qtrue;
  }
  if(modEmbedded && (type==1 || type==2)) {
    if(modControlCapture>=0){if(type==1 && key>0 && key<1024){
      if(key!=input->GetKeyCode("escape",nullptr)){int previous=modControls[modControlCapture].key;for(auto& binding:modControls)if(binding.key==key)binding.key=previous;modControls[modControlCapture].key=key;}
      modControlCapture=-1;}return qtrue;}
    if(key==input->GetKeyCode("escape",nullptr)){if(type==1){if(modMenuOpen)modMenuOpen=false;else modOpenControls();}return qtrue;}
  }
  if(modEmbedded && modPlay && (type==1 || type==2)){
    if(cvars->GetFloat("ftebroom_test_mod_input")){char trace[96];std::snprintf(trace,sizeof trace,"FTEBROOM_MOD_KEY %d %d captured=%d\n",key,type,modCaptured?1:0);core->Print(trace);}
    if(modCaptured){
      if(key==input->GetKeyCode("escape",nullptr)){if(type==1)modReleaseInput();return qtrue;}
      modQueueKey(key,unicode,type==1);return qtrue;
    }
    if(inScene && !modMenuOpen && type==1 && key==mouseKeys[0]){
      lastX=x;lastY=y;modCaptured=true;sceneLook=true;
      cvars->SetFloat("ftebroom_camera_captured",1);
      input->SetMenuFocus(qtrue,nullptr,0,0,1);return qtrue;
    }
    if(inScene)return qtrue;
  }
  if (inScene && !sceneDrag && !sceneLook && !io.WantTextInput)
  {
    if (type == 1 && (key == mouseKeys[0] || key == mouseKeys[1] || key == mouseKeys[2]))
      viewFocusArmed = true;
    if (viewFocusArmed)
      viewActivate(hit);
  }
  pointerView = viewFocus;
  if (type == 0)
  {
    if (!sceneLook)
    {
      lastX = x;
      lastY = y;
    }
    panel(w, h);
  }
  if (type == 1 || type == 2)
  {
    bool down = type == 1;
    if (key == input->GetKeyCode("shift", nullptr))
    {
      shiftHeld = down;
      cvars->SetFloat("ftebroom_shift", down);
    }
    if (key == input->GetKeyCode("ctrl", nullptr))
    {
      ctrlHeld = down;
      cvars->SetFloat("ftebroom_ctrl", down);
    }
    for (int i = 0; i < 5; i++)
      if (key == mouseKeys[i])
        io.AddMouseButtonEvent(i, down);
    if (key == input->GetKeyCode("alt", nullptr))
    {
      altHeld = down;
      io.AddKeyEvent(ImGuiMod_Alt, down);
      cvars->SetFloat("ftebroom_alt", down);
    }
    if (preferencesCaptureKey(key,down)) return qtrue;
    if (modEmbedded && inScene && key == mouseKeys[0])return qtrue;
    if (key == mouseKeys[0])
    {
      if(!down && inScene)if(auto payload=ImGui::GetDragDropPayload();payload&&payload->IsDataType("FTEBROOM_MATERIAL")) {
        viewActivate(hit);sceneDrag=false;
        auto material=std::string((const char*)payload->Data);selectedMaterial=material;
        if(material.rfind("textures/",0)==0)material.erase(0,9);
        cvars->SetString("ca_newbrushtexture",material.c_str());
        command(("ftebroom_material_drop "+organizationQuote(material)+" "+std::to_string(x)+" "+std::to_string(y)+"\n").c_str());return qtrue;
      }
      if(!down && inScene)if(auto payload=ImGui::GetDragDropPayload();payload&&payload->IsDataType("FTEBROOM_PREFAB")) {
        try {if(payload->DataSize==sizeof(int)*2){auto ids=(const int*)payload->Data;viewActivate(hit);prefabInsert(ids[0],ids[1],x,y);}}
        catch(const std::exception& e){prefabError=e.what();}return qtrue;
      }
      if(!down && inScene)if(auto payload=ImGui::GetDragDropPayload();payload&&payload->IsDataType("FTEBROOM_MOD_FILE")) {
        try {
          auto path=std::filesystem::u8path((const char*)payload->Data);modFileValidate(path);auto ext=path.extension().u8string();std::transform(ext.begin(),ext.end(),ext.begin(),::tolower);
          if(ext==".map"){viewActivate(hit);modFileImportMap(path,x,y);}
          if(modelFile(path)) {viewActivate(hit);auto relative=modFileModelPath(path);command(("ftebroom_model_drop "+organizationQuote(relative)+" "+std::to_string(x)+" "+std::to_string(y)+"\n").c_str());}
        }catch(const std::exception& e){modFilesError=e.what();}return qtrue;
      }
      if (!down && inScene)
        if (
          auto payload = ImGui::GetDragDropPayload();
          payload && payload->IsDataType("FTE_ENTITY"))
        {
          viewActivate(hit);
          char query[1024];
          std::snprintf(
            query,
            sizeof query,
            "ftebroom_entity_drop %s %g %g\n",
            organizationQuote((const char*)payload->Data).c_str(),
            x,
            y);
          command(query);
          cvars->SetString("ftebroom_entity_ghost", "");
          return qtrue;
        }
      if (down && inScene && !sceneLook)
      {
        const auto now = core->GetMilliseconds();
        unsigned interval = 500;
#ifdef _WIN32
        interval = GetDoubleClickTime();
#endif
        const float dx = x - selectionClickX, dy = y - selectionClickY;
        const bool twice =
          selectionClickView == viewFocus && now - selectionClickTime <= interval
          && dx * dx + dy * dy <= 16 && selectionClickCtrl == ctrlHeld
          && selectionClickShift == shiftHeld && selectionClickAlt == altHeld
          && cvars->GetFloat("ftebroom_tool") >= 0
          && cvars->GetFloat("ftebroom_tool") <= 2;
        selectionDownX = x;
        selectionDownY = y;
        if (twice)
        {
          pointer("dblclick", x, y);
          selectionClickView = -1;
        }
        else
        {
          sceneDrag = true;
          pointer("down", x, y);
        }
      }
      if (!down && sceneDrag)
      {
        pointer("up", x, y);
        const float dx = x - selectionDownX, dy = y - selectionDownY;
        selectionClickView = dx * dx + dy * dy < 16 ? viewFocus : -1;
        selectionClickTime = core->GetMilliseconds();
        selectionClickX = x;
        selectionClickY = y;
        selectionClickCtrl = ctrlHeld;
        selectionClickShift = shiftHeld;
        selectionClickAlt = altHeld;
        sceneDrag = false;
      }
    }
    if (key == mouseKeys[1] || key == mouseKeys[2])
    {
      if (down && inScene && !io.WantTextInput && !sceneLook)
      {
        cancelGesture();
        sceneLook = true;
        lastX = x;
        lastY = y;
        cameraButton = key == mouseKeys[2] ? 2 : 1;
        if (cameraButton == 1 && !modEmbedded)
          sceneContextBegin(x, y);
        cvars->SetFloat("ftebroom_camera_captured", 1);
        if (cameraButton == 1)
          cameraInput("begin");
        else
          cameraInput("end");
        input->SetMenuFocus(qtrue, nullptr, 0, 0, 1);
      }
      if (
        !down
        && ((key == mouseKeys[1] && cameraButton == 1) || (key == mouseKeys[2] && cameraButton == 2)))
      {
        sceneContextRelease();
        cameraEnd();
        cameraButton = 0;
      }
    }
    if (down && (key == wheelUp || key == wheelDown))
    {
      if ((inScene || sceneLook) && !sceneDrag && !io.WantTextInput && !popupOpen)
      {
        if (ctrlHeld && !altHeld && !shiftHeld && !sceneLook)
        {
          char drill[128];
          std::snprintf(
            drill,
            sizeof drill,
            "ftebroom_pointer drill %g %g %d %d\n",
            x,
            y,
            key == wheelUp ? 1 : -1,
            viewFocus);
          command(drill);
        }
        else if (
          modEmbedded || sceneLook || shiftHeld
          || (cvars->GetFloat("ca_brush_view") != 0 && !(ctrlHeld && altHeld))
          || (!(ctrlHeld && altHeld) && !cvars->GetFloat("ftebroom_wheel_grid")))
        {
          cameraInput("wheel", 0, 0, key == wheelUp ? 1 : -1);
        }
        else
        {
          char gridCommand[96];
          std::snprintf(
            gridCommand,
            sizeof gridCommand,
            "ftebroom_grid wheel %d %d\n",
            key == wheelUp ? 1 : -1,
            ctrlHeld && altHeld);
          if (!io.WantTextInput)
            command(gridCommand);
        }
      }
      else
        io.AddMouseWheelEvent(0, key == wheelUp ? 1.f : -1.f);
    }
    struct Key
    {
      const char* name;
      ImGuiKey code;
    };
    static const Key keys[] = {
      {"escape", ImGuiKey_Escape},
      {"backspace", ImGuiKey_Backspace},
      {"enter", ImGuiKey_Enter},
      {"tab", ImGuiKey_Tab},
      {"del", ImGuiKey_Delete},
      {"ins", ImGuiKey_Insert},
      {"leftarrow", ImGuiKey_LeftArrow},
      {"uparrow", ImGuiKey_UpArrow},
      {"downarrow", ImGuiKey_DownArrow},

      {"rightarrow", ImGuiKey_RightArrow},
      {"home", ImGuiKey_Home},
      {"end", ImGuiKey_End},
      {"ctrl", ImGuiMod_Ctrl},
      {"shift", ImGuiMod_Shift},
      {"a", ImGuiKey_A},
      {"c", ImGuiKey_C},
      {"v", ImGuiKey_V},
      {"x", ImGuiKey_X},
      {"z", ImGuiKey_Z},
      {"y", ImGuiKey_Y}};
    for (auto k : keys)
      if (key == input->GetKeyCode(k.name, nullptr))
        io.AddKeyEvent(k.code, down);
    if (down && unicode >= 32 && unicode <= 0x10ffff && !(unicode >= 0xd800 && unicode <= 0xdfff) && !sceneLook)
      io.AddInputCharacter((unsigned int)unicode);
    dispatchAction(key, down, inScene);
  }
  return qtrue;
}
static void openPanel()
{
  inspector = true;
  input->SetMenuFocus(qtrue, "", 0, 0, 1);
}
static void mapReady()
{
  static std::string loadedAlias;
  entitySelectionEpoch = organizationReadEpoch = -1;
  ++organizationMapEpoch;
  cameraEnd();
  cameraMapReady();
  char loadedName[256];
  if (cvars->GetString("mapname", loadedName, sizeof loadedName))
  {
    auto name = std::filesystem::path(loadedName).stem().string();
    if (safeName(name.c_str()))
    {
      documentUntitled = (name.rfind("_ftebroom_new_", 0) == 0 || name=="_ftebroom_profile_start") || ((name=="start" || name=="start-quake1") && pendingDocumentFile.empty());
      documentName = documentUntitled ? "untitled" : name;
      if (name == pendingDocumentAlias && !pendingDocumentFile.empty())
      {
        documentFile = pendingDocumentFile;
        documentName = documentFile.stem().u8string();
      }
      else if(loadedAlias!=name || documentFile.empty())
        documentFile = documentUntitled ? std::filesystem::path{}
                                        : documentPath("maps/" + name + ".map");
      loadedAlias=name;
      if(!documentFile.empty())documentName=documentFile.stem().u8string();
      pendingDocumentFile.clear();
      pendingDocumentAlias.clear();
      traceAutoLoad();
      if (recoveryOpening && documentFile == recoveryFile)
      {
        documentUntitled = true;
        documentName = recoverySource.empty() ? "recovered" : recoverySource.stem().u8string();
        documentFile.clear(); recoveryOpening = false;
      }
    }
    std::snprintf(mapName, sizeof mapName, "%s", documentName.c_str());
  }
  documentIndexLines(documentFile);
  if(!documentUntitled)recentRemember(documentFile,gameProfile);
  if(!documentFile.empty())fileBrowserFolder=documentFile.parent_path();
  cvars->SetString("ftebroom_document_path",documentFile.u8string().c_str());
  lastBackup = std::chrono::steady_clock::now();
  sceneDrag = sceneLook = false;
  std::fill(std::begin(moveHeld), std::end(moveHeld), false);
  ctrlHeld = shiftHeld = altHeld = false;
  cvars->SetFloat("ftebroom_shift", 0);
  cvars->SetFloat("ftebroom_ctrl", 0);
  cvars->SetFloat("ftebroom_alt", 0);
  cvars->SetFloat("ftebroom_forward", 0);
  cvars->SetFloat("ftebroom_right", 0);
  cvars->SetFloat("ftebroom_up", 0);
  openPanel();
  core->Print("ReyondBroom: permanent workspace restored for map.\n");
}
static void profileStartMap(){static bool first=true;if(first){recentLoad();recentWindow=!cvars->GetFloat("ftebroom_skip_setup")&&firstSetupStep<0&&!recentMaps.empty();first=false;}gameProfileLoad();command(gameProfile==2?"mapedit maps/start-quake1.map\n":"mapedit maps/start.map\n");}
static void QDECL actionFocusTick(int)
{
  libreTick();
  updateTick();
  clipboardPump();
  documentTick();
  // Compiler jobs must advance even while the UI is occluded or another menu owns focus.
  if(ready){runTick();buildTick();pluginsTick();}
  if (!ready || input->HasMenuFocus())
    return;
  modelEditorCameraEnd(false);
  if(modCaptured)modReleaseInput();
  ctrlHeld = shiftHeld = altHeld = false;
  std::fill(std::begin(moveHeld), std::end(moveHeld), false);
  cvars->SetFloat("ftebroom_ctrl", 0);
  cvars->SetFloat("ftebroom_shift", 0);
  cvars->SetFloat("ftebroom_alt", 0);
  cvars->SetFloat("ftebroom_forward", 0);
  cvars->SetFloat("ftebroom_right", 0);
  cvars->SetFloat("ftebroom_up", 0);
  if (sceneDrag || sceneLook)
  {
    cancelGesture();
    // Do not reclaim menu focus from a console or another menu.
    cameraCursorPending = sceneLook;
    sceneLook = false;
    cvars->SetFloat("ftebroom_camera_captured", 0);
    command("ftebroom_camera_input end 0 0 0 0\n");
  }
  ImGui::GetIO().ClearInputKeys();
}
static void invokeActionCommand()
{
  char id[256];
  commands->Argv(1, id, sizeof id);
  initActions();
  for (auto& a : actions)
    if (std::string(a.id) == id)
    {
      if (actionMatches(a.context, currentActionContext()))
        executeAction(a);
      else
        core->Print("ReyondBroom: action is unavailable in the current context\\n");
      return;
    }
  core->Print("ReyondBroom: unknown action\\n");
}
static void uiActionCommand()
{
  char name[64];
  commands->Argv(1, name, sizeof name);
  initActions();
  std::string cmd = std::string("ftebroom_ui ") + name;
  for (auto& a : actions)
    if (a.command && cmd == a.command)
    {
      executeAction(a);
      return;
    }
}
static void testViewLayout()
{
  if (!cvars->GetFloat("ftebroom_skip_setup") || commands->IsInsecure())
    return;
  auto expect = [](bool ok, const char* name) {
    core->Print(
      (std::string(ok ? "FTEBROOM_LAYOUT_PASS " : "FTEBROOM_LAYOUT_FAIL ") + name + "\n")
        .c_str());
  };
  for (int count = 1; count <= 4; count++)
  {
    viewSetLayout(count);
    viewCompute(1280, 800);
    bool valid = true;
    for (int i = 0; i < count; i++)
    {
      auto r = viewRects[i];
      valid &= r.w > 0 && r.h > 0 && viewHit(r.x + r.w * .5f, r.y + r.h * .5f) == i;
    }
    expect(valid, ("rectangles_" + std::to_string(count)).c_str());
  }
  viewSetLayout(1);
  viewAction("cycle");
  expect(viewTypes[0] == 3, "cycle_single_xy");
  viewAction("cycle");
  viewAction("cycle");
  viewAction("cycle");
  expect(viewTypes[0] == 0, "cycle_single_wrap");
  viewSetLayout(2);
  viewAction("cycle");
  expect(viewTypes[0] == 0 && viewTypes[1] == 2, "cycle_two");
  viewSetLayout(3);
  viewAction("cycle");
  expect(viewTypes[2] == 1 && viewTypes[1] == 3, "cycle_three");
  viewSetLayout(4);
  viewAction("cycle");
  expect(viewTypes[3] == 1 && viewTypes[2] == 2, "cycle_four_noop");
  viewActivate(2);
  viewAction("maximize");
  viewCompute(1280, 800);
  expect(viewRects[2].w == viewArea.w && viewRects[0].w == 0, "maximize");
  viewAction("maximize");
  viewCompute(1280, 800);
  expect(viewRects[0].w > 0 && viewRects[2].w < viewArea.w, "restore");
  viewFocusArmed = true;
  viewActivate(0);
  auto r = viewRects[3];
  event(0, 0, 0, r.x + r.w * .5f, r.y + r.h * .5f, 1280, 800);
  expect(viewFocus == 3, "hover_focus");
  sceneDrag = true;
  auto r0 = viewRects[0];
  event(0, 0, 0, r0.x + 40, r0.y + 40, 1280, 800);
  expect(viewFocus == 3, "drag_focus_locked");
  sceneDrag = false;
  float sx = viewArea.x + viewArea.w * viewSplitX;
  float dividerY = viewArea.y + 40;
  float targetX = viewArea.x + viewArea.w * .65f;
  event(1, mouseKeys[0], 0, sx, dividerY, 1280, 800);
  event(0, 0, 0, targetX, dividerY, 1280, 800);
  event(2, mouseKeys[0], 0, targetX, dividerY, 1280, 800);
  expect(viewSplitX > .55f && !sceneDrag && !viewSplitDrag, "divider_drag");
  viewSplitX = viewSplitY = .5f;
  viewActivate(0);
  viewCompute(1280, 800);
}
static void testFilePath()
{
  if (!cvars->GetFloat("ftebroom_skip_setup") || commands->IsInsecure())
    return;
  char mode[32] = {}, path[4096] = {};
  commands->Argv(1, mode, sizeof mode);
  commands->Argv(2, path, sizeof path);
  if (std::string(mode) == "open")
    documentRequest("@" + std::string(path));
  else if (std::string(mode) == "save")
    documentSave(documentName, false, std::filesystem::u8path(path));
  else if (std::string(mode) == "export")
    documentSave(documentName, false, std::filesystem::u8path(path), true);
  else if (std::string(mode) == "exportcheck")
  {
    const bool ok = std::filesystem::exists(std::filesystem::u8path(path))
                    && documentFile != std::filesystem::u8path(path)
                    && documentError.empty() && !savingDocument;
    core->Print(
      ok ? "FTEBROOM_EXPORT_PASS native_destination_preserves_document\n"
         : "FTEBROOM_EXPORT_FAIL native_destination_preserves_document\n");
  }
  else if (std::string(mode) == "current")
    documentSave(documentName);
  else if (std::string(mode) == "view")
    viewActivate(std::clamp(std::atoi(path), 0, viewLayout - 1));
  else if (std::string(mode) == "name")
    std::snprintf(fileBrowserName, sizeof fileBrowserName, "%s", path);
  else if (std::string(mode) == "browserclick")
  {
    char phase[16]{};
    commands->Argv(3, phase, sizeof phase);
    auto found = fileBrowserTestItems.find(path);
    if (found != fileBrowserTestItems.end())
      event(
        std::atoi(phase), mouseKeys[0], 0, found->second.x, found->second.y, 1280, 800);
    else
      core->Print("FTEBROOM_BROWSER_FAIL missing item\n");
  }
  else if (std::string(mode) == "entityclick")
  {
    auto found = entityTestItems.find(path);
    if (found != entityTestItems.end())
    {
      event(1, mouseKeys[0], 0, found->second.x, found->second.y, 1280, 800);
      event(2, mouseKeys[0], 0, found->second.x, found->second.y, 1280, 800);
    }
    else
      core->Print("FTEBROOM_ENTITY_UI_FAIL missing item\n");
  }
  else if (std::string(mode) == "materialtab") selectMaterialTab=true;
  else if (std::string(mode) == "materialclick") {
    auto found=materialTestItems.find(path);
    if(found!=materialTestItems.end()) {
      event(1,mouseKeys[0],0,found->second.x,found->second.y,1280,800);
      event(2,mouseKeys[0],0,found->second.x,found->second.y,1280,800);
    } else core->Print("FTEBROOM_UI_FAIL missing material field\n");
  }
  else if (std::string(mode) == "state") {
    auto line=std::string("FTEBROOM_PATH_STATE default=")+documentDefaultFolder().u8string()+" map="+documentFile.u8string()+" baseq3="+baseq3+" setup="+(setupNeeded?"1":"0")+"\n";
    core->Print(line.c_str());
  }
  else if (std::string(mode) == "uitext")
    ImGui::GetIO().AddInputCharactersUTF8(path);
  else if (std::string(mode) == "selectiontap")
  {
    auto prefix = std::string("ftebroom_test_pick_") + path;
    float x = cvars->GetFloat((prefix + "_x").c_str()),
          y = cvars->GetFloat((prefix + "_y").c_str());
    event(1, mouseKeys[0], 0, x, y, 1280, 800);
    event(2, mouseKeys[0], 0, x, y, 1280, 800);
  }
  else if (std::string(mode) == "tap")
  {
    char y[32] = {};
    commands->Argv(3, y, sizeof y);
    event(1, mouseKeys[0], 0, std::atof(path), std::atof(y), 1280, 800);
    event(2, mouseKeys[0], 0, std::atof(path), std::atof(y), 1280, 800);
  }
  else if (std::string(mode) == "event")
  {
    char phase[16] = {}, x[32] = {}, y[32] = {};
    commands->Argv(3, phase, sizeof phase);
    commands->Argv(4, x, sizeof x);
    commands->Argv(5, y, sizeof y);
    event(
      std::atoi(phase),
      input->GetKeyCode(path, nullptr),
      0,
      std::atof(x),
      std::atof(y),
      1280,
      800);
  }
  else if (std::string(mode) == "click")
  {
    char y[32] = {}, phase[32] = {};
    commands->Argv(3, y, sizeof y);
    commands->Argv(4, phase, sizeof phase);
    event(std::atoi(phase), mouseKeys[0], 0, std::atof(path), std::atof(y), 1280, 800);
  }
}
static void testCameraInput()
{
  if (!cvars->GetFloat("ftebroom_skip_setup") || commands->IsInsecure())
    return;
  char state[16];
  commands->Argv(1, state, sizeof state);
  event(state[0] == '1' ? 1 : 2, input->GetKeyCode("w", nullptr), 0, 640, 400, 1280, 800);
}
static void testNavigationInput()
{
  if (!cvars->GetFloat("ftebroom_skip_setup") || commands->IsInsecure())
    return;
  auto expect = [](bool ok, const char* name) {
    core->Print(
      (std::string(ok ? "FTEBROOM_NATIVE_CAMERA_PASS " : "FTEBROOM_NATIVE_CAMERA_FAIL ")
       + name + "\n")
        .c_str());
  };
  auto key = [&](const char* name, bool down) {
    event(down ? 1 : 2, input->GetKeyCode(name, nullptr), 0, 640, 400, 1280, 800);
  };
  std::fill(std::begin(moveHeld), std::end(moveHeld), false);
  auto& io = ImGui::GetIO();
  bool oldText = io.WantTextInput;
  io.WantTextInput = true;
  key("q", true);
  key("w", true);
  key("mouse2", true);
  expect(!moveHeld[0] && !moveHeld[4] && !sceneLook, "text_guard");
  key("mouse2", false);
  key("q", false);
  key("w", false);
  io.WantTextInput = false;
  key("shift", true);
  key("w", true);
  expect(moveHeld[0], "shift_flight");
  key("w", false);
  key("shift", false);
  key("alt", true);
  key("q", true);
  expect(moveHeld[4], "alt_vertical_flight");
  key("q", false);
  key("alt", false);
  key("ctrl", true);
  key("x", true);
  expect(!moveHeld[5], "ctrl_guard");
  key("x", false);
  key("ctrl", false);
  key("mouse2", true);
  expect(sceneLook && cameraButton == 1, "right_capture");
  event(3, 0, 0, 4000, -2000, 1280, 800);
  key("mouse2", false);
  expect(!sceneLook, "right_release");
  key("mouse3", true);
  expect(sceneLook && cameraButton == 2, "middle_capture");
  event(3, 0, 0, 100, 20, 1280, 800);
  key("escape", true);
  key("escape", false);
  expect(!sceneLook, "escape_release");
  key("mouse3", false);
  key("mouse2", true);
  key("w", true);
  input->SetMenuFocus(qfalse, "", 0, 0, 1);
  actionFocusTick(0);
  expect(
    !sceneLook && cameraCursorPending && !moveHeld[0]
      && !cvars->GetFloat("ftebroom_forward"),
    "focus_loss");
  openPanel();
  key("mouse2", false);
  expect(!cameraCursorPending, "focus_return");
  key("w", false);
  sceneContextPending = false;
  key("mouse2", true);
  key("mouse2", false);
  expect(sceneContextPending, "right_click_context_menu");
  sceneContextPending = false;
  key("mouse2", true);
  event(3, 0, 0, 20, 0, 1280, 800);
  event(3, 0, 0, -20, 0, 1280, 800);
  key("mouse2", false);
  expect(!sceneContextPending, "out_and_back_drag_not_context_menu");
  io.WantTextInput = oldText;
}
static void testEditorInput()
{
  if (!cvars->GetFloat("ftebroom_skip_setup") || commands->IsInsecure())
    return;
  char chord[128], guard[32];
  commands->Argv(1, chord, sizeof chord);
  commands->Argv(2, guard, sizeof guard);
  std::string name = chord;
  bool savedText = ImGui::GetIO().WantTextInput, savedPopup = popupOpen;
  if (std::string(guard) == "text")
    ImGui::GetIO().WantTextInput = true;
  if (std::string(guard) == "popup")
    popupOpen = true;
  ImVec2 pointer =
    std::string(guard) == "ui" ? ImGui::GetIO().MousePos
      : ImVec2(viewRects[viewFocus].x + viewRects[viewFocus].w * .5f,
               viewRects[viewFocus].y + viewRects[viewFocus].h * .5f);
  std::vector<int> mods;
  for (auto mod : {"Ctrl+", "Alt+", "Shift+"})
  {
    auto pos = name.find(mod);
    if (pos == std::string::npos)
      continue;
    const char* key = mod[0] == 'C' ? "ctrl" : mod[0] == 'A' ? "alt" : "shift";
    int code = input->GetKeyCode(key, nullptr);
    mods.push_back(code);
    event(1, code, 0, pointer.x, pointer.y, 1280, 800);
    name.erase(pos, std::strlen(mod));
  }
  int code = input->GetKeyCode(name.c_str(), nullptr);
  char textCode[32]={};commands->Argv(3,textCode,sizeof textCode);int unicode=std::atoi(textCode);
  event(1, code, unicode, pointer.x, pointer.y, 1280, 800);
  event(2, code, 0, pointer.x, pointer.y, 1280, 800);
  for (auto it = mods.rbegin(); it != mods.rend(); ++it)
    event(2, *it, 0, pointer.x, pointer.y, 1280, 800);
  ImGui::GetIO().WantTextInput = savedText;
  popupOpen = savedPopup;
}
#include "ftebroom_workspace_tests.h"
#include "ftebroom_desktop_tests.h"
static void testUVInput()
{
  if(!cvars->GetFloat("ftebroom_skip_setup"))return;
  if(!uvTestDragReady){core->Print("FTEBROOM_UV_NATIVE_FAIL no_canvas_target\n");return;}
  ImVec2 point=uvTestDragPoint;auto& io=ImGui::GetIO();bool trickle=io.ConfigInputTrickleEventQueue;io.ConfigInputTrickleEventQueue=false;
  event(0,0,0,point.x,point.y,1280,800);event(0,0,0,point.x,point.y,1280,800);
  auto expect=[](bool ok,const char* name){core->Print((std::string(ok?"FTEBROOM_UV_NATIVE_PASS ":"FTEBROOM_UV_NATIVE_FAIL ")+name+"\n").c_str());};
  std::vector<std::pair<ImGuiWindow*,float>> scrollPositions;
  for(auto window:ImGui::GetCurrentContext()->Windows)
    if(window->Active)scrollPositions.emplace_back(window,window->Scroll.y);
  const auto originalZoom=uvTestZoom;
  for(int direction:{1,-1}) {
    event(1,direction>0?wheelUp:wheelDown,0,point.x,point.y,1280,800);
    event(2,direction>0?wheelUp:wheelDown,0,point.x,point.y,1280,800);
    event(0,0,0,point.x,point.y,1280,800);
    event(0,0,0,point.x,point.y,1280,800);
    expect(direction>0?uvTestZoom>originalZoom:std::abs(uvTestZoom-originalZoom)<1e-8,"canvas_wheel_zoom");
    bool stationary=true;
    for(auto entry:scrollPositions)stationary=stationary&&std::abs(entry.first->Scroll.y-entry.second)<.01f;
    expect(stationary,"canvas_wheel_does_not_scroll_inspector");
  }
  event(1,mouseKeys[0],0,point.x,point.y,1280,800);event(0,0,0,point.x,point.y,1280,800);
  event(0,0,0,point.x+13,point.y+7,1280,800);
  event(2,mouseKeys[0],0,point.x+13,point.y+7,1280,800);event(0,0,0,point.x+13,point.y+7,1280,800);
  io.ConfigInputTrickleEventQueue=trickle;core->Print("FTEBROOM_UV_NATIVE_PASS canvas_drag_dispatched\n");
}
static void testModControls()
{
  if(!cvars->GetFloat("ftebroom_skip_setup"))return;
  char value[16]={};commands->Argv(1,value,sizeof value);int stage=std::atoi(value);
  auto expect=[](bool ok,const char* name){core->Print((std::string(ok?"FTEBROOM_CONTROLS_PASS ":"FTEBROOM_CONTROLS_FAIL ")+name+"\n").c_str());};
  if(stage==0){expect(modEmbedded,"embedded_preview");modOpenControls();expect(modMenuOpen&&!modCaptured,"escape_releases_input");}
  else if(stage==1){expect(modMenuOpen&&modMenuDrawn,"imgui_menu_visible");
    modControlsApply();expect(modInputQueue.find("bind ")!=std::string::npos&&modInputQueue.find("set sensitivity")!=std::string::npos,"controls_queued");}
  else {modMenuOpen=false;modControlCapture=-1;}
}
static void shutdownEditor()
{
  pluginsShutdown();
  settleCancel=true;if(settleJob.valid())settleJob.wait();
  textureFolderCancel=true;if(textureFolderJob.valid())textureFolderJob.wait();
  soundStop();
  if(soundJob.valid())soundJob.wait();
  desktopDetach();
  modEmbedEnd();
  if(runActive)runStop();
  cameraSave();
  if (ready)
  {
    ImGui::GetIO().WantSaveIniSettings = true;
    uiSaveLayout();
    clearThumbnails();
    clearEntityPreviews();
    clearFileImages();
    ImGui::DestroyContext();
    draw->UnloadImage(atlas);
    ready = false;
  }
}
static void QDECL videoChanged(int width,int height,qboolean restarted)
{
  if(!ready || !restarted)return;
  // Renderer handles have expired; do not unload them in the new renderer.
  thumbnailCache.clear();entityPreviews.clear();fileImages.clear();++fileImageGeneration;
  modelViewerImage=0;modelViewerSkinTexture=0;modelViewerSkinTextureIndex=-1;
  uploadFontAtlas();
  ImGui::GetIO().DisplaySize={float(width),float(height)};
  core->Print("ReyondBroom: restored ImGui atlas after renderer restart.\n");
}
extern "C" qboolean NATIVEEXPORT FTEPlug_Init(plugcorefuncs_t* api)
{
  core = api;
  commands =
    (plugcmdfuncs_t*)core->GetEngineInterface(plugcmdfuncs_name, sizeof(*commands));
  cvars = (plugcvarfuncs_t*)core->GetEngineInterface(plugcvarfuncs_name, sizeof(*cvars));
  draw = (plug2dfuncs_t*)core->GetEngineInterface(plug2dfuncs_name, sizeof(*draw));
  input =
    (pluginputfuncs_t*)core->GetEngineInterface(pluginputfuncs_name, sizeof(*input));
  files = (plugfsfuncs_t*)core->GetEngineInterface(plugfsfuncs_name, sizeof(*files));
  if (!commands || !cvars || !draw || !input || !files)
  {
    core->Print("ReyondBroom: incompatible FTE plugin API.\n");
    return qfalse;
  }
  for (int i = 0; i < 5; ++i)
  {
    char name[16];
    std::snprintf(name, sizeof name, "mouse%d", i + 1);
    mouseKeys[i] = input->GetKeyCode(name, nullptr);
  }
  wheelUp = input->GetKeyCode("mwheelup", nullptr);
  wheelDown = input->GetKeyCode("mwheeldown", nullptr);
  core->ExportFunction("MenuEvent", (funcptr_t)event);
  core->ExportFunction("Shutdown", (funcptr_t)shutdownEditor);
  core->ExportFunction("UpdateVideo", (funcptr_t)videoChanged);
  core->ExportFunction("Tick", (funcptr_t)actionFocusTick);
  commands->AddCommand(
    "ftebroom_actions_dump", dumpActions, "Dump action ids, contexts and shortcuts.");
  commands->AddCommand(
    "ftebroom_action", invokeActionCommand, "Run a context-aware editor action by id.");
  commands->AddCommand(
    "ftebroom_test_clipboard_native",
    clipboardTestCommand,
    "Test clipboard integration while preserving its previous contents.");
  commands->AddCommand(
    "ftebroom_clipboard_ready",
    clipboardReady,
    "Transfer selected MAP text to the system clipboard.");
  commands->AddCommand(
    "ftebroom_test_workspace",
    testWorkspace,
    "Test workspace appearance and input routing.");
  commands->AddCommand(
    "ftebroom_definitions", definitionsLoadCommand, "Load entity definitions.");
  commands->AddCommand("ftebroom_ui", uiActionCommand, "Run an editor UI action.");
  commands->AddCommand(
    "ftebroom_assets_ready", assetsReady, "Index mounted project assets.");
  commands->AddCommand("ftebroom_start_map",profileStartMap,"Open the starter for the selected game profile.");
  commands->AddCommand(
    "ftebroom_project", loadProjectCommand, "Load Quake 3 assets from a baseq3 folder.");
  commands->AddCommand(
    "ftebroom_test_layout", testViewLayout, "Test viewport layouts and focus routing.");
  commands->AddCommand(
    "ftebroom_test_camera",
    testCameraInput,
    "Test editor input routing in automated runs.");
  commands->AddCommand(
    "ftebroom_test_navigation",
    testNavigationInput,
    "Test camera guards and capture lifecycle.");
  commands->AddCommand(
    "ftebroom_test_key",
    testEditorInput,
    "Exercise editor key routing in automated tests.");
  commands->AddCommand(
    "ftebroom_map_ready", mapReady, "Restore editor focus after map loading.");
  commands->AddCommand(
    "ftebroom_test_filepath",
    testFilePath,
    "Test external map paths in disposable installations.");
  commands->AddCommand(
    "ftebroom_save_finish", documentSaveFinished, "Complete a verified map save.");
  commands->AddCommand("ftebroom_obj_finish", objSaveFinished, "Complete OBJ export.");
  commands->AddCommand("ftebroom_test_trace_files", testTraceFiles, "Test diagnostic files in disposable runs.");
  commands->AddCommand("ftebroom_test_preferences", testPreferences, "Test preferences in disposable runs.");
  commands->AddCommand("ftebroom_test_preferences_reload", testPreferencesReload, "Test saved preferences after restart.");
  commands->AddCommand("ftebroom_test_backups", testBackups, "Test backup rotation in disposable runs.");
  commands->AddCommand("ftebroom_test_mod_controls",testModControls,"Test embedded controls menu.");
  commands->AddCommand("ftebroom_test_uv_input", testUVInput, "Test UV canvas pointer routing.");
  commands->AddCommand("ftebroom_test_run", testRunProfiles, "Test run profiles in disposable runs.");
  commands->AddCommand("ftebroom_test_desktop",testDesktop,"Test desktop integration in disposable installations.");
#ifndef _WIN32
  commands->AddCommand("ftebroom_drop_hex",linuxDropCommand,"Import a local desktop WAD drop.");
#endif
  commands->AddCommand("ftebroom_test_build", testBuild, "Test BSP preview in disposable runs.");
  commands->AddCommand(
    "ftebroom_save", documentSaveCommand, "Save the current document.");
  commands->AddCommand("ftebroom", openPanel, "Open the ReyondBroom editor panel.");
  core->Print("ReyondBroom: native module loaded. Type ftebroom.\n");
  return qtrue;
}
