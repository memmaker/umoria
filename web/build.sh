#!/bin/sh
# Build Umoria for the browser (Emscripten + Asyncify) into web/dist;
# deploy with web/deploy.sh. Run with sh (zsh doesn't split $SRCS).
set -e
cd "$(dirname "$0")/.."
OUT=web/dist
rm -rf "$OUT" web/stage && mkdir -p "$OUT" web/stage/data
make -C port ../data/splash.txt ../data/versions.txt >/dev/null
cp data/*.txt data/scores.dat web/stage/data/
# -DUMORIA_X11 selects the curses shim (port/wcurses.h); be_web.cpp replaces be_x11.cpp
em++ -O2 -std=c++14 -DUMORIA_X11 -Isrc -Iport -w \
	src/*.cpp port/wcurses.cpp port/tiles.cpp port/be_web.cpp \
	-o "$OUT/umoria-core.js" \
	-sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 -sSTACK_SIZE=1048576 \
	-sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=32MB \
	-sEXPORTED_FUNCTIONS=_main \
	-sEXPORTED_RUNTIME_METHODS=FS,IDBFS,HEAPU8,addRunDependency,removeRunDependency \
	-sFORCE_FILESYSTEM -lidbfs.js -sENVIRONMENT=web \
	--preload-file web/stage/data@/umoria/data
cp web/index.html web/umoria.js port/tiles.png "$OUT/"
# sound effects (message text -> Dubtrain samples) and the town music
mkdir -p "$OUT/sound" "$OUT/music"
python3 web/sounds.py "$OUT/sound"
cp ~/Projects/heavenAndHell/files/mods/heavenandhell/music/new_town.ogg "$OUT/music/"
python3 web/make-help.py > "$OUT/help.html"
rm -rf web/stage
ls -la "$OUT"
