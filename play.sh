#!/bin/sh
# Umoria, X11 frontend (curses shim, port/): tiled map top left, Status to
# its right, Messages + Inventory below (layout in port/be_x11.cpp; override
# with UMORIA_MAP/_STATUS/_MSG/_INV="x,y"). Save in save/, scores here.
cd "$(dirname "$0")" || exit 1
mkdir -p save
exec ./umoria "$@" save/game.sav
