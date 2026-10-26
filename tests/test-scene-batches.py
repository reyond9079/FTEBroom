"""Ensure a large CSQC scene gets its full batch pool on the first frame."""
import argparse
import subprocess
import tempfile
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--engine', type=Path, required=True)
p.add_argument('--compiler', default='cc')
a = p.parse_args()
s = (a.engine / 'gl/gl_backend.c').read_text()
start = s.index('//reset batches if we needed more mem')
reserve = s[start:s.index('//if the video mode changed', start)]
start = s.index('batch_t *GLBE_GetTempBatch(void)')
get_batch = s[start:s.index('/*called from shadowmapping code*/', start)]
shim = r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define CSQC_DAT
#define max(a,b) ((a)>(b)?(a):(b))
#define BZ_Realloc realloc
typedef struct {void *fog;int marker;} batch_t;
static struct {int recurse;} r_refdef;
static struct {int wbatch,maxwbatches;batch_t *wbatches;} shaderstate;
static int cl_numstris;
'''
checks = r'''
int main(void){
 cl_numstris=50000;reserve();assert(shaderstate.maxwbatches>=50100);
 for(int i=0;i<50000;i++){batch_t *b=GLBE_GetTempBatch();assert(b);b->marker=i;}
 for(int i=0;i<50000;i++)assert(shaderstate.wbatches[i].marker==i);
 cl_numstris=150000;reserve();assert(shaderstate.wbatch==0&&shaderstate.maxwbatches>=150100);
 for(int i=0;i<150000;i++)assert(GLBE_GetTempBatch());
 r_refdef.recurse=1;int before=shaderstate.wbatch;reserve();assert(shaderstate.wbatch==before);
 r_refdef.recurse=0;cl_numstris=0;reserve();assert(shaderstate.wbatch==0);
 free(shaderstate.wbatches);memset(&shaderstate,0,sizeof(shaderstate));reserve();assert(shaderstate.maxwbatches==100);
 for(int i=0;i<100;i++)assert(GLBE_GetTempBatch());assert(!GLBE_GetTempBatch());reserve();assert(shaderstate.maxwbatches>=201);
 free(shaderstate.wbatches);puts("PASS: complete 50,000/150,000-batch scenes on first frame, recursion, ordinary growth and exhaustion");
}
'''
with tempfile.TemporaryDirectory() as folder:
    file, exe = Path(folder) / 'test.c', Path(folder) / 'test.exe'
    file.write_text(shim + 'static void reserve(void){\n' + reserve + '}\n' + get_batch + checks)
    subprocess.run([a.compiler, '-std=c99', '-O2', str(file), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
