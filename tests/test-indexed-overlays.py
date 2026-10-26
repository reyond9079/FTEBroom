"""Check the actual indexed overlay append path, including 16-bit batch boundaries."""
import argparse,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--engine',required=True,type=Path);p.add_argument('--compiler',default='cc');a=p.parse_args()
s=(a.engine/'client/pr_csqc.c').read_text();code=s[s.index('static qboolean RB_QCBuffer'):s.index('// ReyondBroom native face pass begin.')]
shim=r'''
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define MAX_INDICIES 65535
#define DRAWFLAG_2D 4
#define DRAWFLAG_ADD 1
#define DRAWFLAG_MODULATE 3
#define DRAWFLAG_TWOSIDED 16
#define DRAWFLAG_LINES 2048
#define BEF_NOSHADOWS 1
#define BEF_FORCEADDITIVE 2
#define BEF_FORCETRANSPARENT 4
#define BEF_FORCETWOSIDED 8
#define BEF_LINES 16
#define max(a,b) ((a)>(b)?(a):(b))
#define BZ_Realloc realloc
#define VectorCopy(a,b) memcpy(b,a,12)
#define Vector2Copy(a,b) memcpy(b,a,8)
#define Vector4Copy(a,b) memcpy(b,a,16)
typedef float vec3_t[3],vec2_t[2],vec4_t[4];typedef int qboolean,shader_t;
typedef struct {char *stringtable;unsigned int stringtablesize;}pubprogfuncs_t;
typedef struct {vec3_t xyz;vec2_t st;vec4_t rgba;}qcvertex_t;
static int cl_numstrisvert,cl_numstrisidx,cl_maxstrisvert,cl_maxstrisidx,csqc_poly_origvert,csqc_poly_origidx,csqc_poly_startvert,csqc_poly_startidx,csqc_poly_2d;
static vec3_t *cl_strisvertv;static vec2_t *cl_strisvertt;static vec4_t *cl_strisvertc;static unsigned int *cl_strisidx,csqc_poly_flags;static shader_t shader,*csqc_poly_shader;
static void (*R2D_Flush)(void);static int errors,flushes;static unsigned int emitted;
static void CSQC_PolyFlush(void){for(int i=csqc_poly_origidx;i<cl_numstrisidx;i++){assert(cl_strisidx[i]<MAX_INDICIES);unsigned int v=csqc_poly_origvert+cl_strisidx[i];assert(v<(unsigned)cl_numstrisvert);assert(cl_strisvertv[v][0]==(float)(emitted%6==0||emitted%6==3?0:emitted%6==1?1:emitted%6==5?3:2));emitted++;}R2D_Flush=NULL;flushes++;}
static shader_t *PR_R_PolygonShader(const char *s,int twod){(void)s;(void)twod;return &shader;}
static void PR_BIError(pubprogfuncs_t *v,const char *s){(void)v;(void)s;errors++;}
static void cl_stris_ExpandVerts(int n){cl_maxstrisvert=n;cl_strisvertv=realloc(cl_strisvertv,n*sizeof(*cl_strisvertv));cl_strisvertt=realloc(cl_strisvertt,n*sizeof(*cl_strisvertt));cl_strisvertc=realloc(cl_strisvertc,n*sizeof(*cl_strisvertc));}
'''
test=r'''
int main(void){pubprogfuncs_t vm={calloc(1,4096),4096};qcvertex_t *v=(qcvertex_t*)(vm.stringtable+128);unsigned int *idx=(unsigned int*)(vm.stringtable+1024);unsigned int tri[]={0,1,2,0,2,3};memcpy(idx,tri,sizeof(tri));for(int i=0;i<4;i++){v[i].xyz[0]=i;v[i].rgba[3]=1;}
rb_soup_context_t context=RB_SoupContext("grid",3);for(int i=0;i<20000;i++)RB_AddIndexedSoup(&context,&vm,128,4,1024,6);if(R2D_Flush)R2D_Flush();assert(!errors&&emitted==120000&&cl_numstrisvert==80000&&flushes>=2);assert(csqc_poly_startvert==cl_numstrisvert&&csqc_poly_startidx==cl_numstrisidx);
RB_AddIndexedSoup(&context,&vm,0,4,1024,6);assert(errors==1);RB_AddIndexedSoup(&context,&vm,4000,4,1024,6);assert(errors==2);RB_AddIndexedSoup(&context,&vm,128,0,1024,0);assert(errors==2);free(vm.stringtable);free(cl_strisvertv);free(cl_strisvertt);free(cl_strisvertc);free(cl_strisidx);puts("PASS: exact primitive stream, shared vertices, 16-bit flush boundaries, state and range guards");}
'''
with tempfile.TemporaryDirectory() as folder:
 f=Path(folder)/'test.c';exe=Path(folder)/'test.exe';f.write_text(shim+code+test);subprocess.run([a.compiler,'-std=c99','-O2',str(f),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
