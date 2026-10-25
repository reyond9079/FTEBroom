# SPDX-License-Identifier: GPL-2.0-or-later
"""Two local clients, real open-map BSP2 builds, admission and ownership checks."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import threading
import urllib.error
import urllib.request

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'runtime/online'))
from server import make_server, starter
from bsp_limits import validate, MAX_BYTES
from client import request

parser=argparse.ArgumentParser();parser.add_argument('--qbsp',required=True);parser.add_argument('--resources',required=True)
parser.add_argument('--output',required=True);args=parser.parse_args()
root=Path(args.output);root.mkdir(parents=True,exist_ok=False)
server=make_server(('127.0.0.1',0),root/'server',args.resources)
thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
url='http://127.0.0.1:%s'%server.server_port
try:
    first=request(url,'/join',payload={'name':'Alice'})
    second=request(url,'/join',payload={'name':'Bob'})
    assert first['bounds']!=second['bounds'] and first['token']!=second['token']
    textures=server.relay.textures
    texture=next(name for name in sorted(textures) if name[0].isalnum())
    wads=';'.join(p.name for p in sorted((Path(args.resources)/'texture-wads').glob('*.wad')))
    data=[]
    for player in (first,second):
        path=root/('plot%s.map'%player['id'])
        path.write_text(starter(player['bounds'],texture).replace('__ONLINE_WADS__',wads))
        command=[args.qbsp,'-bsp2','-nofill','-wadpath',str(Path(args.resources)/'texture-wads'),str(path)]
        result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        (root/('compile%s.log'%player['id'])).write_bytes(result.stdout)
        assert result.returncode==0,result.stdout.decode(errors='replace')
        bsp=path.with_suffix('.bsp').read_bytes();assert bsp[:4]==b'BSP2'
        assert struct.unpack_from('<ii',bsp,4+9*8)[1]>0,'Collision hulls must remain enabled'
        stats=validate(bsp,player['bounds'],textures);data.append(bsp)
        accepted=request(url,'/bsp',player['token'],bsp,binary=True)
        assert json.loads(accepted)['digest']==hashlib.sha256(bsp).hexdigest()
        request(url,'/map',player['token'],path.read_bytes(),binary=True)
        assert request(url,'/map',player['token'],binary=True)==path.read_bytes()
        print('PASS open BSP2 plot',player['id'],stats)
    scene=request(url,'/scene',first['token'],{'pose':[4000,100,200,0,90]})
    assert len(scene['players'])==2,'Both avatars must be published'
    # Movement is unrestricted even though geometry edits are limited to each plot.
    assert next(p for p in scene['players'] if p['id']==first['id'])['pose'][0]==4000
    other=next(p for p in scene['players'] if p['id']==second['id'])
    assert request(url,'/bsp/%s/%s'%(other['id'],other['digest']),first['token'],binary=True)==data[1]
    for invalid in (data[0][:4],b'IBSP'+data[0][4:],data[0]+b'\0'*MAX_BYTES):
        try:validate(invalid,first['bounds'],textures)
        except ValueError:pass
        else:raise AssertionError('Invalid BSP admitted')
    try:validate(data[0],second['bounds'],textures)
    except ValueError:pass
    else:raise AssertionError('Cross-plot geometry admitted')
    corrupt=bytearray(data[0]);vertex=struct.unpack_from('<i',corrupt,4+3*8)[0];struct.pack_into('<f',corrupt,vertex,float('nan'))
    try:validate(corrupt,first['bounds'],textures)
    except ValueError:pass
    else:raise AssertionError('Nonfinite geometry admitted')
    corrupt=bytearray(data[0]);nodes=struct.unpack_from('<i',corrupt,4+5*8)[0];struct.pack_into('<i',corrupt,nodes+4,0)
    try:validate(corrupt,first['bounds'],textures)
    except ValueError:pass
    else:raise AssertionError('Cyclic BSP admitted')
    corrupt=bytearray(data[0]);tex=struct.unpack_from('<i',corrupt,4+2*8)[0];mip=struct.unpack_from('<i',corrupt,tex+4)[0];pixels=struct.unpack_from('<i',corrupt,tex+mip+24)[0];corrupt[tex+mip+pixels]^=1
    try:validate(corrupt,first['bounds'],textures)
    except ValueError:pass
    else:raise AssertionError('Foreign embedded texture pixels admitted')
    try:request(url,'/scene','invalid',{'pose':[0]*5})
    except ValueError:pass
    else:raise AssertionError('Unauthenticated session accepted')
    print('PASS two clients, separate plots, free travel, MAP/BSP save and sharing, collision hulls, rejected invalid/outside/oversized uploads')
finally:
    server.shutdown();server.server_close()
