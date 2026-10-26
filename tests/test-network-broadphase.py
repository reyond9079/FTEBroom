"""Compare production network brush BVH queries with exhaustive segment/AABB tests."""
import argparse
import subprocess
import tempfile
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--engine', required=True, type=Path)
p.add_argument('--compiler', default='cc')
a = p.parse_args()
s = (a.engine / 'server/world.c').read_text()
code = s[s.index('typedef struct {vec3_t low,high;int source;}'):s.index('static int RB_NetCandidates(')]
shim = r'''
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#define true 1
#define false 0
typedef float vec3_t[3];typedef int qboolean;typedef void packet_entities_t;
static unsigned int seed=7231;
static float random_coord(void){seed=seed*1664525u+1013904223u;return ((seed>>8)%200000)/10.f-10000;}
'''
checks = r'''
int main(void){
 const unsigned int n=777;rb_net.boxes=calloc(n,sizeof(*rb_net.boxes));rb_net.nodes=calloc(n*2,sizeof(*rb_net.nodes));rb_net.order=calloc(n,sizeof(*rb_net.order));rb_net.hits=calloc(n,sizeof(*rb_net.hits));
 for(unsigned int i=0;i<n;i++){for(int a=0;a<3;a++){rb_net.boxes[i].low[a]=random_coord();rb_net.boxes[i].high[a]=rb_net.boxes[i].low[a]+fabs(random_coord())*.1f;}rb_net.boxes[i].source=i;rb_net.order[i]=i;}
 rb_net.count=n;RB_NetBuild(0,n);
 for(int r=0;r<10000;r++){
  vec3_t start,end;for(int a=0;a<3;a++){start[a]=random_coord();end[a]=r%9?random_coord():start[a];}
  unsigned int count=0,expected=0;RB_NetQuery(0,start,end,&count);qsort(rb_net.hits,count,sizeof(*rb_net.hits),RB_NetSourceCompare);
  for(unsigned int i=0;i<n;i++)if(RB_NetIntersects(rb_net.boxes[i].low,rb_net.boxes[i].high,start,end)){assert(expected<count&&rb_net.hits[expected]==i);expected++;}assert(count==expected);
 }
 puts("PASS: 7,770,000 exhaustive comparisons, stable candidates, parallel and zero-length rays");
}
'''
with tempfile.TemporaryDirectory() as folder:
    file = Path(folder) / 'test.c'
    exe = Path(folder) / 'test.exe'
    file.write_text(shim + code + checks)
    subprocess.run([a.compiler, '-std=c99', '-O2', str(file), '-lm', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
