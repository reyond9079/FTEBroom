"""Exercise production snapshot marking and VM-thread sweeping."""
import argparse, subprocess, tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--engine',type=Path,required=True);p.add_argument('--compiler',default='cc');a=p.parse_args()
s=(a.engine/'qclib/initlib.c').read_text()
mark=s[s.index('static smallbool *PR_QCGC_Mark('):s.index('static size_t PR_QCGC_Sweep(')]
start=s.index('static size_t PR_QCGC_Sweep(');sweep=s[start:s.index('\n#ifdef THREADEDGC',start)]
start=s.index('struct qcgccontext_s');end=s.index('\n#endif',s.index('void PR_QCGC_Thread(',start));worker=s[start:end]
prefix=r"""
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef char smallbool;
#define true 1
#define false 0
#define STRING_SPECMASK 0xf0000000u
#define STRING_TEMP 0x80000000u
typedef struct {char value[8];} tempstr_t;
static int on_vm, released;
static void vm_free(void *p){assert(on_vm);released++;free(p);}
typedef struct {tempstr_t **tempstrings;void (*memfree)(void *);} progfuncs_t;
#define prinst (*progfuncs)
#define externs progfuncs
#define WG_MAIN 0
typedef void (*workfn)(void*,void*,size_t,size_t);
static workfn pending;static void *pending_ctx;
static void COM_InsertWork(int group,workfn fn,void *ctx,void *data,size_t a,size_t b){assert(group==WG_MAIN);pending=fn;pending_ctx=ctx;}
"""
main=r"""
static void run(int changed){
 progfuncs_t vm={0};vm.memfree=vm_free;
 tempstr_t **snapshot=calloc(2,sizeof(*snapshot));
 tempstr_t **live=calloc(3,sizeof(*live));
 for(int i=0;i<2;i++)snapshot[i]=live[i]=calloc(1,sizeof(tempstr_t));
 live[2]=calloc(1,sizeof(tempstr_t));vm.tempstrings=live;
 struct qcgccontext_s *gc=calloc(1,sizeof(*gc)+16);
 gc->progfuncs=&vm;gc->memsize=4;gc->maxtemps=2;gc->tempstrings=snapshot;gc->amem[0]=STRING_TEMP;
 released=0;on_vm=0;pending=NULL;PR_QCGC_Thread(gc,NULL,0,0);
 assert(!released && !gc->done && pending && gc->marked[0] && !gc->marked[1]);
 on_vm=1;
 if(changed){free(live[1]);live[1]=calloc(1,sizeof(tempstr_t));
 // Ensure identity differs even when malloc would reuse the old address.
 if(live[1]==snapshot[1]){tempstr_t *hold=live[1];live[1]=calloc(1,sizeof(tempstr_t));free(hold);}}
 pending(pending_ctx,NULL,0,0);assert(gc->done);
 size_t count=PR_QCGC_Sweep(&vm,gc->marked,snapshot,2);
 assert(count==(changed?0:1) && released==(changed?0:1));
 assert(live[0] && live[2]);if(changed)assert(live[1]);else assert(!live[1]);
 for(int i=0;i<3;i++)free(live[i]);free(live);free(snapshot);free(gc);
}
int main(void){run(0);run(1);puts("PASS: worker only marks immutable snapshots; VM owns frees, changed slots and post-snapshot strings survive");}
"""
with tempfile.TemporaryDirectory() as d:
 path=Path(d);(path/'gc.c').write_text(prefix+mark+sweep+worker+main)
 exe=path/'gc.exe';subprocess.run([a.compiler,'-std=c11','-O2','-static',str(path/'gc.c'),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
