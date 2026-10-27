// SPDX-License-Identifier: GPL-2.0-or-later
static bool extraToolsExpanded=false;
static void toolOptionFlow(float width)
{
  const auto right=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x;
  if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+width<=right)ImGui::SameLine();
}
// Put labels above fields in side docks so both remain readable at narrow widths.
static const char* toolOptionLabel(bool horizontal,const char* label)
{
  if(horizontal)return label;
  static std::string id;
  if(std::strncmp(label,"##",2))ImGui::TextWrapped("%s",label);
  id=std::string("##")+label;
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  return id.c_str();
}
static void shapeOptions(bool horizontal)
{
  const char* ids[] = {
    "cuboid", "cylinder", "cone", "uvsphere", "icosphere", "stairs", "arch"};
  char current[64];
  cvars->GetString("ftebroom_shape", current, sizeof current);
  int shape = 0;
  for (int i = 0; i < 7; ++i)
    if (!std::strcmp(current, ids[i]))
      shape = i;
  if(!horizontal)ImGui::TextUnformatted("Shape");
  ImGui::SetNextItemWidth(horizontal?130*uiScale():ImGui::GetContentRegionAvail().x);
  if (
    ImGui::Combo(
      horizontal?"Shape":"##Shape", &shape, "Cuboid\0Cylinder\0Cone\0UV sphere\0Icosahedron\0Stairs\0Arch\0"))
    cvars->SetString("ftebroom_shape", ids[shape]);
  if(horizontal)toolOptionFlow(ImGui::CalcTextSize("Shape settings",nullptr,true).x+2*ImGui::GetStyle().FramePadding.x);
  if (ImGui::Button("Shape settings", {horizontal?0.f:ImGui::GetContentRegionAvail().x,0}))
    ImGui::OpenPopup("Shape parameters");
  if (ImGui::BeginPopup("Shape parameters"))
  {
    int axis = (int)cvars->GetFloat("ftebroom_shape_axis");
    if (ImGui::Combo("Axis", &axis, "X\0Y\0Z\0"))
      cvars->SetFloat("ftebroom_shape_axis", axis);
    int sides = (int)cvars->GetFloat("ftebroom_shape_sides");
    if (ImGui::InputInt("Sides", &sides))
      cvars->SetFloat("ftebroom_shape_sides", std::clamp(sides, 3, 96));
    char mode[32];
    cvars->GetString("ftebroom_shape_circle", mode, sizeof mode);
    int circle =
      !std::strcmp(mode, "scalable") ? 2 : (!std::strcmp(mode, "vertex") ? 1 : 0);
    if (ImGui::Combo("Circle", &circle, "Edge aligned\0Vertex aligned\0Scalable (CZG)\0"))
      cvars->SetString(
        "ftebroom_shape_circle",
        circle == 2 ? "scalable" : (circle == 1 ? "vertex" : "edge"));
    bool hollow = cvars->GetFloat("ftebroom_shape_hollow") != 0;
    if (ImGui::Checkbox("Hollow cylinder", &hollow))
      cvars->SetFloat("ftebroom_shape_hollow", hollow);
    float thickness = cvars->GetFloat("ftebroom_shape_thickness");
    if (ImGui::InputFloat("Wall thickness", &thickness))
      cvars->SetFloat("ftebroom_shape_thickness", std::max(.125f, thickness));
    int rings = (int)cvars->GetFloat("ftebroom_shape_rings");
    if (ImGui::InputInt("Sphere rings", &rings))
      cvars->SetFloat("ftebroom_shape_rings", std::clamp(rings, 2, 32));
    int accuracy = (int)cvars->GetFloat("ftebroom_shape_accuracy");
    if (ImGui::InputInt("Icosahedron accuracy", &accuracy))
      cvars->SetFloat("ftebroom_shape_accuracy", std::clamp(accuracy, 0, 3));
    ImGui::TextWrapped(
      "Brushes support up to 64 faces. Icosahedron accuracy 0 has 20 faces; higher "
      "levels are rejected.");
    float step = cvars->GetFloat("ftebroom_shape_step");
    if (ImGui::InputFloat("Step height", &step))
      cvars->SetFloat("ftebroom_shape_step", std::max(1.f, step));
    int direction = (int)cvars->GetFloat("ftebroom_shape_stairdir");
    if (ImGui::Combo("Stair direction", &direction, "+X\0-X\0+Y\0-Y\0"))
      cvars->SetFloat("ftebroom_shape_stairdir", direction);
    bool spandrel = cvars->GetFloat("ftebroom_shape_spandrel") != 0;
    if (ImGui::Checkbox("Arch spandrels", &spandrel))
      cvars->SetFloat("ftebroom_shape_spandrel", spandrel);
    bool group = cvars->GetFloat("ftebroom_shape_group") != 0;
    if (ImGui::Checkbox("Group multiple parts", &group))
      cvars->SetFloat("ftebroom_shape_group", group);
    ImGui::EndPopup();
  }
  if(horizontal)toolOptionFlow(ImGui::CalcTextSize("Replace selection",nullptr,true).x+2*ImGui::GetStyle().FramePadding.x);
  ImGui::BeginDisabled(cvars->GetFloat("ftebroom_selection_count") < 1);
  if (ImGui::Button("Replace selection", {horizontal?0.f:ImGui::GetContentRegionAvail().x,0}))
    command("ftebroom_shape_apply\n");
  ImGui::EndDisabled();
}
