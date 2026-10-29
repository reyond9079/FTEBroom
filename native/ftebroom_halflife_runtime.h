// SPDX-License-Identifier: GPL-2.0-or-later
// Install only our separately named runtime package; never replace mod files.
static void halfLifeInstallRuntime(){
  if(gameProfile!=4)return;
  auto mod=std::filesystem::absolute(std::filesystem::u8path(hlMod)).lexically_normal();
  if(!*hlMod||!std::filesystem::is_directory(mod))throw std::runtime_error("Choose an existing Half-Life / Counter-Strike mod directory.");
  const char* package=hlRuntime?"reyondbroom-freecs-94.pk3":"reyondbroom-freehl-94.pk3";
  char runtime[4096]{};if(!files->NativePath("ftebroom.cfg",(fs_relative)5,runtime,sizeof runtime))throw std::runtime_error("Cannot locate the editor installation.");
  auto source=std::filesystem::u8path(runtime).parent_path().parent_path()/"tools/runtimes"/package;
  if(!std::filesystem::is_regular_file(source))throw std::runtime_error("Bundled FreeHL / FreeCS runtime package is missing. Extract the complete ReyondBroom distribution.");
  auto target=mod/package;
  auto read=[](const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Cannot read runtime package.");return std::string(std::istreambuf_iterator<char>(f),{});};
  auto bytes=read(source);
  const char* other=hlRuntime?"reyondbroom-freehl-94.pk3":"reyondbroom-freecs-94.pk3";
  auto inactive=mod/other,disabled=inactive;disabled+=".inactive";
  if(std::filesystem::exists(inactive)){
    auto expected=read(source.parent_path()/other);
    if(read(inactive)!=expected||(std::filesystem::exists(disabled)&&read(disabled)!=expected))throw std::runtime_error("A modified editor runtime package exists in this mod. It was not changed: "+inactive.u8string());
  }
  bool installed=std::filesystem::exists(target);
  if(installed&&read(target)!=bytes)throw std::runtime_error("A different runtime package already exists at "+target.u8string()+". It was not replaced.");
  auto pending=target;pending+=".installing";
  if(!installed&&std::filesystem::exists(pending))throw std::runtime_error("Runtime installation temporary file already exists: "+pending.u8string());
  try{
    if(!installed){std::filesystem::copy_file(source,pending);if(read(pending)!=bytes)throw std::runtime_error("Runtime package copy verification failed.");std::filesystem::rename(pending,target);}
    // FTE mounts all PK3 files. Keep only the selected editor runtime active.
    // Only byte-identical, separately named editor packages may be deactivated.
    if(std::filesystem::exists(inactive)){if(std::filesystem::exists(disabled))std::filesystem::remove(inactive);else std::filesystem::rename(inactive,disabled);}
  }
  catch(...){std::error_code error;std::filesystem::remove(pending,error);throw;}
}

// A private, highest-priority home keeps the preview's VM/configuration paired
// with its bundled runtime. Loose QC modules in an existing Steam mod must not
// override that runtime or receive editor configuration writes.
static void halfLifeStageRuntime(const std::filesystem::path& home){
  char native[4096]{};if(!files->NativePath("ftebroom.cfg",(fs_relative)5,native,sizeof native))throw std::runtime_error("Cannot locate the editor runtime.");
  auto package=std::filesystem::u8path(native).parent_path().parent_path()/"tools/runtimes"/(hlRuntime?"reyondbroom-freecs-94.pk3":"reyondbroom-freehl-94.pk3");
  auto entries=ftebroom::pak::directory(package);uint64_t total=0;
  for(const auto& entry:entries){
    auto name=std::filesystem::u8path(entry.name);if(name.empty()||name.is_absolute())throw std::runtime_error("Invalid preview runtime path.");
    for(auto& part:name)if(part=="..")throw std::runtime_error("Invalid preview runtime path.");
    if((total+=entry.size)>64*1024*1024)throw std::runtime_error("Preview runtime exceeds installation limit.");
    if(entry.name.back()=='/')continue;auto target=home/name;std::filesystem::create_directories(target.parent_path());
    auto bytes=ftebroom::pak::read(package/name);std::ofstream out(target,std::ios::binary|std::ios::trunc);out.write(bytes.data(),bytes.size());out.close();if(!out)throw std::runtime_error("Cannot stage preview runtime: "+target.u8string());
  }
  // Never execute a foreign engine configuration from the retail mod directory.
  std::ofstream(home/"fte.cfg")<<"// Private ReyondBroom preview configuration.\n";
}
