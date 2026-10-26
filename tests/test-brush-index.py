"""Compile the actual patched engine cache against a small allocation/model shim."""
import argparse,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--engine',type=Path,required=True);p.add_argument('--compiler',default='cc');a=p.parse_args()
source=(a.engine/'gl/gl_heightmap.c').read_text();code=source.split('// ReyondBroom brush index begin:',1)[1].split('// ReyondBroom brush index end.',1)[0];code=code[code.index('\ntypedef'):]
shim='''#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
typedef struct {unsigned int id;} brushes_t;
typedef struct {brushes_t *wbrushes;unsigned int numbrushes;} heightmap_t;
static void Terr_TraceInvalidate(heightmap_t *hm) {(void)hm;}
static void Terr_TraceClear(heightmap_t *hm) {(void)hm;}
#define qboolean int
#define true 1
#define Con_DPrintf(...) ((void)0)
#define BZ_Malloc malloc
#define BZ_Free free
'''
test='''int main(void){heightmap_t models[12]={0};unsigned int m,i;
 for(m=0;m<12;m++){heightmap_t *hm=&models[m];hm->numbrushes=20000;hm->wbrushes=malloc(20001*sizeof(brushes_t));for(i=0;i<20000;i++)hm->wbrushes[i].id=(i*104729u)^((m+1)<<24);}
 for(m=0;m<12;m++){heightmap_t *hm=&models[m];for(i=0;i<20000;i++)assert(Terr_BrushIndex_Find(hm,hm->wbrushes[i].id)==i);assert(Terr_BrushIndex_Find(hm,0xfffffffeu)==20000);}
 for(m=0;m<12;m++){heightmap_t *hm=&models[m];unsigned int removed=hm->wbrushes[73].id;Terr_BrushIndex_Invalidate(hm);hm->wbrushes[73]=hm->wbrushes[--hm->numbrushes];assert(Terr_BrushIndex_Find(hm,removed)==hm->numbrushes);assert(Terr_BrushIndex_Find(hm,hm->wbrushes[73].id)==73);
 // Same-count delete/append and storage relocation must not retain stale slots.
 hm->wbrushes=realloc(hm->wbrushes,20002*sizeof(brushes_t));hm->wbrushes[hm->numbrushes++].id=removed;Terr_BrushIndex_Invalidate(hm);assert(Terr_BrushIndex_Find(hm,removed)==19999);
 Terr_BrushIndex_Invalidate(hm);hm->wbrushes[17].id=hm->wbrushes[2].id;assert(Terr_BrushIndex_Find(hm,hm->wbrushes[2].id)==2);
 Terr_BrushIndex_Clear(hm);free(hm->wbrushes);hm->wbrushes=NULL;hm->numbrushes=0;assert(Terr_BrushIndex_Find(hm,123)==0);Terr_BrushIndex_Clear(hm);}
 puts("PASS 240000 exact lookups, misses, eviction, removal/swap, same-count replacement, relocation, duplicate IDs and freed/reused models");return 0;}
'''
with tempfile.TemporaryDirectory() as folder:
 root=Path(folder);file=root/'index.c';file.write_text(shim+code+test);binary=root/'test.exe'
 subprocess.run([a.compiler,'-std=c99','-O2',str(file),'-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
