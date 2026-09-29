# Umoria 5.7.15: handover

## Source and changes

- Base: **Umoria 5.7.15**
- Original source: https://github.com/dungeons-of-moria/umoria/tree/3bf8abc (dungeons-of-moria/umoria, commit 3bf8abc)
- Our changes: https://github.com/memmaker/umoria/compare/3bf8abc...master (memmaker/umoria)
- Prompt line (RVIP.md Stage 5, lessons 5.9): the live message row is shown in a
  box over the map by `RvipWM.prompt` (rvip-wm.js). A key hides it only while
  the game waits for a command, so a question stays up until answered.
  Here: `be_prompt(r)` from `msg_refresh()` in `port/wcurses.cpp` (row 0 text),
  `js_key(at_command_prompt)` in `port/be_web.cpp`; `be_x11.cpp` has an empty
  stub.

## Tiles (RVIP.md Stage 4, lessons 5.8)

- Tiles button: Shockbolt (default) → Gervais → DawnLike → DawnLike|a (animated)
  → Amiga → None, stored by name as `tiles` in `save/web-layout.json` (IDBFS);
  a numeric `tiles` maps by the pre-Gervais order (Shockbolt, DawnLike,
  DawnLike|a, None). One slot layout: C (`port/tiles.cpp`) picks each cell's
  slot, JS only blits from the chosen sheet; each sheet fills every slot from
  its own set only (never mixed).
- Coverage (1040 slots: 279 creatures, 200 objects, 164 flavours, 51 scroll
  titles, 96 player race/class/sex, terrain/autotiles), stand-ins = another
  tile of the same set picked by hand (`python3 port/mktiles.py` lists them):
  - Shockbolt: creatures 183 own + 96 stand-ins, objects 157 + 43,
    flavours 132 + 32; terrain and player all own.
  - Gervais: creatures 196 own (13 from Gervais' Angband 3.0.9 drawings) + 83,
    objects 178 (21 from 3.0.9) + 22, flavours 132 + 32.
  - DawnLike: all 1034 used slots DawnLike; creatures 44 by name + 235
    stand-ins (`MON`), objects mostly by hand (`OBJ`, 174 entries).
  - Amiga: Amiga Moria 1.2's own pictures, one per monster letter / object
    group (by design, not per kind). `port/mktiles.py` gives every
  creature/object/flavour/player/terrain its own slot, writes `port/slots.tsv`
  and builds Shockbolt `tiles.png` + Gervais `tiles-gervais.png`: exact Angband
  4.2 name, else (Gervais) the Angband 3.0.9 drawing (`port/gervais30.tsv`,
  `port/mkgervais30.py`), else the hand tables `MON`/`OBJ`/`FLV`/`MUSH`;
  nothing is guessed (a missing tile stops the script). `port/mkdawn.py` builds
  `port/tiles-dawn.png` / `tiles-dawn-1.png` (hand table `MON` for stand-ins);
  `port/mkamiga.py` builds `tiles-amiga.png` from `port/amiga` (Amiga Moria 1.2:
  one picture per monster letter and object group, no stand-ins).
- Order after a layout change: `mktiles.py`, then `mkdawn.py` and `mkamiga.py`.
- Autotiles (`port/tiles.cpp` `floor_tile`): floors are `base + mask` of
  bordered sides from the real level (room tile, corridor dirt, town grass;
  day/night = lit/dark); walls connect only to walls that border open ground
  (inner rock is flat), so rooms and buildings get outlines. Shockbolt repeats
  its one tile in all 16 slots. Credits: `port/dawnlike/CREDITS.txt`.
