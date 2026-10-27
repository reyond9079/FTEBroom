"""Production native attribute builders must match the existing scalar formulas."""
import runpy
import subprocess
import tempfile
from pathlib import Path

shared = runpy.run_path(str(Path(__file__).with_name('test-cached-faces.py')))
s, a = shared['s'], shared['a']
face = s[s.index('typedef struct {\n int node,count,brushEnd,shader;'):s.index('typedef struct {unsigned int grid,gridCount,faces,faceCount,epoch;}')]
code = s[s.index('// Bulk attribute preparation'):s.index('static void RB_IncrementalMarker')]
extras = r'''
#define VectorClear(a) memset(a,0,12)
#define VectorAdd(a,b,c) do{for(int k=0;k<3;k++)(c)[k]=(a)[k]+(b)[k];}while(0)
#define VectorLength(a) sqrt(DotProduct(a,a))
'''
checks = r'''
int main(void){
 pubprogfuncs_t vm={calloc(1,65536),65536};struct globalvars_s globals;struct globalvars_s *pr_globals=&globals;memset(&globals,0,sizeof(globals));
 editorqcface_t *f=(editorqcface_t*)(vm.stringtable+128);vec3_t *source=(vec3_t*)(vm.stringtable+1024),*p=(vec3_t*)(vm.stringtable+32768);
 f->points=2048;f->uv=4096;f->vertices=8192;f->triangles=16384;
 for(int n=3;n<9;n++){
  for(int i=0;i<n;i++){source[i][0]=i*13.25f;source[i][1]=(i&1)?-17.5f:48;source[i][2]=i*.125f;}
  p[0][0]=1;p[0][1]=.2f;p[1][1]=-1;p[1][2]=.1f;p[2][0]=.125f;p[2][1]=-.03125f;p[3][0]=128;p[3][1]=64;p[3][2]=.35f;p[4][0]=.65f;p[4][1]=.68f;p[4][2]=.72f;
  G_INT(OFS_PARM0)=1024;G_INT(OFS_PARM1)=n;G_INT(OFS_PARM2)=128;G_INT(OFS_PARM3)=32768;PF_ReyondBroom_BuildFace(&vm,pr_globals);assert(G_INT(OFS_RETURN)==1);
  qcvertex_t *vertices=(qcvertex_t*)(vm.stringtable+8192);vec3_t center={0};float radius=0;
  for(int i=0;i<n;i++){for(int k=0;k<3;k++){center[k]+=source[i][k];assert(vertices[i].xyz[k]==source[i][k]);assert(vertices[i].rgba[k]==p[4][k]);}assert(vertices[i].rgba[3]==.35f);assert(vertices[i].st[0]==(DotProduct(source[i],p[0])+p[2][0])/128);assert(vertices[i].st[1]==(DotProduct(source[i],p[1])+p[2][1])/64);}
  for(int k=0;k<3;k++){center[k]/=(float)n;assert(center[k]==f->center[k]);}
  for(int i=0;i<n;i++){vec3_t delta;VectorSubtract(source[i],center,delta);float value=VectorLength(delta);if(value>radius)radius=value;}assert(radius==f->radius);
 }
 G_INT(OFS_PARM1)=65;PF_ReyondBroom_BuildFace(&vm,pr_globals);assert(G_INT(OFS_RETURN)==0);
 rb_grid_face_t *grid=(rb_grid_face_t*)(vm.stringtable+40000);grid->count=4;grid->points=1024;grid->vertices=8192;grid->normal[2]=1;
 memset(p,0,6*sizeof(vec3_t));p[0][0]=12;p[1][0]=p[2][1]=p[3][2]=1;p[4][0]=16;p[4][1]=.4f;p[5][0]=.8660254f;p[5][1]=-.5f;
 G_INT(OFS_PARM0)=40000;G_INT(OFS_PARM1)=1;G_INT(OFS_PARM2)=32768;PF_ReyondBroom_BuildGrid(&vm,pr_globals);assert(G_INT(OFS_RETURN)==1&&grid->u==0&&grid->v==1);
 qcvertex_t *v=(qcvertex_t*)(vm.stringtable+8192);
 for(int i=0;i<4;i++){vec3_t delta;VectorSubtract(source[i],p[0],delta);float x=delta[0]*p[5][0]-delta[1]*p[5][1],y=delta[0]*p[5][1]+delta[1]*p[5][0];assert(v[i].st[0]==x/16&&v[i].st[1]==y/16);assert(v[i].xyz[2]==source[i][2]+.02f&&v[i+4].rgba[0]==1&&v[i].rgba[3]==.4f);}
 grid->vertices=65535;PF_ReyondBroom_BuildGrid(&vm,pr_globals);assert(G_INT(OFS_RETURN)==0);
 free(vm.stringtable);puts("PASS scalar-equivalent face UV/color/centers/radii and rotated grid attributes; invalid ranges rejected");
}
'''
with tempfile.TemporaryDirectory() as folder:
    source, exe = Path(folder)/'test.c', Path(folder)/'test.exe'
    source.write_text(shared['shared']['shim'] + shared['shared']['layout'] + '\n' + face + shared['shim'] + extras + code + checks)
    subprocess.run([a.compiler, '-std=c99', '-O2', str(source), '-lm', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
