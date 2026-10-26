"""Check the production resend allocation with full-capacity writes and guards."""
import argparse
import subprocess
import tempfile
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--engine', type=Path, required=True)
p.add_argument('--compiler', default='cc')
a = p.parse_args()
s = (a.engine / 'server/sv_ents.c').read_text()
code = s[s.index('void SV_ExpandNackFrames('):s.index('//=============================================================================', s.index('void SV_ExpandNackFrames('))]
shim = r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define UPDATE_BACKUP 64
#define round64(size) (((size)+7)&~7)
#define min(a,b) ((a)<(b)?(a):(b))
typedef struct {unsigned int entnum,bits;uint64_t flags;} resend_t;
typedef struct {resend_t *resend;unsigned int maxresend,numresend;struct {int max_entities;} qwentities;double senttime;} client_frame_t;
typedef struct {unsigned int *pendingdeltabits;uint64_t *pendingcsqcbits;int max_net_ents;union {client_frame_t *frames;} frameunion;} client_t;
static double realtime=42;
static void *Z_Malloc(size_t size){size_t *p=calloc(1,sizeof(size_t)+size+64);*p=size;memset((char*)(p+1)+size,0xa5,64);return p+1;}
static void Z_Free(void *v){if(!v)return;size_t *p=(size_t*)v-1;for(int i=0;i<64;i++)assert(((unsigned char*)v)[*p+i]==0xa5);free(p);}
'''
checks = r'''
int main(void){
 client_t client={0};client.max_net_ents=32768;
 client.frameunion.frames=Z_Malloc(sizeof(client_frame_t)*UPDATE_BACKUP);
 client.pendingdeltabits=calloc(client.max_net_ents,sizeof(*client.pendingdeltabits));client.pendingcsqcbits=calloc(client.max_net_ents,sizeof(*client.pendingcsqcbits));
 for(int i=0;i<UPDATE_BACKUP;i++){client_frame_t *f=&client.frameunion.frames[i];f->maxresend=4;f->numresend=2;f->resend=calloc(4,sizeof(*f->resend));for(int j=0;j<4;j++)f->resend[j]=(resend_t){j,i,0x1234567800000000ull+i*16+j};}
 resend_t *old[UPDATE_BACKUP];for(int i=0;i<UPDATE_BACKUP;i++)old[i]=client.frameunion.frames[i].resend;
 unsigned int *oldDelta=client.pendingdeltabits;uint64_t *oldCSQC=client.pendingcsqcbits;
 client.pendingdeltabits[32000]=17;client.pendingcsqcbits[32000]=0xfedcba9876543210ull;
 client_frame_t *current=&client.frameunion.frames[23];SV_ExpandNackFrames(&client,5,&current);assert(current==&client.frameunion.frames[23]);
 free(oldDelta);free(oldCSQC);for(int i=0;i<UPDATE_BACKUP;i++)free(old[i]);
 for(int i=0;i<UPDATE_BACKUP;i++){client_frame_t *f=&client.frameunion.frames[i];assert(f->maxresend==10&&f->numresend==2&&f->senttime==42);for(int j=0;j<4;j++)assert(f->resend[j].entnum==j&&f->resend[j].bits==i&&f->resend[j].flags==0x1234567800000000ull+i*16+j);}
 for(int require=11;require<40000;require*=2){
  unsigned int previous=client.frameunion.frames[0].maxresend;
  for(int i=0;i<UPDATE_BACKUP;i++){client_frame_t *f=&client.frameunion.frames[i];for(unsigned int j=0;j<f->maxresend;j++)f->resend[j]=(resend_t){j,i,0xfedcba9800000000ull+j};f->numresend=previous/2;}
  SV_ExpandNackFrames(&client,require,&current);
  assert(current==&client.frameunion.frames[23]);
  for(int i=0;i<UPDATE_BACKUP;i++){client_frame_t *f=&client.frameunion.frames[i];assert(f->maxresend>=previous&&f->maxresend<=32768);for(unsigned int j=0;j<previous;j++)assert(f->resend[j].entnum==j&&f->resend[j].bits==i&&f->resend[j].flags==0xfedcba9800000000ull+j);}
  assert(client.pendingdeltabits[32000]==17&&client.pendingcsqcbits[32000]==0xfedcba9876543210ull);
 }
 for(int i=0;i<UPDATE_BACKUP;i++){client_frame_t *f=&client.frameunion.frames[i];for(unsigned int j=0;j<f->maxresend;j++)f->resend[j]=(resend_t){j,i,0xffffffffffffffffull};}
 Z_Free(client.frameunion.frames);puts("PASS: 64 resend histories, 32,768-record capacity, complete pending-entry preservation, frame relocation, delta flags and allocation guard");
}
'''
with tempfile.TemporaryDirectory() as folder:
    file = Path(folder) / 'test.c'
    exe = Path(folder) / 'test.exe'
    file.write_text(shim + code + checks)
    subprocess.run([a.compiler, '-std=c99', '-O2', str(file), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
