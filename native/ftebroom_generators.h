// SPDX-License-Identifier: GPL-3.0-or-later
static int generatorWindow=0;
static bool generatorPreview=false;
static double generatorChangedAt=0;
struct GeneratorSettings {
  std::array<int,20> ints{10,10,9,9,0,1337,3,0,10,1337,6,2,2,0,3,6,2,1337,8,1337};
  std::array<float,17> floats{128,1,4,1,8,.5f,8,0,1,32,16,16,32,4,0,1,0};
  std::array<bool,12> bools{true,true,false,false,true,false,false,true,true,false,false,false};
  std::array<float,3> delta{0,0,128},rotation{};
  std::array<char,256> material{};
  bool operator==(const GeneratorSettings& other) const{return ints==other.ints&&floats==other.floats&&bools==other.bools&&delta==other.delta&&rotation==other.rotation&&material==other.material;}
};
struct GeneratorHistory {GeneratorSettings state;std::vector<GeneratorSettings> steps;size_t cursor=0;};
static std::array<GeneratorHistory,9> generatorHistories;
static void generatorRecord(GeneratorHistory& h){
  if(!h.steps.empty()&&h.state==h.steps[h.cursor])return;
  if(!h.steps.empty())h.steps.resize(h.cursor+1);
  h.steps.push_back(h.state);if(h.steps.size()>128)h.steps.erase(h.steps.begin());h.cursor=h.steps.size()-1;
}
static bool generatorHistoryStep(bool redo){
  if(generatorWindow<1||generatorWindow>8)return false;
  auto& h=generatorHistories[generatorWindow];generatorRecord(h);
  if(redo?h.cursor+1>=h.steps.size():h.cursor==0)return false;
  if(redo)++h.cursor;else --h.cursor;h.state=h.steps[h.cursor];
  if(generatorPreview)generatorChangedAt=ImGui::GetTime();return true;
}
static void generatorClose(){command("ftebroom_generator cancel\n");generatorPreview=false;generatorWindow=0;generatorChangedAt=0;}
static void generatorPanel() {
  if(!generatorWindow)return;
  auto& settings=generatorHistories[generatorWindow];if(settings.steps.empty())generatorRecord(settings);auto& state=settings.state;
  bool open=true,changed=false;const char* kind=generatorWindow==8?"rock":generatorWindow==7?"spin":generatorWindow==6?"subdivision":generatorWindow==5?"inset":generatorWindow==1?"terrain":generatorWindow==3?"fracture":generatorWindow==4?"perforate":"bevel";
  ImGui::SetNextWindowSize({460*uiScale(),0},ImGuiCond_FirstUseEver);
  if(ImGui::Begin(generatorWindow==8?"Rock":generatorWindow==7?"Spin":generatorWindow==6?"Subdivision":generatorWindow==5?"Inset":generatorWindow==1?"Terrain generator":generatorWindow==3?"Fracture":generatorWindow==4?"Perforate":"Bevel",&open)) {
    ImGui::BeginDisabled(settings.cursor==0);if(ImGui::Button("Undo settings"))changed|=generatorHistoryStep(false);ImGui::EndDisabled();
    ImGui::SameLine();ImGui::BeginDisabled(settings.cursor+1>=settings.steps.size());if(ImGui::Button("Redo settings"))changed|=generatorHistoryStep(true);ImGui::EndDisabled();
    ImGui::SameLine();if(ImGui::Button("Default")){state=GeneratorSettings{};changed=true;}
    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)&&!ImGui::GetIO().WantTextInput&&ImGui::IsKeyPressed(ImGuiKey_Escape))open=false;
    auto& u=state.ints[0]; auto& v=state.ints[1]; auto& pu=state.ints[2]; auto& pv=state.ints[3]; auto& side=state.ints[4]; auto& seed=state.ints[5]; auto& segments=state.ints[6]; auto& mode=state.ints[7];
    auto& height=state.floats[0]; auto& intensity=state.floats[1]; auto& frequency=state.floats[2]; auto& roughness=state.floats[3]; auto& radius=state.floats[4]; auto& profile=state.floats[5];
    auto& triangles=state.bools[0]; auto& clamp=state.bools[1];
    if(generatorWindow==1) {
      changed|=ImGui::SliderInt("Segments U",&u,1,32);changed|=ImGui::SliderInt("Segments V",&v,1,32);
      changed|=ImGui::DragFloat("Height",&height,1,0,8192);changed|=ImGui::SliderFloat("Intensity",&intensity,0,4);
      changed|=ImGui::SliderFloat("Frequency",&frequency,.01f,64);changed|=ImGui::SliderFloat("Roughness",&roughness,0,2);
      changed|=ImGui::InputInt("Seed",&seed);changed|=ImGui::Combo("Side",&side,"+Z (top)\0-Z (bottom)\0+X\0-X\0+Y\0-Y\0");
      changed|=ImGui::Checkbox("Triangular grid",&triangles);
      if(!gameClassic()) {
        ImGui::SeparatorText("Selected patches");
        if(ImGui::SliderInt("Control points U",&pu,3,31)){pu|=1;changed=true;}
        if(ImGui::SliderInt("Control points V",&pv,3,31)){pv|=1;changed=true;}
        ImGui::TextWrapped("Odd point counts only. Reducing the grid approximates the surface.");
      }
      cvars->SetFloat("ftebroom_land_u",u);cvars->SetFloat("ftebroom_land_v",v);cvars->SetFloat("ftebroom_land_patch_u",pu);cvars->SetFloat("ftebroom_land_patch_v",pv);
      cvars->SetFloat("ftebroom_land_height",height);cvars->SetFloat("ftebroom_land_intensity",intensity);cvars->SetFloat("ftebroom_land_frequency",frequency);cvars->SetFloat("ftebroom_land_roughness",roughness);cvars->SetFloat("ftebroom_land_seed",seed);cvars->SetFloat("ftebroom_land_side",side);cvars->SetFloat("ftebroom_land_triangles",triangles);
    }else if(generatorWindow==8) {
      auto& detail=state.ints[8]; auto& rockSeed=state.ints[9];auto& hollow=state.bools[2]; auto& group=state.bools[3];auto& thickness=state.floats[6];
      changed|=ImGui::SliderInt("Segments",&detail,4,48);changed|=ImGui::InputInt("Seed",&rockSeed);
      if(ImGui::Button("Another variant")){rockSeed++;changed=true;}
      changed|=ImGui::Checkbox("Hollow shell",&hollow);if(hollow)changed|=ImGui::DragFloat("Shell thickness",&thickness,.25f,.001f,8192);
      changed|=ImGui::Checkbox("Group generated rocks",&group);
      ImGui::TextWrapped("Uses the selected brush bounds to shape seeded rocks. Detailed solid rocks are partitioned into convex pieces to respect the 64-plane limit. Up to 4096 samples and 2048 pieces.");
      cvars->SetFloat("ftebroom_rock_segments",detail);cvars->SetFloat("ftebroom_rock_seed",rockSeed);cvars->SetFloat("ftebroom_rock_hollow",hollow);cvars->SetFloat("ftebroom_rock_thickness",thickness);cvars->SetFloat("ftebroom_rock_group",group);
    }else if(generatorWindow==7) {
      auto& count=state.ints[10];auto& flex=state.floats[7]; auto& scale=state.floats[8]; auto* delta=state.delta.data();auto* rotation=state.rotation.data();auto& automatic=state.bools[4];
      changed|=ImGui::SliderInt("Segments",&count,1,64);changed|=ImGui::SliderFloat("Flexibility",&flex,-1,1);changed|=ImGui::SliderFloat("End scale",&scale,.05f,8);
      changed|=ImGui::Checkbox("Endpoint along face normal",&automatic);
      if(!automatic)changed|=ImGui::InputFloat3("Endpoint offset",delta);
      changed|=ImGui::InputFloat3("End rotation (degrees)",rotation);
      ImGui::TextWrapped("Shift-click faces. Extrusion retains the original brushes. Adjust the endpoint, rotation and scale, then Preview or Apply.");
      cvars->SetFloat("ftebroom_spin_segments",count);cvars->SetFloat("ftebroom_spin_flex",flex);cvars->SetFloat("ftebroom_spin_scale",scale);cvars->SetFloat("ftebroom_spin_auto",automatic);
      for(int a=0;a<3;a++){cvars->SetFloat(a==0?"ftebroom_spin_x":a==1?"ftebroom_spin_y":"ftebroom_spin_z",delta[a]);cvars->SetFloat(a==0?"ftebroom_spin_rx":a==1?"ftebroom_spin_ry":"ftebroom_spin_rz",rotation[a]);}
    }else if(generatorWindow==6) {
      auto& su=state.ints[11]; auto& sv=state.ints[12]; auto& plane=state.ints[13];auto& texture=state.bools[5]; auto& grid=state.bools[6];auto& cell=state.floats[9];
      changed|=ImGui::Checkbox("Use grid cell size",&grid);
      if(grid)changed|=ImGui::InputFloat("Cell size",&cell,.125f,8);else{changed|=ImGui::SliderInt("Segments U",&su,1,64);changed|=ImGui::SliderInt("Segments V",&sv,1,64);}
      changed|=ImGui::Checkbox("Align to face texture",&texture);
      if(!texture)changed|=ImGui::Combo("Plane",&plane,"XY\0XZ\0YZ\0");
      ImGui::TextWrapped("Splits whole brush volumes. Selected faces orient segment cuts. World cells follow grid rotation; texture cells use texels and the face texture pivot. Without face selection, the plane chooses the reference face. Maximum 512 pieces.");
      cvars->SetFloat("ftebroom_subdivide_u",su);cvars->SetFloat("ftebroom_subdivide_v",sv);cvars->SetFloat("ftebroom_subdivide_plane",plane);cvars->SetFloat("ftebroom_subdivide_texture",texture);cvars->SetFloat("ftebroom_subdivide_grid",grid);cvars->SetFloat("ftebroom_subdivide_cell",cell);
    }else if(generatorWindow==5) {
      auto& amount=state.floats[10];auto& snap=state.bools[7];
      changed|=ImGui::InputFloat("Inset amount",&amount,cvars->GetFloat("ca_grid"),cvars->GetFloat("ca_grid")*4);
      changed|=ImGui::Checkbox("Snap inner contour to grid",&snap);
      ImGui::TextWrapped("Shift-click one face. Creates an inner brush and border pieces through the source brush. Invalid or collapsing contours are rejected.");
      cvars->SetFloat("ftebroom_inset_amount",amount);cvars->SetFloat("ftebroom_inset_snap",snap);
    }else if(generatorWindow==4) {
      auto& holes=state.ints[14]; auto& sides=state.ints[15]; auto& axis=state.ints[16]; auto& holeSeed=state.ints[17];auto& minimum=state.floats[11]; auto& maximum=state.floats[12]; auto& spacing=state.floats[13];auto& keep=state.bools[8];
      changed|=ImGui::SliderInt("Holes",&holes,1,64);changed|=ImGui::SliderInt("Polygon sides",&sides,3,32);
      changed|=ImGui::Combo("Through axis",&axis,"X\0Y\0Z\0");changed|=ImGui::InputInt("Seed",&holeSeed);
      changed|=ImGui::DragFloat("Minimum diameter",&minimum,1,.125f,8192);changed|=ImGui::DragFloat("Maximum diameter",&maximum,1,.125f,8192);
      changed|=ImGui::DragFloat("Spacing",&spacing,1,0,8192);changed|=ImGui::Checkbox("Keep plugs as func_wall",&keep);
      ImGui::TextWrapped("Creates convex polygonal through-holes. Retained plugs are selected after Apply. Parameters that cannot fit fail without changing the brush.");
      ImGui::TextDisabled("Complexity budget: holes x sides <= 128 (larger requests are rejected safely).");
      cvars->SetFloat("ftebroom_hole_count",holes);cvars->SetFloat("ftebroom_hole_sides",sides);cvars->SetFloat("ftebroom_hole_axis",axis);cvars->SetFloat("ftebroom_hole_seed",holeSeed);
      cvars->SetFloat("ftebroom_hole_min",minimum);cvars->SetFloat("ftebroom_hole_max",maximum);cvars->SetFloat("ftebroom_hole_spacing",spacing);cvars->SetFloat("ftebroom_hole_keep",keep);
    }else if(generatorWindow==3) {
      auto& fragments=state.ints[18]; auto& fractureSeed=state.ints[19];auto& separation=state.floats[14];auto& entities=state.bools[9];auto& material=state.material;
      changed|=ImGui::SliderInt("Fragments",&fragments,2,128);changed|=ImGui::InputInt("Seed",&fractureSeed);
      if(ImGui::Button("Another variant")){fractureSeed++;changed=true;}
      changed|=ImGui::SliderFloat("Separation (%)",&separation,0,100);changed|=ImGui::InputText("Cut material",material.data(),material.size());
      changed|=ImGui::Checkbox("No collide (func_wall)",&entities);
      ImGui::TextWrapped("Creates one func_wall per fragment. This does not disable collisions in physics tools. Empty cut material inherits the source material.");
      cvars->SetFloat("ftebroom_fracture_count",fragments);cvars->SetFloat("ftebroom_fracture_seed",fractureSeed);cvars->SetFloat("ftebroom_fracture_separation",separation);cvars->SetFloat("ftebroom_fracture_entities",entities);cvars->SetString("ftebroom_fracture_material",material.data());
    }else {
      changed|=ImGui::Combo("Selection",&mode,"Face boundary / whole brush\0Selected edges (E)\0Selected vertices (V)\0");
      changed|=ImGui::DragFloat("Radius / width",&radius,.25f,.125f,8192);changed|=ImGui::SliderInt("Segments",&segments,1,16);
      auto& extended=state.bools[10];auto& keepCuts=state.bools[11];auto& curve=state.floats[15];auto& proportion=state.floats[16];
      ImGui::BeginDisabled(mode==2);changed|=ImGui::Checkbox("Extended edge profile",&extended);ImGui::EndDisabled();
      if(mode==2&&extended){extended=false;changed=true;}
      if(extended){changed|=ImGui::SliderFloat("Curve",&curve,-1,1);changed|=ImGui::SliderFloat("Proportion",&proportion,-.95f,.95f);ImGui::TextWrapped("Negative curve creates inward rounding as convex pieces. Positive curve rounds outward; 1 uses an arc. Width is measured along the two adjacent faces. Vertex mode uses the original profile.");}
      else changed|=ImGui::SliderFloat("Profile",&profile,0,1);
      changed|=ImGui::Checkbox("Keep cut geometry",&keepCuts);changed|=ImGui::Checkbox("Clamp within each brush",&clamp);
      cvars->SetFloat("ftebroom_bevel_extended",extended);cvars->SetFloat("ftebroom_bevel_keep",keepCuts);cvars->SetFloat("ftebroom_bevel_curve",curve);cvars->SetFloat("ftebroom_bevel_proportion",proportion);
      if(!extended)ImGui::TextWrapped("0: flat chamfer; 0.5: round; 1: square. Results stay convex. Complex bevels must fit the engine's 64-face brush limit.");
      cvars->SetFloat("ftebroom_bevel_radius",radius);cvars->SetFloat("ftebroom_bevel_segments",segments);cvars->SetFloat("ftebroom_bevel_profile",profile);cvars->SetFloat("ftebroom_bevel_clamp",clamp);cvars->SetFloat("ftebroom_bevel_mode",mode);
    }
    if(!ImGui::IsAnyItemActive())generatorRecord(settings);
    if(changed&&generatorPreview)generatorChangedAt=ImGui::GetTime();
    bool refresh=generatorChangedAt>0&&ImGui::GetTime()-generatorChangedAt>.2&&!ImGui::IsAnyItemActive();
    if(ImGui::Button("Preview")||refresh){command((std::string("ftebroom_generator preview ")+kind+"\n").c_str());generatorPreview=true;generatorChangedAt=0;}
    ImGui::SameLine();if(ImGui::Button("Apply")){command((std::string("ftebroom_generator apply ")+kind+"\n").c_str());generatorPreview=false;generatorChangedAt=0;}
    ImGui::SameLine();if(ImGui::Button("Cancel"))open=false;
    char error[1024]={};cvars->GetString("ftebroom_generator_error",error,sizeof error);if(*error)ImGui::TextWrapped("%s",error);
  }ImGui::End();
  if(!open)generatorClose();
}
