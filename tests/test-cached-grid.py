"""Exercise production retained vertex buffers, index updates, chunk limits and lifetime."""
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
code = s[s.index('// Persistent geometry;'):s.index('static void QCBUILTIN PF_ReyondBroom_DrawGridCached')]
shim = r'''
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#define true 1
#define false 0
#define QR_OPENGL 1
#define MAX_INDICIES 65535
#define BZ_Malloc malloc
#define BZ_Free free
#define VectorCopy(a,b) memcpy(b,a,12)
#define Vector2Copy(a,b) memcpy(b,a,8)
#define Vector4Copy(a,b) memcpy(b,a,16)
typedef int qboolean;typedef unsigned short index_t;
typedef float vec3_t[3],vecV_t[4],vec2_t[2],vec4_t[4];
typedef struct {vec3_t xyz;vec2_t st;vec4_t rgba;} qcvertex_t;
typedef struct {char *stringtable;unsigned int stringtablesize;int callargc;} pubprogfuncs_t;
typedef struct {void *root,*data;size_t size;} vboarray_t;
typedef struct {vboarray_t coord,texcoord,colours[1],indicies;void *vbomem,*ebomem;unsigned int vertcount;} vbo_t;
typedef struct {vecV_t *xyz_array;vec2_t *st_array;vec4_t *colors4f_array[1];index_t *indexes;int numvertexes,numindexes;unsigned int vbofirstelement;} mesh_t;
typedef struct {mesh_t **mesh;vbo_t *vbo;void *buildmeshes;} batch_t;
typedef struct {void *root;size_t position,size;} vbobctx_t;
static struct {int valid,reusable;unsigned int generation,accessOffset,accessCount;} rb_geom;
static struct {int valid;} rb_occ,rb_labels;
static int qrenderer=QR_OPENGL,alive,uploads;
static void BE_VBO_Begin(vbobctx_t *c,size_t size){c->root=malloc(size);c->position=0;c->size=size;alive++;}
static void BE_VBO_Data(vbobctx_t *c,void *data,size_t size,vboarray_t *a){assert(c->position+size<=c->size);a->root=c->root;a->data=(char*)c->root+c->position;a->size=size;memcpy(a->data,data,size);c->position+=size;}
static void BE_VBO_Finish(vbobctx_t *c,void *data,size_t size,vboarray_t *a,void **v,void **e){(void)c;(void)v;(void)e;a->root=a->data=malloc(size);a->size=size;memcpy(a->data,data,size);alive++;uploads++;}
static void BE_VBO_Destroy(vboarray_t *a,void *m){(void)m;if(a->root){free(a->root);alive--;}memset(a,0,sizeof(*a));}
static int RB_QCBuffer(pubprogfuncs_t *vm,unsigned int p,unsigned int n,unsigned int stride){return p&&p<=vm->stringtablesize&&n<=(vm->stringtablesize-p)/stride;}
'''
checks = r'''
int main(void){
 pubprogfuncs_t vm={calloc(1,12*1024*1024),12*1024*1024};rb_grid_face_t *faces=(rb_grid_face_t*)(vm.stringtable+128);
 for(int f=0;f<2;f++){faces[f].count=4;faces[f].vertices=1024+f*4096;qcvertex_t *v=(qcvertex_t*)(vm.stringtable+faces[f].vertices);for(int i=0;i<8;i++){v[i].xyz[0]=f*100+i;v[i].rgba[0]=i;}}
 assert(RB_CacheGrid(&vm,faces,128,2,1));assert(rb_grid_cache.count==2&&alive==2);batch_t batch={0};assert(CSQC_ReyondBroomCachedBatch(&batch,0));assert(batch.mesh==&rb_grid_cache.meshes[0].meshptr&&batch.vbo==&rb_grid_cache.meshes[0].vbo);
 for(int pass=0;pass<2;pass++){rb_cached_mesh_t *m=&rb_grid_cache.meshes[pass];for(int f=0;f<2;f++)for(int i=0;i<4;i++)assert(m->mesh.xyz_array[f*4+i][0]==f*100+i+pass*4);assert(!memcmp(m->vbo.coord.data,m->mesh.xyz_array,m->vbo.coord.size));}
 rb_cached_mesh_t *m=&rb_grid_cache.meshes[0];m->pending=6;index_t expected[6]={0,1,2,0,2,3};memcpy(m->scratch,expected,sizeof(expected));RB_CachedIndices(&rb_grid_cache);assert(!memcmp(m->vbo.indicies.data,expected,sizeof(expected)));int before=uploads;RB_CachedIndices(&rb_grid_cache);assert(uploads==before);m->scratch[1]=3;RB_CachedIndices(&rb_grid_cache);assert(uploads==before+1&&alive==2);
 assert(RB_CacheGrid(&vm,faces,128,2,1)&&uploads==before+1);((qcvertex_t*)(vm.stringtable+1024))->xyz[0]=75;assert(RB_CacheGrid(&vm,faces,128,2,2));assert(rb_grid_cache.meshes[0].mesh.xyz_array[0][0]==75&&alive==2);CSQC_ReyondBroomClearMeshes();assert(alive==0&&!CSQC_ReyondBroomCachedBatch(&batch,0));
 // Odd vertex counts must not leave the color array eight bytes off alignment.
 faces[0].count=3;faces[1].count=4;assert(RB_CacheGrid(&vm,faces,128,2,3));
 for(unsigned int i=0;i<rb_grid_cache.count;i++){m=&rb_grid_cache.meshes[i];assert(!((uintptr_t)m->mesh.xyz_array&15)&&!((uintptr_t)m->mesh.st_array&15)&&!((uintptr_t)m->mesh.colors4f_array[0]&15));}
 CSQC_ReyondBroomClearMeshes();assert(alive==0);
 // Many faces force both vertex and index chunk boundaries without truncation.
 const int n=20000;unsigned int at=128+n*sizeof(*faces);for(int f=0;f<n;f++){faces[f].count=4;faces[f].vertices=at;at+=8*sizeof(qcvertex_t);assert(at<vm.stringtablesize);}
 assert(RB_CacheGrid(&vm,faces,128,n,3));unsigned int vertices=0,indices=0;for(unsigned int i=0;i<rb_grid_cache.count;i++){m=&rb_grid_cache.meshes[i];assert(m->mesh.numvertexes<=MAX_INDICIES&&m->capacity<=MAX_INDICIES);vertices+=m->mesh.numvertexes;indices+=m->capacity;}assert(vertices==n*8&&indices==n*14&&rb_grid_cache.count>=4);CSQC_ReyondBroomClearMeshes();assert(alive==0);
 qrenderer=2;assert(!RB_CacheGrid(&vm,faces,128,n,4));qrenderer=1;assert(!RB_CacheGrid(&vm,faces,128,500001,4));faces[0].vertices=vm.stringtablesize-1;assert(!RB_CacheGrid(&vm,faces,128,1,4));assert(alive==0);
 free(vm.stringtable);puts("PASS: retained attributes, index reuse/update, odd-vertex alignment, epoch rebuild, 20,000-face chunk limits, renderer fallback and complete resource release");
}
'''
with tempfile.TemporaryDirectory() as folder:
    file = Path(folder) / 'test.c'
    exe = Path(folder) / 'test.exe'
    file.write_text(shim + layout + '\n' + code + checks)
    subprocess.run([a.compiler, '-std=c99', '-O2', str(file), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
