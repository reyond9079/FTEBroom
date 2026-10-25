#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Local experimental plot relay. No uploaded code or compiler is executed here."""
import argparse
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import math
from pathlib import Path
import re
import secrets
import struct
import threading
import time
from urllib.parse import urlsplit
from bsp_limits import validate, MAX_BYTES

def wad_names(root):
    names = set()
    for path in (Path(root)/'texture-wads').glob('*.wad'):
        data = path.read_bytes()
        if data[:4] != b'WAD2':
            continue
        count, offset = struct.unpack_from('<ii', data, 4)
        if not 0 <= count <= 65536 or offset < 12 or offset + count * 32 > len(data):
            raise ValueError('Invalid resource WAD')
        for i in range(count):
            record = offset + i * 32
            if data[record + 12] == 68:
                names.add(data[record + 16:record + 32].split(b'\0')[0].decode('ascii').lower())
    if not names:
        raise ValueError('LibreQuake texture-wads are required')
    return names

def resource_digest(root):
    root=Path(root);digest=hashlib.sha256()
    for path in sorted(list(root.glob('*.pak'))+list((root/'texture-wads').glob('*.wad'))):
        digest.update(path.relative_to(root).as_posix().encode())
        with path.open('rb') as stream:
            for block in iter(lambda:stream.read(1024*1024),b''):
                digest.update(block)
    return digest.hexdigest()

def wad_pixels(root):
    result={}
    for path in sorted((Path(root)/'texture-wads').glob('*.wad')):
        data=path.read_bytes()
        if data[:4]!=b'WAD2':continue
        count,offset=struct.unpack_from('<ii',data,4)
        for i in range(count):
            record=offset+i*32;position,size=struct.unpack_from('<ii',data,record)
            if data[record+12]!=68:continue
            if position<12 or size<40 or position+size>len(data):raise ValueError('Invalid resource mip texture')
            name=data[record+16:record+32].split(b'\0')[0].decode('ascii').lower()
            width,height,*mips=struct.unpack_from('<6I',data,position+16)
            if not width or not height or not all(mips):continue
            levels=[]
            for level,mip in enumerate(mips):
                length=(width>>level)*(height>>level)
                if mip<40 or mip+length>size:raise ValueError('Invalid resource mip pixels')
                levels.append(data[position+mip:position+mip+length])
            result.setdefault(name,[]).append((width,height,levels))
    return result

def starter(bounds, texture):
    x, y = (bounds[0] + bounds[3]) / 2, (bounds[1] + bounds[4]) / 2
    lo, hi = (x - 128, y - 128, 0), (x + 128, y + 128, 16)
    points = [((lo[0],lo[1],lo[2]),(lo[0],hi[1],lo[2]),(hi[0],hi[1],lo[2])),
              ((lo[0],lo[1],hi[2]),(hi[0],hi[1],hi[2]),(lo[0],hi[1],hi[2])),
              ((lo[0],lo[1],lo[2]),(hi[0],lo[1],lo[2]),(hi[0],lo[1],hi[2])),
              ((lo[0],hi[1],lo[2]),(hi[0],hi[1],hi[2]),(hi[0],hi[1],lo[2])),
              ((lo[0],lo[1],lo[2]),(lo[0],lo[1],hi[2]),(lo[0],hi[1],hi[2])),
              ((hi[0],lo[1],lo[2]),(hi[0],hi[1],hi[2]),(hi[0],lo[1],hi[2]))]
    lines = ['// Game: Quake', '// Format: Standard', '{', '"classname" "worldspawn"', '"wad" "__ONLINE_WADS__"', '{']
    for plane in points:
        lines.append(' '.join('( %g %g %g )' % p for p in (plane[0],plane[2],plane[1])) + ' ' + texture + ' 0 0 0 1 1')
    lines += ['}', '}', '{', '"classname" "info_player_start"', '"origin" "%g %g 64"' % (x,y), '}']
    return '\n'.join(lines) + '\n'

class Relay:
    def __init__(self, root, resources):
        self.root = Path(root); self.root.mkdir(parents=True, exist_ok=True)
        wad_names(resources)
        self.textures = wad_pixels(resources)
        self.lock = threading.RLock()
        self.upload_lock = threading.BoundedSemaphore(1)
        self.players = {}
        self.pack = 'LibreQuake-v0.09'
        self.resource_digest = resource_digest(resources)
        state = self.root / 'state.json'
        if state.exists():
            self.players = json.loads(state.read_text())
            for player in self.players.values():
                player['seen'] = 0
    def save(self):
        temp = self.root / 'state.tmp'
        temp.write_text(json.dumps(self.players), encoding='utf8')
        temp.replace(self.root / 'state.json')
    def join(self, name):
        with self.lock:
            if len(self.players) >= 8:
                raise ValueError('All eight plots are assigned')
            if not re.fullmatch(r'[\w .-]{1,32}', name):
                raise ValueError('Use a name of 1-32 letters, digits, spaces or .-')
            slot = len(self.players)
            x, y = (slot % 4) * 2304 - 1024, (slot // 4) * 2304 - 1024
            token = secrets.token_hex(32)
            player = dict(id=slot, name=name, bounds=[x,y,-1024,x+2048,y+2048,1024], pose=[x+1024,y+1024,128,0,0], seen=time.time(), digest='', revisions=0)
            self.players[token] = player; self.save()
            return dict(token=token, **player, pack=self.pack, resource_digest=self.resource_digest)
    def player(self, token):
        if token not in self.players:
            raise PermissionError('Invalid session token')
        return self.players[token]
    def scene(self, token, pose):
        with self.lock:
            player = self.player(token)
            if len(pose) != 5 or any(not isinstance(v,(int,float)) or not math.isfinite(v) or abs(v)>65536 for v in pose):
                raise ValueError('Invalid camera pose')
            player['pose'] = pose; player['seen'] = time.time()
            return dict(owner=player['id'], bounds=player['bounds'], pack=self.pack, resource_digest=self.resource_digest,
                        players=[{k:p[k] for k in ('id','name','pose','digest','bounds')} for p in self.players.values() if time.time()-p['seen']<30 or p['digest']])
    def publish(self, token, data):
        if not self.upload_lock.acquire(blocking=False):
            raise ValueError('Another upload is being checked; retry shortly')
        try:
            with self.lock:
                player = self.player(token)
                if time.time() - player.get('uploaded',0) < 2:
                    raise ValueError('Wait two seconds between uploads')
                if player['revisions'] >= 64:
                    raise ValueError('Experimental session revision limit reached')
                stats = validate(data, player['bounds'], self.textures)
                digest = hashlib.sha256(data).hexdigest()
                folder = self.root / str(player['id']); folder.mkdir(exist_ok=True)
                (folder / 'latest.bsp.tmp').write_bytes(data)
                (folder / 'latest.bsp.tmp').replace(folder / 'latest.bsp')
                player.update(digest=digest, uploaded=time.time(), revisions=player['revisions']+1)
                self.save()
                return dict(digest=digest, **stats)
        finally:
            self.upload_lock.release()

class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.0'
    def setup(self):
        super().setup(); self.connection.settimeout(10)
    def respond(self, status, data, content='application/json'):
        if not isinstance(data, bytes):
            data = json.dumps(data).encode()
        self.send_response(status); self.send_header('Content-Type',content)
        self.send_header('Content-Length',str(len(data))); self.end_headers(); self.wfile.write(data)
    def do_POST(self):
        try:
            path = urlsplit(self.path).path
            length = int(self.headers.get('Content-Length','0'))
            if length < 0 or length > (MAX_BYTES if path == '/bsp' else 4*1024*1024):
                raise ValueError('Request too large')
            data = self.rfile.read(length)
            if len(data) != length:
                raise ValueError('Incomplete request')
            relay = self.server.relay
            token = self.headers.get('Authorization','').removeprefix('Bearer ')
            if path == '/join':
                result = relay.join(json.loads(data)['name'])
            elif path == '/scene':
                result = relay.scene(token,json.loads(data)['pose'])
            elif path == '/bsp':
                result = relay.publish(token,data)
            elif path == '/map':
                with relay.lock:
                    player=relay.player(token)
                    if b'patchDef' in data or b'\0' in data:
                        raise ValueError('Only Quake MAP source is supported')
                    data.decode('utf8')
                    folder=relay.root/str(player['id']);folder.mkdir(exist_ok=True)
                    (folder/'latest.map.tmp').write_bytes(data);(folder/'latest.map.tmp').replace(folder/'latest.map')
                    result={'saved':True}
            else:
                self.respond(404,{'error':'Unknown endpoint'});return
            self.respond(200,result)
        except PermissionError as error:
            self.respond(403,{'error':str(error)})
        except (ValueError,KeyError,UnicodeError,struct.error) as error:
            self.respond(400,{'error':str(error)})
    def do_GET(self):
        try:
            relay=self.server.relay; token=self.headers.get('Authorization','').removeprefix('Bearer ')
            with relay.lock:
                player=relay.player(token)
                if self.path == '/map':
                    path=relay.root/str(player['id'])/'latest.map'
                    texture=next(name for name in sorted(relay.textures) if name[0].isalnum())
                    data=path.read_bytes() if path.exists() else starter(player['bounds'],texture).encode()
                    self.respond(200,data,'text/plain');return
                match=re.fullmatch(r'/bsp/([0-7])/([0-9a-f]{64})',self.path)
                if not match:
                    self.respond(404,{'error':'Unknown endpoint'});return
                other=next((p for p in relay.players.values() if p['id']==int(match[1]) and p['digest']==match[2]),None)
                if not other:
                    self.respond(404,{'error':'Revision changed; refresh'});return
                self.respond(200,(relay.root/match[1]/'latest.bsp').read_bytes(),'application/octet-stream')
        except PermissionError as error:
            self.respond(403,{'error':str(error)})
    def log_message(self, *_):
        pass

class BoundedServer(ThreadingHTTPServer):
    daemon_threads=True
    def __init__(self,*args):
        self.slots=threading.BoundedSemaphore(16)
        super().__init__(*args)
    def process_request(self,request,address):
        if not self.slots.acquire(blocking=False):
            self.shutdown_request(request);return
        try:super().process_request(request,address)
        except Exception:self.slots.release();raise
    def process_request_thread(self,request,address):
        try:super().process_request_thread(request,address)
        finally:self.slots.release()

def make_server(address, root, resources):
    server=BoundedServer(address,Handler)
    server.relay=Relay(root,resources)
    return server

if __name__ == '__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--host',default='127.0.0.1');parser.add_argument('--port',type=int,default=28777)
    parser.add_argument('--data',required=True);parser.add_argument('--librequake',required=True)
    args=parser.parse_args()
    server=make_server((args.host,args.port),args.data,args.librequake)
    print('ReyondBroom online relay: http://%s:%s' % server.server_address,flush=True)
    server.serve_forever()
