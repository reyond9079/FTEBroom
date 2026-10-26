"""Check conservative coverage and depth using the production geometry mask."""
import argparse
import subprocess
import tempfile
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--engine', type=Path, required=True)
p.add_argument('--compiler', default='cc')
a = p.parse_args()
s = (a.engine / 'client/pr_csqc.c').read_text()
layout = s[s.index('typedef struct {int id,model,node,count,u,v,sky,brushEnd;'):].split('\n', 1)[0]
code = s[s.index('// Conservative screen tiles'):s.index('// Entity-label visibility uses an exact surface index')]
shim = r'''
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define QCBUILTIN
#define true 1
#define false 0
#define M_PI 3.14159265358979323846
#define BZ_Realloc realloc
#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))
#define VectorCopy(a,b) memcpy(b,a,12)
#define VectorSubtract(a,b,c) do{for(int k=0;k<3;k++)(c)[k]=(a)[k]-(b)[k];}while(0)
#define DotProduct(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
typedef float vec3_t[3];typedef int qboolean;
typedef struct {char *stringtable;unsigned int stringtablesize;} pubprogfuncs_t;
struct globalvars_s {int data[32];};
#define OFS_RETURN 1
#define OFS_PARM0 4
#define OFS_PARM1 7
#define OFS_PARM2 10
#define OFS_PARM3 13
#define OFS_PARM4 16
#define OFS_PARM5 19
#define OFS_PARM6 22
#define G_INT(n) (((int*)pr_globals)[n])
static struct {int dirty;vec3_t vieworg,viewangles;float fov_x,fov_y;struct {float x,y,width,height;} grect,vrect;} r_refdef;
static float csqc_proj_matrix[16];static int csqc_rebuildmatricies;
static void V_ApplyRefdef(void){}
static void cs_buildmatricies(void){csqc_rebuildmatricies=0;}
static void AngleVectors(const float *angles,float *forward,float *right,float *up){(void)angles;(void)right;(void)up;forward[0]=forward[1]=0;forward[2]=1;}
static void Matrix4x4_CM_Transform4(const float *m,const float *v,float *o){(void)m;o[0]=v[0];o[1]=v[1];o[2]=0;o[3]=v[2];}
static int RB_QCBuffer(pubprogfuncs_t *vm,unsigned int p,unsigned int n,unsigned int stride){return p&&p<=vm->stringtablesize&&n<=(vm->stringtablesize-p)/stride;}
static void PR_BIError(pubprogfuncs_t *vm,const char *error){(void)vm;puts(error);assert(0);}
'''
checks = r'''
int main(void){
 pubprogfuncs_t vm={calloc(1,4096),4096};struct globalvars_s globals={0};struct globalvars_s *pr_globals=&globals;
 rb_grid_face_t *f=(rb_grid_face_t*)(vm.stringtable+128);int *access=(int*)(vm.stringtable+512);vec3_t *points=(vec3_t*)(vm.stringtable+1024);
 f->node=2;f->count=4;f->points=1024;f->occluder=3;f->center[2]=16;f->normal[2]=-1;f->radius=sqrtf(32);access[2]=5;
 float vertices[4][3]={{-4,-4,16},{4,-4,16},{4,4,16},{-4,4,16}};memcpy(points,vertices,sizeof(vertices));
 csqc_proj_matrix[0]=csqc_proj_matrix[5]=csqc_proj_matrix[11]=1;
 r_refdef.grect.width=r_refdef.vrect.width=1000;r_refdef.grect.height=r_refdef.vrect.height=1000;r_refdef.fov_x=r_refdef.fov_y=90;
 G_INT(OFS_PARM0)=128;G_INT(OFS_PARM1)=1;G_INT(OFS_PARM2)=512;G_INT(OFS_PARM3)=3;G_INT(OFS_PARM4)=sizeof(*f);
 PF_ReyondBroom_PrepareGeometry(&vm,pr_globals);assert(rb_geom.valid);vec3_t center={0,0,24};assert(RB_OccludedSphere(2,center,1));
 center[0]=8;assert(!RB_OccludedSphere(2,center,1));center[0]=0;center[2]=16;assert(!RB_OccludedSphere(2,center,1));center[2]=2;assert(!RB_OccludedSphere(2,center,1));
 int hidden=0,tested=0;
 for(int x=-20;x<=20;x++)for(int y=-20;y<=20;y++)for(int z=18;z<=60;z+=3){
  center[0]=x*.5f;center[1]=y*.5f;center[2]=z;float radius=.1f+(x+20)*.05f;
  if(RB_OccludedSphere(-1,center,radius)){
   hidden++;for(int corner=0;corner<8;corner++){float px=center[0]+((corner&1)?radius:-radius),py=center[1]+((corner&2)?radius:-radius),pz=center[2]+((corner&4)?radius:-radius);assert(pz>16.05&&fabsf(px/pz)<.25f&&fabsf(py/pz)<.25f);}
  }tested++;
 }
 assert(hidden>1000);
 // A sloped plane requires a local perspective-depth bound rather than a
 // polygon-wide maximum. Verify every corner of every rejected box by ray.
 for(int i=0;i<4;i++)points[i][2]=16+points[i][0];
 f->normal[0]=sqrtf(.5f);f->normal[2]=-sqrtf(.5f);f->radius=sqrtf(48);
 G_INT(OFS_PARM6)++;PF_ReyondBroom_PrepareGeometry(&vm,pr_globals);int inclined=0;
 for(int x=-20;x<=20;x++)for(int y=-20;y<=20;y++)for(int z=24;z<=60;z+=3){
  center[0]=x*.5f;center[1]=y*.5f;center[2]=z;float radius=.1f+(x+20)*.05f;
  if(RB_OccludedSphere(-1,center,radius)){
   inclined++;for(int corner=0;corner<8;corner++){double px=center[0]+((corner&1)?radius:-radius),py=center[1]+((corner&2)?radius:-radius),pz=center[2]+((corner&4)?radius:-radius);double t=16/(pz-px);assert(t>0&&t<1&&fabs(t*px)<4&&fabs(t*py)<4);}
  }tested++;
 }
 assert(inclined>1000);hidden+=inclined;
 f->occluder=2;G_INT(OFS_PARM6)++;PF_ReyondBroom_PrepareGeometry(&vm,pr_globals);center[0]=center[1]=0;center[2]=24;assert(!RB_OccludedSphere(2,center,1));
 f->occluder=3;access[2]=1;G_INT(OFS_PARM6)++;PF_ReyondBroom_PrepareGeometry(&vm,pr_globals);assert(!RB_OccludedSphere(2,center,1));
 access[2]=5;f->count=2;G_INT(OFS_PARM6)++;PF_ReyondBroom_PrepareGeometry(&vm,pr_globals);assert(!RB_OccludedSphere(2,center,1));
 free(rb_geom.stamp);free(rb_geom.centers);free(rb_geom.radii);free(rb_geom.hidden);free(vm.stringtable);printf("PASS: %i conservative box queries, %i full-cover proofs, foreground, near plane, sky, access and invalid polygon guards\n",tested,hidden);
}
'''
with tempfile.TemporaryDirectory() as folder:
    file, exe = Path(folder) / 'test.c', Path(folder) / 'test.exe'
    file.write_text(shim + layout + '\n' + code + checks)
    subprocess.run([a.compiler, '-std=c99', '-O2', str(file), '-lm', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
