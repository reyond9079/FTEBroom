"""The production label cache must invalidate on every visibility/view dependency."""
import argparse,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--engine',type=Path,required=True);p.add_argument('--compiler',default='cc');a=p.parse_args();s=(a.engine/'client/pr_csqc.c').read_text()
layout=s[s.index('typedef struct {int node,id,model;vec3_t center,label,color;float radius;int name;}'):].split('\n',1)[0]
code=s[s.index('typedef struct {unsigned int index;vec3_t pixel;}'):s.index('typedef struct {int id,model,node,count,u,v,sky,brushEnd;')]
shim=r"""
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#define QCBUILTIN
#define true 1
#define false 0
#define M_PI 3.14159265358979323846
#define BZ_Realloc realloc
#define VectorCopy(a,b) memcpy(b,a,12)
#define VectorSet(a,x,y,z) do{(a)[0]=x;(a)[1]=y;(a)[2]=z;}while(0)
#define VectorSubtract(a,b,c) do{for(int k=0;k<3;k++)(c)[k]=(a)[k]-(b)[k];}while(0)
#define DotProduct(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define OFS_RETURN 1
#define OFS_PARM0 4
#define OFS_PARM1 7
#define OFS_PARM2 10
#define OFS_PARM3 13
#define OFS_PARM4 16
#define OFS_PARM5 19
#define G_INT(n) (((int*)pr_globals)[n])
#define G_FLOAT(n) (((float*)pr_globals)[n])
#define G_VECTOR(n) (((float*)pr_globals)+n)
#define MOVE_NOMONSTERS 1
#define MOVE_IGNOREHULL 2
#define REYONDBROOM_MOVE_OCCLUSION 4
typedef int qboolean;typedef float vec3_t[3];typedef void wedict_t;
typedef struct {char *stringtable;unsigned int stringtablesize;int callargc;}pubprogfuncs_t;
struct globalvars_s {int data[32];};
static struct {int dirty;struct{float x,y,width,height;}grect;vec3_t vieworg,viewangles;float fov_x,fov_y;}r_refdef;
static float csqc_proj_matrix[16];static unsigned int rb_label_queries,rb_label_tiles,rb_label_hint_hits,rb_label_tree_hits,rb_label_visible;
static unsigned int generation=1;static int checks,draws,enabled=1;
static float Cvar_VariableValue(const char *name){(void)name;return enabled;}
static void V_ApplyRefdef(void){}
static void AngleVectors(const float *a,float *f,float *r,float *u){(void)a;VectorSet(f,0,0,-1);VectorSet(r,1,0,0);VectorSet(u,0,1,0);}
static int RB_QCBuffer(pubprogfuncs_t *vm,unsigned int p,unsigned int n,unsigned int stride){return p&&p<=vm->stringtablesize&&n<=(vm->stringtablesize-p)/stride;}
static void PR_BIError(pubprogfuncs_t *vm,const char *message){(void)vm;puts(message);assert(0);}
static int RB_OccludedPoint(const float *p){checks++;return p[0]>30;}
static int RB_LabelsComplete(void){return 1;}
static const char *PR_GetStringOfs(pubprogfuncs_t *vm,int n){(void)vm;(void)n;return "label";}
static void PF_cs_project(pubprogfuncs_t *vm,struct globalvars_s *pr_globals){(void)vm;VectorCopy(G_VECTOR(OFS_PARM0),G_VECTOR(OFS_RETURN));}
static void PF_CL_drawcolouredstring(pubprogfuncs_t *vm,struct globalvars_s *pr_globals){(void)vm;(void)pr_globals;draws++;}
typedef struct {float fraction;}trace_t;static struct {void *edicts;}csqc_world;
static trace_t World_Move(void *w,const float *a,const float *b,const float *c,const float *d,int f,void *e){(void)w;(void)a;(void)b;(void)c;(void)d;(void)f;(void)e;return (trace_t){1};}
static vec3_t vec3_origin;
"""
checks=r"""
static unsigned int RB_LabelViewGeneration(void){return generation;}
int main(void){pubprogfuncs_t vm={calloc(1,65536),65536,5};struct globalvars_s globals={0},*pr_globals=&globals;editorqcpoint_t *p=(editorqcpoint_t*)(vm.stringtable+128);
r_refdef.grect.width=r_refdef.grect.height=128;r_refdef.fov_x=r_refdef.fov_y=90;
for(int i=0;i<3;i++){p[i].radius=100;p[i].label[0]=10+i*15;p[i].label[1]=16;p[i].label[2]=-10;p[i].center[2]=-10;p[i].name=1;}
G_INT(OFS_PARM0)=128;G_INT(OFS_PARM1)=3;G_INT(OFS_PARM2)=sizeof(*p);G_INT(OFS_PARM3)=0;G_INT(OFS_PARM4)=1;
PF_ReyondBroom_DrawLabels(&vm,pr_globals);assert(checks==3&&draws==2&&G_INT(OFS_PARM4)==1);
PF_ReyondBroom_DrawLabels(&vm,pr_globals);assert(checks==3&&draws==4);
csqc_proj_matrix[0]=2;PF_ReyondBroom_DrawLabels(&vm,pr_globals);assert(checks==6);
G_INT(OFS_PARM4)++;PF_ReyondBroom_DrawLabels(&vm,pr_globals);assert(checks==9);
generation++;PF_ReyondBroom_DrawLabels(&vm,pr_globals);assert(checks==12);
r_refdef.grect.width++;PF_ReyondBroom_DrawLabels(&vm,pr_globals);assert(checks==15);
char *old=vm.stringtable;vm.stringtable=malloc(vm.stringtablesize);memcpy(vm.stringtable,old,vm.stringtablesize);free(old);PF_ReyondBroom_DrawLabels(&vm,pr_globals);assert(checks==18);
enabled=0;PF_ReyondBroom_DrawLabels(&vm,pr_globals);PF_ReyondBroom_DrawLabels(&vm,pr_globals);assert(checks==24);
enabled=1;vm.callargc=4;PF_ReyondBroom_DrawLabels(&vm,pr_globals);PF_ReyondBroom_DrawLabels(&vm,pr_globals);assert(checks==30);
free(rb_label_draw_cache.items);free(vm.stringtable);puts("PASS label cache: warm reuse, projection, document, occlusion, viewport, VM relocation, disable and legacy calls");}
"""
with tempfile.TemporaryDirectory() as d:
 f=Path(d)/'labels.c';exe=Path(d)/'labels.exe';f.write_text(shim+layout+code+checks)
 subprocess.run([a.compiler,'-std=c99','-O2',str(f),'-lm','-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
