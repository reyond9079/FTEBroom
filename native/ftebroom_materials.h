// SPDX-License-Identifier: GPL-2.0-or-later
#include <chrono>
#include <sstream>
static std::map<std::string, qhandle_t> thumbnailCache;
static std::map<std::string,int> thumbnailLastFrame;
static unsigned materialDataRevision=0,materialUsageRevision=0;
static std::map<std::string, ImVec2> materialDimensions;
static bool materialAdvanced=false;
static int materialWidths[2]={0,0},materialHeights[2]={0,0};
static std::map<std::string, ImVec2> materialTestItems;
static std::map<std::string, bool> materialFolders;
static std::set<std::string> removedCollections;
static std::string collectionScope;
static void collectionsLoad() {
  auto scope=std::to_string(gameProfile)+":"+std::string(baseq3);
  if(scope==collectionScope)return;collectionScope=scope;removedCollections.clear();
  std::ifstream input(settingsPath().parent_path()/"ftebroom-collections.txt");std::string key,value;
  while(input>>std::quoted(key)>>std::quoted(value))if(key==scope)removedCollections.insert(value);
}
static void collectionsSave() {
  auto path=settingsPath().parent_path()/"ftebroom-collections.txt";
  std::ifstream input(path);std::string key,value;std::vector<std::pair<std::string,std::string>> other;
  while(input>>std::quoted(key)>>std::quoted(value))if(key!=collectionScope)other.emplace_back(key,value);input.close();
  std::ofstream output(path);for(auto& entry:other)output<<std::quoted(entry.first)<<' '<<std::quoted(entry.second)<<'\n';
  for(auto& entry:removedCollections)output<<std::quoted(collectionScope)<<' '<<std::quoted(entry)<<'\n';
}
static void clearThumbnails()
{
  for (auto& pair : thumbnailCache)
    draw->UnloadImage(pair.second);
  thumbnailCache.clear();thumbnailLastFrame.clear();
}
static void resetMaterialBrowser()
{
  ++materialDataRevision;clearThumbnails();
  materialFolders.clear();materialDimensions.clear();
  selectedMaterial.clear();
}
static std::string materialFolder(const std::string& name)
{
  if(gameWad()){auto shortName=name.rfind("textures/",0)==0?name.substr(9):name;auto wad=q1MaterialWad.find(shortName);if(wad!=q1MaterialWad.end())return wad->second;}
  auto slash = name.find_last_of('/');
  return slash == std::string::npos ? "(root)" : name.substr(0, slash);
}
static qhandle_t thumbnail(const std::string& name)
{
  auto cached = thumbnailCache.find(name);
  if (cached != thumbnailCache.end()) {
    thumbnailLastFrame[name]=ImGui::GetFrameCount();return cached->second;
  }
  // Never release handles referenced by this or the preceding ImGui frame.
  // Evict one old image, rather than flushing every visible thumbnail at once.
  if(thumbnailCache.size()>=512){
    auto victim=thumbnailCache.end();int oldest=ImGui::GetFrameCount()-1;
    for(auto it=thumbnailCache.begin();it!=thumbnailCache.end();++it)if(thumbnailLastFrame[it->first]<oldest){oldest=thumbnailLastFrame[it->first];victim=it;}
    if(victim!=thumbnailCache.end()){draw->UnloadImage(victim->second);thumbnailLastFrame.erase(victim->first);thumbnailCache.erase(victim);}
  }
  auto found = materials.find(name);
  auto image = found != materials.end() && !found->second.preview.empty()
                 ? found->second.preview
                 : name;
  // Browser previews use a dedicated 2D shader, never a material's vertex deformation.
  std::ostringstream quoted;quoted<<std::quoted(image);
  auto shader=std::string("{\n program default2d\n nomipmaps\n nopicmip\n {\n map ")+quoted.str()+"\n rgbgen vertex\n alphagen vertex\n blendfunc blend\n }\n}\n";
  auto handle = draw->LoadImageShader(("ftebroom/thumbnail/"+name+"#resources_"+std::to_string(materialDataRevision)).c_str(),shader.c_str());
  thumbnailCache[name] = handle;thumbnailLastFrame[name]=ImGui::GetFrameCount();
  return handle;
}
static void chooseMaterial(const std::string& name)
{
  selectedMaterial = name;
  auto brushName = name.rfind("textures/", 0) == 0 ? name.substr(9) : name;
  cvars->SetString("ca_newbrushtexture", brushName.c_str());
  action("ftebroom_material\n");
}
static void uvField(const char* label, const char* op, const char* source, float& value)
{
  struct Field
  {
    char text[64] = {};
    bool active = false;
  };
  static std::map<std::string, Field> fields;
  auto& field = fields[op];
  const std::string operation = op;
  const int bit = operation == "sx"      ? 1
                  : operation == "sy"    ? 2
                  : operation == "angle" ? 4
                  : operation == "ox"    ? 8
                                         : 16;
  const bool mixed = (static_cast<int>(cvars->GetFloat("ftebroom_uv_mixed")) & bit) != 0;
  if (!field.active)
  {
    value = cvars->GetFloat(source);
    if (mixed)
      field.text[0] = 0;
    else
      std::snprintf(field.text, sizeof field.text, "%.6g", value);
  }
  ImGui::PushID(op);
  ImGui::SetNextItemWidth(120 * uiScale());
  ImGui::InputTextWithHint(
    "##value", mixed ? "multi" : "", field.text, sizeof field.text);
  materialTestItems[op]={(ImGui::GetItemRectMin().x+ImGui::GetItemRectMax().x)*0.5f,(ImGui::GetItemRectMin().y+ImGui::GetItemRectMax().y)*0.5f};
  field.active = ImGui::IsItemActive();
  const bool edited = ImGui::IsItemDeactivatedAfterEdit();
  const bool fieldFocused = ImGui::IsItemFocused();
  if (edited)
  {
    char* end = nullptr;
    const float number = std::strtof(field.text, &end);
    if (end != field.text && *end == 0 && std::isfinite(number))
    {
      char text[128];
      std::snprintf(text, sizeof text, "ftebroom_uv %s %.9g 0\n", op, number);
      action(text);
    }
  }
  float step = operation == "angle"                     ? 15.f
               : operation == "sx" || operation == "sy" ? 0.1f
                                                        : cvars->GetFloat("ca_grid");
  auto& io = ImGui::GetIO();
  if (io.KeyShift)
    step = operation == "angle"                     ? 90.f
           : operation == "sx" || operation == "sy" ? 0.25f
                                                    : step * 2;
  if (io.KeyCtrl)
    step = operation == "sx" || operation == "sy" ? 0.01f : 1.f;
  int direction = 0;
  if (fieldFocused)
  {
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || io.MouseWheel > 0)
      direction = 1;
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) || io.MouseWheel < 0)
      direction = -1;
  }
  ImGui::SameLine();
  if (ImGui::SmallButton("-"))
    direction = -1;
  ImGui::SameLine();
  if (ImGui::SmallButton("+"))
    direction = 1;
  if (direction)
  {
    char text[128];
    std::snprintf(text, sizeof text, "ftebroom_uv %s %.9g 1\n", op, step * direction);
    action(text);
    field.active = false;
  }
  ImGui::SameLine();
  ImGui::TextUnformatted(label);
  ImGui::PopID();
}
static void materialFlagField(
  const char* label, const char* op, const char* source, int mixedBit)
{
  struct Field
  {
    char text[64] = {};
    bool active = false;
  };
  static std::map<std::string, Field> fields;
  auto& field = fields[op];
  const bool mixed =
    (static_cast<int>(cvars->GetFloat("ftebroom_uv_mixed")) & mixedBit) != 0;
  if (!field.active)
  {
    if (mixed)
      field.text[0] = 0;
    else
      cvars->GetString(source, field.text, sizeof field.text);
  }
  ImGui::PushID(op);
  ImGui::SetNextItemWidth(150 * uiScale());
  ImGui::InputTextWithHint(
    "##flags", mixed ? "multi" : "", field.text, sizeof field.text);
  field.active = ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit())
  {
    char* end = nullptr;
    const auto number = std::strtoll(field.text, &end, 0);
    if (end != field.text && *end == 0 && number >= INT32_MIN && number <= UINT32_MAX)
      action((std::string("ftebroom_flags ") + op + " "
              + std::to_string(static_cast<int32_t>(number)) + " 0\n")
               .c_str());
  }
  if (std::string(op) != "value")
  {
    ImGui::SameLine();
    if (ImGui::SmallButton("..."))
      ImGui::OpenPopup("bits");
    if (ImGui::BeginPopup("bits"))
    {
      char current[64];
      cvars->GetString(source, current, sizeof current);
      const auto flags = static_cast<uint32_t>(std::strtoll(current, nullptr, 10));
      static const char* surfaceNames[] = {
        "No damage",
        "Slick",
        "Sky",
        "Ladder",
        "No impact",
        "No marks",
        "Flesh",
        "No draw",
        "Hint",
        "Skip",
        "No lightmap",
        "Point light",
        "Metal steps",
        "No steps",
        "Non solid",
        "Light filter",
        "Alpha shadow",
        "No dynamic light",
        "Dust"};
      static const char* contentNames[32] = {
        "Solid",          nullptr,        nullptr,        "Lava",       "Slime",
        "Water",          "Fog",          "Not team 1",   "Not team 2", "No bot clip",
        nullptr,          nullptr,        nullptr,        nullptr,      nullptr,
        "Area portal",    "Player clip",  "Monster clip", "Teleporter", "Jump pad",
        "Cluster portal", "Do not enter", "Bot clip",     "Mover",      "Origin",
        "Body",           "Corpse",       "Detail",       "Structural", "Translucent",
        "Trigger",        "No drop"};
      static const char* q2Surfaces[]={"Light","Slick","Sky","Warp","Translucent 33%","Translucent 66%","Flowing","No draw","Hint","Skip"};
      static const char* q2Contents[32]={"Solid","Window","Auxiliary","Lava","Slime","Water","Mist",nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,"Area portal","Player clip","Monster clip","Current 0","Current 90","Current 180","Current 270","Current up","Current down","Origin","Monster","Dead monster","Detail","Translucent","Ladder",nullptr,nullptr};
      for (unsigned bit = 0; bit < 32; ++bit)
      {
        bool enabled = (flags & (uint32_t(1) << bit)) != 0;
        const char* flagName = nullptr;
        if (
          std::string(op) == "surface"
          && bit < sizeof surfaceNames / sizeof *surfaceNames)
          flagName = surfaceNames[bit];
        else if (std::string(op) == "contents")
          flagName = contentNames[bit];
        if(gameProfile==3)flagName=std::string(op)=="surface"?(bit<10?q2Surfaces[bit]:nullptr):std::string(op)=="contents"?q2Contents[bit]:nullptr;
        const auto name = flagName ? std::string(flagName) + "##" + std::to_string(bit)
                                   : "Bit " + std::to_string(bit);
        if (ImGui::Checkbox(name.c_str(), &enabled))
          action((std::string("ftebroom_flags ") + op + " "
                  + std::to_string(static_cast<int32_t>(uint32_t(1) << bit))
                  + (enabled ? " 1\n" : " 2\n"))
                   .c_str());
      }
      ImGui::EndPopup();
    }
  }
  ImGui::SameLine();
  ImGui::TextUnformatted(label);
  ImGui::PopID();
}
static std::map<std::string, int> materialUsage;
static std::string materialCanonical(std::string name)
{
  if (name.rfind("textures/", 0) == 0)
    name.erase(0, 9);
  return name;
}
static void readMaterialUsage()
{
  static auto last = std::chrono::steady_clock::time_point{};
  const auto now = std::chrono::steady_clock::now();
  if (now - last < std::chrono::milliseconds(250))
    return;
  last = now;
  qhandle_t handle;
  const auto length = files->Open("data/material-usage.txt", &handle, 1);
  if (length < 0)
    return;
  if (length > 16 * 1024 * 1024)
  {
    files->Close(handle);
    return;
  }
  std::string text(static_cast<size_t>(length), '\0');
  files->Read(handle, text.data(), static_cast<int>(length));
  files->Close(handle);
  static std::string previous;static unsigned revision=~0u;
  if(previous==text && revision==materialDataRevision)return;
  previous=text;revision=materialDataRevision;++materialUsageRevision;
  materialUsage.clear();
  std::istringstream rows(text);
  std::string row;
  while (std::getline(rows, row))
  {
    auto space = row.find(' ');
    if (space != std::string::npos)
    {
      const auto name = materialCanonical(row.substr(space + 1));
      materialUsage[name] = std::atoi(row.c_str());
      if (!materials.count(name) && !materials.count("textures/" + name))
        materials[name] = MaterialEntry{name, "", false};
    }
  }
}
static bool materialMatches(const std::string& name, const char* query)
{
  auto lower = [](std::string text) {
    for (char& c : text)
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
  };
  const auto target = lower(name);
  std::istringstream words(lower(query));
  std::string word;
  while (words >> word)
    if (target.find(word) == std::string::npos)
      return false;
  return true;
}
#include "ftebroom_uv_editor.h"
static void materialBrowser()
{
  static float propertiesHeight=180;uiInspectorTopBegin("Material properties",propertiesHeight);
  // Distinguish editable UV values from the dark inspector background.
  if(uiTheme==0) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f*uiScale());
    ImGui::PushStyleColor(ImGuiCol_Border,uiColor(105,115,130));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,uiColor(42,46,52));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,uiColor(53,62,76));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,uiColor(62,78,102));
  }
  importMaterialControls();
  graphicalUVEditor();
  readMaterialUsage();

  char current[1024] = {};
  cvars->GetString("ftebroom_current_material", current, sizeof current);
  ImGui::SeparatorText(cvars->GetFloat("ftebroom_face_count") ? "Selected surfaces" : "New brush material");
  if(gameWad() && *current && current[0]!='<') {
    std::string key=current;std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return char(std::tolower(c));});if(key.rfind("textures/",0)!=0)key="textures/"+key;
    if(!materials.count(key)) {
      ImGui::TextWrapped("Texture is not present in the connected WADs. Add the WAD containing this texture.");
    }
  }
  if (*current && current[0] != '<')
  {
    auto name = std::string(current);
    if(gameWad())std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
    if (name.rfind("textures/", 0) != 0 && materials.count("textures/" + name))
      name = "textures/" + name;
    ImGui::Image((ImTextureID)thumbnail(name), {52, 52});
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextWrapped("%s", current);
    ImGui::Text("%i faces", (int)cvars->GetFloat("ftebroom_face_count"));
    ImGui::EndGroup();
  }
  else
    ImGui::TextWrapped("%s", *current ? current : "No brush or face selected");
  static char materialName[1024] = {};
  static bool materialEditing = false;
  if (!materialEditing)
    std::snprintf(
      materialName, sizeof materialName, "%s", current[0] == '<' ? "" : current);
  ImGui::SetNextItemWidth(-1);
  ImGui::InputTextWithHint(
    "##material_name", "Material name", materialName, sizeof materialName);
  materialEditing = ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit() && *materialName)
    chooseMaterial(materialName);
  static float sx = 1, sy = 1, angle = 0, ox = 0, oy = 0;
  uvField("Offset X", "ox", "ftebroom_offset_x", ox);
  uvField("Offset Y", "oy", "ftebroom_offset_y", oy);
  uvField("Scale X", "sx", "ftebroom_scale_x", sx);
  uvField("Scale Y", "sy", "ftebroom_scale_y", sy);
  uvField("Angle", "angle", "ftebroom_angle", angle);
  ImGui::BeginDisabled(cvars->GetFloat("ftebroom_face_count") == 0);
  const char* alignOps[] = {
    "left", "right", "top", "bottom", "edge", "fit_h", "fit_v", "auto"};
  const char* alignLabels[] = {
    "Left", "Right", "Top", "Bottom", "Edge", "Fit H", "Fit V", "Auto"};
  for (int alignIndex = 0; alignIndex < 8; ++alignIndex)
  {
    const auto* op = alignOps[alignIndex];
    if (std::string(op) != "left" && std::string(op) != "edge")
      ImGui::SameLine();
    if (ImGui::SmallButton(alignLabels[alignIndex]))
      action((std::string("ftebroom_texture_align ") + op
              + (ImGui::GetIO().KeyShift ? " -1 " : " 1 ")
              + (ImGui::GetIO().KeyCtrl ? "1\n" : "0\n"))
               .c_str());
  }
  if (!gameWad() && ImGui::CollapsingHeader("Surface attributes"))
  {
    materialFlagField("Surface Flags", "surface", "ftebroom_surface_flags", 32);
    materialFlagField("Content Flags", "contents", "ftebroom_content_flags", 64);
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Content flags apply to the entire brush.");
    materialFlagField("Surface Value", "value", "ftebroom_surface_value", 128);
  }
  ImGui::EndDisabled();
  if (cvars->GetFloat("ftebroom_uv_mixed"))
    ImGui::TextWrapped("Mixed UV values; editing changes selected faces.");
  ImGui::TextWrapped("Shift+click: select a face. Enter: commit UV.");
  if(uiTheme==0){ImGui::PopStyleColor(4);ImGui::PopStyleVar();}
  uiInspectorTopEnd("Material browser divider",propertiesHeight);
  ImGui::SeparatorText("Material collections");
  if(gameWad() && !q1MissingWads.empty()) {
    ImGui::TextColored({1,.7f,.25f,1},"Missing WAD archives: %zu",q1MissingWads.size());
    if(ImGui::IsItemHovered()){ImGui::BeginTooltip();ImGui::TextUnformatted("The MAP stores texture names, not texture pixels. Reconnect these WADs with Add WAD(s):");for(const auto& path:q1MissingWads)ImGui::TextUnformatted(path.u8string().c_str());ImGui::EndTooltip();}
  }
  collectionsLoad();
  static unsigned folderRevision=~0u;static size_t folderCount=0;
  if(folderRevision!=materialDataRevision || folderCount!=materials.size()) {
    folderRevision=materialDataRevision;folderCount=materials.size();
    for(auto& pair:materials)materialFolders.emplace(materialFolder(pair.first),true);
  }
  if (ImGui::BeginCombo("##collections", "Choose visible folders..."))
  {
    if (ImGui::Button("All"))
      for (auto& pair : materialFolders)
        pair.second = true;
    ImGui::SameLine();
    if (ImGui::Button("None"))
      for (auto& pair : materialFolders)
        pair.second = false;
    static char search[256]={};ImGui::InputTextWithHint("##collection_search","Search collections",search,sizeof search);
    if(!removedCollections.empty() && ImGui::Button("Restore removed collections")){removedCollections.clear();collectionsSave();}
    for (auto& pair : materialFolders) {
      if(removedCollections.count(pair.first)||!materialMatches(pair.first,search))continue;
      ImGui::PushID(pair.first.c_str());ImGui::Checkbox(pair.first.c_str(), &pair.second);ImGui::SameLine();
      if(ImGui::SmallButton("Remove")){removedCollections.insert(pair.first);collectionsSave();}
      if(ImGui::IsItemHovered())ImGui::SetTooltip("Remove from this browser. Files and map materials are preserved.");ImGui::PopID();
    }
    ImGui::EndCombo();
  }
  static char filter[256] = {};
  char reveal[1024]{};cvars->GetString("ftebroom_reveal_material",reveal,sizeof reveal);
  if(*reveal){selectedMaterial=std::string("textures/")+reveal;materialFolders[materialFolder(selectedMaterial)]=true;std::snprintf(filter,sizeof filter,"%s",reveal);cvars->SetString("ftebroom_reveal_material","");}

  ImGui::SetNextItemWidth(-1);
  ImGui::InputTextWithHint(
    "##material_filter", "Filter: all words", filter, sizeof filter);
  auto& advanced=materialAdvanced;auto& widths=materialWidths;auto& heights=materialHeights;
  ImGui::Checkbox("Advanced search",&advanced);
  if(advanced){ImGui::InputInt2("Width min / max",widths);ImGui::InputInt2("Height min / max",heights);for(auto v:{widths,heights})for(int n=0;n<2;n++)v[n]=std::max(0,v[n]);ImGui::TextDisabled("Pixels; 0 = no limit. Equal min/max = exact size.");}
  int dimensionBudget=12,dimensionPending=0;
  auto dimensionsMatch=[&](const std::string& name){
    if(!advanced||(!widths[0]&&!widths[1]&&!heights[0]&&!heights[1]))return true;
    auto found=materialDimensions.find(name);
    if(found==materialDimensions.end()){
      auto wadSize=q1MaterialDimensions.find(materialCanonical(name));
      if(gameWad() && wadSize!=q1MaterialDimensions.end())found=materialDimensions.emplace(name,ImVec2(wadSize->second.first,wadSize->second.second)).first;
    }
    if(found==materialDimensions.end()){
      if(!dimensionBudget--){dimensionBudget=0;++dimensionPending;return false;}
      float w=0,h=0;if(draw->ImageSize(thumbnail(name),&w,&h)<=0||w<=0||h<=0){materialDimensions.emplace(name,ImVec2(-1,-1));return false;}
      found=materialDimensions.emplace(name,ImVec2(w,h)).first;
    }
    auto d=found->second;if(d.x<=0||d.y<=0)return false;return (!widths[0]||d.x>=widths[0])&&(!widths[1]||d.x<=widths[1])&&(!heights[0]||d.y>=heights[0])&&(!heights[1]||d.y<=heights[1]);
  };
  static bool usedOnly = false, byUsage = false, grouped = false;
  ImGui::Checkbox("Used", &usedOnly);
  ImGui::SameLine();
  ImGui::Checkbox("Group", &grouped);
  ImGui::Checkbox("Sort by usage", &byUsage);
  static int size = 64;
  ImGui::SetNextItemWidth(160 * uiScale());
  ImGui::SliderInt("Thumbnail size", &size, 48, 128);
  static std::vector<const MaterialEntry*> visible;
  static std::string cachedQuery;static std::map<std::string,bool> cachedFolders;static std::set<std::string> cachedRemoved;
  static unsigned cachedRevision=~0u,cachedUsage=~0u;static size_t cachedCount=0;static bool pendingDimensions=false;
  auto query=std::string(filter)+":"+std::to_string(usedOnly)+":"+std::to_string(byUsage)+":"+std::to_string(grouped)+":"+std::to_string(advanced);
  for(auto values:{widths,heights})for(int i=0;i<2;i++)query+=":"+std::to_string(values[i]);
  if(cachedRevision!=materialDataRevision || cachedCount!=materials.size() || cachedQuery!=query || cachedFolders!=materialFolders || cachedRemoved!=removedCollections || ((usedOnly||byUsage)&&cachedUsage!=materialUsageRevision) || pendingDimensions) {
    cachedRevision=materialDataRevision;cachedCount=materials.size();cachedQuery=query;cachedFolders=materialFolders;cachedRemoved=removedCollections;cachedUsage=materialUsageRevision;
  visible.clear();
  for (auto& pair : materials)
    if (
      !removedCollections.count(materialFolder(pair.first)) && materialFolders[materialFolder(pair.first)] && materialMatches(pair.first, filter)
      && (!usedOnly || materialUsage[materialCanonical(pair.first)] > 0) && dimensionsMatch(pair.first))
      visible.push_back(&pair.second);
  std::stable_sort(visible.begin(), visible.end(), [&](const auto* a, const auto* b) {
    if (grouped && materialFolder(a->name) != materialFolder(b->name))
      return materialFolder(a->name) < materialFolder(b->name);
    if (
      byUsage
      && materialUsage[materialCanonical(a->name)]
           != materialUsage[materialCanonical(b->name)])
      return materialUsage[materialCanonical(a->name)]
             > materialUsage[materialCanonical(b->name)];
    return a->name < b->name;
  });
    pendingDimensions=dimensionPending!=0;
  }
  ImGui::Text("%zu / %zu materials", visible.size(), materials.size());
  if(dimensionPending)ImGui::TextDisabled("Reading image dimensions: %i pending",dimensionPending);
  ImGui::BeginChild(
    "material_tiles",
    {0, std::max(160.f * uiScale(), ImGui::GetContentRegionAvail().y)},
    ImGuiChildFlags_Borders);
  int columns = std::max(1, int(ImGui::GetContentRegionAvail().x) / (size + 12));
  float rowHeight = float(size + 12) + ImGui::GetTextLineHeightWithSpacing() * 2;
  ImGuiListClipper clipper;
  clipper.Begin((int(visible.size()) + columns - 1) / columns, rowHeight);
  while (clipper.Step())
    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
    {
      float top = ImGui::GetCursorPosY();
      for (int col = 0; col < columns; ++col)
      {
        size_t index = size_t(row * columns + col);
        if (index >= visible.size())
          break;
        auto& mat = *visible[index];
        ImGui::SetCursorPos({float(8 + col * (size + 12)), top});
        ImGui::PushID(mat.name.c_str());
        auto p = ImGui::GetCursorScreenPos();
        bool selected =
          selectedMaterial == mat.name || mat.name == std::string("textures/") + current;
        if (ImGui::InvisibleButton("tile", {float(size + 6), rowHeight - 4}))
          chooseMaterial(mat.name);
        if(ImGui::BeginDragDropSource()) {
          ImGui::SetDragDropPayload("FTEBROOM_MATERIAL",mat.name.c_str(),mat.name.size()+1);
          ImGui::TextUnformatted(mat.name.c_str());ImGui::EndDragDropSource();
        }
        if (ImGui::BeginPopupContextItem("material_context"))
        {
          for (const auto* mode : {"faces", "brushes"})
          {
            if (ImGui::MenuItem((std::string("Select ") + mode).c_str()))
            {
              cvars->SetString("ftebroom_material_query", mat.name.c_str());
              action((std::string("ftebroom_material_select ") + mode + "\n").c_str());
            }
          }
          ImGui::EndPopup();
        }
        auto d = ImGui::GetWindowDrawList();
        d->AddRectFilled(p, {p.x + size + 6, p.y + size + 6}, IM_COL32(65, 68, 72, 255));
        auto tex = thumbnail(mat.name);
        float w = 1, h = 1;
        draw->ImageSize(tex, &w, &h);
        float factor = float(size) / std::max(1.f, std::max(w, h));
        ImVec2 lo{p.x + 3 + (size - w * factor) / 2, p.y + 3 + (size - h * factor) / 2};
        d->PushClipRect(p,{p.x+size+6,p.y+size+6},true);
        d->AddImage((ImTextureID)tex, lo, {lo.x + w * factor, lo.y + h * factor});
        d->PopClipRect();
        if (materialUsage[materialCanonical(mat.name)] > 0)
          d->AddRect(
            p, {p.x + size + 6, p.y + size + 6}, IM_COL32(230, 205, 45, 255), 0, 0, 2);
        if (selected)
          d->AddRect(
            p, {p.x + size + 6, p.y + size + 6}, IM_COL32(225, 55, 55, 255), 0, 0, 2);
        auto slash = mat.name.find_last_of('/');
        auto label = mat.name.substr(slash == std::string::npos ? 0 : slash + 1);
        d->PushClipRect(
          {p.x, p.y + size + 8}, {p.x + size + 6, p.y + rowHeight - 2}, true);
        d->AddText(
          {p.x, p.y + size + 8}, ImGui::GetColorU32(ImGuiCol_Text), label.c_str());
        d->PopClipRect();
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip(
            "%s\n%s - %.0f x %.0f\nUsed on %d surfaces",
            mat.name.c_str(),
            mat.shader ? "Shader" : "Texture",
            w,
            h,
            materialUsage[materialCanonical(mat.name)]);
        ImGui::PopID();
      }
      ImGui::SetCursorPosY(top + rowHeight - ImGui::GetStyle().ItemSpacing.y);
      ImGui::Dummy({0, 0});
    }
  ImGui::EndChild();
}


static bool replaceMaterialRequest;
static void replaceMaterialDialog()
{
  static std::string from, to;
  static char findFrom[256] = {}, findTo[256] = {};
  static bool used = true;
  if (replaceMaterialRequest)
  {
    replaceMaterialRequest = false;
    char current[1024];
    cvars->GetString("ca_newbrushtexture", current, sizeof current);
    from = current;
    ImGui::OpenPopup("Replace Material");
  }
  ImGui::SetNextWindowSize({760, 480}, ImGuiCond_FirstUseEver);
  if (!uiDialog("Replace Material",{760,480}))
    return;
  ImGui::Checkbox("Used materials only on the left", &used);
  if (ImGui::BeginTable("replace_choices", 2, ImGuiTableFlags_SizingStretchSame))
  {
    for (int column = 0; column < 2; ++column)
    {
      ImGui::TableNextColumn();
      ImGui::PushID(column);
      ImGui::TextUnformatted(column == 0 ? "Replace" : "With");
      auto& selected = column == 0 ? from : to;
      auto* filter = column == 0 ? findFrom : findTo;
      ImGui::SetNextItemWidth(-1);
      ImGui::InputTextWithHint("##filter", "Filter materials", filter, 256);
      if (ImGui::BeginListBox("##choices", {-1, 300}))
      {
        for (const auto& pair : materials)
        {
          if (!materialMatches(pair.first, filter))
            continue;
          if (column == 0 && used && materialUsage[materialCanonical(pair.first)] == 0)
            continue;
          ImGui::Image((ImTextureID)thumbnail(pair.first), {24, 24});
          ImGui::SameLine();
          if (
            ImGui::Selectable(
              pair.first.c_str(),
              materialCanonical(selected) == materialCanonical(pair.first)))
            selected = pair.first;
        }
        ImGui::EndListBox();
      }
      ImGui::TextWrapped("%s", selected.c_str());
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  ImGui::TextUnformatted(
    "Applies to selected surfaces, or the whole map when nothing is selected.");
  ImGui::BeginDisabled(
    from.empty() || to.empty() || materialCanonical(from) == materialCanonical(to));
  if (ImGui::Button("Replace"))
  {
    cvars->SetString("ftebroom_replace_from", from.c_str());
    cvars->SetString("ftebroom_replace_to", to.c_str());
    action("ftebroom_material_replace\n");
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  if (ImGui::Button("Close") || ImGui::IsKeyPressed(ImGuiKey_Escape))
    ImGui::CloseCurrentPopup();
  ImGui::EndPopup();
}
