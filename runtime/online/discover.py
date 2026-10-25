#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Fetch a directory off the render thread and atomically publish bounded UI rows."""
import argparse
from pathlib import Path
from master import PROTOCOL, origin, read_json


def discover(master):
    data = read_json(origin(master) + '/servers')
    if not isinstance(data, dict) or data.get('protocol') != PROTOCOL:
        raise ValueError('This is not a compatible ReyondBroom master')
    servers = data.get('servers')
    if not isinstance(servers, list) or len(servers) > 256:
        raise ValueError('Invalid server list')
    rows = []
    for item in servers:
        if not isinstance(item, dict) or item.get('protocol') != PROTOCOL or item.get('pack') != 'LibreQuake-v0.09':
            continue
        url = origin(item['url'])
        name = item['name']
        active, free = item['active'], item['free']
        if (len(url) > 500 or not isinstance(name, str) or not 1 <= len(name) <= 64 or
                any(ord(c) < 32 for c in name+url) or type(active) is not int or type(free) is not int or
                not 0 <= active <= 8 or not 0 <= free <= 8):
            raise ValueError('Invalid server row')
        rows.append('%s\t%s\t%d\t%d' % (url, name, active, free))
    return '\n'.join(rows)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--master', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output)
    try:
        text = 'OK\n' + discover(args.master)
    except (ValueError, KeyError, TypeError, OSError) as error:
        text = 'ERROR\n' + str(error).replace('\n', ' ')[:512]
    temporary = output.with_suffix('.tmp')
    temporary.write_text(text, encoding='utf8')
    temporary.replace(output)
