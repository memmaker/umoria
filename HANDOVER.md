# Umoria 5.7.15: handover

## Source and changes

- Base: **Umoria 5.7.15**
- Original source: https://github.com/dungeons-of-moria/umoria/tree/3bf8abc (dungeons-of-moria/umoria, commit 3bf8abc)
- Our changes: https://github.com/memmaker/umoria/compare/3bf8abc...master (memmaker/umoria)
- Prompt line (RVIP step 5 / W4, 2026-09-26): the live message row is shown in a
  box over the map by `RvipWM.prompt` (rvip-wm.js). A key hides it only while
  the game waits for a command, so a question stays up until answered.
  Here: `be_prompt(r)` from `msg_refresh()` in `port/wcurses.cpp` (row 0 text),
  `js_key(at_command_prompt)` in `port/be_web.cpp`; `be_x11.cpp` has an empty
  stub.

## DawnLike tiles (2026-09-28)

- Tiles button: Shockbolt → Gervais → DawnLike → DawnLike|a (animated) → Amiga →
  None (stored by name); one slot layout. `port/mktiles.py` gives every
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
