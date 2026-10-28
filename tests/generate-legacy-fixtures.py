from pathlib import Path
import shutil,struct,subprocess,json
import argparse
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
r=Path(__file__).resolve().parents[1];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
def box(a,b,tex,flags=''):
 x,y,z=a;X,Y,Z=b
 planes=[[(x,Y,Z),(x,y,Z),(x,y,z)],[(X,y,z),(X,y,Z),(X,Y,Z)],[(X,y,Z),(X,y,z),(x,y,z)],[(x,Y,z),(X,Y,z),(X,Y,Z)],[(X,Y,z),(x,Y,z),(x,y,z)],[(x,y,Z),(x,Y,Z),(X,Y,Z)]]
 axes=['[ 0 1 0 0 ] [ 0 0 -1 0 ]','[ 0 1 0 0 ] [ 0 0 -1 0 ]','[ 1 0 0 0 ] [ 0 0 -1 0 ]','[ 1 0 0 0 ] [ 0 0 -1 0 ]','[ 1 0 0 0 ] [ 0 -1 0 0 ]','[ 1 0 0 0 ] [ 0 -1 0 0 ]']
 return '{\n'+''.join(' '.join('( %g %g %g )'%p for p in ps)+' '+tex+' '+axes[i]+' 0 1 1'+flags+'\n' for i,ps in enumerate(planes))+'}\n'
walls=[((-144,-144,-16),(144,144,0)),((-144,-144,256),(144,144,272)),((-144,-144,0),(-128,144,256)),((128,-144,0),(144,144,256)),((-128,-144,0),(128,-128,256)),((-128,128,0),(128,144,256))]
for game in ['q2','hl']:
 p=out/game;p.mkdir(exist_ok=True);(p/'maps').mkdir(exist_ok=True)
 if game=='q2':
  (p/'textures').mkdir(exist_ok=True);shutil.copy2(r/'runtime/textures/fb_grey.wal',p/'textures/fb_grey.wal')
  tex='fb_grey';wad='';flags=' 1 0 0'
  (p/'pics').mkdir(exist_ok=True)
  pcx=bytearray(128);pcx[:4]=bytes([10,5,1,8]);struct.pack_into('<4H',pcx,4,0,0,15,15);pcx[65]=1;struct.pack_into('<H',pcx,66,16)
  (p/'pics/colormap.pcx').write_bytes(pcx+bytes([160]*256)+bytes([12])+b''.join(bytes([i,i,i]) for i in range(256)))
 else:shutil.copy2(r/'runtime/halflife-editor.wad',p/'textures.wad');tex='fb_grey';wad='"mapversion" "220"\n"wad" "textures.wad"\n';flags=''
 source='// ReyondBroom profile='+('quake2' if game=='q2' else 'halflife')+' format=valve220\n{\n"classname" "worldspawn"\n'+wad+''.join(box(a,b,tex,flags) for a,b in walls)+'}\n{\n"classname" "info_player_start"\n"origin" "0 0 32"\n}\n{\n"classname" "light"\n"origin" "0 0 192"\n"light" "300"\n}\n'
 (p/'maps/room.map').write_text(source,newline='\n')

out=out/'models';out.mkdir(exist_ok=True)
def md2():
 b=bytearray(68)
 def add(data):at=len(b);b.extend(data);return at
 skins=add(b'models/qa/skin94.pcx'+bytes(64-20))
 st=add(struct.pack('<6h',0,0,15,0,0,15));tri=add(struct.pack('<6H',0,1,2,0,1,2))
 frames=add(struct.pack('<6f16s',1,1,1,-8,-8,0,b'idle')+bytes([0,0,0,0,16,0,0,0,0,0,32,0]))
 struct.pack_into('<4s16I',b,0,b'IDP2',8,16,16,52,1,3,3,1,0,1,skins,st,tri,frames,len(b),len(b));return b
def studio():
 b=bytearray(244)
 def add(n):at=len(b);b.extend(bytes(n));return at
 def u(at,*v):struct.pack_into('<'+'I'*len(v),b,at,*v)
 def f(at,*v):struct.pack_into('<'+'f'*len(v),b,at,*v)
 def name(at,s):b[at:at+len(s)]=s
 bones=add(224)
 for i,n in enumerate([b'root',b'tip']):
  at=bones+i*112;name(at,n);struct.pack_into('<i',b,at+32,i-1);struct.pack_into('<6i',b,at+40,*([-1]*6));f(at+64,0,0,32*i,0,0,0)
 seq=add(176);name(seq,b'idle');f(seq+32,10);u(seq+36,1);u(seq+56,2);f(seq+96,-8,-8,0,8,8,32);u(seq+120,1)
 group=add(104);anim=add(24);u(seq+124,anim)
 textures=add(80);name(textures,b'skin94');u(textures+68,16,16)
 skins=add(2);body=add(76);name(body,b'body');u(body+64,1,1)
 model=add(112);name(model,b'triangle');u(body+72,model)
 mesh=add(20);u(model+72,1,mesh,3)
 vertbones=add(3);b[vertbones:vertbones+3]=bytes([0,0,1]);u(model+84,vertbones)
 vertices=add(36);f(vertices,-8,-8,0,8,-8,0,0,8,0);u(model+88,vertices)
 normbones=add(1);normals=add(12);f(normals,0,-1,0);u(model+92,1,normbones,normals)
 command=add(2+3*8+2);struct.pack_into('<h',b,command,3)
 for i,(s,t) in enumerate([(0,0),(15,0),(0,15)]):struct.pack_into('<4h',b,command+2+i*8,i,0,s,t)
 u(mesh,1,command,0,1,0)
 pixels=add(16*16+768);b[pixels:pixels+256]=bytes([160]*256);b[pixels+256:pixels+1024]=b''.join(bytes([i,i,i]) for i in range(256));u(textures+76,pixels)
 b[:4]=b'IDST';u(4,10);name(8,b'qa studio model');u(72,len(b));f(76,0,0,28);f(88,-8,-8,0);f(100,8,8,32);f(112,-8,-8,0);f(124,8,8,32)
 u(140,2,bones,0,0,0,0,1,seq,1,group,1,textures,0,1,1,skins,1,body)
 return b
(out/'test94.md2').write_bytes(md2());(out/'studio94.mdl').write_bytes(studio())
(out/'bones94.md5mesh').write_text('''MD5Version 10
commandline ""
numJoints 2
numMeshes 1
joints {
 "root" -1 ( 0 0 0 ) ( 0 0 0 )
 "tip" 0 ( 0 0 32 ) ( 0 0 0 )
}
mesh {
 shader "ftebroom/grid"
 numverts 3
 vert 0 ( 0 0 ) 0 1
 vert 1 ( 1 0 ) 1 1
 vert 2 ( 0 1 ) 2 1
 numtris 1
 tri 0 0 1 2
 numweights 3
 weight 0 0 1 ( -8 -8 0 )
 weight 1 0 1 ( 8 -8 0 )
 weight 2 1 1 ( 0 8 0 )
}
''',newline='\n')
