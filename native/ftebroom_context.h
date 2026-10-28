// SPDX-License-Identifier: GPL-2.0-or-later
static void bspModelShow();
static bool sceneContextPending = false, sceneContextCandidate = false;
static int sceneContextSerial = 0;
static quintptr_t sceneContextTime;
static float sceneContextDX = 0, sceneContextDY = 0, sceneContextX = 0, sceneContextY = 0;
static bool organizationNameRequest = false, organizationRename = false;
static void sceneContextBegin(float x, float y)
{
  sceneContextCandidate = !altHeld && !ctrlHeld && !shiftHeld
    && double(core->GetMilliseconds())/1000.0-cvars->GetFloat("ftebroom_focus_time")>.35;
  sceneContextTime = core->GetMilliseconds();
  sceneContextDX = sceneContextDY = 0;
  sceneContextX = x;
  sceneContextY = y;
}
static void sceneContextRelease()
{
  if (
    !sceneLook || cameraButton != 1 || !sceneContextCandidate
    || core->GetMilliseconds() - sceneContextTime >= 300)
    return;
  if (sceneContextDX * sceneContextDX + sceneContextDY * sceneContextDY >= 16)
    return;
  sceneContextSerial = (int)cvars->GetFloat("ftebroom_context_serial");
  sceneContextPending = true;
  char query[128];
  std::snprintf(
    query, sizeof query, "ftebroom_context %g %g\n", sceneContextX, sceneContextY);
  command(query);
}
static void sceneContextPanel()
{
  if (
    sceneContextPending
    && sceneContextSerial != (int)cvars->GetFloat("ftebroom_context_serial"))
  {
    sceneContextPending = false;
    ImGui::OpenPopup("Scene context");
    ImGui::SetNextWindowPos({sceneContextX, sceneContextY});
  }
  if (ImGui::BeginPopup("Scene context"))
  {
    if(gameProfile==1 && ImGui::MenuItem("Create FTE Engine terrain here...")){terrainCreateWindow=true;command(("ftebroom_terrain_position "+std::to_string(sceneContextX)+" "+std::to_string(sceneContextY)+"\n").c_str());}
    bool selected = cvars->GetFloat("ftebroom_selection_count")
                    || cvars->GetFloat("ftebroom_entity_selection_count");
    int group = (int)cvars->GetFloat("ftebroom_context_group"),
        owner = (int)cvars->GetFloat("ftebroom_context_entity"),
        layer = (int)cvars->GetFloat("ftebroom_context_layer");
    if(ImGui::MenuItem("Export selected BSP model...",nullptr,false,cvars->GetFloat("ftebroom_selection_count")>0))bspModelShow();
    if (ImGui::MenuItem("Group...", nullptr, false, selected))
    {
      organizationNameRequest = true;
      organizationRename = false;
    }
    if (ImGui::MenuItem("Ungroup", nullptr, false, selected))
      organize("ungroup");
    if (ImGui::MenuItem("Merge groups", nullptr, false, selected))
      organize("mergegroups");
    if (ImGui::MenuItem("Rename groups...", nullptr, false, selected))
    {
      organizationNameRequest = true;
      organizationRename = true;
    }
    if (group && ImGui::MenuItem("Add objects to this group", nullptr, false, selected))
      organize("move", group);
    if (ImGui::MenuItem("Remove objects from group", nullptr, false, selected))
      organize("removefromgroup");
    if (ImGui::BeginMenu("Move to layer", selected))
    {
      for (const auto& n : organizationNodes)
        if (n.kind == 1 && ImGui::MenuItem(n.name.c_str()))
          organize("move", n.id);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Make layer active"))
    {
      for (const auto& n : organizationNodes)
        if (n.kind == 1 && ImGui::MenuItem(n.name.c_str()))
          organize("layeractive", n.id);
      ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Hide this layer"))
      organize("layerhidden", layer, "1");
    if (ImGui::MenuItem("Select all in this layer"))
      organize("layerselect", layer);
    if (ImGui::MenuItem("Isolate this layer"))
    {
      for (const auto& n : organizationNodes)
        if (n.kind == 1)
          organize("layerhidden", n.id, n.id == layer ? "0" : "1");
    }
    if (ImGui::MenuItem("Make structural", nullptr, false, selected))
      organize("structural");
    char classname[256]{}, material[1024]{};
    cvars->GetString("ftebroom_context_class", classname, sizeof classname);
    cvars->GetString("ftebroom_context_material", material, sizeof material);
    if (*classname && ImGui::MenuItem(("Select all " + std::string(classname)).c_str()))
      command(
        ("ftebroom_entity_select_class " + organizationQuote(classname) + "\n").c_str());
    if (
      owner && cvars->GetFloat("ftebroom_context_brush_entity")
      && ImGui::MenuItem(
        "Move brushes to this entity",
        nullptr,
        false,
        cvars->GetFloat("ftebroom_selection_count") > 0))
      command(("ftebroom_entity_move_to " + std::to_string(owner) + "\n").c_str());
    if (*material && ImGui::MenuItem("Reveal material in browser"))
    {
      cvars->SetString("ftebroom_reveal_material", material);
      selectMaterialTab = true;
    }
    ImGui::Separator();
    for (int type = 0; type < 2; type++)
      if (
        ImGui::BeginMenu(
          type ? "Create brush entity" : "Create point entity",
          !type || cvars->GetFloat("ftebroom_selection_count") > 0))
      {
        std::map<std::string, std::vector<std::string>> categories;
        for (const auto& item : entityDefinitions)
          if (item.first != "worldspawn" && item.second.brush == (type != 0))
            categories[item.first.substr(0, item.first.find('_'))].push_back(item.first);
        for (const auto& category : categories)
          if (ImGui::BeginMenu(category.first.c_str()))
          {
            for (const auto& name : category.second)
            {
              const auto& item = *entityDefinitions.find(name);
              if (ImGui::MenuItem(item.first.c_str()))
              {
                if (type)
                {
                  float position[3]{};
                  entityCreate(item.second, position);
                }
                else
                {
                  char query[512];
                  std::snprintf(
                    query,
                    sizeof query,
                    "ftebroom_entity_drop %s %g %g\n",
                    organizationQuote(item.first).c_str(),
                    sceneContextX,
                    sceneContextY);
                  command(query);
                }
              }
            }
            ImGui::EndMenu();
          }
        ImGui::EndMenu();
      }
    ImGui::EndPopup();
  }
  if (organizationNameRequest)
  {
    organizationNameRequest = false;
    ImGui::OpenPopup("Group name");
  }
  if (uiDialog("Group name"))
  {
    static char name[256] = "Group";
    ImGui::InputText("Name", name, sizeof name);
    if (ImGui::Button(organizationRename ? "Rename" : "Create group"))
    {
      if (organizationRename)
        organize("rename", 0, name);
      else
        command(("ftebroom_group " + organizationQuote(name) + "\n").c_str());
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
}
