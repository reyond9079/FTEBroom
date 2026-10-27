"""Exercise the production entity-label surface index and invalidation guards."""
import argparse
import subprocess
import tempfile
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--engine', required=True, type=Path)
p.add_argument('--compiler', default='cc')
a = p.parse_args()
source = (a.engine / 'client/pr_csqc.c').read_text()
code = source[source.index('typedef struct {int id,model,node,count,u,v,sky,brushEnd;'):source.index('typedef struct {int stride,flags,gridShader,edgeShader;')]
code = code[:code.index('// Conservative screen tiles')] + code[code.index('// Entity-label visibility uses an exact surface index'):]
shim = r'''
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#define QCBUILTIN
static unsigned int rb_label_queries,rb_label_tiles,rb_label_hint_hits,rb_label_tree_hits,rb_label_visible;
static int RB_OccludedTilePoint(const float *point){(void)point;return 0;}
static struct {int valid;} rb_geom;
static float Cvar_VariableValue(const char *name){(void)name;return 0;}
static void PF_ReyondBroom_PrepareGeometry(void *vm,void *globals){(void)vm;(void)globals;}
#define true 1
#define false 0
#define M_PI 3.14159265358979323846
#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))
#define BZ_Realloc realloc
#define OFS_RETURN 1
#define OFS_PARM0 4
#define OFS_PARM1 7
#define OFS_PARM2 10
#define OFS_PARM3 13
#define OFS_PARM4 16
#define OFS_PARM5 19
#define OFS_PARM6 22
#define OFS_PARM7 25
#define G_INT(o) (((int*)pr_globals)[o])
#define VectorCopy(a,b) memcpy(b,a,12)
#define VectorSubtract(a,b,c) for(int axis=0;axis<3;axis++)(c)[axis]=(a)[axis]-(b)[axis]
#define DotProduct(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
typedef float vec3_t[3],vec2_t[2];typedef int qboolean;
typedef struct {char *stringtable;unsigned int stringtablesize;int callargc;} pubprogfuncs_t;
struct globalvars_s {int words[64];};
static struct {int dirty;vec3_t viewangles,vieworg;float fov_x,fov_y;struct {float x,y,width,height;} grect,vrect;} r_refdef;
static int csqc_rebuildmatricies,errors;static float csqc_proj_matrix[16],csqc_proj_matrix_inverse[16];
static void cs_buildmatricies(void){}
static void V_ApplyRefdef(void){}
static void AngleVectors(float *a,float *f,float *r,float *u){(void)a;(void)r;(void)u;f[0]=1;f[1]=f[2]=0;}
static void Matrix4x4_CM_Transform4(float *matrix,float *in,float *out){if(matrix==csqc_proj_matrix){out[0]=in[1];out[1]=in[2];out[2]=0;out[3]=in[0];}else{out[0]=1;out[1]=in[0];out[2]=in[1];out[3]=1;}}
static int RB_QCBuffer(pubprogfuncs_t *vm,unsigned int offset,unsigned int count,unsigned int stride){return offset && offset<vm->stringtablesize && count<=(vm->stringtablesize-offset)/stride;}
static void PR_BIError(pubprogfuncs_t *vm,const char *s){(void)vm;(void)s;errors++;}
'''
shim += 'typedef struct {unsigned int grid,gridCount,faces,faceCount,epoch;} rb_incremental_config_t;static rb_incremental_config_t rb_incremental;static pubprogfuncs_t *rb_incremental_vm;static rb_incremental_config_t RB_IncrementalConfig(pubprogfuncs_t *vm,unsigned int offset){rb_incremental_config_t value={0};if(offset&&RB_QCBuffer(vm,offset,1,sizeof(value)))memcpy(&value,vm->stringtable+offset,sizeof(value));return value;}\n'
checks = r'''
int main(void){
 pubprogfuncs_t vm={calloc(1,8192),8192};struct globalvars_s globals={0},*pr_globals=&globals;
 rb_grid_face_t *f=(rb_grid_face_t*)(vm.stringtable+128);int *access=(int*)(vm.stringtable+1024);vec3_t *points=(vec3_t*)(vm.stringtable+2048);
 assert(sizeof(*f)==84);f->node=1;f->count=4;f->center[0]=20;f->normal[0]=-1;f->radius=14.2;f->points=2048;f->occluder=3;access[1]=5;
 float yz[4][2]={{-10,-10},{10,-10},{10,10},{-10,10}};for(int i=0;i<4;i++){points[i][0]=20;points[i][1]=yz[i][0];points[i][2]=yz[i][1];}
 csqc_proj_matrix[4]=csqc_proj_matrix[9]=1;r_refdef.fov_x=r_refdef.fov_y=90;r_refdef.grect.width=r_refdef.vrect.width=1280;r_refdef.grect.height=r_refdef.vrect.height=720;
 G_INT(OFS_PARM0)=128;G_INT(OFS_PARM1)=1;G_INT(OFS_PARM2)=1024;G_INT(OFS_PARM3)=2;G_INT(OFS_PARM4)=84;G_INT(OFS_PARM5)=3;G_INT(OFS_PARM6)++;
 PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(!errors&&rb_occ.valid);
 vec3_t behind={30,0,0},front={10,0,0},outside={30,20,0};assert(RB_OccludedPoint(behind));assert(!RB_OccludedPoint(front));assert(!RB_OccludedPoint(outside));
 // Exact tests must not invent coverage at a polygon boundary.
 for(int i=-160;i<=160;i++)for(int j=-160;j<=160;j++){vec3_t point={30,i*.1f,j*.1f};if(RB_OccludedPoint(point))assert(fabs(point[1])<15&&fabs(point[2])<15);}
 // Sky can hide labels without changing the geometry stream.
 G_INT(OFS_PARM6)++;f->occluder=2;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(RB_OccludedPoint(behind));
 G_INT(OFS_PARM6)++;f->occluder=0;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(!RB_OccludedPoint(behind));
 // Hidden and trigger brushes cannot contribute occluders.
 G_INT(OFS_PARM6)++;f->occluder=3;access[1]=1;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(!RB_OccludedPoint(behind));G_INT(OFS_PARM6)++;access[1]=4;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(!RB_OccludedPoint(behind));
 access[1]=5;G_INT(OFS_PARM5)=0;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(!rb_occ.valid&&!RB_OccludedPoint(behind));
 G_INT(OFS_PARM5)=3;G_INT(OFS_PARM6)++;G_INT(OFS_PARM0)=8100;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(errors==0);G_INT(OFS_PARM0)=8150;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(errors==1&&!rb_occ.valid);
 // Sparse edits retire a wall immediately without rebuilding the static BVH.
 rb_incremental_config_t *config=(rb_incremental_config_t*)(vm.stringtable+4096);*config=(rb_incremental_config_t){4352,0,0,0,999};
 rb_grid_face_t *edited=(rb_grid_face_t*)(vm.stringtable+4352);*edited=*f;edited->points=4608;vec3_t *editedPoints=(vec3_t*)(vm.stringtable+4608);memcpy(editedPoints,points,4*sizeof(vec3_t));
 G_INT(OFS_PARM0)=128;G_INT(OFS_PARM5)=2;G_INT(OFS_PARM7)=4096;vm.callargc=8;access[1]=5;f->occluder=3|8;G_INT(OFS_PARM6)++;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(!RB_OccludedPoint(behind));void *tree=rb_labels.nodes;unsigned int entries=rb_labels.count;
 edited->occluder=3;edited->center[0]=25;for(int i=0;i<4;i++)editedPoints[i][0]=25;config->gridCount=1;G_INT(OFS_PARM6)++;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(RB_OccludedPoint(behind)&&rb_labels.nodes==tree&&rb_labels.count==entries);
 edited->center[0]=40;for(int i=0;i<4;i++)editedPoints[i][0]=40;G_INT(OFS_PARM6)++;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(!RB_OccludedPoint(behind)&&rb_labels.nodes==tree&&rb_labels.count==entries);
 edited->center[0]=25;for(int i=0;i<4;i++)editedPoints[i][0]=25;edited->occluder=0;G_INT(OFS_PARM6)++;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(!RB_OccludedPoint(behind));
 f->occluder=3;G_INT(OFS_PARM6)++;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(RB_OccludedPoint(behind));access[1]=1;G_INT(OFS_PARM6)++;PF_ReyondBroom_PrepareOcclusion(&vm,pr_globals);assert(!RB_OccludedPoint(behind));
 puts("PASS: label index, foreground, boundary, sky, transparency, access, invalidation and invalid buffers");free(vm.stringtable);
}
'''
with tempfile.TemporaryDirectory() as directory:
    file = Path(directory) / 'test.c'
    exe = Path(directory) / 'test.exe'
    file.write_text(shim + code + checks)
    subprocess.run([a.compiler, '-std=c99', '-O2', str(file), '-lm', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
