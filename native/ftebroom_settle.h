// SPDX-License-Identifier: GPL-3.0-or-later
#include "ftebroom_settle_physics.h"
#include <atomic>
#include <future>
#include <sstream>
static bool settleWindow=false,settleWaiting=false;
static std::atomic<bool> settleCancel{false};
static std::atomic<float> settleProgress{0};
static std::atomic<int> settleMoving{0};
static std::future<std::vector<ftebroom::settle::Transform>> settleJob;
static std::string settleError;
static ftebroom::settle::Options settleOptions;
static void settleStart(){if(settleWaiting||settleJob.valid())return;settleError.clear();settleCancel=false;settleProgress=0;settleMoving=0;cvars->SetFloat("ftebroom_settle_moving",0);cvars->SetFloat("ftebroom_settle_ready",0);cvars->SetFloat("ftebroom_settle_valid",0);command("ftebroom_settle export\n");settleWaiting=true;}
static void settleTick(){
 if(settleWaiting&&cvars->GetFloat("ftebroom_settle_ready")){
  settleWaiting=false;
  if(cvars->GetFloat("ftebroom_settle_ready")>0){auto path=documentPath("data/settle-input.txt");auto options=settleOptions;options.bound=cvars->GetFloat("ftebroom_world_bound");
   settleJob=std::async(std::launch::async,[path,options]{
    using namespace ftebroom::settle;std::ifstream in(path);std::vector<Body> selected,obstacles;std::vector<Triangle> patches;std::string kind;bool complete=false;
    auto point=[&]{Point p;for(auto& v:p)if(!(in>>v)||!std::isfinite(v))throw std::runtime_error("Invalid Settle snapshot coordinate.");return p;};
    while(in>>kind){if(settleCancel)throw std::runtime_error("Settle cancelled.");if(kind=="END"){complete=true;break;}
     if(kind=="B"){Body body;int dynamic=0,count=0;if(!(in>>dynamic>>body.volume>>count)||count<4||count>4096||(dynamic!=0&&dynamic!=1))throw std::runtime_error("Invalid Settle brush snapshot.");body.center=point();for(int i=0;i<count;i++)body.vertices.push_back(point());auto& target=dynamic?selected:obstacles;if(target.size()>=size_t(dynamic?128:4096))throw std::runtime_error("Too many Settle brushes.");target.push_back(std::move(body));}
     else if(kind=="T"){if(patches.size()>=262144)throw std::runtime_error("Too many Settle patch triangles.");patches.push_back({point(),point(),point()});}
     else throw std::runtime_error("Unknown Settle snapshot record.");
    }
    if(!complete)throw std::runtime_error("Incomplete Settle snapshot.");Simulation simulation(std::move(selected),obstacles,patches,options);auto result=simulation.run([](double value,const auto&){settleProgress=float(value);return !settleCancel.load();});settleMoving=simulation.movingCount();return result;
   });
  }
 }
 if(settleJob.valid()&&settleJob.wait_for(std::chrono::seconds(0))==std::future_status::ready){try{auto result=settleJob.get();if(!settleCancel){std::ostringstream out;out.precision(17);out<<result.size()<<'\n';for(const auto& m:result){for(auto p:m.columns)out<<p[0]<<' '<<p[1]<<' '<<p[2]<<'\n';out<<m.translation[0]<<' '<<m.translation[1]<<' '<<m.translation[2]<<'\n';}out<<"END\n";auto text=out.str();qhandle_t handle;if(files->Open("data/settle-result.txt",&handle,2)<0)throw std::runtime_error("Cannot write Settle result.");int written=files->Write(handle,text.data(),int(text.size()));files->Close(handle);if(written!=int(text.size()))throw std::runtime_error("Incomplete Settle result write.");command("ftebroom_settle preview\n");}}catch(const std::exception& e){settleError=e.what();}}
}
static void settlePanel(){
 settleTick();if(!settleWindow)return;bool open=true;
 ImGui::SetNextWindowSize({430*uiScale(),340*uiScale()},ImGuiCond_FirstUseEver);
 if(ImGui::Begin("Settle",&open)){
  bool busy=settleWaiting||settleJob.valid();ImGui::BeginDisabled(busy);static float friction=.7f,bounce=.1f,kick=32,duration=8;static int seed=1;
  ImGui::SliderFloat("Friction",&friction,0,1);ImGui::SliderFloat("Bounce",&bounce,0,1);ImGui::SliderFloat("Initial impulse",&kick,0,256);ImGui::SliderFloat("Duration (seconds)",&duration,.1f,20);ImGui::InputInt("Seed",&seed);
  settleOptions.friction=friction;settleOptions.restitution=bounce;settleOptions.kick=kick;settleOptions.duration=duration;settleOptions.seed=uint32_t(seed);
  if(ImGui::Button("Simulate"))settleStart();ImGui::SameLine();ImGui::BeginDisabled(!cvars->GetFloat("ftebroom_settle_valid"));if(ImGui::Button("Apply"))command("ftebroom_settle apply\n");ImGui::EndDisabled();ImGui::EndDisabled();
  ImGui::SameLine();if(ImGui::Button("Cancel")){settleCancel=true;settleWaiting=false;command("ftebroom_settle cancel\n");}
  if(busy)ImGui::ProgressBar(settleProgress.load());
  ImGui::TextWrapped("Selected convex brushes collide with visible brushes and tessellated patches. Simulate previews the result; Apply records one Undo step.");
  if(!settleError.empty())ImGui::TextWrapped("%s",settleError.c_str());char error[1024]={};cvars->GetString("ftebroom_settle_error",error,sizeof error);if(*error)ImGui::TextWrapped("%s",error);
 }ImGui::End();if(!open){settleWindow=false;settleWaiting=false;settleCancel=true;command("ftebroom_settle cancel\n");}
}
