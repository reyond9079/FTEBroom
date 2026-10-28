from pathlib import Path
import subprocess,zipfile,json,hashlib
import argparse
p=argparse.ArgumentParser();p.add_argument('--sdk',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
r=Path(__file__).resolve().parents[1];sdk=a.sdk.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
for game in ['valve','cstrike']:
 payload={}
 for base in [sdk/'base',sdk/'valve']+([sdk/'cstrike'] if game=='cstrike' else []):
  for p in sorted(base.rglob('*')):
   if not p.is_file():continue
   rel=p.relative_to(base);name=rel.as_posix()
   if base==sdk/'base' and rel.parts[0] not in ['fonts','gfx','glsl','particles','scripts','resource']:continue
   if any(part.startswith('.') for part in rel.parts) or rel.parts[0] in ['src','Documentation','mapsrc','img','maps','modelsrc','music'] or p.suffix.lower() in ['.lno','.sh','.sha256','.ico','.h','.xcf']:continue
   if name in ['PROJECT','DEPENDS','PLUGINS','PAK_NAME']:continue
   payload[name]=p.read_bytes()
 payload['reyondbroom-halflife.fgd']=(r/'runtime/halflife.fgd').read_bytes()
 if game=='cstrike':payload['reyondbroom-cstrike.fgd']=(r/'runtime/counterstrike.fgd').read_bytes().replace(b'"halflife.fgd"',b'"reyondbroom-halflife.fgd"')
 payload['reyondbroom-runtime.json']=json.dumps({name:subprocess.check_output(['git','rev-parse','HEAD'],cwd=path,text=True).strip() for name,path in [('nuclide',sdk),('freehl',sdk/'valve'),('freecs',sdk/'cstrike')]}).encode()
 for name,base in [('nuclide',sdk),('freehl',sdk/'valve'),('freecs',sdk/'cstrike')]:payload['reyondbroom-licenses/'+name+'.txt']=(base/'COPYING').read_bytes()
 target=out/('reyondbroom-'+('freehl' if game=='valve' else 'freecs')+'-94.pk3')
 with zipfile.ZipFile(target,'w',compression=zipfile.ZIP_DEFLATED) as z:
  for name,data in sorted(payload.items()):
   info=zipfile.ZipInfo(name,date_time=(1980,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.create_system=3;info.external_attr=0o100644<<16;z.writestr(info,data)
 print(target.name,target.stat().st_size,hashlib.sha256(target.read_bytes()).hexdigest())
