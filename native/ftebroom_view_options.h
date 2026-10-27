// SPDX-License-Identifier: GPL-2.0-or-later
static void publishMaterialTags()
{
  std::ostringstream out;
  for (const auto& item : materials)
  {
    auto name = item.first;
    if (name.rfind("textures/", 0) == 0)
      name = name.substr(9);
    out << std::quoted(name) << ' ' << item.second.tags << ' '
        << std::quoted(item.second.preview) << '\n';
  }
  const auto content=out.str();
  const auto scope=std::to_string(gameProfile)+":"+std::string(baseq3)+":"+gameRuntime();
  static std::string published, publishedScope;
  // Applying an unrelated profile option must not rebuild every MAP face cache.
  if(content==published && scope==publishedScope)return;
  std::ofstream file(documentPath("ftebroom-material-tags.txt"),std::ios::binary);
  file<<content;file.close();
  if (file) {
    published=content;publishedScope=scope;
    cvars->SetFloat(
      "ftebroom_material_tag_epoch", cvars->GetFloat("ftebroom_material_tag_epoch") + 1);
  }
}
static void viewOption(const char* label, const char* cvar)
{
  bool value = cvars->GetFloat(cvar) != 0;
  if (ImGui::Checkbox(label, &value))
    cvars->SetFloat(cvar, value);
}
static void viewOptionsPanel()
{
  if (!ImGui::CollapsingHeader("View options"))
    return;
  if(ImGui::TreeNode("Antialiasing and texture filtering")) {
    int samples=(int)cvars->GetFloat("vid_multisample");int choice=samples>=8?3:samples>=4?2:samples>=2?1:0;
    if(ImGui::Combo("Edge MSAA",&choice,"Off\0 2x\0 4x\0 8x\0"))cvars->SetFloat("vid_multisample",choice?float(1<<choice):1);
    if(ImGui::Button("Apply MSAA (restart renderer)")) {
      if(documentTerrain || modEmbedded)ImGui::OpenPopup("Apply MSAA");
      else command("ftebroom_renderer_restart\n");
    }
    if(ImGui::BeginPopup("Apply MSAA")){ImGui::TextWrapped("MSAA is saved. Restart the application to apply it when using engine terrain or mod preview.");ImGui::EndPopup();}
    float aa=cvars->GetFloat("ftebroom_grid_aa");if(ImGui::SliderFloat("Grid smoothing",&aa,.05f,2.f))cvars->SetFloat("ftebroom_grid_aa",aa);
    float gridStart=cvars->GetFloat("ftebroom_grid_fade_start"),gridEnd=cvars->GetFloat("ftebroom_grid_fade_end");
    if(ImGui::InputFloat("Grid fade start",&gridStart,64.f,256.f,"%.0f"))cvars->SetFloat("ftebroom_grid_fade_start",std::max(0.f,gridStart));
    if(ImGui::InputFloat("Grid draw distance",&gridEnd,64.f,256.f,"%.0f"))cvars->SetFloat("ftebroom_grid_fade_end",std::max(0.f,gridEnd));
    ImGui::TextDisabled("World units; distance 0 keeps the grid unlimited. Brush edges are unaffected.");
    int anisotropy=std::clamp((int)cvars->GetFloat("gl_texture_anisotropy"),1,16);
    if(ImGui::SliderInt("Texture anisotropy",&anisotropy,1,16))cvars->SetFloat("gl_texture_anisotropy",float(anisotropy));
    char mode[80]={};cvars->GetString("gl_texturemode",mode,sizeof mode);bool smooth=std::string(mode)=="GL_LINEAR_MIPMAP_LINEAR";
    if(ImGui::Checkbox("Trilinear texture filtering",&smooth))cvars->SetString("gl_texturemode",smooth?"GL_LINEAR_MIPMAP_LINEAR":"GL_NEAREST_MIPMAP_NEAREST");
    ImGui::TreePop();
  }
  viewOption("Brushes", "ftebroom_show_brushes");
  viewOption("Sky surface overlays", "ftebroom_sky_overlays");
  const char* tags[] = {
    "Trigger",
    "Clip",
    "Caulk",
    "Hint",
    "Detail",
    "Liquid",
    "Translucent",
    "Areaportal",
    "Clusterportal"};
  int hidden = (int)cvars->GetFloat("ftebroom_hidden_tags");
  for (int i = 0; i < 9; i++)
  {
    bool show = !(hidden & (1 << i));
    if (ImGui::Checkbox(tags[i], &show))
    {
      hidden = show ? hidden & ~(1 << i) : hidden | (1 << i);
      cvars->SetFloat("ftebroom_hidden_tags", float(hidden));
    }
    if (i % 3 != 2)
      ImGui::SameLine();
  }
  ImGui::SeparatorText("Entities");
  viewOption("Point entities", "ftebroom_show_points");
  viewOption("Models", "ftebroom_show_models");
  viewOption("Class names", "ftebroom_show_entity_names");
  float labelDistance=cvars->GetFloat("ftebroom_label_distance");
  if(ImGui::InputFloat("Entity label distance",&labelDistance,128.f,512.f,"%.0f"))cvars->SetFloat("ftebroom_label_distance",std::max(0.f,labelDistance));
  ImGui::TextDisabled("World units; 0 is unlimited. Selected labels stay visible at any distance.");
  viewOption("Point bounds", "ftebroom_show_entity_bounds");
  viewOption("Group bounds", "ftebroom_show_group_bounds");
  viewOption("Brush entity bounds", "ftebroom_show_brush_entity_bounds");
  static char query[128]{};
  ImGui::InputTextWithHint(
    "##classvisibility", "Filter entity classes", query, sizeof query);
  char hiddenBuffer[16384]{};
  cvars->GetString("ftebroom_hidden_classes", hiddenBuffer, sizeof hiddenBuffer);
  std::string hiddenClasses = hiddenBuffer;
  if (ImGui::Button("Show all classes"))
    cvars->SetString("ftebroom_hidden_classes", "");
  ImGui::SameLine();
  if (ImGui::Button("Hide all classes"))
  {
    hiddenClasses.clear();
    std::set<std::string> names;
    for (const auto& item : entityDefinitions)
      names.insert(item.first);
    for (const auto& item : entityUsage)
      names.insert(item.first);
    for (const auto& name : names)
      if (name != "worldspawn")
        hiddenClasses += "|" + name + "|";
    cvars->SetString("ftebroom_hidden_classes", hiddenClasses.c_str());
  }
  ImGui::BeginChild(
    "Class visibility",
    {0, 140 * uiScale()},
    ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY);
  std::map<std::string, std::vector<std::string>> categories;
  std::set<std::string> names;
  for (const auto& item : entityDefinitions)
    names.insert(item.first);
  for (const auto& item : entityUsage)
    names.insert(item.first);
  for (const auto& name : names)
    if (
      name != "worldspawn"
      && entityLower(name).find(entityLower(query)) != std::string::npos)
      categories[name.substr(0, name.find('_'))].push_back(name);
  for (const auto& category : categories)
    if (ImGui::TreeNodeEx(category.first.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
      for (const auto& name : category.second)
      {
        std::string token = "|" + name + "|";
        auto at = hiddenClasses.find(token);
        bool show = at == std::string::npos;
        if (ImGui::Checkbox(name.c_str(), &show))
        {
          if (show)
            hiddenClasses.erase(at, token.size());
          else
            hiddenClasses += token;
          cvars->SetString("ftebroom_hidden_classes", hiddenClasses.c_str());
        }
      }
      ImGui::TreePop();
    }
  ImGui::EndChild();
  ImGui::SeparatorText("Renderer");
  int mode = (int)cvars->GetFloat("ftebroom_render_faces");
  if (ImGui::Combo("Faces", &mode, "Textured\0Flat color\0Wireframe\0"))
    cvars->SetFloat("ftebroom_render_faces", float(mode));
  viewOption("Shade faces", "ftebroom_shade_faces");
  viewOption("Use fog", "r_fog_enabled");
  viewOption("Edges", "ftebroom_show_edges");
  viewOption("Soft map bounds", "ftebroom_bounds_visible");
  int links = (int)cvars->GetFloat("ftebroom_entity_links");
  if (ImGui::Combo("Entity links", &links, "None\0Direct\0Transitive\0All\0"))
    cvars->SetFloat("ftebroom_entity_links", float(links));
  ImGui::SeparatorText("Selection tags");
  for (const char* type : {"trigger", "clip", "caulk", "hint", "detail"})
  {
    ImGui::PushID(type);
    ImGui::TextUnformatted(type);
    ImGui::SameLine();
    if (ImGui::SmallButton("Set"))
      command(("ftebroom_tag " + std::string(type) + " 1\n").c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("Remove"))
      command(("ftebroom_tag " + std::string(type) + " 0\n").c_str());
    ImGui::PopID();
  }
}
