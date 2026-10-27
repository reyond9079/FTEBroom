"""Exercise the production mutable EBO transition and reuse without a GL context."""
import argparse
import subprocess
import tempfile
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--engine', type=Path, required=True)
p.add_argument('--compiler', default='cc')
a = p.parse_args()
s = (a.engine / 'gl/gl_backend.c').read_text()
code = s[s.index('void GLBE_BeginEditorMeshUpload('):s.index('// Retained editor meshes keep their attribute bindings')] + s[s.index('qboolean GLBE_StreamEditorIndices('):s.index('void GLBE_DestroyEditorMesh(')]
shim = r'''
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
typedef int qboolean;
typedef unsigned int index_t;
typedef struct {struct {unsigned int vbo;void *addr;} gl;} vboarray_t;
#define true 1
#define false 0
#define GL_ELEMENT_ARRAY_BUFFER_ARB 1
#define GL_STREAM_DRAW_ARB 2
static int destroys,generated,selected,deselected,uploads;static struct {unsigned int currentebo;int currentvbo;} shaderstate;static unsigned int boundVao=9,elementBindings[10];
static size_t bytes;
static const void *last;
static void GL_DeselectVAO(void){boundVao=0;deselected++;}
static void GL_SelectVBO(int id){assert(id==0);shaderstate.currentvbo=id;}
static void GLBE_VBO_Destroy(vboarray_t *v,void *memory){assert(memory==(void*)17);destroys++;v->gl.vbo=0;}
static void qglBindBufferARB(int target,unsigned int id){assert(target==GL_ELEMENT_ARRAY_BUFFER_ARB&&(id==44||id==0));elementBindings[boundVao]=id;selected++;}
static void gen(int count,unsigned int *id){assert(count==1);generated++;*id=44;}
static void data(int target,size_t size,const void *indices,int usage){assert(target==1&&usage==2);bytes=size;last=indices;uploads++;}
static void (*qglGenBuffersARB)(int,unsigned int*)=gen;
static void (*qglBufferDataARB)(int,size_t,const void*,int)=data;
'''
checks = r'''
int main(void){
 elementBindings[9]=77;shaderstate.currentebo=77;GLBE_BeginEditorMeshUpload();assert(boundVao==0&&elementBindings[9]==77&&elementBindings[0]==0&&shaderstate.currentebo==0&&shaderstate.currentvbo==0);selected=deselected=0;
 vboarray_t buffer={{12,(void*)99}};void *memory=(void*)17;index_t indices[]={1,2,3,4,5,6};
 assert(GLBE_StreamEditorIndices(&buffer,&memory,indices,6,0));
 assert(destroys==1&&generated==1&&buffer.gl.vbo==44&&!buffer.gl.addr&&!memory&&bytes==sizeof(indices)&&last==indices);
 elementBindings[0]=22;shaderstate.currentebo=44;assert(GLBE_StreamEditorIndices(&buffer,&memory,indices,3,1));assert(elementBindings[0]==44&&elementBindings[9]==77);
 assert(destroys==1&&generated==1&&uploads==2&&selected==2&&deselected==2&&bytes==3*sizeof(index_t));
 qglBufferDataARB=NULL;assert(!GLBE_StreamEditorIndices(&buffer,&memory,indices,3,1));assert(uploads==2);
 qglBufferDataARB=data;qglGenBuffersARB=NULL;assert(!GLBE_StreamEditorIndices(&buffer,&memory,indices,3,1));assert(uploads==2);
 puts("PASS: one immutable-to-mutable EBO transition, retained buffer reuse, preserved foreign VAO bindings, exact upload size and unsupported-API fallback");
}
'''
with tempfile.TemporaryDirectory() as folder:
    c, exe = Path(folder) / 'test.c', Path(folder) / 'test.exe'
    c.write_text(shim + code + checks)
    subprocess.run([a.compiler, '-std=c99', str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
