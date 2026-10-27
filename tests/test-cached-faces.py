"""Exercise the production retained face pass and its source mutations."""
import runpy
import subprocess
import tempfile
from pathlib import Path

shared = runpy.run_path(str(Path(__file__).with_name('test-cached-grid.py')))
s, a = shared['s'], shared['a']
face = s[s.index('typedef struct {\n int node,count,brushEnd,shader;'):s.index('typedef struct {unsigned int grid,gridCount,faces,faceCount,epoch;}')]
face += '\n'+next(line for line in s.splitlines() if line.startswith('typedef struct {int node,id,model;vec3_t center,label,color;float radius;int name;}'))+'\n'
function = s[s.index('static void RB_IncrementalMarker'):s.index('// ReyondBroom bulk overlay passes end.')]
shim = r'''
#include <math.h>
#define M_PI 3.14159265358979323846
#define QCBUILTIN
#define Sys_DoubleTime() 0.0
#define Con_Printf(...) ((void)0)
static float occlusionPlane=-1e30f;
static int RB_OccludedSphere(int node,const float *center,float radius){(void)node;return center[0]+radius<occlusionPlane;}
static int RB_OccludedExtent(const float *center,const float *extent){return center[0]+extent[0]<occlusionPlane;}
#define OFS_RETURN 1
#define OFS_PARM0 4
#define OFS_PARM1 7
#define OFS_PARM2 10
#define OFS_PARM3 13
#define OFS_PARM4 16
#define OFS_PARM5 19
#define OFS_PARM6 22
#define OFS_PARM7 25
#define G_INT(n) (((int*)pr_globals)[n])
#define DotProduct(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VectorSubtract(a,b,c) do{for(int k=0;k<3;k++)(c)[k]=(a)[k]-(b)[k];}while(0)
#define BZ_Realloc realloc
struct globalvars_s {int data[64];};
#define BEF_NOSHADOWS 32
#define SHADER_SORT_OPAQUE 4
#define SBITS_SRCBLEND_ONE 16
#define SBITS_DSTBLEND_ZERO 32
#define SBITS_BLEND_BITS 49
#define SBITS_ATEST_BITS 2
#define SBITS_ATEST_SHIFT 0
#define SBITS_MISC_DEPTHWRITE 4
typedef struct {int shaderbits;} shaderpass_t;
typedef struct {int id,sort,numpasses;char name[64];shaderpass_t passes[1];} shader_t;
static int batchMode;static unsigned int rb_access_revision;
static float Cvar_VariableValue(const char *name){(void)name;return batchMode;}
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
config = s[s.index('typedef struct {unsigned int grid,gridCount,faces,faceCount,epoch;}'):s.index('static void QCBUILTIN PF_ReyondBroom_IncrementalSupport')]
raw = s[s.index('void QCBUILTIN PF_ReyondBroom_DrawFaces('):s.index('// Compact point snapshots')]
shim += r'''
static void PR_BIError(pubprogfuncs_t *vm,const char *message){(void)vm;puts(message);assert(0);}
static void RB_AddIndexedSoup(rb_soup_context_t *context,pubprogfuncs_t *vm,unsigned int vertices,unsigned int count,unsigned int indices,unsigned int total){
 (void)vm;(void)indices;if(cl_numstris==cl_maxstris){cl_maxstris+=8;cl_stris=realloc(cl_stris,cl_maxstris*sizeof(*cl_stris));}
 cl_stris[cl_numstris++]=(scenetris_t){context->shader,context->flags,vertices,0,count,total};
}
'''
checks = r'''
int main(void){
 rb_order_bounds_t plane84={0},box84={0};plane84.normal[2]=1;plane84.distance=10;box84.low[2]=12;box84.high[2]=13;
 assert(RB_OrderPlaneSeparated(&plane84,&box84,.1f));box84.low[2]=9;assert(!RB_OrderPlaneSeparated(&plane84,&box84,.1f));
 box84.high[2]=9;assert(RB_OrderPlaneSeparated(&plane84,&box84,.1f));box84.high[2]=9.95f;assert(!RB_OrderPlaneSeparated(&plane84,&box84,.1f));
 plane84.normal[0]=plane84.normal[2]=0;assert(!RB_OrderPlaneSeparated(&plane84,&box84,.1f));

 pubprogfuncs_t vm={calloc(1,32*1024*1024),32*1024*1024};struct globalvars_s globals;struct globalvars_s *pr_globals=&globals;activeGlobals=pr_globals;memset(&globals,0,sizeof(globals));
 const int count=30000;unsigned int offset=128,at=offset+count*sizeof(editorqcface_t),accessOffset=at;at+=32;int *access=(int*)(vm.stringtable+accessOffset);access[2]=1;
 editorqcface_t *faces=(editorqcface_t*)(vm.stringtable+offset);
 for(int i=0;i<count;i++){editorqcface_t *f=&faces[i];f->node=2;f->count=4;f->brushEnd=count;f->shader=1;f->enabled=1;f->alpha=1;f->normal[2]=1;f->radius=f->brushRadius=100;f->vertices=at;at+=4*sizeof(qcvertex_t);f->triangles=at;at+=6*sizeof(int);unsigned int *indices=(unsigned int*)(vm.stringtable+f->triangles);indices[0]=0;indices[1]=1;indices[2]=2;indices[3]=0;indices[4]=2;indices[5]=3;}
 assert(at<vm.stringtablesize);r_refdef.vieworg[2]=10;r_refdef.fov_x=r_refdef.fov_y=90;
 G_INT(OFS_PARM0)=offset;G_INT(OFS_PARM1)=count;G_INT(OFS_PARM2)=accessOffset;G_INT(OFS_PARM3)=8;G_INT(OFS_PARM4)=sizeof(*faces);G_INT(OFS_PARM5)=1;int saved[18];memcpy(saved,globals.data+OFS_PARM0,sizeof(saved));
 PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&cl_numstris>0);assert(!memcmp(saved,globals.data+OFS_PARM0,sizeof(saved)));assert(rb_mesh_caches[2].valid&&alive==2);unsigned int submitted=0;
 for(int i=0;i<cl_numstris;i++){assert(cl_stris[i].shader==&shaders[1]&&cl_stris[i].firstvert<0);submitted+=cl_stris[i].numidx;}assert(submitted==count*6);
 int before=uploads;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&uploads==before);
 currentShader=2;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&cl_stris[0].shader==&shaders[2]&&uploads>before);before=uploads;
 access[2]=0;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==0&&!cl_numstris);access[2]=1;
 unsigned int *bad=(unsigned int*)(vm.stringtable+faces[0].triangles);bad[1]=4;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==-1&&!cl_numstris);bad[1]=1;
 G_INT(OFS_PARM5)=2;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&uploads>before);
 // Alternating materials retain compact buffers and exact primitive order.
 currentShader=1;for(int i=0;i<count;i++){faces[i].shader=1+(i&1);qcvertex_t *v=(qcvertex_t*)(vm.stringtable+faces[i].vertices);for(int j=0;j<4;j++){v[j].xyz[0]=(i&31)*64;v[j].xyz[1]=((i>>5)&31)*64;v[j].xyz[2]=(i>>10)*64;}}
 G_INT(OFS_PARM5)=3;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);
 assert(G_INT(OFS_RETURN)==count&&cl_numstris==count&&rb_mesh_caches[2].count<8);
 for(int i=0;i<count;i++){assert(cl_stris[i].shader==&shaders[1+(i&1)]);batch_t batch={0};assert(CSQC_ReyondBroomCachedBatch(&batch,-1-cl_stris[i].firstvert));mesh_t *mesh=*batch.mesh;assert(mesh->numindexes==6);assert(mesh->indexes[1]==mesh->indexes[0]+1);}
 rb_geom.valid=rb_geom.reusable=1;rb_geom.generation=100;rb_geom.accessOffset=accessOffset;rb_geom.accessCount=8;
 cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&rb_mesh_caches[2].viewValid);before=uploads;
 cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&cl_numstris==count&&uploads==before);
 access[2]=0;rb_geom.generation++;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(!G_INT(OFS_RETURN)&&!cl_numstris);
 access[2]=1;rb_geom.generation++;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count&&cl_numstris==count);rb_geom.valid=0;
 // Large independent opaque spans collapse to material/chunk draws.
 for(int i=1;i<3;i++){shaders[i].sort=SHADER_SORT_OPAQUE;shaders[i].numpasses=1;strcpy(shaders[i].name,"ftebroom/opaque/test");shaders[i].passes[0].shaderbits=SBITS_MISC_DEPTHWRITE;}
 batchMode=1;rb_geom.generation++;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);
 assert(G_INT(OFS_RETURN)==count&&cl_numstris<16);submitted=0;for(int i=0;i<cl_numstris;i++)submitted+=cl_stris[i].numidx;assert(submitted==count*6);
 // A tolerated back face must keep the original-order fallback, not disappear.
 faces[0].distance=10.005f;rb_geom.generation++;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(G_INT(OFS_RETURN)==count);submitted=0;for(int i=0;i<cl_numstris;i++)submitted+=cl_stris[i].numidx;assert(submitted==count*6);faces[0].distance=0;
 // Generated alpha-tested depth writers share the graph; custom shaders stay barriers.
 shaders[1].passes[0].shaderbits|=SBITS_ATEST_BITS;rb_geom.generation++;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(cl_numstris<16);submitted=0;for(int i=0;i<cl_numstris;i++)submitted+=cl_stris[i].numidx;assert(submitted==count*6);
 strcpy(shaders[1].name,"custom/alpha-test");rb_geom.generation++;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(cl_numstris==count);strcpy(shaders[1].name,"ftebroom/opaque/test");
 shaders[1].passes[0].shaderbits=SBITS_MISC_DEPTHWRITE;batchMode=0;rb_geom.generation++;
 // The edited layer must remain interleaved and must not re-upload the base.
 unsigned int configOffset=at;at+=sizeof(rb_incremental_config_t);unsigned int editOffset=at;at+=2*sizeof(editorqcface_t);
 rb_incremental_config_t *config=(rb_incremental_config_t*)(vm.stringtable+configOffset);*config=(rb_incremental_config_t){0,0,editOffset,2,1};
 editorqcface_t *edited=(editorqcface_t*)(vm.stringtable+editOffset);edited[0]=edited[1]=faces[0];edited[0].node=1;edited[1].node=3;edited[0].brushEnd=1;edited[1].brushEnd=2;edited[0].enabled=edited[1].enabled=2;edited[0].alpha=edited[1].alpha=.35f;
 access[1]=access[3]=17;G_INT(OFS_PARM6)=configOffset;vm.callargc=7;rb_geom.valid=1;rb_geom.generation++;
 cl_numstris=0;before=uploads;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(cl_numstris==count+2&&uploads==before);assert(cl_stris[0].firstvert==(int)faces[0].vertices&&cl_stris[count+1].firstvert==(int)faces[0].vertices);
 cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(cl_numstris==count+2&&uploads==before&&vm.callargc==7);
 for(int i=0;i<count;i++)faces[i].enabled=0;rb_geom.generation++;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(cl_numstris==2&&uploads==before);
 access[1]=access[3]=1;rb_geom.generation++;cl_numstris=0;PF_ReyondBroom_DrawFacesCached(&vm,pr_globals);assert(!cl_numstris&&uploads==before);
 // The uncached path merges the exact same edited ranges.
 access[1]=access[3]=17;G_INT(OFS_PARM5)=configOffset;vm.callargc=6;cl_numstris=0;PF_ReyondBroom_DrawFaces(&vm,pr_globals);assert(cl_numstris==2&&vm.callargc==6);
 // A hidden first brush in a sliced edited range must not skip its neighbor.
 config->faceCount=4;edited[2]=edited[3]=faces[0];edited[2].node=4;edited[3].node=5;edited[2].brushEnd=3;edited[3].brushEnd=4;edited[2].enabled=edited[3].enabled=2;
 access[1]=17;access[3]=0;access[4]=access[5]=17;vm.callargc=6;G_INT(OFS_PARM5)=configOffset;cl_numstris=0;PF_ReyondBroom_DrawFaces(&vm,pr_globals);assert(cl_numstris==3);
 rb_cache_source_t *many=malloc(70000*sizeof(*many));for(int i=0;i<70000;i++)many[i]=(rb_cache_source_t){faces[0].vertices,4,6,32,i&1,1};
 assert(RB_CacheGeometry(&vm,2,offset,70000,4,many,70000));assert(rb_mesh_caches[2].count<16);free(many);
 // Overlapping depth ranges retain their original material order.
 rb_face_draw_t overlap[4];memset(overlap,0,sizeof(overlap));unsigned int order[4];for(int i=0;i<4;i++){overlap[i].slot=0;overlap[i].shader=&shaders[1+(i&1)];overlap[i].low[0]=overlap[i].low[1]=overlap[i].low[2]=0;overlap[i].high[0]=overlap[i].high[1]=overlap[i].high[2]=16;}
 assert(RB_OrderSpan(overlap,4,order));for(int i=0;i<4;i++)assert(order[i]==(unsigned int)i);
 // Random overlapping faces exercise same-material and cross-material dependencies.
 rb_face_draw_t randomRuns[128];unsigned int randomOrder[128],positions[128];unsigned int seed=1337;
 for(int i=0;i<128;i++){memset(&randomRuns[i],0,sizeof(randomRuns[i]));randomRuns[i].slot=i%3;randomRuns[i].shader=&shaders[1+(i&1)];for(int a=0;a<3;a++){seed=seed*1664525u+1013904223u;randomRuns[i].low[a]=(seed>>24)*.25f;randomRuns[i].high[a]=randomRuns[i].low[a]+12;}}
 assert(RB_OrderSpan(randomRuns,128,randomOrder));for(int i=0;i<128;i++)positions[randomOrder[i]]=i;
 for(int i=0;i<128;i++)for(int j=i+1;j<128;j++)if(RB_OrderOverlap(randomRuns[i].low,randomRuns[i].high,randomRuns[j].low,randomRuns[j].high,.01f))assert(positions[i]<positions[j]);
 r_refdef.vieworg[2]=10010;for(int i=0;i<4;i++){overlap[i].low[0]=i*19;overlap[i].high[0]=i*19+16;}
 assert(RB_OrderSpan(overlap,4,order));for(int i=0;i<4;i++)assert(order[i]==(unsigned int)i);

 // A retained whole-document order remains valid for changing visible subsets.
 RB_ClearOrderCache();rb_order_cache.total=128;rb_order_cache.bounds=malloc(128*sizeof(*rb_order_cache.bounds));rb_order_cache.tree=malloc(256*sizeof(*rb_order_cache.tree));rb_order_cache.ids=malloc(128*sizeof(unsigned int));rb_order_cache.counts=malloc(128*sizeof(unsigned int));rb_order_cache.offsets=malloc(128*sizeof(unsigned int));rb_order_cache.local=malloc(128*sizeof(unsigned int));rb_order_cache.stamps=calloc(128,sizeof(unsigned int));memset(rb_order_cache.counts,255,128*sizeof(unsigned int));
 rb_cached_grid_ref_t rankRefs[128];rb_cached_mesh_t rankMeshes[3];memset(rankMeshes,0,sizeof(rankMeshes));editorqcface_t rankFaces[128];memset(rankFaces,0,sizeof(rankFaces));
 for(int i=0;i<3;i++){rankMeshes[i].shader=&shaders[1+(i&1)];rankMeshes[i].pass=BEF_NOSHADOWS;}
 for(int i=0;i<128;i++){randomRuns[i].source=i;rankRefs[i]=(rb_cached_grid_ref_t){i%3,0};rankFaces[i].alpha=1;rb_order_cache.ids[i]=i;memset(&rb_order_cache.bounds[i],0,sizeof(rb_order_cache.bounds[i]));VectorCopy(randomRuns[i].low,rb_order_cache.bounds[i].low);VectorCopy(randomRuns[i].high,rb_order_cache.bounds[i].high);randomRuns[i].shader=rankMeshes[i%3].shader;}
 unsigned int treeUsed=0;rb_order_bounds=rb_order_cache.bounds;RB_OrderTree(rb_order_cache.tree,&treeUsed,rb_order_cache.ids,0,128);
 rb_mesh_cache_t rankCache={0};rankCache.refs=rankRefs;rankCache.meshes=rankMeshes;rankCache.count=3;r_refdef.vieworg[2]=10;RB_PrepareOrderRanks(&rankCache,rankFaces,128);rb_order_cache.useRanks=1;
 assert(rb_order_cache.ranks);assert(RB_OrderSpan(randomRuns,128,randomOrder));for(int i=0;i<128;i++)positions[randomOrder[i]]=i;
 for(int i=0;i<128;i++)for(int j=i+1;j<128;j++)if(RB_OrderOverlap(randomRuns[i].low,randomRuns[i].high,randomRuns[j].low,randomRuns[j].high,.01f))assert(positions[i]<positions[j]);
 vec3_t origin84;VectorCopy(rb_order_cache.rankEye,origin84);r_refdef.vieworg[0]+=100;RB_PrepareOrderRanks(&rankCache,rankFaces,128);assert(!memcmp(origin84,rb_order_cache.rankEye,sizeof(origin84)));
 r_refdef.vieworg[0]+=600;RB_PrepareOrderRanks(&rankCache,rankFaces,128);assert(rb_order_cache.rankEye[0]==r_refdef.vieworg[0]);r_refdef.vieworg[0]-=700;RB_PrepareOrderRanks(&rankCache,rankFaces,128);
 for(int i=0;i<128;i++){double squared84=0;for(int a=0;a<3;a++){double lo=fabs(rb_order_cache.bounds[i].low[a]-r_refdef.vieworg[a]),hi=fabs(rb_order_cache.bounds[i].high[a]-r_refdef.vieworg[a]),extent=fmax(lo,hi);squared84+=extent*extent;}double bound84=sqrt(squared84)+512;assert(rb_order_cache.bounds[i].depthPad+1e-5>=.01+bound84*bound84/16777216.0);}
 // Region-inactive faces cannot become front-facing anywhere in its camera ball.
 for(int i=0;i<128;i++){double norm=sqrt((double)DotProduct(rankFaces[i].normal,rankFaces[i].normal)),side=DotProduct(rb_order_cache.rankEye,rankFaces[i].normal)-rankFaces[i].distance;if(rb_order_cache.rankInactive[i])assert(side+512*norm<-.1);}
 rb_face_draw_t subset[64];unsigned int subsetOrder[64];for(int i=0;i<64;i++)subset[i]=randomRuns[i*2];assert(RB_OrderSpan(subset,64,subsetOrder));for(int i=1;i<64;i++)assert(rb_order_cache.ranks[subset[subsetOrder[i-1]].source]<rb_order_cache.ranks[subset[subsetOrder[i]].source]);
 // A discarded back face re-enters the dependency schedule before the camera
 // can reach its plane; staying inside the region cannot activate it.
 rankFaces[0].normal[2]=1;rankFaces[0].distance=1034;rb_order_cache.rankRegion=0;RB_PrepareOrderRanks(&rankCache,rankFaces,128);assert(rb_order_cache.rankInactive[0]&&rb_order_cache.sorted[127]==0);
 r_refdef.vieworg[2]+=511;RB_PrepareOrderRanks(&rankCache,rankFaces,128);assert(rb_order_cache.rankInactive[0]);r_refdef.vieworg[2]+=2;RB_PrepareOrderRanks(&rankCache,rankFaces,128);assert(!rb_order_cache.rankInactive[0]);
 r_refdef.vieworg[2]-=513;rankFaces[0].normal[2]=0;rankFaces[0].distance=0;rb_order_cache.rankRegion=0;RB_PrepareOrderRanks(&rankCache,rankFaces,128);
 // A branch must never hide a leaf that survives the individual proof.
 unsigned char visible83[128];vec3_t zero83={0};
 for(int test83=0;test83<100;test83++){
  occlusionPlane=test83;memset(visible83,0,sizeof(visible83));
  RB_VisitVisibility(rb_order_cache.tree,rb_order_cache.ids,rb_order_cache.bounds,0,visible83,0,zero83,zero83,zero83,0,0);
  for(int leaf83=0;leaf83<128;leaf83++){
   vec3_t center83;float radius83=0;
   for(int a=0;a<3;a++){center83[a]=(randomRuns[leaf83].low[a]+randomRuns[leaf83].high[a])*.5f;float half83=(randomRuns[leaf83].high[a]-randomRuns[leaf83].low[a])*.5f;if(radius83<half83)radius83=half83;}
   if(randomRuns[leaf83].high[0]+.01f>=occlusionPlane)assert(visible83[leaf83]);
  }
 }
 occlusionPlane=-1e30f;
 RB_ClearOrderCache();assert(!rb_order_cache.total&&!rb_order_cache.ranks&&!rb_order_cache.neighbors);
 // Generated later-sort helpers preserve their own order while opaque runs join.
 shader_t transparent83={0};transparent83.sort=SHADER_SORT_OPAQUE+1;transparent83.numpasses=1;strcpy(transparent83.name,"ftebroom/filtered/test");
 rb_cached_mesh_t mergeMeshes83[3];memset(mergeMeshes83,0,sizeof(mergeMeshes83));index_t mergeIndices83[4]={10,11,20,21};rb_mesh_cache_t mergeCache83={0};mergeCache83.meshes=mergeMeshes83;mergeCache83.count=3;mergeCache83.scratch=mergeIndices83;mergeCache83.indexCapacity=4;mergeMeshes83[0].scratch=mergeIndices83;mergeMeshes83[0].pending=2;mergeMeshes83[2].scratch=mergeIndices83+2;mergeMeshes83[2].pending=2;
 rb_face_draw_count=4;for(int i=0;i<4;i++){memset(&rb_face_draws[i],0,sizeof(rb_face_draws[i]));rb_face_draws[i].slot=i&1?2:0;rb_face_draws[i].offset=i/2;rb_face_draws[i].count=1;rb_face_draws[i].shader=i&1?&transparent83:&shaders[1];rb_face_draws[i].flags=BEF_NOSHADOWS;rb_face_draws[i].batchable=!(i&1);rb_face_draws[i].low[0]=i*100;rb_face_draws[i].high[0]=i*100+1;}
 RB_BatchOpaqueRuns(&mergeCache83);assert(rb_face_draw_count==2&&rb_face_draws[0].slot==0&&rb_face_draws[0].count==2&&rb_face_draws[1].slot==2&&rb_face_draws[1].count==2);assert(mergeIndices83[0]==10&&mergeIndices83[1]==11&&mergeIndices83[2]==20&&mergeIndices83[3]==21);


 // Match backend bucket ordering, and keep every edited-layer barrier intact.
 mergeMeshes83[0].multi=1;mergeMeshes83[2].multi=0;rb_face_draw_count=4;
 for(int i=0;i<4;i++){memset(&rb_face_draws[i],0,sizeof(rb_face_draws[i]));rb_face_draws[i].slot=i&1?2:0;rb_face_draws[i].offset=i/2;rb_face_draws[i].count=1;rb_face_draws[i].shader=i&1?&transparent83:&shaders[1];rb_face_draws[i].flags=BEF_NOSHADOWS;}
 RB_DeferRetainedTransparency(&mergeCache83);assert(rb_face_draw_count==2&&rb_face_draws[0].slot==0&&rb_face_draws[0].offset==0&&rb_face_draws[0].count==2&&rb_face_draws[1].slot==2&&rb_face_draws[1].count==2);
 rb_face_draw_count=5;for(int i=0;i<5;i++){memset(&rb_face_draws[i],0,sizeof(rb_face_draws[i]));if(i==2){rb_face_draws[i].slot=~0u;rb_face_draws[i].count=1;continue;}int j=i<2?i:i-1;rb_face_draws[i].slot=j&1?2:0;rb_face_draws[i].offset=j/2;rb_face_draws[i].count=1;rb_face_draws[i].shader=j&1?&transparent83:&shaders[1];rb_face_draws[i].flags=BEF_NOSHADOWS;}
 RB_DeferRetainedTransparency(&mergeCache83);assert(rb_face_draw_count==5&&rb_face_draws[2].slot==~0u&&rb_face_draws[0].slot==0&&rb_face_draws[1].slot==2&&rb_face_draws[3].slot==0&&rb_face_draws[4].slot==2);
 // Retained GPU command lists preserve source order and survive camera travel.
 rb_cache_source_t *gpuSources=calloc(count,sizeof(*gpuSources));for(int i=0;i<count;i++){gpuSources[i]=(rb_cache_source_t){faces[i].vertices,faces[i].count,(faces[i].count-2)*3,BEF_NOSHADOWS,1,1,1,0};VectorCopy(faces[i].normal,gpuSources[i].normal);gpuSources[i].distance=faces[i].distance;}
 assert(RB_CacheGeometry(&vm,2,offset,count,20,gpuSources,count));free(gpuSources);
 rb_mesh_cache_t *retained=&rb_mesh_caches[2];assert(retained->valid&&retained->total==count);for(unsigned int i=0;i<retained->count;i++){retained->meshes[i].shader=&shaders[1];assert(!retained->meshes[i].extras&&retained->meshes[i].vbo.normals.size>0&&retained->meshes[i].vbo.lmcoord[0].size>0);}
 for(unsigned int i=0;i<retained->count;i++)retained->meshes[i].multi=1;
 for(int i=0;i<count;i++){faces[i].enabled=1;faces[i].alpha=1;access[faces[i].node]=17;}
 retained->retainedEligible=0;retained->retained=false;batchMode=1;rb_incremental_config_t noEdits={0};int gpuDrawn=0;
 assert(RB_RetainMaterialFaces(&vm,retained,faces,count,access,8,70,noEdits,&gpuDrawn));assert(gpuDrawn==count);
 unsigned int previousSource=0,totalIndices=0;for(unsigned int i=0;i<rb_face_draw_count;i++){assert(rb_face_draws[i].slot!=~0u&&rb_face_draws[i].source>=previousSource);previousSource=rb_face_draws[i].source;totalIndices+=rb_face_draws[i].count;}assert(totalIndices==count*6);
 before=uploads;for(int i=0;i<25;i++){r_refdef.vieworg[0]+=1000;r_refdef.viewangles[1]+=20;assert(RB_RetainMaterialFaces(&vm,retained,faces,count,access,8,70,noEdits,&gpuDrawn));}assert(uploads==before);
 // Permanent incremental edits may hide old source faces without changing access.
 faces[0].enabled=0;assert(RB_RetainMaterialFaces(&vm,retained,faces,count,access,8,71,noEdits,&gpuDrawn));assert(gpuDrawn==count-1);
 before=uploads;assert(RB_RetainMaterialFaces(&vm,retained,faces,count,access,8,71,noEdits,&gpuDrawn)&&uploads==before);
 // Live delta draw markers stay interleaved at their original node positions.
 edited[0].node=faces[1].node;edited[1].node=faces[count-1].node+1;rb_incremental_config_t gpuEdits={0,0,editOffset,2,0};
 assert(RB_RetainMaterialFaces(&vm,retained,faces,count,access,8,72,gpuEdits,&gpuDrawn));int markers=0;
 for(unsigned int i=0;i<rb_face_draw_count;i++)if(rb_face_draws[i].slot==~0u){markers+=rb_face_draws[i].count;assert(rb_face_draws[i].offset<2);}assert(markers==2);
 before=uploads;assert(RB_RetainMaterialFaces(&vm,retained,faces,count,access,8,72,gpuEdits,&gpuDrawn)&&uploads==before);
 // A material without a GPU plane test must keep the complete legacy fallback.
 faces[0].enabled=1;retained->meshes[retained->refs[0].slot].multi=0;retained->retainedEligible=0;
 assert(!RB_RetainMaterialFaces(&vm,retained,faces,count,access,8,73,noEdits,&gpuDrawn));
 CSQC_ReyondBroomClearMeshes();assert(alive==0);free(cl_stris);free(vm.stringtable);puts("PASS: 30,000 retained faces, unchanged arguments, exact index count, warm reuse, access changes, invalid triangles, revision rebuild and complete release");
}
'''
with tempfile.TemporaryDirectory() as folder:
    file, exe = Path(folder) / 'faces.c', Path(folder) / 'faces.exe'
    file.write_text(shared['shim'] + shared['layout'] + '\n' + shared['code'] + face + shim + config + raw + function + checks)
    subprocess.run([a.compiler, '-std=c99', '-O2', str(file), '-lm', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
