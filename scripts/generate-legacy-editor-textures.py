"""Generate original neutral editor textures for GoldSrc and Quake II."""
from pathlib import Path
import struct
root=Path(__file__).resolve().parents[1]/'runtime'
palette=b''.join(bytes((i,i,i)) for i in range(256))
name=b'fb_grey'+b'\0'*9
mips=[bytes([160]*(16>>m)**2) for m in range(4)]
offsets=[40]
for pixels in mips[:-1]:offsets.append(offsets[-1]+len(pixels))
lump=name+struct.pack('<6I',16,16,*offsets)+b''.join(mips)+struct.pack('<H',256)+palette
directory=struct.pack('<3I4B16s',12,len(lump),len(lump),67,0,0,0,name)
(root/'halflife-editor.wad').write_bytes(b'WAD3'+struct.pack('<2I',1,12+len(lump))+lump+directory)
wal=bytearray(100);wal[:7]=b'fb_grey';struct.pack_into('<6I',wal,32,16,16,*[p+60 for p in offsets])
# WAL pixels use the game's palette; choose the closest neutral grey at load/export.
(root/'textures/fb_grey.wal').write_bytes(wal+b''.join(mips))
