#!/usr/bin/env python3
"""Second tile set: DawnLike (DragonDePlatino, palette DawnBringer, CC BY 4.0),
sprites picked by name from rvip-tools/tilesets/dawnlike_names.tsv
(names: Tommy Ettinger's DawnLikeAtlas).

Writes tiles-dawn.png and tiles-dawn-1.png (2nd animation frame, web only)
with the same slot layout as tiles.png: mktiles.py lists every slot in
slots.tsv. Every slot gets a DawnLike sprite, nothing is left Shockbolt:
tile sets are never mixed. Stand-ins where DawnLike lacks a creature are in MON.
Run after mktiles.py: python3 port/mkdawn.py"""
import os, re, sys
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
TS = os.path.expanduser('~/Games/rvip-tools/tilesets')
sys.path.insert(0, TS)
from dawnlike_preview import pos, sprite

MON = {  # Umoria creature -> DawnLike name, where they differ (stand-ins)
 'Filthy Street Urchin': 'peasant man', 'Blubbering Idiot': 'meathead', 'Pitiful-Looking Beggar': 'prisoner',
 'Mangy-Looking Leper': 'convict', 'Squint-Eyed Rogue': 'thief', 'Singing, Happy Drunk': 'farmer man',
 'Mean-Looking Mercenary': 'soldier', 'Battle-Scarred Veteran': 'captain',
 'Grey Mushroom patch': 'saddle mushroom', 'Giant Yellow Centipede': 'centipede', 'Giant White Centipede': 'centipede',
 'White Icky-Thing': 'larva', 'Clear Icky-Thing': 'invisible creature', 'Giant White Mouse': 'rodent of unusual size',
 'Large Brown Snake': 'snake', 'Large White Snake': 'garter snake', 'White Worm mass': 'maggot',
 'Shrieker Mushroom patch': 'shrieker', 'Blubbering Icky-Thing': 'baby slug', 'Metallic Green Centipede': 'centipede',
 'Novice Warrior': 'fighter', 'Novice Rogue': 'thief', 'Novice Priest': 'aide', 'Novice Mage': 'healer',
 'Yellow Mushroom patch': 'umbrella mushroom', 'White Jelly': 'quivering blob', 'Giant Green Frog': 'frog',
 'Giant Black Ant': 'giant ant', 'White Harpy': 'dove', 'Blue Yeek': 'gnome', 'Green Worm mass': 'dung worm',
 'Large Black Snake': 'water moccasin', 'Poltergeist': 'wisp', 'Metallic Blue Centipede': 'centipede',
 'Giant White Louse': 'giant louse', 'Spotted Mushroom patch': 'spotted mushroom', 'Yellow Jelly': 'ochre jelly',
 'Scruffy-Looking Hobbit': 'hobbit', 'Huge Brown Bat': 'giant bat', 'Giant White Ant': 'snow ant',
 'Metallic Red Centipede': 'centipede', 'Yellow Worm mass': 'larva', 'Large Green Snake': 'snake',
 'Radiation Eye': 'evil eye', 'Drooling Harpy': 'seagull', 'Silver Mouse': 'gray squirrel',
 'Black Mushroom patch': 'hardball mushroom', 'Creeping Copper Coins': 'pile of copper coins',
 'Giant White Rat': 'giant rat', 'Giant Black Centipede': 'centipede', 'Giant Blue Centipede': 'centipede',
 'Blue Worm mass': 'bobbit worm', 'Large Grey Snake': 'python', 'Green Naga': 'white naga',
 'Green Glutton Ghost': 'wisp', 'White Mushroom patch': 'luminous mushroom', 'Green Jelly': 'green slime',
 'Skeleton Kobold': 'skeleton', 'Silver Jelly': 'blue slime', 'Giant Black Frog': 'frog',
 'Grey Icky-Thing': 'giant slug', 'Disenchanter Eye': 'eye tyrant', 'Black Yeek': 'gnome lord',
 'Red Worm mass': 'tunnel worm', 'Giant House Fly': 'tsetse fly', 'Copperhead Snake': 'pit viper',
 'Rot Jelly': 'brown pudding', 'Purple Mushroom patch': 'red cap mushroom', 'Giant Brown Bat': 'giant bat',
 'Creeping Silver Coins': 'pile of silver coins', 'Grey Harpy': 'cormorant', 'Blue Icky-Thing': 'giant snail',
 'Rattlesnake': 'snake', 'Bloodshot Eye': 'floating eye', 'Red Jelly': 'spotted jelly',
 'Giant Red Frog': 'frog', 'Green Icky-Thing': 'leech', 'Zombie Kobold': 'kobold zombie',
 'Lost Soul': 'shade', 'Greedy Little Gnome': 'leprechaun', 'Giant Green Fly': 'tsetse fly',
 'Brown Yeek': 'gnome', 'Skeleton Orc': 'shadow skeleton', 'Seedy Looking Human': 'desperado',
 'Red Icky-Thing': 'bee grub', 'Bloodshot Icky-Thing': 'giant slug', 'Giant Grey Rat': 'sewer rat',
 'Black Harpy': 'nighthawk', 'Giant Black Bat': 'vampire bat', 'Clear Yeek': 'invisible creature',
 'Giant Red Ant': 'fire ant', 'King Cobra': 'cobra', 'Clear Mushroom patch': 'fairy step mushrooms',
 'Giant White Tick': 'giant tick', 'Hairy Mold': 'mould', 'Disenchanter Mold': 'ungenomold',
 'Giant Red Centipede': 'centipede', 'Creeping Gold Coins': 'pile of gold coins', 'Giant Fruit Fly': 'firefly',
 'Brigand': 'bandit', 'Orc Warrior': 'militant orc', 'Vorpal Bunny': 'jackrabbit', 'Nasty Little Gnome': 'gnome wizard',
 'Black Mamba': 'asp', 'Grape Jelly': 'black pudding', 'Master Yeek': 'gnome king', 'Giant Clear Ant': 'soldier ant',
 'Air Spirit': 'air elemental', 'Skeleton Human': 'skeleton', 'Moaning Spirit': 'spirit', 'Swordsman': 'samurai',
 'Killer Brown Beetle': 'giant beetle', 'Giant Red Speckled Frog': 'frog', 'Magic User': 'wizard',
 'Black Orc': 'deep orc', 'Giant Long-Eared Bat': 'white nosed bat', 'Giant Gnat': 'giant flea',
 'Killer Green Beetle': 'killer beetle', 'Giant White Dragon Fly': 'dragonfly', 'Skeleton Hobgoblin': 'skeleton',
 'White Dragon Bat': 'baby bat', 'Giant Black Louse': 'giant louse', 'Giant Grey Bat': 'giant bat',
 'Giant Clear Centipede': 'centipede', 'Giant Yellow Tick': 'giant tick', 'Giant Ebony Ant': 'giant ant',
 'Huge White Bat': 'white nosed bat', 'Giant Tan Bat': 'giant bat', 'Violet Mold': 'violet fungus',
 'Giant Black Rat': 'enormous rat', 'Giant Green Dragon Fly': 'dragonfly', 'Green Dragon Bat': 'baby bat',
 'Water Spirit': 'water elemental', 'Giant Brown Scorpion': 'scorpion', 'Earth Spirit': 'earth elemental',
 'Fire Spirit': 'fire elemental', 'Uruk-Hai Orc': 'uruk', 'Grey Ooze': 'gray ooze', 'Disenchanter Ooze': 'blue slime',
 'Giant Spotted Rat': 'rabid rat', 'Mummified Kobold': 'kobold mummy', 'Killer Black Beetle': 'killer beetle',
 'Quylthulg': 'huge meat blob', 'Giant Red Bat': 'vampire bat', 'Giant Black Dragon Fly': 'dragonfly',
 'Cloud Giant': 'storm giant', 'Black Dragon Bat': 'vampire bat', 'Blue Dragon Bat': 'baby bat',
 'Mummified Orc': 'orc mummy', 'Killer Boring Beetle': 'giant beetle', 'Killer Stag Beetle': 'spitting beetle',
 'Black Mold': 'mould', 'Giant Yellow Scorpion': 'giant scorpion', 'Green Ooze': 'green slime',
 'Black Ooze': 'black pudding', 'Warrior': 'barbarian', 'Red Dragon Bat': 'vampire bat',
 'Killer Blue Beetle': 'killer beetle', 'Giant Silver Ant': 'soldier ant', 'Crimson Mold': 'red mold',
 'Forest Wight': 'barrow wight', 'Berzerker': 'valkyrie', 'Mummified Human': 'human mummy', 'Banshee': 'weeping angel',
 'Giant Troll': 'olog hai', 'Giant Brown Tick': 'giant tick', 'Killer Red Beetle': 'spitting beetle',
 'Wooden Mold': 'brown mold', 'Giant Blue Dragon Fly': 'dragonfly', 'Giant Grey Ant Lion': 'giant ant',
 'Disenchanter Bat': 'werebat', 'Giant Fire Tick': 'giant tick', 'White Wraith': 'wraith',
 'Giant Black Scorpion': 'giant scorpion', 'Clear Ooze': 'gelatinous cube', 'Killer Fire Beetle': 'spitting beetle',
 'Giant Red Dragon Fly': 'dragonfly', 'Shimmering Mold': 'yellow light', 'Black Knight': 'knight', 'Mage': 'archmage',
 'Giant Purple Worm': 'purple worm', 'Young Blue Dragon': 'baby stormwyrm', 'Young White Dragon': 'baby icewyrm',
 'Young Green Dragon': 'baby glendrake', 'Giant Fire Bat': 'vampire bat', 'Giant Glowing Rat': 'rabid rat',
 'Skeleton Troll': 'fire skeleton', 'Giant Lightning Bat': 'werebat', 'Giant Static Ant': 'fire ant',
 'Grave Wight': 'barrow wight', 'Killer Slicer Beetle': 'killer beetle', 'Giant White Ant Lion': 'snow ant',
 'Giant Black Ant Lion': 'giant ant', 'Death Watch Beetle': 'killer beetle', 'Ogre Mage': 'ogre lord',
 'Two-Headed Troll': 'ettin', 'Invisible Stalker': 'stalker', 'Giant Hunter Ant': 'soldier ant',
 'Skeleton 2-Headed Troll': 'plague skeleton', 'Master Vampire': 'vampire lord', 'Spirit Troll': 'water troll',
 'Giant Red Scorpion': 'scorpius', 'Young Black Dragon': 'baby darkwyrm', 'Young Red Dragon': 'baby firedrake',
 'Necromancer': 'vampire mage', 'Mummified Troll': 'giant mummy', 'Giant Red Ant Lion': 'fire ant',
 'Mature White Dragon': 'icewyrm', 'Giant Mottled Ant Lion': 'giant ant', 'Grey Wraith': 'wraith',
 'Young Multi-Hued Dragon': 'baby kingwyrm', 'Mature Blue Dragon': 'storrmwyrm', 'Mature Green Dragon': 'glendrake',
 'Iridescent Beetle': 'killer beetle', 'King Vampire': 'vlad the impaler', 'King Lich': 'master lich',
 'Mature Red Dragon': 'firedrake', 'Mature Black Dragon': 'darkwyrm', 'Mature Multi-Hued Dragon': 'kingwyrm',
 'Ancient White Dragon': 'icewyrm', 'Emperor Wight': 'nazgul', 'Black Wraith': 'wraith', 'Nether Wraith': 'dark one',
 'Sorcerer': 'archmage', 'Ancient Blue Dragon': 'storrmwyrm', 'Ancient Green Dragon': 'glendrake',
 'Ancient Black Dragon': 'dreadwyrm', 'Crystal Ooze': 'crystal golem', 'Disenchanter Worm': 'long worm',
 'Rotting Quylthulg': 'spoiled huge meat blob', 'Ancient Red Dragon': 'firedrake', 'Death Quasit': 'imp',
 'Emperor Lich': 'demilich', 'Ancient Multi-Hued Dragon': 'kingwyrm', 'Evil Iggy': 'wizard of yendor',
}
OBJ = {  # Umoria object (without '& ' and '~') -> DawnLike
 'Human Skeleton': 'bones', 'Dwarf Skeleton': 'bones', 'Elf Skeleton': 'bones', 'Gnome Skeleton': 'old bones',
 'Rat Skeleton': 'old bones', 'Giant Centipede Skeleton': 'old bones', 'large broken bone': 'old bones',
 'broken set of teeth': 'old skull', 'empty bottle': 'bottle', 'some shards of pottery': 'shards a',
 'broken stick': 'club',
 'Ration of Food': 'food ration', 'Slime Mold': 'slime mold', 'Piece of Elvish Waybread': 'lembas wafer',
 'Hard Biscuit': 'k ration', 'Strip of Beef Jerky': 'strip of meat', 'Pint of Fine Ale': 'tin coffee cup',
 'Pint of Fine Wine': 'closed keg', 'Pint of Fine Grade Mush': 'c ration',
 'Dagger (Main Gauche)': 'elven dagger', 'Dagger (Misericorde)': 'dagger', 'Dagger (Stiletto)': 'stiletto',
 'Dagger (Bodkin)': 'orcish dagger', 'Broken Dagger': 'knife', 'Backsword': 'short sword',
 'Bastard Sword': 'long sword', 'Thrusting Sword (Bilbo)': 'dwarvish short sword',
 'Thrusting Sword (Baselard)': 'elven short sword', 'Broadsword': 'broadsword',
 'Two-Handed Sword (Claymore)': 'two handed sword', 'Cutlass': 'scimitar',
 'Two-Handed Sword (Espadon)': 'elven broadsword', "Executioner's Sword": 'tsurugi',
 'Two-Handed Sword (Flamberge)': 'runesword', 'Foil': 'silver saber', 'Katana': 'katana', 'Longsword': 'long sword',
 'Two-Handed Sword (No-Dachi)': 'tsurugi', 'Rapier': 'silver saber', 'Sabre': 'scimitar',
 'Small Sword': 'orcish short sword', 'Two-Handed Sword (Zweihander)': 'two handed sword', 'Broken Sword': 'crysknife',
 'Ball and Chain': 'flail', "Cat-o'-Nine-Tails": 'bullwhip', 'Wooden Club': 'club', 'Flail': 'flail',
 'Two-Handed Great Flail': 'nunchaku', 'Morningstar': 'morning star', 'Mace': 'mace', 'War Hammer': 'war hammer',
 'Lead-Filled Mace': 'aklys', 'Awl-Pike': 'vulgar polearm', 'Beaked Axe': 'beaked polearm', 'Fauchard': 'pole sickle',
 'Glaive': 'single edged polearm', 'Halberd': 'angled poleaxe', 'Lucerne Hammer': 'hooked polearm', 'Pike': 'long poleaxe',
 'Spear': 'spear', 'Lance': 'lance', 'Javelin': 'javelin', 'Battle Axe (Balestarius)': 'battle axe',
 'Battle Axe (European)': 'axe', 'Broad Axe': 'pole cleaver', 'Short Bow': 'shortbow', 'Long Bow': 'longbow',
 'Composite Bow': 'composite bow', 'Light Crossbow': 'crossbow', 'Heavy Crossbow': 'crossbow', 'Sling': 'sling',
 'Arrow': 'arrow', 'Bolt': 'crossbow bolt', 'Rounded Pebble': 'pebble', 'Iron Shot': 'large bullets',
 'Iron Spike': 'dart', 'Brass Lantern': 'brass lantern', 'Wooden Torch': 'tallow candle',
 'Orcish Pick': 'pick axe', 'Dwarven Pick': 'mattock', 'Gnomish Shovel': 'pick axe', 'Dwarven Shovel': 'mattock',
 'Pick': 'pick axe', 'Shovel': 'mattock',
 'Pair of Soft Leather Shoes': 'leather shoes', 'Pair of Soft Leather Boots': 'leather boots',
 'Pair of Hard Leather Boots': 'mountaineer boots', 'Soft Leather Cap': 'headband', 'Hard Leather Cap': 'elven leather helm',
 'Metal Cap': 'dented pot', 'Iron Helm': 'orcish helm', 'Steel Helm': 'visored helm', 'Silver Crown': 'princely crown',
 'Golden Crown': 'kingly crown', 'Jewel-Encrusted Crown': 'kingly crown', 'Robe': 'monk robes',
 'Soft Leather Armor': 'animal hide', 'Soft Studded Leather': 'bronze armor', 'Hard Leather Armor': 'brass armor',
 'Hard Studded Leather': 'lacquered armor', 'Woven Cord Armor': 'peasant robes', 'Soft Leather Ring Mail': 'hotrock mail',
 'Hard Leather Ring Mail': 'chain shirt', 'Leather Scale Mail': 'scale armor', 'Metal Scale Mail': 'scale armor',
 'Chain Mail': 'grandmaster mail', 'Rusty Chain Mail': 'blacksludge mail', 'Double Chain Mail': 'banded mail',
 'Augmented Chain Mail': 'shockfrost mail', 'Bar Chain Mail': 'colorwind mail', 'Metal Brigandine Armor': 'iron armor',
 'Laminated Armor': 'breastplate', 'Partial Plate Armor': 'breastplate', 'Metal Lamellar Armor': 'iron armor',
 'Full Plate Armor': 'full plate', 'Ribbed Plate Armor': 'mirror plate', 'Cloak': 'hill cloak',
 'Set of Leather Gloves': 'leather glove', 'Set of Gauntlets': 'iron gauntlet', 'Small Leather Shield': 'small shield',
 'Medium Leather Shield': 'orcish shield', 'Large Leather Shield': 'white handed shield', 'Small Metal Shield': 'elven shield',
 'Medium Metal Shield': 'dwarvish shield', 'Large Metal Shield': 'large shield', 'Flask of Oil': 'lamp',
 '[Beginners-Magick]': 'blank book', '[Magick I]': 'elemental tome', '[Magick II]': 'shimmering tome',
 "[The Mages' Guide to Power]": 'chaos tome', '[Beginners Handbook]': 'worn book', '[Words of Wisdom]': 'light tome',
 '[Chants and Blessings]': 'order tome', '[Exorcisms and Dispellings]': 'book of the dead',
 'Small Wooden Chest': 'closed chest', 'Large Wooden Chest': 'closed big chest', 'Small Iron Chest': 'closed ice chest',
 'Large Iron Chest': 'closed big chest', 'Small Steel Chest': 'closed safe', 'Large Steel Chest': 'closed big safe',
 'ruined chest': 'broken chest', 'some Filthy Rags': 'prisoner outfit',
 'open door': 'open wooden door front', 'closed door': 'closed wooden door front',
 'an up staircase': 'small stairs up', 'a down staircase': 'small stairs down',
 'General Store': 'empty shop sign', 'Armory': 'armory sign', 'Weapon Smiths': 'smithy sign', 'Temple': 'church sign',
 'Alchemy Shop': 'pub sign', 'Magic Shop': 'spooky sign',
 'an open pit': 'pit tile', 'a covered pit': 'spiked pit tile', 'an arrow trap': 'arrow trap tile',
 'a trap door': 'trap door tile', 'a gas trap': 'sleeping gas trap tile', 'a loose rock': 'falling rock trap tile',
 'some loose rock': 'rolling boulder trap tile', 'a dart trap': 'dart trap tile', 'a strange rune': 'magic trap tile',
 'a blackened spot': 'fire trap tile', 'some corroded rock': 'rust trap tile', 'some rubble': 'pile of stones',
 'copper': 'pile of copper coins', 'silver': 'pile of silver coins', 'gold': 'pile of gold coins',
 'garnets': 'dull red gem', 'opals': 'gleaming white gem', 'sapphires': 'gleaming blue gem', 'rubies': 'gleaming red gem',
 'diamonds': 'gleaming clear gem', 'emeralds': 'gleaming green gem', 'mithril': 'silvery metal stone',
}
MUSHROOMS = ['spotted mushroom', 'red cap mushroom', 'umbrella mushroom', 'saddle mushroom', 'hardball mushroom',
             'hog ear mushrooms', 'luminous mushroom', 'slimy mushroom', 'big snout mushroom', 'fairy step mushrooms',
             'puffball fungus', 'globe fungus', 'disc fungus', 'christmas fungus']
FLV = {  # Umoria appearance -> DawnLike, beyond "<appearance> <kind>"
 'potion': {'Icky Green': 'dark green potion', 'Light Brown': 'murky potion', 'Azure': 'sky blue potion',
            'Blue': 'brilliant blue potion', 'Bubbling': 'bubbly potion', 'Crimson': 'ruby potion',
            'Dark Red': 'sanguine potion', 'Gold Speckled': 'golden potion', 'Green': 'emerald potion',
            'Hazy': 'milky potion', 'Purple': 'purple red potion', 'Metallic Purple': 'magenta potion',
            'Silver Speckled': 'sparkly potion', 'Misty': 'fizzy potion', 'Dark Blue': 'dark potion'},
 'ring': {'Onyx': 'black onyx ring', 'Tiger Eye': 'tiger eye ring'},
 'amulet': {'Amethyst': 'amethyst pendant', 'Agate': 'jasper amulet', 'Coral': 'choker necklace',
            'Ivory': 'pearl necklace', 'Obsidian': 'balance talisman', 'Bone': 'misshapen talisman',
            'Amber': 'garnet pendant', 'Driftwood': 'keepsake necklace', 'Brass': 'fuzzy amulet',
            'Bronze': 'ruby pendant', 'Pewter': 'sapphire pendant', 'Tortoise Shell': 'jade pendant'},
 'wand': {'Rusty': 'rusted wand', 'Cast Iron': 'iron wand', 'Gold': 'glimmering wand', 'Steel-Plated': 'cold steel wand'},
}
POOL = {'potion': ' potion', 'ring': ' ring', 'amulet': None, 'wand': ' wand', 'staff': ' wand', 'scroll': ' scroll'}
# class sprites per race: Warrior Mage Priest Rogue Ranger Paladin; (male, female) where they differ
HUMAN = [('fighter', 'valkyrie'), 'wizard', ('priest', 'priestess'), 'thief', 'ranger', 'knight']
PLAYER = {
 'Human': HUMAN, 'Half-Elf': ['barbarian', 'archmage', 'healer', 'ninja', 'wood elf', 'samurai'],
 'Elf': ['elf lord', 'gray elf', 'green elf', 'elf', 'wood elf', 'elf king'],
 'Halfling': ['hobbit'] * 6, 'Gnome': ['gnome lord', 'gnome wizard', 'gnome', 'gnome', 'gnome', 'gnome king'],
 'Dwarf': ['dwarf barbarian', 'dwarf expert', 'dwarf healer', 'dwarf nomad', 'dwarf ranger', 'dwarf knight'],
 'Half-Orc': ['orc barbarian', 'orc wizard', 'orc shaman', 'orc rogue', 'orc samurai', 'orc knight'],
 'Half-Troll': ['olog hai', 'troll', 'water troll', 'rock troll', 'ice troll', 'troll'],
}
CLASSES = ['Warrior', 'Mage', 'Priest', 'Rogue', 'Ranger', 'Paladin']
# terrain: autotile styles; mask 0 = inner rock (flat) (day/night floors, lit/dark walls)
FLOOR = {'room': 'tile', 'corr': 'dirt', 'town': 'grass'}
WALL = {'GRANITE': 'rock', 'MAGMA': 'infernal', 'QUARTZ': 'deep', 'PERM': 'brick'}
WALLDIR = ['flat', 'right', 'left', 'left right', 'down', 'right down', 'left down', 'left right down',
           'up', 'right up', 'left up', 'left right up', 'up down', 'right up down', 'left up down', 'left right up down']
FIX = {'right': 'left right', 'left': 'left right', 'down': 'up down', 'up': 'up down'}
T_NAMES = {'floor': 'day tile floor c', 'granite': 'lit rock wall flat', 'magma': 'lit infernal wall flat',
           'quartz': 'lit deep wall flat', 'magma_k': 'lit infernal wall center', 'quartz_k': 'lit deep wall center',
           'perm': 'lit brick wall flat', 'up': 'small stairs up', 'down': 'small stairs down', 'rubble': 'pile of stones'}


def wall(style, light, m):   # m: connected sides, n8 s4 w2 e1
    d = WALLDIR[m]
    return '%s %s wall %s' % ('lit' if light == 'lit' else 'dark', style, FIX.get(d, d))


def floor(style, light, m):  # m: bordered sides, n8 s4 w2 e1
    sides = ''.join(c for b, c in ((8, 'n'), (4, 's'), (2, 'w'), (1, 'e')) if m & b) or 'c'
    return '%s %s floor %s' % ('day' if light == 'lit' else 'night', style, sides)


rows = [l.rstrip('\n').split('\t') for l in open(os.path.join(HERE, 'slots.tsv'))]
h = open(os.path.join(HERE, 'tilemap.h')).read()
tname = {}
for n, lit, dark in re.findall(r'short T_(\w+)\[2\] = \{(\d+), (\d+)\}', h):
    tname[int(lit)] = tname[int(dark)] = n.lower()

want = {}
spare = {k: iter([]) for k in POOL}
taken = set()


def pool(kind):
    suf = POOL[kind]
    names = sorted(k for k in pos if (k.endswith(suf) if suf else 'Amulet' in pos[k][0])
                   and 'grenade' not in k and k not in taken)
    wood = [k for k in names if kind == 'staff' and k.split()[0] in
            ('balsa', 'maple', 'oak', 'pine', 'redwood', 'cherry', 'ebony', 'bone', 'ivory', 'curved', 'forked')]
    return (wood + [k for k in names if k not in wood]) * 3


flv_rows = [r for r in rows if r[1] in ('flv', 'scroll')]
for r in flv_rows:   # exact names first
    kind = r[2] if r[1] == 'flv' else 'scroll'
    if kind == 'mushroom':
        continue
    n = FLV.get(kind, {}).get(r[3]) if r[1] == 'flv' else None
    if n is None and r[1] == 'flv' and POOL.get(kind):
        n = r[3].lower().replace('aluminum-plated', 'aluminum') + POOL[kind]
    if n in pos and n not in taken:
        want[int(r[0])] = n
        taken.add(n)
left = {}
mush = 0
for r in flv_rows:
    s = int(r[0])
    kind = r[2] if r[1] == 'flv' else 'scroll'
    if kind == 'mushroom':
        want[s] = MUSHROOMS[mush % len(MUSHROOMS)]
        mush += 1
    elif s not in want:
        if kind not in left:
            left[kind] = iter(pool(kind))
        want[s] = next(left[kind])
        if kind in ('wand', 'staff', 'ring', 'amulet'):
            taken.add(want[s])
            left[kind] = iter([k for k in left[kind] if k not in taken] or pool(kind))

nfood = 0
for r in rows:
    s, what = int(r[0]), r[1]
    if what == 'mon':
        want[s] = MON.get(r[2], r[2].lower())
    elif what == 'obj':
        name = r[4].replace('& ', '').replace('~', '')
        if r[3] == 'TV_FOOD' and name not in OBJ:     # mushrooms: flavoured at run time
            want[s] = MUSHROOMS[nfood % len(MUSHROOMS)]
            nfood += 1
        else:
            want[s] = OBJ[name]
    elif what == 'player':
        c = PLAYER[r[2]][CLASSES.index(r[3])]
        want[s] = c if isinstance(c, str) else c[r[4] == 'Female']
    elif what == 'floor':
        want[s] = floor(FLOOR[r[2]], r[3], int(r[4]))
    elif what == 'wall':
        want[s] = wall(WALL[r[2]], r[3], int(r[4]))
    elif what == 'tile':
        want[s] = T_NAMES[tname[s]]

missing = sorted(set(n for n in want.values() if n not in pos))
if missing:
    sys.exit('no DawnLike sprite: ' + '; '.join(missing))
used = [r for r in rows if r[1] != 'pad']
assert all(int(r[0]) in want for r in used), [r for r in used if int(r[0]) not in want]


def sprite1(name):
    sheet, c, r = pos[name]
    p = os.path.join(TS, 'DawnLike', sheet.replace('0.png', '1.png'))
    if not sheet.endswith('0.png') or not os.path.exists(p):
        return sprite(name)
    return Image.open(p).convert('RGBA').crop((c * 16, r * 16, c * 16 + 16, r * 16 + 16))


img = Image.new('RGBA', (32 * 16, (len(rows) + 31) // 32 * 16))
img1 = img.copy()
for s, n in want.items():
    xy = ((s % 32) * 16, (s // 32) * 16)
    img.paste(sprite(n), xy)
    img1.paste(sprite1(n), xy)
img.save(os.path.join(HERE, 'tiles-dawn.png'), optimize=True)
img1.save(os.path.join(HERE, 'tiles-dawn-1.png'), optimize=True)
print(len(want), 'slots, all DawnLike;', len(MON), 'creature stand-ins by hand')
