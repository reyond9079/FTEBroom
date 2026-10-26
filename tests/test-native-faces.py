"""Validate the native draw-buffer ABI, guards, culling and VM argument lifetime."""
from pathlib import Path
import argparse, subprocess, tempfile
p=argparse.ArgumentParser();p.add_argument('--engine',required=True,type=Path);p.add_argument('--compiler',default='cc');a=p.parse_args()
s=(a.engine/'client/pr_csqc.c').read_text();code=s.split('// ReyondBroom native face pass begin.',1)[1].split('// ReyondBroom native face pass end.',1)[0];code=code[code.index('\ntypedef'):]
shim=r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#define QCBUILTIN
#define false 0
#define M_PI 3.14159265358979323846
#define OFS_RETURN 1
#define OFS_PARM0 4
#define OFS_PARM1 7
#define OFS_PARM2 10
#define OFS_PARM3 13
#define OFS_PARM4 16
#define G_INT(o) (((int*)pr_globals)[o])
#define VectorSubtract(a,b,c) for(int axis=0;axis<3;axis++)(c)[axis]=(a)[axis]-(b)[axis]
#define DotProduct(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
typedef float vec3_t[3];
typedef struct {char *stringtable;unsigned int stringtablesize;}pubprogfuncs_t;
struct globalvars_s {int words[64];};
static struct {int dirty;vec3_t viewangles,vieworg;float fov_x,fov_y;}r_refdef;
static int errors,draws;
static void V_ApplyRefdef(void){r_refdef.dirty=0;}
static void AngleVectors(float *a,float *f,float *r,float *u){(void)a;memset(f,0,12);memset(r,0,12);memset(u,0,12);f[0]=r[1]=u[2]=1;}
static void PR_BIError(pubprogfuncs_t *p,const char *s){(void)p;(void)s;errors++;}
static void PF_R_AddTrisoup_Simple(pubprogfuncs_t *p,struct globalvars_s *pr_globals){(void)p;assert(G_INT(OFS_PARM0)==23&&G_INT(OFS_PARM1)==3&&G_INT(OFS_PARM4)==6);draws++;}
'''
checks=r'''
int main(void){
 pubprogfuncs_t vm={calloc(1,4096),4096};struct globalvars_s globals={0},*pr_globals=&globals;
 editorqcface_t *face=(editorqcface_t*)(vm.stringtable+128);int *access=(int*)(vm.stringtable+1024);int saved[15];
 assert(sizeof(*face)==100);face->node=1;face->count=4;face->brushEnd=1;face->enabled=1;face->shader=23;face->vertices=2048;face->triangles=3072;
 face->alpha=1;face->center[0]=face->brushCenter[0]=10;face->radius=face->brushRadius=1;face->normal[0]=-1;face->distance=-10;access[1]=1;r_refdef.fov_x=r_refdef.fov_y=90;
 G_INT(OFS_PARM0)=128;G_INT(OFS_PARM1)=1;G_INT(OFS_PARM2)=1024;G_INT(OFS_PARM3)=2;G_INT(OFS_PARM4)=sizeof(*face);memcpy(saved,globals.words+OFS_PARM0,sizeof(saved));
 PF_ReyondBroom_DrawFaces(&vm,pr_globals);assert(!errors&&draws==1&&G_INT(OFS_RETURN)==1);assert(!memcmp(saved,globals.words+OFS_PARM0,sizeof(saved)));
 access[1]=0;PF_ReyondBroom_DrawFaces(&vm,pr_globals);assert(G_INT(OFS_RETURN)==0&&draws==1);
 access[1]=1;face->brushCenter[0]=-20;PF_ReyondBroom_DrawFaces(&vm,pr_globals);assert(G_INT(OFS_RETURN)==0&&draws==1);
 G_INT(OFS_PARM0)=0;G_INT(OFS_PARM1)=0;PF_ReyondBroom_DrawFaces(&vm,pr_globals);assert(G_INT(OFS_RETURN)==0&&!errors);
 G_INT(OFS_PARM1)=1;PF_ReyondBroom_DrawFaces(&vm,pr_globals);assert(errors==1&&G_INT(OFS_RETURN)==-1);
 G_INT(OFS_PARM4)=4;PF_ReyondBroom_DrawFaces(&vm,pr_globals);assert(errors==1&&G_INT(OFS_RETURN)==-1);
 editorqcpoint_t *points=(editorqcpoint_t*)(vm.stringtable+128);int *indices=(int*)(vm.stringtable+1024);
 memset(points,0,3*sizeof(*points));assert(sizeof(*points)==56);
 points[0].center[0]=10;points[0].radius=1;points[1].center[0]=-20;points[1].radius=1;
 points[2].center[0]=-20;points[2].model=1;
 G_INT(OFS_PARM0)=128;G_INT(OFS_PARM1)=3;G_INT(OFS_PARM2)=1024;G_INT(OFS_PARM3)=56;
 PF_ReyondBroom_CullPoints(&vm,pr_globals);assert(G_INT(OFS_RETURN)==2&&indices[0]==0&&indices[1]==2);
 G_INT(OFS_PARM1)=0;G_INT(OFS_PARM0)=0;PF_ReyondBroom_CullPoints(&vm,pr_globals);assert(G_INT(OFS_RETURN)==0);
 G_INT(OFS_PARM1)=3;PF_ReyondBroom_CullPoints(&vm,pr_globals);assert(G_INT(OFS_RETURN)==-1&&errors==2);
 free(vm.stringtable);puts("PASS: face ABI, exact argument restoration, visibility/frustum culling, empty pass and invalid buffers");
}
'''
with tempfile.TemporaryDirectory() as folder:
 file=Path(folder)/'test.c';exe=Path(folder)/'test.exe';file.write_text(shim+code+checks)
 subprocess.run([a.compiler,'-std=c99','-O2',str(file),'-lm','-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
