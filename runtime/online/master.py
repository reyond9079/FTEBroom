#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Expiring directory of verified ReyondBroom relays, not a game/data proxy."""
import argparse
import hashlib
import http.client
import ipaddress
import json
import re
import socket
import ssl
import threading
import time
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

TTL = 90
PROTOCOL = 'reyondbroom-online-1'


def origin(value):
    from urllib.parse import urlsplit
    if not isinstance(value, str) or len(value) > 512 or any(ord(c) < 32 for c in value):
        raise ValueError('Origin must be text')
    u = urlsplit(value)
    if (u.scheme not in ('http', 'https') or not u.hostname or u.username or
            u.password or u.path not in ('', '/') or u.query or u.fragment):
        raise ValueError('Use an HTTP(S) origin without a path or credentials')
    u.port  # Reject invalid ports.
    return value.rstrip('/')


def read_json(url, payload=None):
    data = None if payload is None else json.dumps(payload).encode()
    request = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'})
    # Directory verification must not follow redirects or use environment proxies.
    class NoRedirect(urllib.request.HTTPRedirectHandler):
        def redirect_request(self, *args):
            return None
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect())
    with opener.open(request, timeout=4) as response:
        body = response.read(256 * 1024 + 1)
        if len(body) > 256 * 1024:
            raise ValueError('Directory response is too large')
        return json.loads(body)


def verify_info(url, allow_private):
    """Pin the validated numeric peer while retaining HTTPS SNI/certificate checks."""
    from urllib.parse import urlsplit
    u = urlsplit(origin(url))
    port = u.port or (443 if u.scheme == 'https' else 80)
    addresses = socket.getaddrinfo(u.hostname, port, type=socket.SOCK_STREAM)
    if not addresses or len(addresses) > 32:
        raise ValueError('Invalid relay DNS response')
    for address in addresses:
        ip = ipaddress.ip_address(address[4][0])
        if not allow_private and not ip.is_global:
            raise ValueError('Relay callback must resolve only to public addresses')
    # No second DNS lookup, proxy or redirect can change the verified target.
    deadline = time.monotonic() + 4
    sock = None
    for address in addresses:
        try:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError('Relay verification timed out')
            sock = socket.create_connection((address[4][0], port), timeout=remaining)
            break
        except OSError:
            if address == addresses[-1]:
                raise
    if sock is None:
        raise ValueError('Relay address is unreachable')
    connection = http.client.HTTPConnection(u.hostname, port, timeout=4)
    try:
        if u.scheme == 'https':
            sock = ssl.create_default_context().wrap_socket(sock, server_hostname=u.hostname)
        connection.sock = sock
        connection.request('GET', '/info', headers={'Accept': 'application/json'})
        response = connection.getresponse()
        body = response.read(8193)
        if response.status != 200 or len(body) > 8192:
            raise ValueError('Relay /info is unavailable or too large')
        return json.loads(body)
    finally:
        connection.close()
        sock.close()


class Directory:
    def __init__(self, allow_private=False):
        self.entries = {}
        self.lock = threading.Lock()
        self.allow_private = allow_private

    def list(self):
        with self.lock:
            now = time.monotonic()
            self.entries = {k: v for k, v in self.entries.items() if now - v['seen'] < TTL}
            return [dict(v['info'], url=k) for k, v in sorted(self.entries.items())]

    def register(self, address, data):
        if not isinstance(data, dict):
            raise ValueError('Invalid registration object')
        ip = ipaddress.ip_address(address)
        if not self.allow_private and not ip.is_global and not data.get('url'):
            raise ValueError('Private/loopback relays require --allow-private on the master')
        port = data.get('port')
        key = data.get('key', '')
        if type(port) is not int or not 1024 <= port <= 65535 or not isinstance(key, str) or not re.fullmatch('[0-9a-f]{64}', key):
            raise ValueError('Invalid relay port or registration key')
        host = '[' + str(ip) + ']' if ip.version == 6 else str(ip)
        url = origin(data['url']) if data.get('url') else 'http://%s:%s' % (host, port)
        if len(url) > 500 or any(ord(c) < 32 for c in url):
            raise ValueError('Invalid advertised URL')
        self.list()
        with self.lock:
            if url not in self.entries and len(self.entries) >= 256:
                raise ValueError('Directory is full')
            old = self.entries.get(url)
            if old and old['key'] != key:
                raise ValueError('This endpoint is already registered by another relay')
            if old and time.monotonic() - old['seen'] < 10:
                return
        info = verify_info(url, self.allow_private)
        if (not isinstance(info, dict) or info.get('protocol') != PROTOCOL or info.get('pack') != 'LibreQuake-v0.09' or
                info.get('registration_hash') != hashlib.sha256(key.encode()).hexdigest()):
            raise ValueError('Relay verification failed')
        name = info.get('name', '')
        active, free = info.get('active'), info.get('free')
        if (not isinstance(name, str) or not 1 <= len(name) <= 64 or any(ord(c) < 32 for c in name) or
                type(active) is not int or type(free) is not int or not 0 <= active <= 8 or not 0 <= free <= 8):
            raise ValueError('Invalid relay information')
        public = dict(name=name, active=active, free=free, protocol=PROTOCOL, pack=info['pack'])
        with self.lock:
            if url not in self.entries and len(self.entries) >= 256:
                raise ValueError('Directory is full')
            self.entries[url] = dict(info=public, key=key, seen=time.monotonic())


class Handler(BaseHTTPRequestHandler):
    def setup(self):
        super().setup()
        self.connection.settimeout(5)

    def reply(self, code, value):
        body = json.dumps(value).encode()
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path != '/servers':
            self.reply(404, {'error': 'Use /servers'})
            return
        self.reply(200, {'protocol': PROTOCOL, 'servers': self.server.directory.list()})

    def do_POST(self):
        try:
            if self.path != '/register':
                self.reply(404, {'error': 'Unknown endpoint'})
                return
            size = int(self.headers.get('Content-Length', '0'))
            if not 1 <= size <= 4096:
                raise ValueError('Invalid request size')
            self.server.directory.register(self.client_address[0], json.loads(self.rfile.read(size)))
            self.reply(200, {'registered': True, 'expires_in': TTL})
        except (ValueError, KeyError, OSError) as error:
            self.reply(400, {'error': str(error)})

    def log_message(self, *_):
        pass


class Master(ThreadingHTTPServer):
    daemon_threads = True
    def __init__(self, address, allow_private=False):
        self.slots = threading.BoundedSemaphore(16)
        self.directory = Directory(allow_private)
        super().__init__(address, Handler)

    def process_request(self, request, address):
        if not self.slots.acquire(blocking=False):
            self.shutdown_request(request)
            return
        try:
            super().process_request(request, address)
        except Exception:
            self.slots.release()
            raise

    def process_request_thread(self, request, address):
        try:
            super().process_request_thread(request, address)
        finally:
            self.slots.release()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='127.0.0.1')
    parser.add_argument('--port', type=int, default=28776)
    parser.add_argument('--allow-private', action='store_true', help='Allow LAN/VPN/localhost relay registration')
    args = parser.parse_args()
    server = Master((args.host, args.port), args.allow_private)
    print('ReyondBroom master: http://%s:%s' % server.server_address, flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
