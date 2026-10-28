"""Exercise the production read-only VM adapter, including memory relocation."""
import argparse, subprocess, tempfile
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('--compiler',default='c++');args=parser.parse_args()
header=(Path(__file__).resolve().parents[1]/'native/ftebroom_qc_bridge.h').read_text(encoding='utf8')
prefix=r"""
#include <array>
#include <string>
#include <cstring>
#include <cassert>
#include <cstdio>
using pint_t=int;using pvec_t=float;
constexpr int OFS_RETURN=1,OFS_PARM0=4,OFS_PARM1=7,OFS_PARM7=25,PR_CURRENT=-1,PR_ANY=-2;
struct pubprogfuncs_t {
 unsigned (*FindFunction)(pubprogfuncs_t*,const char*,int);
 void* (*globals)(pubprogfuncs_t*,int);
 void (*ExecuteProgram)(pubprogfuncs_t*,unsigned);
 const char* (*StringToNative)(pubprogfuncs_t*,int);
};
struct Core {void* (*GetEngineInterface)(const char*,unsigned);};
static Core* core;static bool modEmbedded;
static int before[28],after[28];static int* memory=before;static bool available=true,functionAvailable=true;
static int calls;static std::string transient="RBUI1 snapshot";
"""
body=prefix+header+r"""
static pubprogfuncs_t vm;
static unsigned find(pubprogfuncs_t*,const char* name,int){assert(std::string(name)=="ReyondBroom_UIQuery");return functionAvailable?17:0;}
static void* globals(pubprogfuncs_t*,int){return memory;}
static const char* text(pubprogfuncs_t*,int id){assert(id==91);return transient.c_str();}
static void execute(pubprogfuncs_t*,unsigned id){
 assert(id==17);assert(reinterpret_cast<float*>(memory)[OFS_PARM0]==2);assert(reinterpret_cast<float*>(memory)[OFS_PARM1]==128);
 std::string nested;assert(!qcQuery(0,0,nested));
 memory=after;std::memset(after,0,sizeof(after));after[OFS_RETURN]=91;++calls;
}
static void* interface(const char* name,unsigned size){assert(std::string(name)=="CSQCVM");assert(size==sizeof(vm));return available?&vm:nullptr;}
int main(){
 Core engine{interface};core=&engine;vm={find,globals,execute,text};
 for(int i=0;i<28;i++)before[i]=1000+i;
 std::string result;assert(qcQuery(2,128,result));assert(calls==1);assert(result=="RBUI1 snapshot");
 for(int i=0;i<28;i++)assert(after[i]==1000+i);
 transient="expired";assert(result=="RBUI1 snapshot");
 available=false;assert(!qcQuery(2,128,result));available=true;
 functionAvailable=false;assert(!qcQuery(2,128,result));functionAvailable=true;
 modEmbedded=true;assert(!qcQuery(2,128,result));assert(calls==1);
 std::puts("PASS QC bridge: register restoration, relocated globals, copied strings, missing VM/function, reentry and mod isolation");
}
"""
with tempfile.TemporaryDirectory() as folder:
 source=Path(folder)/'bridge.cpp';binary=Path(folder)/'bridge-test.exe'
 source.write_text(body,encoding='utf8');subprocess.run([args.compiler,'-std=c++17',str(source),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
