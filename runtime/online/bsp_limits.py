# SPDX-License-Identifier: GPL-2.0-or-later
"""Conservative BSP2 admission checks. The relay never loads uploaded geometry."""
import math
import struct

MAX_BYTES = 16 * 1024 * 1024
SIZES = {1: 20, 3: 12, 5: 44, 6: 40, 7: 28, 9: 12, 10: 44, 11: 4, 12: 8, 13: 4, 14: 64}
CAPS = {1: 32768, 3: 32768, 5: 32768, 6: 4096, 7: 16384, 9: 32768, 10: 32768, 11: 65536, 12: 65536, 13: 131072, 14: 128}

def validate(data, bounds, textures):
    if len(data) < 124 or len(data) > MAX_BYTES or data[:4] != b'BSP2':
        raise ValueError('Only BSP2 files up to 16 MiB are accepted')
    lumps = [struct.unpack_from('<ii', data, 4 + i * 8) for i in range(15)]
    ranges = []
    for offset, size in lumps:
        if offset < 124 or size < 0 or offset + size > len(data):
            raise ValueError('Invalid BSP lump range')
        if size:
            ranges.append((offset, offset + size))
    ranges.sort()
    if any(a[1] > b[0] for a, b in zip(ranges, ranges[1:])):
        raise ValueError('Overlapping BSP lumps')
    counts = {}
    for index, stride in SIZES.items():
        size = lumps[index][1]
        if size % stride or size // stride > CAPS[index]:
            raise ValueError('BSP geometry exceeds the plot budget')
        counts[index] = size // stride
    if not counts[14] or not counts[3] or not counts[7]:
        raise ValueError('Empty BSP')
    def records(index, fmt):
        offset, size = lumps[index]
        return list(struct.iter_unpack(fmt, data[offset:offset + size]))
    for vertex in records(3, '<3f'):
        if any(not math.isfinite(x) or x < bounds[i] - .125 or x > bounds[i + 3] + .125 for i, x in enumerate(vertex)):
            raise ValueError('Geometry is outside your plot')
    for record in records(1, '<4fi'):
        if any(not math.isfinite(x) or abs(x)>1e7 for x in record[:4]):
            raise ValueError('Invalid plane')
    for edge in records(12, '<2I'):
        if max(edge) >= counts[3]:
            raise ValueError('Invalid edge vertex')
    edges = records(13, '<i')
    if any(abs(edge[0]) >= counts[12] for edge in edges):
        raise ValueError('Invalid surface edge')
    for face in records(7, '<5i4Bi'):
        plane, side, first, count, texture = face[:5]
        if not 0 <= plane < counts[1] or side not in (0, 1) or count < 3 or count > 256 or first < 0 or first + count > len(edges) or not 0 <= texture < counts[6]:
            raise ValueError('Invalid face')
        if face[-1] < -1 or face[-1] >= max(1, lumps[8][1]):
            raise ValueError('Invalid light offset')
    offset, size = lumps[2]
    if size < 4:
        raise ValueError('Missing texture directory')
    number = struct.unpack_from('<i', data, offset)[0]
    if not 0 <= number <= 2048 or 4 + number * 4 > size:
        raise ValueError('Invalid texture directory')
    for i in range(number):
        relative = struct.unpack_from('<i', data, offset + 4 + i * 4)[0]
        if relative == -1:
            continue
        if relative < 4 + number * 4 or relative + 40 > size:
            raise ValueError('Invalid mip texture')
        name = data[offset + relative:offset + relative + 16].split(b'\0')[0].decode('ascii').lower()
        width, height, *mips = struct.unpack_from('<6I', data, offset + relative + 16)
        if name not in textures or not width or not height or width > 1024 or height > 1024 or width % 16 or height % 16:
            raise ValueError('Only LibreQuake textures are permitted')
        for level, mip in enumerate(mips):
            if mip and (mip < 40 or relative + mip + (width >> level) * (height >> level) > size):
                raise ValueError('Invalid texture pixels')
        if isinstance(textures,dict) and any(mips):
            levels=[data[offset+relative+mip:offset+relative+mip+(width>>level)*(height>>level)] for level,mip in enumerate(mips)]
            if not any(width==w and height==h and levels==pixels for w,h,pixels in textures[name]):
                raise ValueError('Embedded pixels do not match the LibreQuake resource pack')
    for info in records(6, '<8f2i'):
        if any(not math.isfinite(x) or abs(x)>1e7 for x in info[:8]) or not 0 <= info[8] < number:
            raise ValueError('Invalid texture mapping')
    nodes = records(5, '<3i6f2I')
    for node in nodes:
        if not 0 <= node[0] < counts[1] or node[-2] + node[-1] > counts[7] or any(not math.isfinite(x) for x in node[3:9]):
            raise ValueError('Invalid BSP node')
        if any(child >= len(nodes) or child < -counts[10] for child in node[1:3]):
            raise ValueError('Invalid BSP child')
    # Reject cyclic/deep trees before any client sees the file.
    state = [0] * len(nodes)
    def visit(root):
        stack = [(root, False, 0)]
        while stack:
            index, leaving, depth = stack.pop()
            if index < 0:
                continue
            if leaving:
                state[index] = 2
                continue
            if depth > 256 or state[index] == 1:
                raise ValueError('Cyclic or excessively deep BSP')
            if state[index] == 2:
                continue
            state[index] = 1
            stack.append((index, True, depth))
            stack.extend((child, False, depth + 1) for child in nodes[index][1:3])
    for model in records(14, '<9f7i'):
        if any(not math.isfinite(x) for x in model[:9]) or model[-2] < 0 or model[-1] < 0 or model[-2] + model[-1] > counts[7]:
            raise ValueError('Invalid BSP model')
        if model[9] < 0 or model[9] >= len(nodes):
            raise ValueError('Invalid model root')
        visit(model[9])
    for index in range(len(nodes)):
        visit(index)
    marks=records(11,'<I')
    if any(mark[0]>=counts[7] for mark in marks):
        raise ValueError('Invalid leaf face')
    vis_offset,vis_size=lumps[4];checked=set();row=(max(0,counts[10]-1)+7)//8
    for leaf in records(10,'<2i6f2I4B'):
        if any(not math.isfinite(x) for x in leaf[2:8]) or leaf[8]+leaf[9]>len(marks):
            raise ValueError('Invalid BSP leaf')
        start=leaf[1]
        if vis_size==0 and start in (-1,0):
            continue
        if start==-1 or start in checked:
            continue
        if start<0 or start>=vis_size:
            raise ValueError('Invalid visibility offset')
        checked.add(start);position=start;written=0
        while written<row:
            if position>=vis_size:
                raise ValueError('Truncated visibility row')
            value=data[vis_offset+position];position+=1
            if value:written+=1
            else:
                if position>=vis_size or not data[vis_offset+position]:
                    raise ValueError('Invalid visibility run')
                written+=data[vis_offset+position];position+=1
            if written>row:
                raise ValueError('Visibility row overflow')
    clips=records(9,'<3i')
    if not clips:
        raise ValueError('Online BSPs must retain collision hulls')
    for model in records(14,'<9f7i'):
        if any(root>=len(clips) or root<-15 for root in model[10:13]):
            raise ValueError('Invalid model collision root')
    clip_state=[0]*len(clips)
    for root in range(len(clips)):
        stack=[(root,False,0)]
        while stack:
            index,leaving,depth=stack.pop()
            if index<0:
                if index<-15:raise ValueError('Invalid collision contents')
                continue
            if index>=len(clips):raise ValueError('Invalid collision node')
            if leaving:clip_state[index]=2;continue
            if depth>256 or clip_state[index]==1:raise ValueError('Cyclic collision tree')
            if clip_state[index]==2:continue
            if not 0<=clips[index][0]<counts[1]:raise ValueError('Invalid collision plane')
            clip_state[index]=1;stack.append((index,True,depth))
            stack.extend((child,False,depth+1) for child in clips[index][1:])
    if lumps[0][1] > 65536 or lumps[4][1] > 2 * 1024 * 1024 or lumps[8][1] > 8 * 1024 * 1024:
        raise ValueError('Entity, visibility or lighting budget exceeded')
    # Embedded entities are never instantiated: clients draw only the world model.
    return {'vertices': counts[3], 'faces': counts[7], 'bytes': len(data)}
