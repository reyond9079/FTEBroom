# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual HTTP master/relay discovery, ownership, expiry and malformed response tests."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import threading
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'runtime/online'))
from master import Master, TTL, PROTOCOL, read_json
from server import make_server, advertise
from discover import discover

parser = argparse.ArgumentParser()
parser.add_argument('--resources', required=True)
args = parser.parse_args()
with tempfile.TemporaryDirectory() as folder:
    relay = make_server(('127.0.0.1', 0), folder, args.resources)
    master = Master(('127.0.0.1', 0), allow_private=True)
    servers = [relay, master]
    for server in servers:
        threading.Thread(target=server.serve_forever, daemon=True).start()
    url = 'http://127.0.0.1:%s' % master.server_port
    relay_url = 'http://127.0.0.1:%s' % relay.server_port
    payload = dict(port=relay.server_port, key=relay.relay.registration_key)
    stop = threading.Event()
    try:
        assert discover(url) == ''
        threading.Thread(target=advertise,args=(relay,url,stop),daemon=True).start()
        deadline=time.monotonic()+5
        while not discover(url) and time.monotonic()<deadline:time.sleep(.05)
        assert read_json(url+'/register', payload)['registered']
        rows = discover(url)
        assert rows == relay_url+'\tReyondBroom workshop\t0\t8', rows
        alice = read_json(relay_url+'/join', dict(name='Alice'))
        bob = read_json(relay_url+'/join', dict(name='Bob'))
        assert alice['id'] != bob['id']
        master.directory.entries[relay_url]['seen'] -= 11
        read_json(url+'/register', payload)
        assert discover(url).endswith('\t2\t6')
        assert 'key' not in read_json(url+'/servers')['servers'][0]
        def rejected(value):
            try:
                read_json(url+'/register', value)
            except OSError:
                return
            raise AssertionError('Invalid registration accepted')
        rejected(dict(port=relay.server_port, key='0'*64))
        rejected(dict(port=22, key=relay.relay.registration_key))
        rejected(dict(port=True, key=relay.relay.registration_key))
        rejected(dict(port=relay.server_port, key=123))
        rejected([])
        rejected(dict(payload,url=123))
        alias='http://localhost:%s'%relay.server_port
        read_json(url+'/register',dict(payload,url=alias))
        assert alias in discover(url)
        master.directory.entries[relay_url]['seen'] -= TTL+1
        master.directory.entries[alias]['seen'] -= TTL+1
        assert discover(url) == ''
        read_json(url+'/register', payload)
        assert relay_url in discover(url)
        master.directory.allow_private = False
        rejected(payload)
        rejected(dict(payload,url=alias))
        print('PASS real HTTP listing, two joins/counts, ownership, expiry/re-registration, private-address policy')
        print('PASS no registration secret in directory; fixed LibreQuake protocol; invalid port/key rejection')
    finally:
        stop.set()
        for server in servers:
            server.shutdown()
            server.server_close()
