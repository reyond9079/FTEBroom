"""Compile the actual engine face clipper and test its conservative fallback."""
from pathlib import Path
import argparse, subprocess, tempfile, os
p=argparse.ArgumentParser();p.add_argument('engine',type=Path);p.add_argument('--compiler',default='g++');a=p.parse_args()
source=(a.engine/'common/q1bsp.c').read_text()
source=source[source.index('#define MAXFRAGMENTVERTS'):source.index('\n#ifdef HAVE_CLIENT')]
source=source.replace('Fragment_ClipPlaneToBrush(', 'Original_ClipPlaneToBrush(')
heightmap=(a.engine/'gl/gl_heightmap.c').read_text()
fallback=heightmap[heightmap.index('static unsigned int Terr_EditorClipFace('):heightmap.index('static brushes_t *Terr_Brush_Insert(')]
stub=r'''
#include <cmath>
#include <cstring>
#include <cassert>
#include <iostream>
using vec_t=float;using qbyte=unsigned char;using qboolean=int;
typedef float vec3_t[3],vec4_t[4],vecV_t[3];
#define DotProduct(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VectorCopy(a,b) for(int v=0;v<3;v++)(b)[v]=(a)[v]
#define VectorSet(a,x,y,z) ((a)[0]=(x),(a)[1]=(y),(a)[2]=(z))
#define CrossProduct(a,b,c) ((c)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1],(c)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2],(c)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0])
#define VectorScale(a,s,b) for(int v=0;v<3;v++)(b)[v]=(a)[v]*(s)
#define VectorMA(a,s,b,c) for(int v=0;v<3;v++)(c)[v]=(a)[v]+(s)*(b)[v]
#define VectorNegate(a,b) for(int v=0;v<3;v++)(b)[v]=-(a)[v]
#define VectorInterpolate(a,s,b,c) for(int v=0;v<3;v++)(c)[v]=(a)[v]+(s)*((b)[v]-(a)[v])
void VectorNormalize(float* p){float n=std::sqrt(DotProduct(p,p));if(n)for(int v=0;v<3;v++)p[v]/=n;}
'''
adapter=r'''
bool forceFallback=false;
size_t Fragment_ClipPlaneToBrush(vecV_t* points,size_t capacity,void* planes,size_t stride,size_t count,vec4_t face){return forceFallback?0:Original_ClipPlaneToBrush(points,capacity,planes,stride,count,face);}
'''
checks=r'''
int main(){
 vec4_t planes[6]={{1,0,0,128},{-1,0,0,0},{0,1,0,128},{0,-1,0,0},{0,0,1,128},{0,0,-1,0}};
 vecV_t before[64],after[64];
 for(int face=0;face<6;face++){
  auto n=Original_ClipPlaneToBrush(before,64,planes,sizeof(*planes),6,planes[face]);
  auto m=Terr_EditorClipFace(after,64,planes,6,face);
  assert(n==4&&m==n&&std::memcmp(before,after,n*sizeof(*before))==0);
 }
 forceFallback=true;
 for(int face=0;face<6;face++){
  auto n=Terr_EditorClipFace(after,64,planes,6,face);assert(n==4);
  for(unsigned v=0;v<n;v++)for(int axis=0;axis<3;axis++)assert(after[v][axis]>=-.001&&after[v][axis]<=128.001);
 }
 assert(Terr_EditorClipFace(after,2,planes,6,0)==0);
 planes[1][3]=-256;for(int face=0;face<6;face++)assert(Terr_EditorClipFace(after,64,planes,6,face)==0);
 std::cout<<"PASS: exact fast-path preservation, double fallback bounds, capacity and empty intersection\n";
}
'''
with tempfile.TemporaryDirectory(prefix='rb-precision-') as folder:
 cpp=Path(folder)/'test.cpp';exe=Path(folder)/('test.exe' if os.name=='nt' else 'test')
 cpp.write_text(stub+source+adapter+fallback+checks)
 subprocess.run([a.compiler,'-std=c++17','-O2',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
