// SPDX-License-Identifier: GPL-2.0-or-later
#include "ftebroom_leak_path.h"
static int traceRevisionNow();
static void traceEditingView();
static bool traceWindow=false,traceVisible=true;
static int tracePointIndex=-1,traceRevision=-1;
static uint64_t traceGeneration=0;
static std::vector<ftebroom::leak::Point> tracePoints;
static std::filesystem::path traceDocument;
// Parse external diagnostic files before replacing the current overlay.
static int traceRequest, fileBrowserTrace;
static std::filesystem::path tracePaths[2];
static bool traceChoose(int kind, const std::filesystem::path& path)
{
  try
  {
    std::ifstream input(path);
    if (!input || std::filesystem::file_size(path) > 32 * 1024 * 1024)
      throw std::runtime_error("Cannot read diagnostic file (maximum size: 32 MiB).");
    std::ostringstream result;
    result.precision(9);
    auto point = [&]() {
      double x, y, z;
      if (!(input >> x >> y >> z) || !std::isfinite(x) || !std::isfinite(y)
          || !std::isfinite(z) || std::max({std::abs(x),std::abs(y),std::abs(z)}) > 1e7)
        throw std::runtime_error("Invalid diagnostic point.");
      result << x << ' ' << y << ' ' << z << '\n';
    };
    std::vector<ftebroom::leak::Point> parsed;
    if (kind == 1)
    {
      std::string text((std::istreambuf_iterator<char>(input)),{});parsed=ftebroom::leak::parse(text);
      for(const auto& p:parsed)result<<p[0]<<' '<<p[1]<<' '<<p[2]<<'\n';
    }
    else
    {
      std::string magic;
      int leaves, portals;
      if (!(input >> magic >> leaves >> portals) || magic != "PRT1" || leaves < 0
          || portals < 0 || portals > 262144)
        throw std::runtime_error("Expected a PRT1 portal file.");
      int total = 0;
      for (int i = 0; i < portals; ++i)
      {
        int count, a, b;
        if (!(input >> count >> a >> b) || count < 3 || count > 1024
            || (total += count) > 262144)
          throw std::runtime_error("Invalid portal polygon.");
        result << count << '\n';
        for (int v = 0; v < count; ++v)
        {
          char delimiter;
          if (!(input >> delimiter) || delimiter != '(')
            throw std::runtime_error("Expected portal vertex.");
          point();
          if (!(input >> delimiter) || delimiter != ')')
            throw std::runtime_error("Expected closing portal vertex delimiter.");
        }
      }
    }
    qhandle_t output;
    if(files->Open(kind == 1 ? "data/leak.txt" : "data/portals.txt",&output,2)<0)
      throw std::runtime_error("Cannot stage diagnostic file.");
    auto data=result.str();files->Write(output,data.data(),data.size());files->Close(output);
    tracePaths[kind-1] = path;
    if(kind==1){tracePoints=std::move(parsed);tracePointIndex=-1;traceWindow=traceVisible=true;traceRevision=traceRevisionNow();traceDocument=documentFile;traceGeneration=documentGeneration;}
    command(kind == 1 ? "ftebroom_trace loadpoints\n" : "ftebroom_trace loadportals\n");
    return true;
  }
  catch (const std::exception& error) { documentError = error.what(); return false; }
}
static void traceAutoLoad()
{
  action("ftebroom_trace unloadpoints\nftebroom_trace unloadportals\n");
  tracePaths[0].clear(); tracePaths[1].clear();tracePoints.clear();tracePointIndex=-1;traceWindow=false;
  if (documentFile.empty()) return;
  auto path = ftebroom::leak::find(documentFile);if(path.empty())return;
  std::error_code error;
  auto modified = std::filesystem::last_write_time(path, error);
  if (error) return;
  auto mapModified = std::filesystem::last_write_time(documentFile, error);
  if (!error && modified > mapModified) traceChoose(1, path);
}
static void traceFocus(int point){
 if(tracePoints.empty())return;traceEditingView();tracePointIndex=std::clamp(point,0,int(tracePoints.size())-1);
 traceVisible=true;action(("ftebroom_trace visible 1\nftebroom_trace goto "+std::to_string(tracePointIndex)+"\n").c_str());
}
static bool traceAction(const std::string& cmd){
 if(cmd=="ftebroom_trace next"){traceFocus(tracePointIndex+1);return true;}
 if(cmd=="ftebroom_trace previous"){traceFocus(tracePointIndex-1);return true;}
 if(cmd=="ftebroom_trace unloadpoints"){tracePoints.clear();tracePointIndex=-1;traceWindow=false;}
 return false;
}
static void tracePanel(){
 if(!traceWindow||tracePoints.empty())return;
 ImGui::SetNextWindowSize({450,0},ImGuiCond_FirstUseEver);
 if(ImGui::Begin("Leak route",&traceWindow,ImGuiWindowFlags_AlwaysAutoResize)){
  bool stale=traceDocument!=documentFile||traceGeneration!=documentGeneration||traceRevision!=traceRevisionNow();
  ImGui::TextColored(stale?ImVec4{1,.7f,.2f,1}:ImVec4{1,.35f,.2f,1},stale?"Outdated route - recompile the map":"Compiler leak route");
  ImGui::Text("%zu points",tracePoints.size());
  if(ImGui::Checkbox("Show route",&traceVisible))action(traceVisible?"ftebroom_trace visible 1\n":"ftebroom_trace visible 0\n");
  ImGui::BeginDisabled(modEmbedded);
  if(ImGui::Button("Focus leak"))traceFocus(std::max(0,tracePointIndex));
  if(ImGui::Button("First"))traceFocus(0);ImGui::SameLine();
  if(ImGui::Button("Previous"))traceFocus(tracePointIndex-1);ImGui::SameLine();
  if(ImGui::Button("Next"))traceFocus(tracePointIndex+1);ImGui::SameLine();
  if(ImGui::Button("Last"))traceFocus(int(tracePoints.size())-1);
  int number=std::max(0,tracePointIndex)+1;ImGui::SetNextItemWidth(130);
  if(ImGui::InputInt("Point",&number))traceFocus(std::clamp(number,1,int(tracePoints.size()))-1);
  ImGui::EndDisabled();
  auto p=tracePoints[std::max(0,tracePointIndex)];ImGui::Text("X %.2f   Y %.2f   Z %.2f",p[0],p[1],p[2]);
  ImGui::TextWrapped("The route connects an entity to the outside. Follow it to locate the opening; an endpoint is not necessarily the hole. Comma/period: previous/next point.");
  ImGui::TextWrapped("%s",tracePaths[0].u8string().c_str());
 }ImGui::End();
}
static void testTraceFiles()
{
  if (!cvars->GetFloat("ftebroom_skip_setup")) return;
  auto path = documentPath("data/trace-test.lin");
  std::ofstream(path) << "0 0 64\n16 0 64\n32 16 64\n48 32 64\n64 32 80\n";
  core->Print(traceChoose(1,path) ? "FTEBROOM_TRACE_PASS native_points\n" : "FTEBROOM_TRACE_FAIL native_points\n");
  path = documentPath("data/trace-test.prt");
  std::ofstream(path) << "PRT1\n2\n1\n4 0 1 (0 0 64) (64 0 64) (64 64 64) (0 64 64)\n";
  core->Print(traceChoose(2,path) ? "FTEBROOM_TRACE_PASS native_portals\n" : "FTEBROOM_TRACE_FAIL native_portals\n");
  path = documentPath("data/invalid.prt");
  std::ofstream(path) << "PRT1\n2\n1\n2000000000 0 1\n";
  core->Print(!traceChoose(2,path) ? "FTEBROOM_TRACE_PASS rejects_invalid\n" : "FTEBROOM_TRACE_FAIL rejects_invalid\n");
  documentError.clear();
}
