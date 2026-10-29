#include "../native/ftebroom_resource_directory.h"
#include <cassert>
#include <chrono>
int main(){namespace fs=std::filesystem;auto root=fs::temp_directory_path()/("q2-roots-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));fs::create_directories(root/"baseq2");assert(ftebroom::quake2FallbackDirectory(root/"baseq2").empty());fs::create_directories(root/"rerelease/baseq2");assert(ftebroom::quake2FallbackDirectory(root/"baseq2")==root/"rerelease/baseq2");assert(ftebroom::quake2FallbackDirectory(root/"rerelease/baseq2").empty());fs::create_directories(root/"custom");assert(ftebroom::quake2FallbackDirectory(root/"custom")==root/"baseq2");fs::remove_all(root);}
