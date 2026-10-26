"""Exercise the actual engine broad phase against brute-force box/segment queries."""
import argparse,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--engine',type=Path,required=True);p.add_argument('--compiler',default='cc');a=p.parse_args()
source=(a.engine/'gl/gl_heightmap.c').read_text();code=source.split('// ReyondBroom trace index begin:',1)[1].split('// ReyondBroom trace index end.',1)[0];code=code[code.index('\ntypedef'):]
shim=r'''
#include <stdlib.h>
#include <stdint.h>
#define World_ReyondBroomInvalidateLabels() ((void)0)
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
typedef float vec3_t[3];typedef int qboolean;
typedef struct {vec3_t mins,maxs;} brushes_t;
typedef struct {brushes_t *wbrushes;unsigned int numbrushes;} heightmap_t;
#define true 1
#define false 0
#define BZ_Malloc malloc
#define BZ_Free free
#define ClearBounds(lo,hi) for(int axis=0;axis<3;axis++){(lo)[axis]=1e30f;(hi)[axis]=-1e30f;}
static int mainThread=1;
static int Sys_IsMainThread(void){return mainThread;}
'''
test=r'''
static unsigned int randomState=17;
static float randomCoordinate(void){randomState=randomState*1664525u+1013904223u;return (float)(randomState%100000)/10-5000;}
static int exactRay(brushes_t *b,float *s,float *e){double enter=0,leave=1;for(int a=0;a<3;a++){double d=(double)e[a]-s[a];if(fabs(d)<1e-20){if(s[a]<b->mins[a]||s[a]>b->maxs[a])return 0;}else{double x=(b->mins[a]-s[a])/d,y=(b->maxs[a]-s[a])/d,t;if(x>y){t=x;x=y;y=t;}if(enter<x)enter=x;if(leave>y)leave=y;if(enter>leave)return 0;}}return 1;}
int main(void){
 heightmap_t hm={0};hm.numbrushes=4096;hm.wbrushes=calloc(4097,sizeof(*hm.wbrushes));
 for(unsigned i=0;i<hm.numbrushes;i++)for(int a=0;a<3;a++){hm.wbrushes[i].mins[a]=randomCoordinate();hm.wbrushes[i].maxs[a]=hm.wbrushes[i].mins[a]+16+i%96;}
 for(int test=0;test<12000;test++){
  float s[3],e[3],lo[3],hi[3];unsigned *hits=NULL;unsigned char found[4096]={0};
  for(int a=0;a<3;a++){s[a]=randomCoordinate();e[a]=randomCoordinate();lo[a]=fmin(s[a],e[a]);hi[a]=fmax(s[a],e[a]);}
  int point=test%2,count=Terr_TraceCandidates(&hm,lo,hi,s,e,point,&hits);assert(count>=0);
  for(int i=0;i<count;i++){assert(hits[i]<hm.numbrushes);assert(!i||hits[i]>hits[i-1]);found[hits[i]]=1;}
  for(unsigned i=0;i<hm.numbrushes;i++){
   brushes_t *b=&hm.wbrushes[i];int intersects=1;
   for(int a=0;a<3;a++)if(hi[a]<b->mins[a]||lo[a]>b->maxs[a])intersects=0;
   if(point)intersects=intersects&&exactRay(b,s,e);
   assert(!intersects||found[i]);
  }
 }
 // Same-count edits, swap removal and owner reuse must rebuild before the next query.
 float lo[3]={-2,-2,-2},hi[3]={2,2,2},s[3]={-2,0,0},e[3]={2,0,0};unsigned *hits=NULL;
 for(int a=0;a<3;a++){hm.wbrushes[17].mins[a]=-1;hm.wbrushes[17].maxs[a]=1;}
 Terr_TraceInvalidate(&hm);int count=Terr_TraceCandidates(&hm,lo,hi,s,e,1,&hits),seen=0;for(int i=0;i<count;i++)seen|=hits[i]==17;assert(seen);
 hm.wbrushes[17]=hm.wbrushes[--hm.numbrushes];Terr_TraceInvalidate(&hm);Terr_TraceCandidates(&hm,lo,hi,s,e,1,&hits);
 mainThread=0;assert(Terr_TraceCandidates(&hm,lo,hi,s,e,1,&hits)==-1&&hits==NULL);mainThread=1;
 Terr_TraceClear(&hm);free(hm.wbrushes);
 heightmap_t models[288]={0};
 for(int m=0;m<288;m++){models[m].numbrushes=32;models[m].wbrushes=calloc(32,sizeof(brushes_t));for(int i=0;i<32;i++)for(int a=0;a<3;a++){models[m].wbrushes[i].mins[a]=-1;models[m].wbrushes[i].maxs[a]=1;}}
 for(int repeat=0;repeat<2;repeat++)for(int m=0;m<288;m++)assert(Terr_TraceCandidates(&models[m],lo,hi,s,e,1,&hits)==32);
 for(int m=0;m<288;m++){Terr_TraceClear(&models[m]);free(models[m].wbrushes);}

 heightmap_t large[3]={0};
 for(int m=0;m<3;m++){large[m].numbrushes=262144;large[m].wbrushes=calloc(262144,sizeof(brushes_t));float far[3]={100,100,100};Terr_TraceCandidates(&large[m],far,far,far,far,1,&hits);assert(editortracebytes<=RB_TRACE_CACHE_BUDGET);}
 assert(!Terr_TraceOwnerFind(&large[0]));
 for(int m=0;m<3;m++){Terr_TraceClear(&large[m]);free(large[m].wbrushes);}assert(editortracebytes==0);
 puts("PASS: 49 million box comparisons, segment coverage, stable order, edit/delete invalidation, bounded eviction and worker fallback");
}
'''
with tempfile.TemporaryDirectory() as folder:
 root=Path(folder);file=root/'index.c';file.write_text(shim+code+test);binary=root/'test.exe'
 subprocess.run([a.compiler,'-std=c99','-O2',str(file),'-lm','-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
