"""Stage built/pinned legacy tools, licenses and corresponding sources for packaging."""
from pathlib import Path
import argparse,shutil,zipfile,json,hashlib,subprocess
p=argparse.ArgumentParser()
for name in ('sdhlt','ericw-source','ericw-windows','ericw-linux','sdk','runtimes','msvc'):p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--output',type=Path)
a=p.parse_args();r=Path(__file__).resolve().parents[1];cache=a.output or r/'deps/legacy-games';cache.mkdir(parents=True,exist_ok=True)
sd=a.sdhlt.resolve();er=a.ericw_source.resolve();sdk=a.sdk.resolve()
def cp(source,group,name):
 target=cache/group/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
for platform,label in [('win64','Windows-x64'),('Linux','Linux-x64')]:
 folder=(a.ericw_windows if platform=='win64' else a.ericw_linux).resolve()
 names=['qbsp.exe','vis.exe','light.exe','embree3.dll','tbb.dll','tbbmalloc.dll','gpl_v3.txt','LICENSE-embree.txt'] if platform=='win64' else ['qbsp','vis','light','libembree3.so.3','libtbb.so.12','libtbbmalloc.so.2','gpl_v3.txt','LICENSE-embree.txt']
 for name in names:cp(folder/name,label,'tools/ericw-q2/'+name)
 if platform=='win64':
  for name in ['msvcp140.dll','vcruntime140.dll','vcruntime140_1.dll']:cp(a.msvc.resolve()/name,label,'tools/ericw-q2/'+name)
 for tool in ['CSG','BSP','VIS','RAD']:cp(sd/'tools'/('sdHL'+tool+('_x64.exe' if platform=='win64' else '')),label,'tools/sdhlt/sdHL'+tool+('.exe' if platform=='win64' else ''))
 for name in ['lights.rad','sdhlt.fgd','sdhlt.wad','settings.txt','wad.cfg']:cp(sd/'tools'/name,label,'tools/sdhlt/'+name)
for p in a.runtimes.resolve().glob('*.pk3'):cp(p,'common','tools/runtimes/'+p.name)
for name,folder in [('Nuclide',sdk),('FreeHL',sdk/'valve'),('FreeCS',sdk/'cstrike')]:cp(folder/'COPYING','common','licenses/'+name+'.txt')
cp(sd/'LICENSE.md','common','licenses/SDHLT.txt')
for license in (r/'tools/legacy-games').glob('LICENSE-TBB-*.txt'):cp(license,'common','licenses/'+license.name)
sources=cache/'common/sources';sources.mkdir(parents=True,exist_ok=True)
def archive(folder,name,skip):
 with zipfile.ZipFile(sources/name,'w',compression=zipfile.ZIP_DEFLATED) as package:
  for p in sorted(folder.rglob('*')):
   if p.is_file() and not skip(p.relative_to(folder)):
    package.write(p,Path(folder.name)/p.relative_to(folder))
archive(sd,'sdhlt-source.zip',lambda p:'.git' in p.parts or p.suffix.lower() in ['.exe','.lno'] or (p.parts[0]=='tools' and p.name.startswith('sdHL') and not p.suffix))
archive(er,'ericw-q2-source.zip',lambda p:'.git' in p.parts or p.name=='.git')
with zipfile.ZipFile(sources/'freehl-freecs-nuclide-source.zip','w',compression=zipfile.ZIP_DEFLATED) as package:
 for p in sorted(sdk.rglob('*')):
  if not p.is_file():continue
  rel=p.relative_to(sdk)
  if '.git' in rel.parts or p.name=='.git' or p.suffix in ['.dat','.lno'] or any(x in rel.parts for x in ['maps','mapsrc','modelsrc','music','img','ThirdParty']):continue
  if rel.parts[0]=='base' and len(rel.parts)>1 and rel.parts[1] not in ['fonts','gfx','glsl','particles','scripts','resource']:continue
  package.write(p,Path('nuclide')/rel)
 revisions={name:subprocess.check_output(['git','rev-parse','HEAD'],cwd=folder,text=True).strip() for name,folder in [('nuclide',sdk),('freehl',sdk/'valve'),('freecs',sdk/'cstrike')]}
 package.writestr('runtime-sources.json',json.dumps(revisions,indent=2))
manifest={group:{p.relative_to(cache/group).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((cache/group).rglob('*')) if p.is_file()} for group in ['common','Windows-x64','Linux-x64']}
(cache/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('LEGACY_GAMES_STAGED',[(g,len(v)) for g,v in manifest.items()])
