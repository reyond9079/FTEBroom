"""Production overflow batches must remain address-stable and never omit geometry."""
from pathlib import Path
import argparse,subprocess,tempfile
p=argparse.ArgumentParser();p.add_argument('--engine',type=Path,required=True);p.add_argument('--compiler',default='cc');a=p.parse_args()
s=(a.engine/'gl/gl_backend.c').read_text()
helper=s[s.index('typedef struct rb_batch_overflow_s'):s.index('#ifdef RTLIGHTS',s.index('typedef struct rb_batch_overflow_s'))]
fn=s[s.index('batch_t *GLBE_GetTempBatch(void)'):s.index('/*called from shadowmapping code*/',s.index('batch_t *GLBE_GetTempBatch(void)'))]
code="""#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define BZ_Malloc malloc
#define BZ_Free free
typedef struct {void *fog;unsigned int tag;} batch_t;
static struct {int wbatch,maxwbatches;batch_t *wbatches;} shaderstate;
"""+helper+fn+"""
int main(void){
shaderstate.maxwbatches=2;shaderstate.wbatches=calloc(2,sizeof(batch_t));
batch_t **refs=calloc(100000,sizeof(*refs));
for(int i=0;i<100000;i++){refs[i]=GLBE_GetTempBatch();assert(refs[i]);refs[i]->tag=i+1;}
for(int i=0;i<100000;i++)assert(refs[i]->tag==(unsigned int)i+1);
assert(shaderstate.wbatch==100000);RB_ClearBatchOverflow();assert(!rb_batch_overflow&&!rb_batch_current);
free(refs);free(shaderstate.wbatches);puts("PASS 100,000 renderer batches: no omissions, stable pointers and complete overflow release");}
"""
with tempfile.TemporaryDirectory() as d:
 src=Path(d)/'pool.c';exe=Path(d)/'pool.exe';src.write_text(code)
 subprocess.run([a.compiler,'-std=c99','-O2',str(src),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
