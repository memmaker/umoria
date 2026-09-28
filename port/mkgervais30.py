#!/usr/bin/env python3
"""Gervais tiles that Angband 4.2 no longer names, for Umoria (read by mktiles.py).

Angband 3.0.9 still had many Moria monsters and junk items, and its
graf-dvg.prf places David Gervais's drawings for them on the same 32x32 sheet
that Angband 4.2 ships (lib/tiles/gervais/32x32.png keeps the 3.0 layout:
489 of the 519 monsters both versions name sit on the same cell). This reads
3.0.9's monster.txt, object.txt and graf-dvg.prf and writes
port/gervais30.tsv: kind, Umoria name, row, col, for every Umoria creature or
object that 4.2 does not name but 3.0.9 does (plus the renames in RENAME).
Cells that 4.2 gave to something else are left out (SKIP) where the drawing
there is no longer the thing's own.
Run from anywhere: python3 port/mkgervais30.py [angband-4.2-dir]
"""
import os
import re
import sys
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', 'src')
ANG = sys.argv[1] if len(sys.argv) > 1 else os.path.expanduser('~/Games/angband-4.2.6')
URL = 'https://raw.githubusercontent.com/angband/angband/v3.0.9/lib/'
# Umoria name -> Angband 3.0.9 name, where the words differ
RENAME = {'some Filthy Rags': 'Filthy Rag', 'some shards of pottery': 'Shard of Pottery',
          '& large broken bone': 'Broken Bone', '& broken set of teeth': 'Broken Skull',
          '& Rat Skeleton': 'Rodent Skeleton', '& Giant Centipede Skeleton': 'Broken Bone'}
# 4.2 put other drawings on these cells (a crown, a caestus)
SKIP = {'& Pair of Soft Leather Boots', '& Large Leather Shield'}


def norm(s):
    s = s.lower().replace('armor', 'armour').replace('&', '').replace('~', '').replace('^', '')
    return re.sub(r'[^a-z]', '', s)


def get(path):
    return urllib.request.urlopen(URL + path).read().decode('latin-1').splitlines()


def names(lines):
    return {int(l.split(':')[1]): l.split(':', 2)[2] for l in lines if l.startswith('N:')}


mon30, obj30 = names(get('edit/monster.txt')), names(get('edit/object.txt'))
pos30 = {'R': {}, 'K': {}}
for l in get('pref/graf-dvg.prf'):
    p = l.strip().split(':')
    if p[0] in pos30 and len(p) >= 4:
        n = (mon30 if p[0] == 'R' else obj30).get(int(p[1]))
        if n:
            pos30[p[0]][norm(n)] = (int(p[2], 16) - 0x80, int(p[3], 16) - 0x80)

named42 = set()
for l in open(os.path.join(ANG, 'lib/tiles/gervais/graf-dvg.prf')):
    p = l.strip().split(':')
    if p[0] == 'monster':
        named42.add(('R', norm(p[1])))
    elif p[0] == 'object':
        named42.add(('K', norm(p[2])))

creatures = re.findall(r'^\s*\{"([^"]*)",.*?\'.\',.*?\d+\},?\s*(?://.*)?$',
                       open(os.path.join(SRC, 'data_creatures.cpp')).read(), re.M)
objects = re.findall(r'^\s*\{"([^"]*)",\s*0x[0-9A-F]+L,\s*TV_(?!SCROLL|POTION|FOOD|WAND|STAFF|RING|AMULET)\w+',
                     open(os.path.join(SRC, 'data_treasure.cpp')).read(), re.M)
with open(os.path.join(HERE, 'gervais30.tsv'), 'w') as f:
    f.write('# kind\tUmoria name\trow\tcol  (Gervais 32x32 cells from Angband 3.0.9 graf-dvg.prf; port/mkgervais30.py)\n')
    for kind, lst in (('R', creatures), ('K', objects)):
        for n in dict.fromkeys(lst):
            key = norm(RENAME.get(n, n))
            if n in SKIP or (kind, norm(n)) in named42 or key not in pos30[kind]:
                continue
            r, c = pos30[kind][key]
            f.write('%s\t%s\t%d\t%d\n' % ('mon' if kind == 'R' else 'obj', n, r, c))
