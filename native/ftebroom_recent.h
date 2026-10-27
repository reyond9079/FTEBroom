// SPDX-License-Identifier: GPL-2.0-or-later
struct RecentMap { std::string path; int profile=-1; bool favorite=false; };
static std::vector<RecentMap> recentMaps;
static bool recentLoaded=false,recentWindow=false,recentFavorites=false;
static const char* recentProfileName(int profile) {
  return profile==0?"Quake III Arena":profile==1?"FTE Custom game":profile==2?"Quake 1":"Unknown profile";
}
static void recentLoad() {
  if(recentLoaded)return;recentLoaded=true;
  std::ifstream in(settingsPath().parent_path()/"ftebroom-recent-maps.txt");
  RecentMap item;while(in>>item.profile>>item.favorite>>std::quoted(item.path))
    if(!item.path.empty()&&recentMaps.size()<1000)recentMaps.push_back(item);
}
static void recentSave() {
  auto path=settingsPath().parent_path()/"ftebroom-recent-maps.txt";
  std::ofstream out(path,std::ios::trunc);for(const auto& item:recentMaps)out<<item.profile<<' '<<item.favorite<<' '<<std::quoted(item.path)<<'\n';
}
static void recentRemember(const std::filesystem::path& path,int profile) {
  if(path.empty())return;recentLoad();auto name=path.lexically_normal().u8string();bool favorite=false;
  for(auto it=recentMaps.begin();it!=recentMaps.end();)if(it->path==name){favorite=it->favorite;it=recentMaps.erase(it);}else ++it;
  recentMaps.insert(recentMaps.begin(),{name,profile,favorite});
  // Favorites survive ordinary history pruning.
  int ordinary=0;for(auto it=recentMaps.begin();it!=recentMaps.end();)if(!it->favorite&&++ordinary>100)it=recentMaps.erase(it);else ++it;
  recentSave();
}
static void documentRequest(const std::string& next);
static bool recentStar(const char* id,bool& value) {
  const auto pos=ImGui::GetCursorScreenPos();const float size=ImGui::GetFrameHeight();
  bool changed=ImGui::Button(id,{size,size});if(changed)value=!value;
  auto draw=ImGui::GetWindowDrawList();ImVec2 center{pos.x+size*.5f,pos.y+size*.5f},points[10];
  for(int i=0;i<10;++i){float a=-1.570796327f+i*.628318531f,r=size*(i%2?.16f:.36f);points[i]={center.x+std::cos(a)*r,center.y+std::sin(a)*r};}
  auto color=value?IM_COL32(244,195,67,255):ImGui::GetColorU32(ImGuiCol_TextDisabled);
  if(value)draw->AddConcavePolyFilled(points,10,color);
  draw->AddPolyline(points,10,color,ImDrawFlags_Closed,1.5f);
  return changed;
}
static void recentPanel() {
  if(firstSetupStep>=0){recentWindow=false;return;}
  if(!recentWindow)return;recentLoad();ImGui::SetNextWindowSize({760,520},ImGuiCond_FirstUseEver);
  if(ImGui::Begin("Welcome to ReyondBroom",&recentWindow)) {
    ImGui::TextUnformatted("Recently edited maps");recentStar("##favorites_only",recentFavorites);if(ImGui::IsItemHovered())ImGui::SetTooltip("Show favorites only");ImGui::SameLine();ImGui::AlignTextToFramePadding();ImGui::TextUnformatted("Favorites only");
    ImGui::BeginChild("recent-list",{0,-ImGui::GetFrameHeightWithSpacing()*2},ImGuiChildFlags_Borders);
    std::string open;
    if(ImGui::BeginTable("recent-map-rows",3,ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_NoSavedSettings)) {
    ImGui::TableSetupColumn("Favorite",ImGuiTableColumnFlags_WidthFixed,ImGui::GetFrameHeight());
    ImGui::TableSetupColumn("Map",ImGuiTableColumnFlags_WidthStretch,1.f);
    ImGui::TableSetupColumn("Game profile",ImGuiTableColumnFlags_WidthFixed,ImGui::CalcTextSize("FTE Custom game (missing)",nullptr,true).x);
    for(size_t i=0;i<recentMaps.size();++i) {
      auto& item=recentMaps[i];if(recentFavorites&&!item.favorite)continue;ImGui::PushID(int(i));
      ImGui::TableNextRow();ImGui::TableSetColumnIndex(0);
      if(recentStar("##favorite",item.favorite))recentSave();if(ImGui::IsItemHovered())ImGui::SetTooltip(item.favorite?"Remove from favorites":"Add to favorites");
      ImGui::TableSetColumnIndex(1);
      auto path=std::filesystem::u8path(item.path);bool exists=ftebroom::pak::exists(path);
      ImGui::BeginDisabled(!exists);
      ImGui::AlignTextToFramePadding();
      if(ImGui::Selectable(path.filename().u8string().c_str(),false,ImGuiSelectableFlags_None,{0,ImGui::GetFrameHeight()})){open=item.path;}
      if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))ImGui::SetTooltip("%s",item.path.c_str());ImGui::EndDisabled();
      ImGui::TableSetColumnIndex(2);ImGui::AlignTextToFramePadding();ImGui::TextDisabled("%s%s",recentProfileName(item.profile),exists?"":" (missing)");ImGui::PopID();
    }
    ImGui::EndTable();}
    if(recentMaps.empty())ImGui::TextDisabled("Opened and saved maps will appear here.");
    ImGui::EndChild();
    if(!open.empty()) {recentWindow=false;documentRequest("@"+open);}
    if(ImGui::Button("Open another map...",{-1,0})){recentWindow=false;openRequest=true;}
    if(ImGui::Button("Create new map...",{-1,0})){recentWindow=false;documentRequest("__new");}
  }ImGui::End();
}
