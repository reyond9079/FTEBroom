#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Editor-owned background client; the render thread never waits for the network."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import time
import urllib.error
import urllib.request
from urllib.parse import urlsplit
from server import wad_names, resource_digest
from bsp_limits import validate, MAX_BYTES

def atomic(path, text):
    temporary=path.with_suffix(path.suffix+'.tmp')
    temporary.write_text(text,encoding='utf8');temporary.replace(path)

def request(server, endpoint, token='', payload=None, binary=False):
    headers={'Authorization':'Bearer '+token, 'Content-Type':'application/octet-stream' if binary else 'application/json'}
    if payload is not None and not binary:
        payload=json.dumps(payload).encode()
    req=urllib.request.Request(server+endpoint,data=payload,headers=headers)
    try:
        with urllib.request.urlopen(req,timeout=8) as response:
            result=response.read(MAX_BYTES+1)
            if len(result)>MAX_BYTES:
                raise ValueError('Server response exceeds the size limit')
            return result if binary else json.loads(result)
    except urllib.error.HTTPError as error:
        try:
            message=json.loads(error.read(8192))['error']
        except Exception:
            message=str(error)
        raise ValueError(message) from error

def run(directory):
    directory=Path(directory).resolve()
    config=json.loads((directory/'connect.json').read_text(encoding='utf8'))
    server=config['server'].rstrip('/')
    url=urlsplit(server)
    if url.scheme not in ('http','https') or not url.netloc or url.username or url.password or url.query or url.fragment or url.path not in ('','/'):
        raise ValueError('Enter a server origin such as http://127.0.0.1:28777')
    resources=Path(config['resources']).resolve()
    textures=wad_names(resources)
    pack_digest=resource_digest(resources)
    session_file=directory/'session.json'
    if session_file.exists():
        session=json.loads(session_file.read_text())
        if session.get('server')!=server:
            session={}
    else:
        session={}
    if not session:
        session=request(server,'/join',payload={'name':config['name']})
        session['server']=server
        atomic(session_file,json.dumps(session));session_file.chmod(0o600)
    if session['pack']!='LibreQuake-v0.09' or session['resource_digest']!=pack_digest:
        raise ValueError('Server and client need the same LibreQuake WAD collection')
    owner=session['id'];bounds=session['bounds'];token=session['token']
    game=directory.parent.parent
    maps=game/'maps';maps.mkdir(exist_ok=True)
    source=request(server,'/map',token,binary=True).decode('utf8')
    wads=';'.join(p.name for p in sorted((resources/'texture-wads').glob('*.wad')))
    source=source.replace('__ONLINE_WADS__',wads)
    atomic(maps/'online-own.map',source)
    atomic(directory/'materials.txt','\n'.join('textures/'+name for name in sorted(textures))+'\n')
    atomic(directory/'default-material.txt',next(name for name in sorted(textures) if name[0].isalnum()))
    atomic(directory/'ready.txt',str(owner)+' '+' '.join(str(v) for v in bounds))
    cache=maps/'ftebroom-build';cache.mkdir(exist_ok=True)
    current={};downloaded=0
    pose=[(bounds[0]+bounds[3])/2,(bounds[1]+bounds[4])/2,128,0,0]
    atomic(directory/'status.txt','Connected. Edit inside your outlined plot; compile locally, then publish.')
    while not (directory/'stop').exists():
        try:
            posefile=directory/'pose.txt'
            if posefile.exists():
                values=[float(v) for v in posefile.read_text().split()]
                if len(values)>=5:
                    pose=values[:5]
            job=directory/'request.json'
            if job.exists():
                action=json.loads(job.read_text(encoding='utf8'));job.unlink()
                if action['action']=='publish':
                    source_path=Path(action['map']);bsp_path=Path(action['bsp'])
                    if source_path.stat().st_size>4*1024*1024 or bsp_path.stat().st_size>MAX_BYTES:
                        raise ValueError('Map or BSP exceeds the upload budget')
                    data=bsp_path.read_bytes();stats=validate(data,bounds,textures)
                    result=request(server,'/bsp',token,data,binary=True)
                    # Keep the last saved MAP independently from the last compiled BSP.
                    request(server,'/map',token,source_path.read_bytes(),binary=True)
                    atomic(directory/'status.txt','Published: %s faces, %s vertices.' % (stats['faces'],stats['vertices']))
            scene=request(server,'/scene',token,{'pose':pose})
            if scene['owner']!=owner or scene['bounds']!=bounds or scene['resource_digest']!=pack_digest:
                raise ValueError('Server changed the session identity or resource pack')
            lines=[]
            for player in scene['players'][:8]:
                identity=player['id'];digest=player['digest']
                if identity not in range(8):
                    raise ValueError('Invalid player identity')
                if digest and current.get(identity)!=digest:
                    if not re.fullmatch('[0-9a-f]{64}',digest) or downloaded>=128:
                        raise ValueError('Session model-cache limit reached; restart the editor')
                    data=request(server,'/bsp/%s/%s' % (identity,digest),token,binary=True)
                    if hashlib.sha256(data).hexdigest()!=digest:
                        raise ValueError('BSP checksum mismatch')
                    validate(data,player['bounds'],textures)
                    path=cache/('online-%s-%s.bsp' % (identity,digest))
                    path.write_bytes(data);current[identity]=digest;downloaded+=1
                if digest:
                    model='maps/ftebroom-build/online-%s-%s.bsp' % (identity,digest)
                    if identity==owner:
                        atomic(directory/'own-bsp.txt',model)
                    else:
                        lines.append('model '+model)
                if identity!=owner:
                    position=player['pose']
                    if len(position)!=5 or any(not isinstance(v,(int,float)) or not abs(v)<=65536 for v in position):
                        raise ValueError('Invalid remote pose')
                    lines.append('avatar '+' '.join(str(v) for v in position))
            atomic(directory/'scene.txt','\n'.join(lines)+'\n')
        except (ValueError,OSError,KeyError,urllib.error.URLError) as error:
            atomic(directory/'status.txt','Online: '+str(error))
        time.sleep(.25)
    atomic(directory/'ready.txt','')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--directory',required=True)
    args=parser.parse_args()
    try:
        run(args.directory)
    except Exception as error:
        atomic(Path(args.directory)/'status.txt','Cannot connect: '+str(error))
        raise SystemExit(1)
