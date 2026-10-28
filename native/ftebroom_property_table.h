// SPDX-License-Identifier: GPL-2.0-or-later
struct PropertyRow
{
  std::string key, value;
  bool explicitValue = false, complete = true, mixed = false, internal = false;
};
static std::set<std::string> protectedPropertyKeys(const std::map<std::string,std::string>& fields)
{
  std::set<std::string> result;
  auto found=fields.find("_tb_protected_properties");
  if(found==fields.end())return result;
  std::string key;bool escaped=false;
  for(char c:found->second)
  {
    if(escaped){key+=c;escaped=false;}
    else if(c=='\\')escaped=true;
    else if(c==';'){if(!key.empty())result.insert(key);key.clear();}
    else key+=c;
  }
  if(!key.empty())result.insert(key);
  return result;
}
static void entityPropertyTable()
{
  const bool linked=entitySelectionLinked;
  static bool showDefaults = true;
  ImGui::Checkbox("Show default properties", &showDefaults);
  static std::map<std::string, ftebroom::EntityProperty> definitions;
  static std::set<std::string> keys;
  static std::vector<PropertyRow> rows;
  static std::vector<int> now;
  static std::map<std::string, unsigned> protectedCounts;
  static int selectionEpoch=-2, definitionsEpoch=-2, mapEpoch=-2;
  static bool defaults=false;
  // Aggregation depends on the published data, not the frame rate or camera.
  if(selectionEpoch!=entitySelectionEpoch || definitionsEpoch!=entityDefinitionEpoch || mapEpoch!=organizationMapEpoch || defaults!=showDefaults)
  {
    selectionEpoch=entitySelectionEpoch;definitionsEpoch=entityDefinitionEpoch;mapEpoch=organizationMapEpoch;defaults=showDefaults;
    definitions.clear();keys.clear();rows.clear();now.clear();protectedCounts.clear();
    std::set<std::string> classes;
  for (const auto& entity : entityProperties)
  {
    now.push_back(entity.first);
    auto cls = entity.second.find("classname");
    if (cls != entity.second.end() && classes.insert(cls->second).second)
      if (auto def = entityDefinitions.find(cls->second); def != entityDefinitions.end())
        for (const auto& p : def->second.properties)
        {
          definitions.emplace(p);
          keys.insert(p.first);
        }
    for (const auto& p : entity.second)
      keys.insert(p.first);
    for(const auto& key:protectedPropertyKeys(entity.second)){keys.insert(key);protectedCounts[key]++;}
  }
  for (const auto& key : keys)
  {
    if (key.rfind("_tb_", 0) == 0 || key.rfind("_ftebroom_",0)==0)
      continue;
    PropertyRow row;
    row.key = key;
    bool first = true;
    for (const auto& entity : entityProperties)
    {
      auto found = entity.second.find(key);
      row.explicitValue |= found != entity.second.end();
      row.complete &= found != entity.second.end();
      std::string value;
      if (found != entity.second.end())
        value = found->second;
      else if (auto cls = entity.second.find("classname"); cls != entity.second.end())
        if (
          auto def = entityDefinitions.find(cls->second); def != entityDefinitions.end())
          if (
            auto p = def->second.properties.find(key); p != def->second.properties.end())
            value = p->second.value;
      if (first)
      {
        row.value = value;
        first = false;
      }
      else
        row.mixed |= row.value != value;
      row.internal |= key == "model" && !value.empty() && value[0] == '*';
    }
    if (row.explicitValue || showDefaults)
      rows.push_back(row);
  }
  std::stable_sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
    return a.explicitValue > b.explicitValue;
  });
  }
  static std::string selected, editing, pending;
  static int column = 0, editColumn = 0, pendingColumn = 0;
  static std::set<std::string> selectedRows;
  static std::array<char, 4096> buffer{};
  static bool focusEdit = false, editDirty = false;
  static std::vector<int> targets;
  static int targetMapEpoch = -1, targetSerial = 0;
  if (targets != now || targetMapEpoch != organizationMapEpoch)
  {
    if (!editing.empty())
      ImGui::ClearActiveID();
    targets = now;
    targetMapEpoch = organizationMapEpoch;
    ++targetSerial;
    selected.clear();
    selectedRows.clear();
    editing.clear();
    pending.clear();
  }
  auto startEdit = [&](const PropertyRow& row, int col) {
    if (row.internal || (col == 0 && row.key == "classname"))
      return;
    selected = editing = row.key;
    column = editColumn = col;
    focusEdit = true;
    editDirty = false;
    std::snprintf(
      buffer.data(),
      buffer.size(),
      "%s",
      col == 0 ? row.key.c_str() : (row.mixed ? "" : row.value.c_str()));
  };
  auto indexOf = [&](const std::string& key) {
    for (int i = 0; i < (int)rows.size(); ++i)
      if (rows[i].key == key)
        return i;
    return -1;
  };
  if (!pending.empty())
    if (int i = indexOf(pending); i >= 0)
    {
      startEdit(rows[i], pendingColumn);
      pending.clear();
    }
  entityPropertyFocused =
    !selected.empty() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
  auto removeSelected = [&]() {
    std::string list;
    for (const auto& key : selectedRows)
    {
      int i = indexOf(key);
      if (i >= 0 && key != "classname" && !rows[i].internal)
        list += organizationQuote(key) + " ";
    }
    if (!list.empty())
      command(("ftebroom_entity_remove " + list + "\n").c_str());
  };
  ImGui::BeginChild(
    "Property list",
    {0, 260 * uiScale()},
    ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY);
  if (
    editing.empty() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
    && !ImGui::IsAnyItemActive())
  {
    int i = indexOf(selected), next = i, nextColumn = column;
    if (
      ImGui::IsKeyPressed(ImGuiKey_DownArrow)
      || (ImGui::IsKeyPressed(ImGuiKey_Enter) && !ImGui::GetIO().KeyCtrl))
      ++next;
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
      --next;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
      nextColumn = 0;
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
      nextColumn = 1;
    if (ImGui::IsKeyPressed(ImGuiKey_Tab))
    {
      int cell = std::max(0, i) * 2 + column + (ImGui::GetIO().KeyShift ? -1 : 1);
      cell = std::clamp(cell, 0, std::max(0, (int)rows.size() * 2 - 1));
      next = cell / 2;
      nextColumn = cell % 2;
    }
    if (!rows.empty() && (next != i || nextColumn != column))
    {
      next = std::clamp(next, 0, (int)rows.size() - 1);
      selected = rows[next].key;
      column = nextColumn;
      if (!ImGui::GetIO().KeyShift)
        selectedRows.clear();
      selectedRows.insert(selected);
    }
    if (i >= 0 && ImGui::IsKeyPressed(ImGuiKey_Enter))
      startEdit(rows[i], column);
    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
      removeSelected();
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))
      for (const auto& row : rows)
        selectedRows.insert(row.key);
  }
  if (
    ImGui::BeginTable(
      "Properties table",
      linked ? 3 : 2,
      ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
  {
    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch, .4f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, .6f);
    if(linked)ImGui::TableSetupColumn("Protected",ImGuiTableColumnFlags_WidthFixed,80*uiScale());
    ImGui::TableHeadersRow();
    for (int i = 0; i < (int)rows.size(); ++i)
    {
      const auto& row = rows[i];
      ImGui::PushID(row.key.c_str());
      ImGui::TableNextRow();
      if (!row.complete)
        ImGui::PushStyleColor(
          ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
      if (!row.explicitValue && uiItalicFonts[uiScaleChoice])
        ImGui::PushFont(uiItalicFonts[uiScaleChoice]);
      for (int col = 0; col < 2; ++col)
      {
        ImGui::TableSetColumnIndex(col);
        ImGui::PushID(col);
        if (editing == row.key && editColumn == col)
        {
          const bool startingEdit = focusEdit;
          if (focusEdit)
          {
            ImGui::SetKeyboardFocusHere();
            focusEdit = false;
          }
          ImGui::SetNextItemWidth(-1);
          const ImGuiID editId = ImGui::GetID("##edit");
          bool enter = ImGui::InputText(
            "##edit", buffer.data(), buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue);
          entityTestItem((col ? "value/" : "key/") + row.key);
          editDirty |= ImGui::IsItemEdited();
          bool active = ImGui::IsItemActive() || ImGui::IsItemDeactivated();
          bool tab = active && ImGui::IsKeyPressed(ImGuiKey_Tab);
          bool escape = active && ImGui::IsKeyPressed(ImGuiKey_Escape);
          bool done = (!startingEdit && ImGui::IsItemDeactivated()) || enter || tab;
          if (escape)
          {
            if (ImGui::GetActiveID() == editId)
              ImGui::ClearActiveID();
            editing.clear();
          }
          else if (done)
          {
            std::string changed = buffer.data();
            if (col == 0 && changed != row.key)
              entityProperty("rename", row.key, changed);
            if (col == 1 && editDirty && (row.mixed || changed != row.value))
              entityProperty("set", row.key, changed);
            editing.clear();
            // A click on another cell may already own the active ID. Do not
            // clear that ID while committing the field that just lost focus.
            if (ImGui::GetActiveID() == editId)
              ImGui::ClearActiveID();
            if (tab || enter)
            {
              int cell = i * 2 + col + (tab ? (ImGui::GetIO().KeyShift ? -1 : 1) : 2);
              if (cell >= 0 && cell < (int)rows.size() * 2)
              {
                pending = cell / 2 == i && col == 0 ? changed : rows[cell / 2].key;
                pendingColumn = cell % 2;
              }
            }
          }
        }
        else
        {
          std::string label = col == 0 ? row.key : (row.mixed ? "" : row.value);
          label += "##cell";
          if (
            ImGui::Selectable(
              label.c_str(),
              selectedRows.count(row.key) && (selected != row.key || column == col),
              ImGuiSelectableFlags_AllowDoubleClick))
          {
            bool again = selected == row.key && column == col;
            if (ImGui::GetIO().KeyShift && indexOf(selected) >= 0)
            {
              int a = indexOf(selected);
              for (int j = std::min(a, i); j <= std::max(a, i); ++j)
                selectedRows.insert(rows[j].key);
            }
            else if (ImGui::GetIO().KeyCtrl)
            {
              if (!selectedRows.erase(row.key))
                selectedRows.insert(row.key);
            }
            else
            {
              selectedRows.clear();
              selectedRows.insert(row.key);
            }
            selected = row.key;
            column = col;
            if (again && !ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
              startEdit(row, col);
          }
          entityTestItem((col ? "value/" : "key/") + row.key);
        }
        if (ImGui::IsItemHovered())
          if (auto def = definitions.find(row.key); def != definitions.end())
            ImGui::SetTooltip("%s", def->second.description.c_str());
        ImGui::PopID();
      }
      if (!row.explicitValue && uiItalicFonts[uiScaleChoice])
        ImGui::PopFont();
      if (!row.complete)
        ImGui::PopStyleColor();
      if(linked)
      {
        ImGui::TableSetColumnIndex(2);
        unsigned protectedCount=protectedCounts[row.key];
        bool value=protectedCount!=0,mixed=value && protectedCount!=entityProperties.size();
        ImGui::BeginDisabled(row.internal || row.key=="classname" || row.key=="model");
        if(mixed)ImGui::PushItemFlag(ImGuiItemFlags_MixedValue,true);
        if(ImGui::Checkbox("##protected",&value))
          command((std::string("ftebroom_protect ")+(value?"on ":"off ")+organizationQuote(row.key)+"\n").c_str());
        if(mixed)ImGui::PopItemFlag();
        ImGui::EndDisabled();
      }
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  ImGui::EndChild();
  cvars->SetString(
    "ftebroom_property_cell", (selected + "/" + std::to_string(column)).c_str());
  cvars->SetString("ftebroom_property_editing", editing.c_str());
  if (
    ImGui::Button("+ New property")
    || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Enter, false)))
  {
    int n = 1;
    while (keys.count("property " + std::to_string(n)))
      ++n;
    pending = "property " + std::to_string(n);
    pendingColumn = 0;
    entityProperty("set", pending, "");
  }
  ImGui::SameLine();
  ImGui::BeginDisabled(selectedRows.empty());
  if (ImGui::Button("- Remove"))
    removeSelected();
  ImGui::EndDisabled();
  if(linked)
  {
    ImGui::SameLine();
    if(ImGui::Button("+ Protected"))
    {
      int n=1;while(keys.count("property "+std::to_string(n)))++n;
      pending="property "+std::to_string(n);pendingColumn=0;
      command(("ftebroom_protect add "+organizationQuote(pending)+"\n").c_str());
    }
  }
  if (ImGui::BeginMenu("Set default properties"))
  {
    for (const char* mode : {"existing", "missing", "all"})
      if (ImGui::MenuItem(mode))
        command((std::string("ftebroom_entity_defaults ") + mode + "\n").c_str());
    ImGui::EndMenu();
  }
  int selectedIndex = indexOf(selected);
  if (selectedIndex < 0)
    return;
  const auto& row = rows[selectedIndex];
  auto definition = definitions.find(selected);
  ImGui::SeparatorText(selected.c_str());
  if (definition != definitions.end() && definition->second.type == "flags")
  {
    for (const auto& choice : definition->second.choices)
    {
      unsigned bit;
      try
      {
        bit = std::stoul(choice.value);
      }
      catch (...)
      {
        continue;
      }
      bool enabled = false, first = true, mixed = false;
      for (const auto& e : entityProperties)
      {
        std::string value;
        if (auto p = e.second.find(selected); p != e.second.end())
          value = p->second;
        else if (auto cls = e.second.find("classname"); cls != e.second.end())
          if (
            auto def = entityDefinitions.find(cls->second);
            def != entityDefinitions.end())
            if (
              auto p = def->second.properties.find(selected);
              p != def->second.properties.end())
              value = p->second.value;
        unsigned flags = 0;
        try
        {
          flags = std::stoul(value);
        }
        catch (...)
        {
        }
        bool current = (flags & bit) != 0;
        if (first)
        {
          enabled = current;
          first = false;
        }
        else
          mixed |= current != enabled;
      }
      if (mixed)
        ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
      bool changed = ImGui::Checkbox(choice.label.c_str(), &enabled);
      if (mixed)
        ImGui::PopItemFlag();
      if (changed)
        entityProperty(enabled ? "setflag" : "clearflag", selected, std::to_string(bit));
    }
  }
  else if (definition != definitions.end() && !definition->second.choices.empty())
  {
    if (ImGui::BeginCombo("Choose", row.mixed ? "<multiple values>" : row.value.c_str()))
    {
      for (const auto& choice : definition->second.choices)
        if (ImGui::Selectable(choice.label.c_str(), choice.value == row.value))
          entityProperty("set", selected, choice.value);
      ImGui::EndCombo();
    }
  }
  else if (selected == "_color" || selected == "color" || (definition != definitions.end() && (definition->second.type == "color" || definition->second.type == "color255")))
  {
    static std::string colorKey;
    static int colorTargetSerial = -1;
    static bool bytes = false, active = false;
    static float rgb[3] = {1, 1, 1};
    if (colorKey != selected || colorTargetSerial != targetSerial)
    {
      colorKey = selected;
      colorTargetSerial = targetSerial;
      bytes = definition != definitions.end() && definition->second.type == "color255";
      active = false;
    }
    if (!active)
    {
      rgb[0] = rgb[1] = rgb[2] = 1;
      std::sscanf(row.value.c_str(), "%f %f %f", &rgb[0], &rgb[1], &rgb[2]);
      if (bytes)
        for (float& v : rgb)
          v /= 255;
    }
    auto commit = [&]() {
      std::ostringstream value;
      float factor = bytes ? 255.f : 1.f;
      value << rgb[0] * factor << ' ' << rgb[1] * factor << ' ' << rgb[2] * factor;
      entityProperty("set", selected, value.str());
    };
    if (ImGui::Checkbox("Store bytes (0-255)", &bytes))
      commit();
    bool changed = ImGui::ColorEdit3(
      "Color", rgb, bytes ? ImGuiColorEditFlags_Uint8 : ImGuiColorEditFlags_Float);
    if (changed)
      active = true;
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
      commit();
      active = false;
    }
    ImGui::TextDisabled("Colors used in this map");
    int n = 0;
    for (const auto& color : entityMapColors)
    {
      float c[3] = {1, 1, 1};
      if (std::sscanf(color.c_str(), "%f %f %f", &c[0], &c[1], &c[2]) != 3)
        continue;
      if (std::max({c[0], c[1], c[2]}) > 1)
        for (float& v : c)
          v /= 255;
      ImGui::PushID(n++);
      if (ImGui::ColorButton("##mapcolor", {c[0], c[1], c[2], 1}))
      {
        std::copy(c, c + 3, rgb);
        commit();
      }
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", color.c_str());
      ImGui::PopID();
      if (n % 8)
        ImGui::SameLine();
    }
    if (n % 8)
      ImGui::NewLine();
  }
}
