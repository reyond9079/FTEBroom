# SPDX-License-Identifier: GPL-2.0-or-later
"""Integration test: python tests/gpu-light.py HELPER SMALL_BSP."""
from pathlib import Path
import hashlib,struct,subprocess,sys,tempfile
helper,source=map(lambda x:Path(x).resolve(),sys.argv[1:3])
def lumps(p):
 b=p.read_bytes();return [b[o:o+n] for o,n in [struct.unpack_from('<II',b,4+i*8) for i in range(15)]]
before=hashlib.sha256(source.read_bytes()).digest()
with tempfile.TemporaryDirectory(prefix='rb-gpu-') as tmp:
 p=Path(tmp); outputs=[]
 for name,rays in [('direct',0),('bounce',32),('repeat',32)]:
  out=p/(name+'.bsp');r=subprocess.run([str(helper),str(source),str(out),str(rays)],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr;print(r.stdout)
  a,b=lumps(source),lumps(out)
  for i in range(15):
   if i not in (7,8):assert a[i]==b[i],f'Geometry/entity lump changed: {i}'
  lit=out.with_suffix('.lit').read_bytes();assert lit[:8]==b'QLIT\x01\0\0\0' and len(lit)==8+len(b[8])*3
  outputs.append(lit[8:])
 assert outputs[1]==outputs[2],'Bake is not deterministic'
 assert all(a>=b for a,b in zip(outputs[1],outputs[0])),'Bounce removed direct light'
 assert any(a>b for a,b in zip(outputs[1],outputs[0])),'No bounced light'
 assert subprocess.run([str(helper),str(source),str(p/'bounce.bsp'),'32'],capture_output=True).returncode!=0,'Existing output overwritten'
 bad=p/'bad.bsp';bad.write_bytes(b'bad');assert subprocess.run([str(helper),str(bad),str(p/'bad-out.bsp')],capture_output=True).returncode!=0
assert before==hashlib.sha256(source.read_bytes()).digest(),'Source was modified'
print('GPU_LIGHT_PASS: immutable geometry/source, valid LIT, additive bounce, deterministic, failures handled')
