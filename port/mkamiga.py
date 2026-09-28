#!/usr/bin/env python3
"""Amiga Moria 1.2 tile sheet for Umoria: port/tiles-amiga.png, same slots as tiles.png.

Amiga Moria (Henrik Harmsen, 1992, drawn for UMoria 5.5) has a picture for
every map symbol: each monster letter, each object group (its treasure.c
codes), terrain, doors, stairs, traps, shop entrances. So every slot gets
the Amiga picture of what the Amiga game shows there, with no stand-ins; the
catch is that one picture serves all monsters of a letter. Tables and sheet
come from port/amiga (extract.py). An 8x8 cell (one text cell of the
Amiga's 80-column screen) is scaled 5x to a 40x40 slot. Dark
terrain (remembered, not lit) is the lit picture at 60 % brightness, as the
Amiga has no dark versions.
Run from anywhere: python3 port/mkamiga.py
"""
import os
import re
from PIL import Image, ImageEnhance

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', 'src')
CELL = 40

gfx, amiga_obj = {}, []
for l in open(os.path.join(HERE, 'amiga', 'amiga.tsv')):
    p = l.rstrip('\n').split('\t')
    if p[0] == 'gfx':
        gfx[int(p[1])] = (int(p[2]), int(p[3]))
    elif p[0] == 'obj':
        amiga_obj.append((p[1], int(p[2])))
creature_char = {m.group(1): m.group(2) for m in re.finditer(
    r'^\s*\{"([^"]*)",.*?\'(.)\',.*?\d+\},?\s*(?://.*)?$', open(os.path.join(SRC, 'data_creatures.cpp')).read(), re.M)}
assert len(amiga_obj) == 420, len(amiga_obj)

# symbols of things the Amiga game draws by symbol
FLV = {'potion': '!', 'ring': '=', 'amulet': '"', 'wand': '-', 'staff': '_', 'mushroom': ','}
TILE = {'floor': '.', 'granite': '#', 'magma': '%', 'quartz': '%', 'magma_k': '*', 'quartz_k': '*',
        'perm': '#', 'up': '<', 'down': '>', 'rubble': ':'}
WALL = {'GRANITE': '#', 'MAGMA': '%', 'QUARTZ': '%', 'PERM': '#'}

sheet = Image.open(os.path.join(HERE, 'amiga', 'moria_gfx.png')).convert('RGBA')


def picture(code, dark=False):
    r, c = gfx[code]
    im = sheet.crop((c * 8, r * 8, c * 8 + 8, r * 8 + 8)).resize((CELL, CELL), Image.NEAREST)
    return ImageEnhance.Brightness(im).enhance(0.6) if dark else im


rows = [l.rstrip('\n').split('\t') for l in open(os.path.join(HERE, 'slots.tsv'))]
out = Image.new('RGBA', (32 * CELL, (len(rows) + 31) // 32 * CELL))
for r in rows:
    s, what = int(r[0]), r[1]
    dark = False
    if what == 'mon':
        code = ord(creature_char[r[2]])
    elif what == 'obj':
        code = amiga_obj[int(r[2])][1]      # same object list as 5.5 (5.7 only fixed some spellings)
    elif what == 'flv':
        code = ord(FLV[r[2]])
    elif what == 'scroll':
        code = ord('?')
    elif what == 'player':
        code = ord('@')
    elif what == 'tile':
        code, dark = ord(TILE[r[2]]), r[3] == 'dark'
    elif what == 'floor':
        code, dark = ord('.'), r[3] == 'dark'
    elif what == 'wall':
        code, dark = ord(WALL[r[2]]), r[3] == 'dark'
    else:
        continue    # pad
    out.paste(picture(code, dark), ((s % 32) * CELL, (s // 32) * CELL))
out.save(os.path.join(HERE, 'tiles-amiga.png'), optimize=True)
print(len(rows), 'slots, all Amiga Moria pictures')
