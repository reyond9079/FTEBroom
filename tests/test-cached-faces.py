"""Exercise the production retained face pass and its source mutations."""
import runpy
import subprocess
import tempfile
from pathlib import Path

shared = runpy.run_path(str(Path(__file__).with_name('test-cached-grid.py')))
s, a = shared['s'], shared['a']
face = s[s.index('typedef struct {\n int node,count,brushEnd,shader;'):s.index('void QCBUILTIN PF_ReyondBroom_DrawFaces(')]
function = s[s.index('// Retain face attributes'):s.index('// ReyondBroom bulk overlay passes end.')]
shim = r'''
#include <math.h>
#define M_PI 3.14159265358979323846
#define QCBUILTIN
static int RB_OccludedSphere(int node,const float *center,float radius){(void)node;(void)center;(void)radius;return 0;}
#define OFS_RETURN 1
#define OFS_PARM0 4
#define OFS_PARM1 7
#define OFS_PARM2 10
#define OFS_PARM3 13
#define OFS_PARM4 16
#define OFS_PARM5 19
#define G_INT(n) (((int*)pr_globals)[n])
#define DotProduct(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VectorSubtract(a,b,c) do{for(int k=0;k<3;k++)(c)[k]=(a)[k]-(b)[k];}while(0)
#define BZ_Realloc realloc
struct globalvars_s {int data[64];};
typedef struct {int id;} shader_t;
typedef struct {shader_t *shader;unsigned int flags;int twod;} rb_soup_context_t;
static shader_t shaders[3]={{0},{1},{2}};static int currentShader=1;
static struct globalvars_s *activeGlobals;
static const char *PR_GetStringOfs(pubprogfuncs_t *vm,int n){(void)vm;return ((int*)activeGlobals)[n]==2?"2":"1";}
static rb_soup_context_t RB_SoupContext(const char *name,unsigned int flags){(void)flags;return (rb_soup_context_t){&shaders[currentShader==2?2:atoi(name)],32,0};}
static void RB_MissingBegin(unsigned int total){(void)total;}
static shader_t *RB_CanonicalMissing(shader_t *shader){return shader;}
static struct {int dirty;vec3_t vieworg,viewangles;float fov_x,fov_y;} r_refdef;
static void V_ApplyRefdef(void){}
static void AngleVectors(const float *angles,float *forward,float *right,float *up){(void)angles;forward[0]=forward[1]=0;forward[2]=-1;right[0]=1;right[1]=right[2]=0;up[1]=1;up[0]=up[2]=0;}
typedef struct {shader_t *shader;int flags,firstvert,firstidx,numvert,numidx;} scenetris_t;
static int cl_numstris,cl_maxstris;static scenetris_t *cl_stris;static void (*R2D_Flush)(void);
'''
checks = r'''
int main(void){
 pubprogfuncs_t vm={calloc(1,32*1024*1024),32*1024*1024};struct globalvars_s globals;struct globalvars_s *pr_globals=&globals;activeGlobals=pr_globals;memset(&globals,0,sizeof(globals));
 const int count=30000;unsigned int offset=128,at=offset+count*sizeof(editorqcface_t),accessOffset=at;at+=16;int *access=(int*)(vm.stringtable+accessOffset);access[2]=1;
 editorqcface_t *faces=(editorqcface_t*)(vm.stringtable+offset);
 for(int i=0;i<count;i++){editorqcface_t *f=&faces[i];f->node=2;f->count=4;f->brushEnd=count;f->shader=1;f->enabled=1;f->alpha=1;f->normal[2]=1;f->radius=f->brushRadius=100;f->vertices=at;at+=4*sizeof(qcvertex_t);f->triangles=at;at+=6*sizeof(int);unsigned int *indices=(unsigned int*)(vm.stringtable+f->triangles);indices[0]=0;indices[1]=1;indices[2]=2;indices[3]=0;indices[4]=2;indices[5]=3;}
 assert(at<vm.stringtablesize);r_refdef.vieworg[2]=10;r_refdef.fov_x=r_refdef.fov_y=90;
 G_INT(OFS_PARM0)=offset;G_INT(OFS_PARM1)=count;G_INT(OFS_PARM2)=accessOffset;G_INT(OFS_PARM3)=4;G_INT(OFS_PARM4)=sizeof(*faces);G_INT(OFS_PARM5)=1;int saved[18];memcpy(saved,globals.data+OFS_PARM0,sizeof(saved));
 PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&cl_numstris>0);assert(!memcmp(saved,globals.data+OFS_PARM0,sizeof(saved)));assert(rb_mesh_caches[2].valid&&alive==2);unsigned int submitted=0;
 for(int i=0;i<cl_numstris;i++){assert(cl_stris[i].shader==&shaders[1]&&cl_stris[i].firstvert<0);submitted+=cl_stris[i].numidx;}assert(submitted==count*6);
 int before=uploads;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&uploads==before);
 currentShader=2;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&cl_stris[0].shader==&shaders[2]&&uploads>before);before=uploads;
 access[2]=0;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==0&&!cl_numstris);access[2]=1;
 unsigned int *bad=(unsigned int*)(vm.stringtable+faces[0].triangles);bad[1]=4;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==-1&&!cl_numstris);bad[1]=1;
 G_INT(OFS_PARM5)=2;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&uploads>before);
 // Alternating materials retain compact buffers and exact primitive order.
 currentShader=1;for(int i=0;i<count;i++)faces[i].shader=1+(i&1);
 G_INT(OFS_PARM5)=3;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);
 assert(G_INT(OFS_RETURN)==count&&cl_numstris==count&&rb_mesh_caches[2].count<8);
 for(int i=0;i<count;i++){assert(cl_stris[i].shader==&shaders[1+(i&1)]);batch_t batch={0};assert(CSQC_ReyondBroomCachedBatch(&batch,-1-cl_stris[i].firstvert));mesh_t *mesh=*batch.mesh;assert(mesh->numindexes==6);assert(mesh->indexes[1]==mesh->indexes[0]+1);}
 rb_geom.valid=rb_geom.reusable=1;rb_geom.generation=100;rb_geom.accessOffset=accessOffset;rb_geom.accessCount=4;
 cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&rb_mesh_caches[2].viewValid);before=uploads;
 cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&cl_numstris==count&&uploads==before);
 access[2]=0;rb_geom.generation++;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(!G_INT(OFS_RETURN)&&!cl_numstris);
 access[2]=1;rb_geom.generation++;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&cl_numstris==count);rb_geom.valid=0;
 rb_cache_source_t *many=malloc(70000*sizeof(*many));for(int i=0;i<70000;i++)many[i]=(rb_cache_source_t){faces[0].vertices,4,6,32,i&1,1};
 assert(RB_CacheGeometry(&vm,2,offset,70000,4,many,70000));assert(rb_mesh_caches[2].count<16);free(many);
 CSQC_ReyondBroomClearMeshes();assert(alive==0);free(cl_stris);free(vm.stringtable);puts("PASS: 30,000 retained faces, unchanged arguments, exact index count, warm reuse, access changes, invalid triangles, revision rebuild and complete release");
}
'''
with tempfile.TemporaryDirectory() as folder:
    file, exe = Path(folder) / 'faces.c', Path(folder) / 'faces.exe'
    file.write_text(shared['shim'] + shared['layout'] + '\n' + shared['code'] + face + shim + function + checks)
    subprocess.run([a.compiler, '-std=c99', '-O2', str(file), '-lm', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
