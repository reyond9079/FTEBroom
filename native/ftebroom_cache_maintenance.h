// SPDX-License-Identifier: GPL-2.0-or-later
#include "ftebroom_disk_cache.h"
#ifndef _WIN32
#include <sys/file.h>
#endif
// Every instance holds a shared lifetime lock. Cleanup requires exclusivity.
#ifdef _WIN32
static HANDLE cacheLock=INVALID_HANDLE_VALUE;
static OVERLAPPED cacheLockRange{};
static bool cacheExclusive(){return LockFileEx(cacheLock,LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY,0,1,0,&cacheLockRange)!=0;}
static void cacheUnlock(){UnlockFileEx(cacheLock,0,1,0,&cacheLockRange);}
static void cacheShared(){LockFileEx(cacheLock,0,0,1,0,&cacheLockRange);}
#else
static int cacheLock=-1;
static bool cacheExclusive(){return flock(cacheLock,LOCK_EX|LOCK_NB)==0;}
static void cacheUnlock(){flock(cacheLock,LOCK_UN);}
static void cacheShared(){flock(cacheLock,LOCK_SH);}
#endif
static std::filesystem::path cacheRoot;
static std::set<std::string> cacheProjectImages;
static std::string cacheProjectLastKey;
static void cacheProjectIndexed(){
 cacheProjectImages.clear();
 cacheProjectLastKey.clear();
 for(const auto& pair:materials)if(ftebroom::diskcache::cachedImage(pair.second.preview))cacheProjectImages.insert(pair.second.preview);
 cacheProjectRemember();
}
static void cacheProjectRemember(){
 try{
  auto source=pendingDocumentFile.empty()?documentFile:pendingDocumentFile;
  if(source.empty()||cacheRoot.empty()||cacheProjectImages.empty())return;
  auto key=std::to_string(gameProfile)+"\n"+source.parent_path().lexically_normal().generic_u8string()+"\n"+gameRuntime()+"\n"+std::string(baseq3);
  if(gameWad())for(const auto& wad:q1WadPaths())key+="\n"+wad.generic_u8string();
  if(key==cacheProjectLastKey)return;
  if(ftebroom::diskcache::rememberProject(cacheRoot,key,cacheProjectImages))cacheProjectLastKey=key;
 }catch(const std::exception& e){core->Print((std::string("ReyondBroom project-cache history skipped: ")+e.what()+"\n").c_str());}
}
static void cacheSweep(bool allowFull){
 try{
  ftebroom::diskcache::Options options;
  options.fullScan=allowFull&&ftebroom::diskcache::fullScanDue(cacheRoot);
  if(!documentFile.empty())options.protectedFiles.insert(documentFile);
  if(!pendingDocumentFile.empty())options.protectedFiles.insert(pendingDocumentFile);
  auto result=ftebroom::diskcache::clean(cacheRoot,options);
  if(result.fullScan&&!result.errors)ftebroom::diskcache::markFullScan(cacheRoot);
  core->Print(("ReyondBroom cache cleanup: "+std::to_string(result.files)+" generated files, "+std::to_string(result.bytes/1048576)+" MiB reclaimed.\n").c_str());
 }catch(const std::exception& e){core->Print((std::string("ReyondBroom cache cleanup skipped: ")+e.what()+"\n").c_str());}
}
static void cacheMaintenanceStart(){
 char writable[4096]{};if(!files->NativePath("ftebroom.cfg",(fs_relative)5,writable,sizeof writable))return;
 cacheRoot=std::filesystem::u8path(writable).parent_path();auto lock=cacheRoot/"cache-maintenance.lock";
#ifdef _WIN32
 cacheLock=CreateFileW(lock.wstring().c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(cacheLock==INVALID_HANDLE_VALUE)return;
#else
 cacheLock=open(lock.c_str(),O_CREAT|O_RDWR,0600);if(cacheLock<0)return;
#endif
 if(cacheExclusive()){cacheSweep(false);cacheUnlock();}cacheShared();
}
static void cacheMaintenanceStop(){
#ifdef _WIN32
 if(cacheLock==INVALID_HANDLE_VALUE)return;
#else
 if(cacheLock<0)return;
#endif
 cacheProjectRemember();
 cacheUnlock();if(cacheExclusive()){cacheSweep(true);cacheUnlock();}
#ifdef _WIN32
 CloseHandle(cacheLock);cacheLock=INVALID_HANDLE_VALUE;
#else
 close(cacheLock);cacheLock=-1;
#endif
}
