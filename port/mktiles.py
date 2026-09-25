#!/usr/bin/env python3
"""Shockbolt tiles for Umoria (RVIP step 4, case A: no tiles upstream).

Reads Vanilla Angband 4.2's Shockbolt mapping (graf-shb-dark.prf,
flvr-shb.prf, xtra-shb.prf, flavor.txt, monster.txt) and Umoria's own
tables, matches by name, and writes:
  port/tilemap.h   tile index per creature / object / flavour / player
  port/tiles.png   the used 64x64 tiles, 32 per row (committed)
  port/tiles.rgba  same, raw RGBA with a w,h header (loaded by be_x11)
Unmatched things fall back to a similar tile; anything left is ASCII.
Run from anywhere: python3 port/mktiles.py [angband-dir]
"""
import os
import re
import struct
import sys
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', 'src')
ANG = sys.argv[1] if len(sys.argv) > 1 else os.path.expanduser('~/Games/angband-4.2.6')
SHB = os.path.join(ANG, 'lib/tiles/shockbolt')
GD = os.path.join(ANG, 'lib/gamedata')


def rc(a, c):
    return (int(a, 16) - 0x80, int(c, 16) - 0x80)


def norm(s):
    s = s.lower().replace('armor', 'armour').replace('&', '').replace('~', '').replace('^', '')
    return re.sub(r'[^a-z]', '', s)


# ---- Shockbolt mapping ----
feat, trap, mon, obj = {}, {}, {}, {}
for line in open(os.path.join(SHB, 'graf-shb-dark.prf')):
    p = line.strip().split(':')
    if p[0] == 'feat':
        feat[(p[1], p[2])] = rc(p[3], p[4])
    elif p[0] == 'trap':
        trap[(p[1], p[2])] = rc(p[3], p[4])
    elif p[0] == 'monster':
        mon[p[1]] = rc(p[2], p[3])
    elif p[0] == 'object':
        obj.setdefault(p[1], []).append((p[2], rc(p[3], p[4])))

flvtile = {}
for line in open(os.path.join(SHB, 'flvr-shb.prf')):
    p = line.strip().split(':')
    if p[0] == 'flavor':
        flvtile[int(p[1])] = rc(p[2], p[3])

# flavour.txt: kind -> [(text, tile)]
flavors, kind = {}, None
for line in open(os.path.join(GD, 'flavor.txt')):
    p = line.strip().split(':')
    if p[0] == 'kind':
        kind = p[1]
    elif p[0] in ('flavor', 'fixed') and int(p[1]) in flvtile:
        flavors.setdefault(kind, []).append((p[-1], flvtile[int(p[1])]))

# monster.txt: name -> (glyph, depth)
glyph = {}
base = None
for line in open(os.path.join(GD, 'monster_base.txt')):
    p = line.strip().split(':')
    if p[0] == 'name':
        base = p[1]
    elif p[0] == 'glyph':
        glyph[base] = p[1]
amon, name = {}, None
for line in open(os.path.join(GD, 'monster.txt')):
    p = line.strip().split(':')
    if p[0] == 'name':
        name = p[1]
        amon[name] = ['?', 0]
    elif p[0] == 'base' and name:
        amon[name][0] = glyph.get(p[1], '?')
    elif p[0] == 'glyph' and name:
        amon[name][0] = p[1]
    elif p[0] == 'depth' and name:
        amon[name][1] = int(p[1])

# ---- Umoria tables ----
creatures = []
for m in re.finditer(r'^\s*\{"([^"]*)",.*?\'(.)\',.*?(\d+)\},?\s*(//.*)?$',
                     open(os.path.join(SRC, 'data_creatures.cpp')).read(), re.M):
    creatures.append((m.group(1), m.group(2), int(m.group(3))))
objects = []
for m in re.finditer(r'^\s*\{"([^"]*)",\s*0x[0-9A-F]+L,\s*(TV_\w+),\s*\'(\\?.)\',\s*-?\d+,\s*\d+,\s*(\d+)',
                     open(os.path.join(SRC, 'data_treasure.cpp')).read(), re.M):
    objects.append((m.group(1), m.group(2), m.group(3), int(m.group(4))))
assert len(creatures) == 279, len(creatures)
assert len(objects) == 420, len(objects)

used = {}


def tid(t):
    if t is None:
        return -1
    if t not in used:
        used[t] = len(used)
    return used[t]


# ---- monsters: exact name, else same letter with most words in common ----
mon_by_norm = {norm(k): k for k in mon}
report = []


def words(s):
    return set(re.findall(r'[a-z]+', s.lower())) - {'giant', 'the', 'of'}


mon_tile = []
for name, ch, lvl in creatures:
    k = mon_by_norm.get(norm(name))
    if not k:
        best = None
        for a, (g, d) in amon.items():
            if a not in mon:
                continue
            score = (g == ch) * 10 + len(words(name) & words(a)) * 4 - abs(d - lvl) / 10.0
            if best is None or score > best[0]:
                best = (score, a)
        k = best[1]
        report.append('monster %-32s %s -> %s' % (name, ch, k))
    mon_tile.append(tid(mon[k]))

# ---- objects ----
TVAL = {'TV_SWORD': ['sword'], 'TV_HAFTED': ['hafted'], 'TV_POLEARM': ['polearm'],
        'TV_BOW': ['bow'], 'TV_ARROW': ['arrow'], 'TV_BOLT': ['bolt'], 'TV_SLING_AMMO': ['shot'],
        'TV_DIGGING': ['digger'], 'TV_BOOTS': ['boots'], 'TV_HELM': ['helm', 'crown'],
        'TV_SOFT_ARMOR': ['soft armour'], 'TV_HARD_ARMOR': ['hard armour', 'soft armour'],
        'TV_CLOAK': ['cloak'], 'TV_GLOVES': ['gloves'], 'TV_SHIELD': ['shield'],
        'TV_LIGHT': ['light'], 'TV_FLASK': ['flask'], 'TV_FOOD': ['food'], 'TV_CHEST': ['chest'],
        'TV_MAGIC_BOOK': ['magic book'], 'TV_PRAYER_BOOK': ['prayer book'], 'TV_GOLD': ['gold'],
        'TV_SPIKE': ['shot'], 'TV_AMULET': ['amulet'], 'TV_RING': ['ring'],
        'TV_WAND': ['wand'], 'TV_STAFF': ['staff'], 'TV_SCROLL1': ['scroll'], 'TV_SCROLL2': ['scroll'],
        'TV_POTION1': ['potion'], 'TV_POTION2': ['potion']}
# Moria name (normalised) -> Angband name, where the words differ
ALIAS = {'broadsword': 'Broad Sword', 'ballandchain': 'Ball-and-Chain', 'twohandedsword': 'Zweihander',
         'twohandedgreatflail': 'Two-Handed Great Flail', 'smallsword': 'Short Sword', 'foil': 'Rapier',
         'sabre': 'Cutlass', 'backsword': 'Long Sword', 'executionerssword': "Executioner's Sword",
         'lance': 'Lance', 'warpick': 'Pick', 'lucernhammer': 'Lucerne Hammer', 'morningstar': 'Morning Star',
         'orcishpick': 'Pick', 'dwarvenpick': 'Mattock', 'gnomishshovel': 'Shovel', 'dwarvenshovel': 'Shovel',
         'softleathercap': 'Hard Leather Cap', 'silvercrown': 'Iron Crown',
         'jewelencrustedcrown': 'Jewel Encrusted Crown', 'brasslantern': 'Lantern',
         'flaskofoil': 'Flask of oil', 'ironspike': 'Iron Shot', 'ironshot': 'Iron Shot',
         'rationoffood': 'Ration of Food', 'hardbiscuit': 'Hard Biscuit', 'beefjerky': 'Slice of Meat',
         'pintoffinealeale': 'Pint of Fine Wine', 'pintoffinewine': 'Pint of Fine Wine',
         'pintoffinegrademush': 'Slime Mold', 'slimemold': 'Slime Mold',
         'pieceofelvishwaybread': 'Piece of Elvish Waybread', 'pintoffineale': 'Swig of Orcish Liquor',
         'woodenbow': 'Short Bow', 'hardleatherbodyarmour': 'Hard Leather Armour'}


def find_obj(tvals, cands):
    for tv in tvals:
        lst = obj.get(tv, [])
        for c in cands:
            for n, t in lst:
                if norm(n) == norm(c):
                    return t
    for c in cands:     # any kind with that name
        for lst in obj.values():
            for n, t in lst:
                if norm(n) == norm(c):
                    return t
    return None


obj_tile = []
for name, tv, ch, sub in objects:
    t = None
    tvals = TVAL.get(tv)
    if tvals and tv not in ('TV_AMULET', 'TV_RING', 'TV_WAND', 'TV_STAFF', 'TV_SCROLL1', 'TV_SCROLL2',
                            'TV_POTION1', 'TV_POTION2'):
        base_name = re.sub(r'\s*\(.*', '', name)
        paren = re.findall(r'\(([^)%]*)\)', name)
        cands = [name, base_name] + paren
        cands = [ALIAS[norm(c)] for c in cands if norm(c) in ALIAS] + cands
        t = find_obj(tvals, cands)
        if t is None and tv == 'TV_GOLD':
            t = find_obj(['gold'], [name])
        if t is None and tvals[0] in obj:   # same kind, any tile
            t = obj[tvals[0]][0][1]
            report.append('object  %-32s %s -> first %s' % (name, tv, tvals[0]))
    if tv == 'TV_CHEST' and 'ruined' in name:
        t = obj['chest'][0][1]
    if tv == 'TV_RUBBLE':
        t = feat[('RUBBLE', 'lit')]
    if tv in ('TV_OPEN_DOOR',):
        t = feat[('OPEN', '*')]
    if tv in ('TV_CLOSED_DOOR',):
        t = feat[('CLOSED', '*')]
    if tv in ('TV_UP_STAIR',):
        t = feat[('LESS', 'lit')]
    if tv in ('TV_DOWN_STAIR',):
        t = feat[('MORE', 'lit')]
    if tv in ('TV_VIS_TRAP', 'TV_INVIS_TRAP'):
        n = name.lower()
        key = ('pit' if 'pit' in n else 'trap door' if 'door' in n else 'teleport rune' if 'rune' in n
               else 'poison gas trap' if 'gas' in n else 'blinding flash trap' if 'blackened' in n
               else 'rock fall trap' if 'rock' in n else 'acid trap' if 'corroded' in n
               else 'knife trap' if 'dart' in n or 'arrow' in n else None)
        if 'loose rock' in n:
            key = None      # a loose rock (';') looks like rubble
            t = feat[('PASS_RUBBLE', '*')]
        if key:
            t = trap[(key, 'lit')]
    if tv == 'TV_STORE_DOOR':
        t = feat[(['STORE_GENERAL', 'STORE_ARMOR', 'STORE_WEAPON', 'STORE_BOOK', 'STORE_ALCHEMY',
                   'STORE_MAGIC'][int(ch) - 1], '*')]
    if t is None and tv not in ('TV_AMULET', 'TV_RING', 'TV_WAND', 'TV_STAFF', 'TV_SCROLL1',
                                'TV_SCROLL2', 'TV_POTION1', 'TV_POTION2', 'TV_NOTHING'):
        report.append('object  %-32s %s -> ASCII' % (name, tv))
    obj_tile.append(tid(t))


# ---- flavours: Umoria's appearance names -> Angband flavour tiles ----
def arr(name):
    m = re.search(r'const char \*' + name + r'\[[A-Z_]+\] = \{(.*?)\};',
                  open(os.path.join(SRC, 'data_tables.cpp')).read(), re.S)
    return re.findall(r'"([^"]*)"', m.group(1))


def flv_table(moria, kind):
    tiles = flavors[kind]
    out = []
    for i, n in enumerate(moria):
        t = next((t for a, t in tiles if norm(a) == norm(n) or norm(a) == norm(n).replace('aluminum', 'aluminium')), None)
        if t is None:
            t = tiles[i % len(tiles)][1]
            report.append('flavour %-10s %-20s -> %s' % (kind, n, tiles[i % len(tiles)][0]))
        out.append((n, tid(t)))
    return out


flv = {'potion': flv_table(arr('colors'), 'potion'), 'ring': flv_table(arr('rocks'), 'ring'),
       'amulet': flv_table(arr('amulets'), 'amulet'), 'wand': flv_table(arr('metals'), 'wand'),
       'staff': flv_table(arr('woods'), 'staff'), 'mushroom': flv_table(arr('mushrooms'), 'mushroom')}
scroll_tiles = [tid(t) for _, t in flavors['scroll']]

# ---- player: xtra-shb.prf conditions, last match wins ----
RACES = ['Human', 'Half-Elf', 'Elf', 'Halfling', 'Gnome', 'Dwarf', 'Half-Orc', 'Half-Troll']
CLASSES = ['Warrior', 'Mage', 'Priest', 'Rogue', 'Ranger', 'Paladin']
xtra = [l.strip() for l in open(os.path.join(SHB, 'xtra-shb.prf'))]


def player_tile(race, cls, sex):
    env = {'$RACE': race, '$CLASS': cls, '$GENDER': sex}
    ok, tile = True, mon['<player>']
    for l in xtra:
        if l.startswith('?:'):
            conds = re.findall(r'EQU (\$\w+) ([\w-]+)', l)
            ok = all(env[v] == x for v, x in conds) if conds else True
        elif ok and l.startswith('monster:<player>:'):
            p = l.split('#')[0].strip().split(':')
            tile = rc(p[2], p[3])
    return tid(tile)


players = [[[player_tile(r, c, s) for s in ('Female', 'Male')] for c in CLASSES] for r in RACES]

# ---- terrain ----
T = {}
for key, name in [('FLOOR', 'floor'), ('GRANITE', 'granite'), ('MAGMA', 'magma'), ('QUARTZ', 'quartz'),
                  ('MAGMA_K', 'magma_k'), ('QUARTZ_K', 'quartz_k'), ('PERM', 'perm'),
                  ('LESS', 'up'), ('MORE', 'down'), ('RUBBLE', 'rubble')]:
    T[name] = (tid(feat[(key, 'lit')]), tid(feat[(key, 'dark')]))

# ---- output ----
os.chdir(HERE)


def carr(name, vals):
    return 'static const short %s[] = {%s};\n' % (name, ','.join(str(v) for v in vals))


with open('tilemap.h', 'w') as f:
    f.write('// Generated by port/mktiles.py from Angband 4.2 Shockbolt tiles. Do not edit.\n#pragma once\n')
    f.write(carr('mon_tile', mon_tile))
    f.write(carr('obj_tile', obj_tile))
    for k, lst in flv.items():
        f.write('static const struct { const char *name; short tile; } flv_%s[] = {%s};\n'
                % (k, ','.join('{"%s",%d}' % (n, t) for n, t in lst)))
    f.write(carr('scroll_tiles', scroll_tiles))
    f.write('static const short player_tiles[8][6][2] = {%s};\n' % ','.join(
        '{%s}' % ','.join('{%d,%d}' % tuple(s) for s in c) for c in players))
    for k, (lit, dark) in T.items():
        f.write('static const short T_%s[2] = {%d, %d}; // lit, dark\n' % (k.upper(), lit, dark))

sheet = Image.open(os.path.join(SHB, '64x64.png')).convert('RGBA')
rows = (len(used) + 31) // 32
out = Image.new('RGBA', (32 * 64, rows * 64))
for (r, c), i in used.items():
    out.paste(sheet.crop((c * 64, r * 64, c * 64 + 64, r * 64 + 64)), ((i % 32) * 64, (i // 32) * 64))
out.save('tiles.png', optimize=True)
with open('tiles.rgba', 'wb') as f:
    f.write(struct.pack('<II', out.width, out.height) + out.tobytes())
print('%d tiles; fallbacks:' % len(used))
print('\n'.join(report))
