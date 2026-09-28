#!/usr/bin/env python3
"""Shockbolt and Gervais tiles for Umoria (RVIP step 4, case A: no tiles upstream).

Reads Vanilla Angband 4.2's mappings for both sets (graf-*.prf, flvr-*.prf,
xtra-*.prf, flavor.txt, monster.txt) and Umoria's own tables, matches by
name, and writes:
  port/tilemap.h         tile index per creature / object / flavour / player
  port/tiles.png         Shockbolt: the used 64x64 tiles, 32 per row (committed)
  port/tiles.rgba        same, raw RGBA with a w,h header (loaded by be_x11)
  port/tiles-gervais.png Gervais: the same slots, 32x32 (committed)
  port/slots.tsv         what each slot shows (read by mkdawn.py, mkamiga.py)
Every slot is keyed by what it shows, so all sheets share one layout.
Where Angband 4.2 has no such name: Gervais first takes the drawing Angband
3.0.9 gave it (port/gervais30.tsv, from mkgervais30.py), then both sets take
the hand-picked Angband thing of the same kind in MON / OBJ / FLV below.
Nothing is guessed: a thing without a tile stops the script.
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
GD = os.path.join(ANG, 'lib/gamedata')
# set: (folder, graf prf, flavour prf, player prf, sheet, tile size, output)
SETS = {'shockbolt': ('shockbolt', 'graf-shb-dark.prf', 'flvr-shb.prf', 'xtra-shb.prf', '64x64.png', 64, 'tiles.png'),
        'gervais': ('gervais', 'graf-dvg.prf', 'flvr-dvg.prf', 'xtra-dvg.prf', '32x32.png', 32, 'tiles-gervais.png')}

# Umoria creature -> Angband 4.2 creature of the same kind, where 4.2 has no such name
MON = {
 'Large Brown Snake': 'serpent of the brownlands', 'Large Black Snake': 'black mamba',
 'Large Green Snake': 'king cobra', 'Novice Warrior': 'soldier', 'Novice Rogue': 'cutpurse',
 'Novice Priest': 'acolyte', 'Novice Mage': 'apprentice', 'Huge Brown Bat': 'giant brown bat',
 'Drooling Harpy': 'white harpy', 'Grey Harpy': 'black harpy', 'Black Mushroom patch': 'purple mushroom patch',
 'White Mushroom patch': 'grey mushroom patch', 'Giant Black Centipede': 'stegocentipede',
 'Giant Blue Centipede': 'metallic blue centipede', 'Giant Red Centipede': 'metallic red centipede',
 'Jackal': 'wild dog', 'Giant Black Frog': 'giant green frog', 'Giant Red Speckled Frog': 'giant red frog',
 'Black Yeek': 'Orfax, Son of Boldor', 'Brown Yeek': 'terrified yeek', 'Clear Yeek': 'Boldor, King of the Yeeks',
 'Giant House Fly': 'giant fruit fly', 'Giant Green Fly': 'hummerhorn', 'Giant Gnat': 'giant flea',
 'Orc': 'cave orc', 'Orc Warrior': 'orc captain', 'Black Orc': 'orc archer', 'Uruk-Hai Orc': 'uruk',
 'Hobgoblin': 'half-orc', 'Zombie Kobold': 'zombified kobold', 'Orc Zombie': 'zombified orc',
 'Human Zombie': 'zombified human', 'Mummified Kobold': 'mummified orc',
 'Greedy Little Gnome': 'Ibun, Son of Mîm', 'Nasty Little Gnome': 'Khîm, Son of Mîm',
 'Seedy Looking Human': 'ruffian', 'Bandit': 'scout', 'Swordsman': 'gallant', 'Magic User': 'illusionist',
 'Berzerker': 'berserker', 'Ninja': 'southron assassin', 'Evil Iggy': 'Harowen the Black Hand',
 'Red Icky-Thing': 'bloodshot icky thing', 'Giant Black Bat': 'doombat', 'Giant Long-Eared Bat': 'giant tan bat',
 'White Dragon Bat': 'blue dragon bat', 'Green Dragon Bat': 'blue dragon bat', 'Black Dragon Bat': 'red dragon bat',
 'Giant Grey Bat': 'bat of Gorgoroth', 'Huge White Bat': 'fruit bat', 'Giant Red Bat': 'giant brown bat',
 'Giant Fire Bat': 'red dragon bat', 'Giant Lightning Bat': 'blue dragon bat', 'Vorpal Bunny': 'silver mouse',
 'Giant Black Rat': 'giant grey rat', 'Giant Spotted Rat': 'giant white rat', 'Giant Glowing Rat': 'wererat',
 'Giant Clear Ant': 'giant white ant', 'Giant Ebony Ant': 'giant black ant', 'Giant Static Ant': 'giant blue ant',
 'Giant Hunter Ant': 'giant army ant', 'Giant Grey Ant Lion': 'giant grey ant', 'Giant White Ant Lion': 'giant white ant',
 'Giant Black Ant Lion': 'giant black ant', 'Giant Red Ant Lion': 'giant red ant', 'Giant Mottled Ant Lion': 'giant fire ant',
 'Killer Green Beetle': 'killer stag beetle', 'Killer Black Beetle': 'death watch beetle',
 'Killer Boring Beetle': 'killer slicer beetle', 'Killer Blue Beetle': 'killer white beetle',
 'Iridescent Beetle': 'killer iridescent beetle', 'Skeleton Hobgoblin': 'skeleton orc',
 'Skeleton 2-Headed Troll': 'skeleton etten', 'Giant Yellow Tick': 'giant white tick', 'Giant Brown Tick': 'giant fire tick',
 'Violet Mold': 'disenchanter mold', 'Black Mold': 'death mold', 'Crimson Mold': 'red mold', 'Wooden Mold': 'brown mold',
 'Troll': 'forest troll', 'Giant Troll': 'hill troll', 'Ice Troll': 'snow troll', 'Two-Headed Troll': 'etten',
 'Giant Brown Scorpion': 'giant yellow scorpion', 'Grey Ooze': 'black ooze', 'Disenchanter Ooze': 'green ooze',
 'Clear Ooze': 'gelatinous cube', 'Crystal Ooze': 'blue ooze', 'Giant Blue Dragon Fly': 'giant green dragon fly',
 'Giant Red Dragon Fly': 'giant gold dragon fly', 'Giant Purple Worm': 'purple worm',
 'Disenchanter Worm': 'disenchanter worm mass', 'King Vampire': 'vampire lord', 'King Lich': 'master lich',
 'Emperor Lich': 'archlich', 'Balrog': 'The Balrog of Moria',
}
# Umoria object -> Angband 4.2 object of the same kind (or ('monster', name)), where the names differ
OBJ = {
 'some Filthy Rags': 'Robe', '[Beginners-Magick]': '[Magic for Beginners]', '[Magick I]': '[Conjurings and Tricks]',
 '[Magick II]': '[Incantations and Illusions]', "[The Mages' Guide to Power]": '[Sorcery and Evocations]',
 '[Exorcisms and Dispellings]': '[Exorcism and Dispelling]', 'Woven Cord Armor': 'Leather Scale Mail',
 'Soft Studded Leather': 'Studded Leather Armour', 'Hard Studded Leather': 'Studded Leather Armour',
 'Soft Leather Ring Mail': 'Leather Scale Mail', 'Hard Leather Ring Mail': 'Metal Scale Mail',
 'Rusty Chain Mail': 'Chain Mail', 'Double Chain Mail': 'Augmented Chain Mail', 'Laminated Armor': 'Metal Lamellar Armour',
 '& Wooden Club': 'Quarterstaff', '& Thrusting Sword (Bilbo)': 'Rapier', '& Thrusting Sword (Baselard)': 'Main Gauche',
 '& Broken Sword': 'Short Sword', '& Broken Dagger': 'Dagger', '& Small Leather Shield': 'Leather Shield',
 '& Medium Leather Shield': 'Leather Shield', '& Large Leather Shield': 'Leather Shield',
 '& Medium Metal Shield': 'Small Metal Shield', '& Pair of Soft Leather Shoes': 'Pair of Leather Sandals',
 '& Pair of Soft Leather Boots': 'Pair of Leather Boots', '& Pair of Hard Leather Boots': 'Pair of Iron Shod Boots',
 '& Javelin': 'Spear', '& Fauchard': 'Glaive', '& Composite Bow': 'Long Bow', "& Cat-o'-Nine-Tails": 'Whip',
 '& ruined chest': 'Small wooden chest', '& Strip~ of Beef Jerky': 'Slice of Meat', '& Human Skeleton': ('monster', 'skeleton human'),
 '& Dwarf Skeleton': ('monster', 'skeleton human'), '& Elf Skeleton': ('monster', 'skeleton human'),
 '& Gnome Skeleton': ('monster', 'skeleton kobold'), '& Rat Skeleton': ('monster', 'skeleton kobold'),
 '& Giant Centipede Skeleton': ('monster', 'skeleton kobold'), '& large broken bone': ('monster', 'skeleton kobold'),
 '& broken set of teeth': ('monster', 'skeleton kobold'), '& empty bottle': 'Flask of Oil',
 '& broken stick': 'Quarterstaff', 'some shards of pottery': 'Flask of Oil',
}
# Umoria flavour -> Angband 4.2 flavour, where 4.2 has no such colour / stone
FLV = {('ring', 'Granite'): 'Quartzite', ('potion', 'Silver Speckled'): 'Shimmering',
       ('potion', 'Red Speckled'): 'Metallic Red', ('potion', 'Puce'): 'Light Purple',
       ('potion', 'Pink Speckled'): 'Light Pink', ('potion', 'Grey Speckled'): 'Grey',
       ('potion', 'Green Speckled'): 'Metallic Green', ('potion', 'Gold Speckled'): 'Gold',
       ('potion', 'Dark Blue'): 'Indigo', ('potion', 'Blue Speckled'): 'Cerulean',
       ('potion', 'Brown Speckled'): 'Ochre', ('amulet', 'Amber'): 'Golden', ('amulet', 'Coral'): 'Sea Shell'}
# Umoria mushroom colour -> Angband 4.2 mushroom (named by texture; each set draws them in other colours)
MUSH = {
 'shockbolt': {'Blue': 'Crumbly', 'Black': 'Spotted', 'Black Spotted': 'Spotted', 'Brown': 'Moist',
               'Dark Blue': 'Rubbery', 'Dark Green': 'Firm', 'Dark Red': 'Woody', 'Ecru': 'Withered',
               'Green': 'Fragile', 'Grey': 'Marbled', 'Light Blue': 'Gelatinous', 'Light Green': 'Slimy',
               'Plaid': 'Glowing', 'Red': 'Striped', 'Tan': 'Moldy', 'White': 'Waxy', 'White Spotted': 'Mottled',
               'Wooden': 'Smelly', 'Yellow': 'Wrinkled'},
 'gervais': {'Blue': 'Mottled', 'Black': 'Rubbery', 'Black Spotted': 'Crumbly', 'Brown': 'Smelly',
             'Dark Blue': 'Spotted', 'Dark Green': 'Firm', 'Dark Red': 'Woody', 'Ecru': 'Waxy',
             'Green': 'Fragile', 'Grey': 'Marbled', 'Light Blue': 'Gelatinous', 'Light Green': 'Speckled',
             'Plaid': 'Glowing', 'Red': 'Striped', 'Tan': 'Moist', 'White': 'Withered', 'White Spotted': 'Waxy',
             'Wooden': 'Smelly', 'Yellow': 'Moldy'}}


def rc(a, c):
    return (int(a, 16) - 0x80, int(c, 16) - 0x80)


def norm(s):
    s = s.lower().replace('armor', 'armour').replace('&', '').replace('~', '').replace('^', '')
    return re.sub(r'[^a-z]', '', s)


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
G30 = {}
for l in open(os.path.join(HERE, 'gervais30.tsv')):
    if not l.startswith('#'):
        k, n, r, c = l.rstrip('\n').split('\t')
        G30[(k, n)] = (int(r), int(c))


def arr(name):
    m = re.search(r'const char \*' + name + r'\[[A-Z_]+\] = \{(.*?)\};',
                  open(os.path.join(SRC, 'data_tables.cpp')).read(), re.S)
    return re.findall(r'"([^"]*)"', m.group(1))


# flavour.txt: kind -> [(text, flavour index)]
flavor_txt, kind = {}, None
for line in open(os.path.join(GD, 'flavor.txt')):
    p = line.strip().split(':')
    if p[0] == 'kind':
        kind = p[1]
    elif p[0] in ('flavor', 'fixed'):
        flavor_txt.setdefault(kind, []).append((p[-1], int(p[1])))

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
FLAVOURED = ('TV_AMULET', 'TV_RING', 'TV_WAND', 'TV_STAFF', 'TV_SCROLL1', 'TV_SCROLL2', 'TV_POTION1', 'TV_POTION2')
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


def build(set_name):
    """slots [(tile or None, key)] and the report of hand-picked things, for one tile set."""
    folder, graf, flvr, xtra_prf, _, _, _ = SETS[set_name]
    base = os.path.join(ANG, 'lib/tiles', folder)
    feat, trap, mon, obj = {}, {}, {}, {}
    for line in open(os.path.join(base, graf)):
        p = line.strip().split(':')
        if p[0] == 'feat':
            feat[(p[1], p[2])] = rc(p[3], p[4])
        elif p[0] == 'trap':
            trap[(p[1], p[2])] = rc(p[3], p[4])
        elif p[0] == 'monster':
            mon[norm(p[1])] = rc(p[2], p[3])
        elif p[0] == 'object':
            obj.setdefault(p[1].replace('armor', 'armour'), []).append((p[2], rc(p[3], p[4])))
    flvtile = {}
    for line in open(os.path.join(base, flvr)):
        p = line.strip().split(':')
        if p[0] == 'flavor':
            flvtile[int(p[1])] = rc(p[2], p[3])
    flavors = {k: [(n, flvtile[i]) for n, i in lst if i in flvtile] for k, lst in flavor_txt.items()}
    for d in (feat, trap):   # '*' = every light level
        for k in d.copy():
            if k[1] == '*':
                for lvl in ('lit', 'dark'):
                    d.setdefault((k[0], lvl), d[k])

    used, slots, report = {}, [], []

    def tid(t, key):
        """Slot for thing key (drawn with tile t): each thing gets its own slot."""
        if t is None:
            return -1
        if key not in used:
            used[key] = len(slots)
            slots.append((t, key))
        return used[key]

    def block16(t, key):
        """16 slots (one per autotile mask), aligned so they share a sheet row."""
        while len(slots) % 16:
            slots.append((None, ('pad', len(slots))))
        return [tid(t, key + (m,)) for m in range(16)][0]

    def find_obj(tvals, cands):
        for tv in tvals:
            for c in cands:
                for n, t in obj.get(tv, []):
                    if norm(n) == norm(c):
                        return t
        for c in cands:     # any kind with that name
            for lst in obj.values():
                for n, t in lst:
                    if norm(n) == norm(c):
                        return t
        return None

    # ---- monsters: exact name, else Gervais 3.0.9, else the hand table ----
    mon_tile = []
    for name, ch, lvl in creatures:
        t = mon.get(norm(name))
        if t is None and set_name == 'gervais' and ('mon', name) in G30:
            t = G30[('mon', name)]
            report.append('monster %-32s -> Angband 3.0.9 drawing' % name)
        if t is None:
            t = mon.get(norm(MON.get(name, '?')))
            if t is None:
                missing.append('%s: no tile for creature %s (hand: %s)' % (set_name, name, MON.get(name)))
            report.append('monster %-32s -> %s' % (name, MON[name]))
        mon_tile.append(tid(t, ('mon', name)))

    # ---- objects ----
    obj_tile = []
    for oi, (name, tv, ch, sub) in enumerate(objects):
        t = None
        tvals = TVAL.get(tv)
        if tvals and tv not in FLAVOURED:
            base_name = re.sub(r'\s*\(.*', '', name)
            paren = re.findall(r'\(([^)%]*)\)', name)
            cands = [name, base_name] + paren
            cands = [ALIAS[norm(c)] for c in cands if norm(c) in ALIAS] + cands
            t = find_obj(tvals, cands)
            if t is None and tv == 'TV_GOLD':
                t = find_obj(['gold'], [name])
        if tv == 'TV_RUBBLE':
            t = feat[('RUBBLE', 'lit')]
        if tv in ('TV_OPEN_DOOR',):
            t = feat[('OPEN', 'lit')]
        if tv in ('TV_CLOSED_DOOR',):
            t = feat[('CLOSED', 'lit')]
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
                t = feat[('PASS_RUBBLE', 'lit')]
            if key:
                t = trap[(key, 'lit')]
        if tv == 'TV_STORE_DOOR':
            t = feat[(['STORE_GENERAL', 'STORE_ARMOR', 'STORE_WEAPON', 'STORE_BOOK', 'STORE_ALCHEMY',
                       'STORE_MAGIC'][int(ch) - 1], 'lit')]
        # never shown: flavoured kinds and mushrooms (flavour tile at run time), a secret door (wall)
        unseen = tv in FLAVOURED or tv in ('TV_NOTHING', 'TV_SECRET_DOOR') or \
            (tv == "TV_FOOD" and (sub & 63) <= 20)
        if t is None and set_name == 'gervais' and ('obj', name) in G30:
            t = G30[('obj', name)]
            report.append('object  %-32s -> Angband 3.0.9 drawing' % name)
        if t is None and name in OBJ:
            h = OBJ[name]
            t = mon[norm(h[1])] if isinstance(h, tuple) else find_obj(tvals or [], [h])
            if t is None:
                missing.append('%s: no tile for %s (hand: %s)' % (set_name, name, h))
            report.append('object  %-32s -> %s' % (name, h if isinstance(h, str) else 'monster ' + h[1]))
        if t is None and tv == 'TV_CHEST' and 'ruined' in name:
            t = obj['chest'][0][1]
        if t is None and not unseen:
            missing.append('%s: no tile for object %s (%s)' % (set_name, name, tv))
        obj_tile.append(tid(t, ('obj', oi, tv, name)))

    # ---- flavours: Umoria's appearance names -> Angband flavour tiles ----
    def flv_table(moria, kind):
        tiles = flavors[kind]
        out = []
        for n in moria:
            want = MUSH[set_name].get(n, n) if kind == 'mushroom' else FLV.get((kind, n), n)
            t = next((t for a, t in tiles if norm(a) in (norm(want), norm(want).replace('aluminum', 'aluminium'))), None)
            if t is None:
                missing.append('%s: no tile for %s flavour %s (hand: %s)' % (set_name, kind, n, want))
            if want != n:
                report.append('flavour %-10s %-20s -> %s' % (kind, n, want))
            out.append((n, tid(t, ('flv', kind, n))))
        return out

    flv = {'potion': flv_table(arr('colors'), 'potion'), 'ring': flv_table(arr('rocks'), 'ring'),
           'amulet': flv_table(arr('amulets'), 'amulet'), 'wand': flv_table(arr('metals'), 'wand'),
           'staff': flv_table(arr('woods'), 'staff'), 'mushroom': flv_table(arr('mushrooms'), 'mushroom')}
    scroll_tiles = [tid(t, ('scroll', i)) for i, (_, t) in enumerate(flavors['scroll'])]

    # ---- player: xtra-*.prf conditions, last match wins ----
    races = ['Human', 'Half-Elf', 'Elf', 'Halfling', 'Gnome', 'Dwarf', 'Half-Orc', 'Half-Troll']
    classes = ['Warrior', 'Mage', 'Priest', 'Rogue', 'Ranger', 'Paladin']
    xtra = [l.strip() for l in open(os.path.join(base, xtra_prf))]

    def player_tile(race, cls, sex):
        env = {'$RACE': race, '$CLASS': cls, '$GENDER': sex}
        ok, tile = True, mon[norm('<player>')]
        for l in xtra:
            if l.startswith('?:'):
                conds = re.findall(r'EQU (\$\w+) ([\w-]+)', l)
                ok = all(env[v] == x for v, x in conds) if conds else True
            elif ok and l.startswith('monster:<player>:'):
                p = l.split('#')[0].strip().split(':')
                tile = rc(p[2], p[3])
        return tid(tile, ('player', race, cls, sex))

    players = [[[player_tile(r, c, s) for s in ('Female', 'Male')] for c in classes] for r in races]

    # ---- terrain ----
    T = {}
    for key, name in [('FLOOR', 'floor'), ('GRANITE', 'granite'), ('MAGMA', 'magma'), ('QUARTZ', 'quartz'),
                      ('MAGMA_K', 'magma_k'), ('QUARTZ_K', 'quartz_k'), ('PERM', 'perm'),
                      ('LESS', 'up'), ('MORE', 'down'), ('RUBBLE', 'rubble')]:
        T[name] = tuple(tid(feat[(key, l)], ('tile', name, l)) for l in ('lit', 'dark'))
    # autotiles (DawnLike): base + mask of bordered/connected sides (n8 s4 w2 e1);
    # the Angband sets have one tile per terrain, repeated 16 times
    auto_floor = [[block16(feat[('FLOOR', l)], ('floor', k, l)) for l in ('lit', 'dark')]
                  for k in ('room', 'corr', 'town')]
    auto_wall = [[block16(feat[(w, l)], ('wall', w, l)) for l in ('lit', 'dark')]
                 for w in ('GRANITE', 'MAGMA', 'QUARTZ', 'PERM')]
    tables = dict(mon_tile=mon_tile, obj_tile=obj_tile, flv=flv, scroll_tiles=scroll_tiles, players=players,
                  T=T, auto_floor=auto_floor, auto_wall=auto_wall)
    return slots, report, tables


def carr(name, vals):
    return 'static const short %s[] = {%s};\n' % (name, ','.join(str(v) for v in vals))


def sheet(set_name, slots):
    folder, _, _, _, png, size, out_name = SETS[set_name]
    src = Image.open(os.path.join(ANG, 'lib/tiles', folder, png)).convert('RGBA')
    out = Image.new('RGBA', (32 * size, (len(slots) + 31) // 32 * size))
    for i, (t, k) in enumerate(slots):
        if t is not None:
            r, c = t
            out.paste(src.crop((c * size, r * size, c * size + size, r * size + size)),
                      ((i % 32) * size, (i // 32) * size))
    out.save(os.path.join(HERE, out_name), optimize=True)
    return out


os.chdir(HERE)
missing = []
built = {s: build(s) for s in SETS}
if missing:
    sys.exit('\n'.join(missing))
slots, _, tb = built['shockbolt']
assert [k for _, k in slots] == [k for _, k in built['gervais'][0]], 'the sets must share one slot layout'
with open('tilemap.h', 'w') as f:
    f.write('// Generated by port/mktiles.py from Angband 4.2 Shockbolt/Gervais tiles. Do not edit.\n#pragma once\n')
    f.write(carr('mon_tile', tb['mon_tile']))
    f.write(carr('obj_tile', tb['obj_tile']))
    for k, lst in tb['flv'].items():
        f.write('static const struct { const char *name; short tile; } flv_%s[] = {%s};\n'
                % (k, ','.join('{"%s",%d}' % (n, t) for n, t in lst)))
    f.write(carr('scroll_tiles', tb['scroll_tiles']))
    f.write('static const short player_tiles[8][6][2] = {%s};\n' % ','.join(
        '{%s}' % ','.join('{%d,%d}' % tuple(s) for s in c) for c in tb['players']))
    for k, (lit, dark) in tb['T'].items():
        f.write('static const short T_%s[2] = {%d, %d}; // lit, dark\n' % (k.upper(), lit, dark))
    f.write('static const short T_AUTO_FLOOR[3][2] = {%s}; // room/corr/town, lit/dark: base + mask\n'
            % ','.join('{%d,%d}' % tuple(b) for b in tb['auto_floor']))
    f.write('static const short T_AUTO_WALL[4][2] = {%s}; // granite/magma/quartz/perm, lit/dark\n'
            % ','.join('{%d,%d}' % tuple(b) for b in tb['auto_wall']))
with open('slots.tsv', 'w') as f:   # what each slot shows, for mkdawn.py / mkamiga.py
    for i, (t, k) in enumerate(slots):
        f.write('%d\t%s\n' % (i, '\t'.join(str(v) for v in k)))
shb = sheet('shockbolt', slots)
with open('tiles.rgba', 'wb') as f:
    f.write(struct.pack('<II', shb.width, shb.height) + shb.tobytes())
sheet('gervais', built['gervais'][0])
for s in SETS:
    print('%s: %d slots; hand-picked:' % (s, len(built[s][0])))
    print('  ' + '\n  '.join(built[s][1]))
