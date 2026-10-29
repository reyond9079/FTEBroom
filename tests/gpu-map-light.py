# SPDX-License-Identifier: GPL-2.0-or-later
"""Deterministic direct-MAP GPU regression fixtures; no BSP compiler is used."""
from pathlib import Path
import struct,subprocess,tempfile,sys,json
helper=Path(sys.argv[1]).resolve()
root=Path(sys.argv[2]).resolve() if len(sys.argv)>2 else Path(tempfile.mkdtemp(prefix='rb-map-gpu-'))
root.mkdir(exist_ok=True,parents=True)
def face(key,poly,normal=(0,0,1),name='floor'):
    import math
    uv=(1/16,0,0,2,0,-1/16,0,2) if name=='{mask' else (1,0,0,0,0,-1,0,0)
    us=[sum(p[i]*uv[i] for i in range(3))+uv[3] for p in poly];vs=[sum(p[i]*uv[4+i] for i in range(3))+uv[7] for p in poly]
    bx,by=math.floor(min(us)/16),math.floor(min(vs)/16);w,h=math.ceil(max(us)/16)-bx+1,math.ceil(max(vs)/16)-by+1
    name=name.encode();b=struct.pack('<QI4f8fIiiIII',key,len(poly),*normal,poly[0][2],*uv,16,bx,by,w,h,len(name))+name
    for p in poly:b+=struct.pack('<3f',*p)
    return b,(w,h)
def tga(path,masked=False):
    b=bytearray(18);b[2]=2;b[12]=4;b[14]=4;b[16]=32;b[17]=40
    for y in range(4):
        for x in range(4):b+=bytes([255,255,255,0 if masked and x<2 else 255])
    path.write_bytes(b)
tga(root/'white.tga');tga(root/'mask.tga',True)
(root/'fixture.map').write_text('{\n"classname" "worldspawn"\n}\n{\n"classname" "light"\n"origin" "0 0 96"\n"light" "300"\n}\n')
items=[face(1,[(-64,-64,0),(0,-64,0),(0,64,0),(-64,64,0)]),face(2,[(0,-64,0),(64,-64,0),(64,64,0),(0,64,0)]),face(3,[(-32,-32,48),(32,-32,48),(32,32,48),(-32,32,48)],name='{mask'),face(4,[(-128,-128,128),(128,-128,128),(128,128,128),(-128,128,128)],name='skytest')]
(root/'scene.rbgs').write_bytes(b'RBGS103\0'+struct.pack('<I',len(items))+b''.join(b for b,_ in items))
def read(path):
    b=path.read_bytes();assert b[:8]==b'RBLM103\0';at=12;out=[]
    for _,(w,h) in items:
        key,n=struct.unpack_from('<QI',b,at);at+=12;assert n==w*h*3;out.append(b[at:at+n]);at+=n
    assert at==len(b);return out
def run(name,masked,rays=0,samples=1):
    manifest=root/(name+'.txt');manifest.write_text(f'"floor" "{(root/"white.tga").as_posix()}" 0 1\n"{{mask" "{(root/"mask.tga").as_posix()}" {masked} 1\n')
    output=root/(name+'.rblm');p=subprocess.run([str(helper),'--map',str(root/'scene.rbgs'),str(root/'fixture.map'),str(output),str(rays),str(manifest),str(samples)],capture_output=True,text=True);(root/(name+'.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr;return read(output)
masked=run('masked',1);opaque=run('opaque',0)
# The last column of the left polygon is the first column of the right one.
w0,h=items[0][1];w1,_=items[1][1]
seam=max(abs(masked[0][(y*w0+w0-1)*3+c]-masked[1][(y*w1)*3+c]) for y in range(h) for c in range(3));assert seam==0,seam
left=(4*w0+3)*3;right=(4*w1+1)*3
assert masked[0][left]>opaque[0][left]+50,(masked[0][left],opaque[0][left])
assert masked[1][right]==opaque[1][right]==0,(masked[1][right],opaque[1][right])
# A one-sided panel must not reflect its front-face irradiance through its back.
bounce=run('bounce',1,8,4);assert bounce[1][right]==0,bounce[1][right]
(root/'fixture.map').write_text('{\n"classname" "worldspawn"\n}\n{\n"classname" "light"\n"origin" "0 0 100"\n"light" "100"\n"_sun" "1"\n"target" "sun_target"\n}\n{\n"classname" "info_null"\n"targetname" "sun_target"\n"origin" "0 0 0"\n}\n')
sun=run('sun',1);assert sun[0][left]==100 and sun[1][right]==0,(sun[0][left],sun[1][right])
# Failures must not overwrite a completed sidecar.
before=(root/'masked.rblm').read_bytes();p=subprocess.run([str(helper),'--map',str(root/'scene.rbgs'),str(root/'fixture.map'),str(root/'masked.rblm'),'0'],capture_output=True);assert p.returncode!=0 and (root/'masked.rblm').read_bytes()==before
proof={'seam_max_rgb_difference':seam,'masked_open_rgb':masked[0][left],'opaque_shadow_rgb':opaque[0][left],'masked_closed_rgb':masked[1][right],'existing_output_preserved':True,'sun_open_rgb':sun[0][left],'sun_shadow_rgb':sun[1][right],'no_backface_light_leak':bounce[1][right]==0,'bsp_files':list(map(str,root.glob('*.bsp')))};assert not proof['bsp_files'];(root/'results.json').write_text(json.dumps(proof,indent=2));print(json.dumps(proof,indent=2))
