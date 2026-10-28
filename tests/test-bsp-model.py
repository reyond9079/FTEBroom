"""Compile isolated open brush models, then validate UV atlases and unchanged collision."""
from pathlib import Path
import argparse
import struct
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--qbsp", required=True, type=Path)
parser.add_argument("--q3map2", required=True, type=Path)
parser.add_argument("--unit", required=True, type=Path)
args = parser.parse_args()
args.qbsp, args.q3map2, args.unit = (p.resolve() for p in (args.qbsp, args.q3map2, args.unit))

def box(material, flags):
    x, y, z, X, Y, Z = 16, 32, 48, 80, 96, 112
    planes = [[(x,Y,Z),(x,y,Z),(x,y,z)], [(X,y,z),(X,y,Z),(X,Y,Z)],
              [(X,y,Z),(X,y,z),(x,y,z)], [(x,Y,z),(X,Y,z),(X,Y,Z)],
              [(X,Y,z),(x,Y,z),(x,y,z)], [(x,y,Z),(x,Y,Z),(X,Y,Z)]]
    return "{\n" + "".join(" ".join("( %s )" % " ".join(map(str,p)) for p in plane)
        + " " + material + " 3 7 17 0.75 1.25" + flags + "\n" for plane in planes) + "}\n"

def run(command, cwd):
    result = subprocess.run(list(map(str, command)), cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(result.stdout.decode("utf-8", errors="replace"))
    return result.stdout.decode("utf-8", errors="replace")

with tempfile.TemporaryDirectory(prefix="reyond-bsp-test-") as temporary:
    root = Path(temporary)
    palette = bytearray(v for i in range(256) for v in (i,i,i))
    # Distinct ordinary/fullbright indices deliberately have identical RGB colors.
    palette[224*3:225*3] = palette[32*3:33*3]
    (root / "palette.lmp").write_bytes(palette)
    name = b"checker" + bytes(9)
    levels = [bytes(224 if (x//4+y//4)%2 else 32 for y in range(w) for x in range(w)) for w in (16,8,4,2)]
    offsets, at = [], 40
    for level in levels:
        offsets.append(at)
        at += len(level)
    lump = name + struct.pack("<6I",16,16,*offsets) + b"".join(levels)
    (root / "checker.wad").write_bytes(b"WAD2" + struct.pack("<II",1,12+len(lump)) + lump
        + struct.pack("<III",12,len(lump),len(lump)) + bytes((68,0,0,0)) + name)
    game = root / "game/baseq3"
    (game / "textures").mkdir(parents=True)
    tga = bytearray(18)
    tga[2], tga[12], tga[14], tga[16], tga[17] = 2,16,16,32,40
    for y in range(16):
        for x in range(16):
            tga.extend((0,y*16,x*16,255))
    (game / "textures/checker.tga").write_bytes(tga)
    for kind in ("q1", "q1-bsp2", "q3"):
        source = root / (kind + ".map")
        world = '{\n"classname" "worldspawn"\n'
        if kind != "q3":
            world += '"wad" "' + (root / "checker.wad").as_posix() + '"\n'
        world += box("checker", " 0 0 0" if kind == "q3" else "") + "}\n"
        if kind == "q3":
            world += '{\n"classname" "info_player_start"\n"origin" "-16 0 16"\n}\n'
        source.write_text(world, encoding="utf-8", newline="\n")
        compiler = [args.q3map2, "-game", "quake3", "-fs_basepath", game.parent, "-fs_basegame", "baseq3", "-threads", "2", "-meta", source] if kind == "q3" else [args.qbsp, "-nofill", *(["-bsp2"] if kind == "q1-bsp2" else []), source]
        run(compiler, root)
        print(run([args.unit, source.with_suffix(".bsp"), root / "palette.lmp", root / (kind + "-atlas.bsp")], root), end="")
