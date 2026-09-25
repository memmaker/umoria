#!/usr/bin/env python3
"""Copy the Dubtrain samples for the sound events Umoria raises (soundEvent()
in src/, grep for it) into <out> and write <out>/sounds.json {event: [files]}."""
import json, os, shutil, sys
PACK = os.path.expanduser('~/Downloads/Dubtrain Angband Sound Pack v3.1.0')
EVENTS = ['hit', 'miss', 'kill', 'kill_king', 'mon_hit', 'money1', 'level', 'hungry', 'stairs_up',
          'stairs_down', 'drop', 'wield', 'eat', 'quaff', 'use_staff', 'zap_rod', 'pray_prayer',
          'cast_spell', 'shoot', 'teleport', 'death', 'store5']
out = sys.argv[1]
cfg = {}
for line in open(os.path.join(PACK, 'sound.cfg'), encoding='latin-1'):
    if '=' in line and not line.lstrip().startswith('#'):
        k, v = line.split('=', 1)
        cfg[k.strip()] = v.split()
os.makedirs(out, exist_ok=True)
used = {e: cfg.get(e, []) for e in EVENTS}
for files in used.values():
    for f in files:
        shutil.copy(os.path.join(PACK, f), out)
json.dump(used, open(os.path.join(out, 'sounds.json'), 'w'))
