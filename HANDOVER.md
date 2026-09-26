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
