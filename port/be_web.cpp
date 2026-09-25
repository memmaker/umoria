// Browser frontend for the curses shim (RVIP step 7): web/umoria.js draws
// the panes (Module.um); input waits with Asyncify. Autosave at the command
// prompt when the page asks for it (autosaveGame(): the game goes on).
#include <emscripten.h>
#include "../src/headers.h"
#include "wcurses.h"

EM_JS(void, js_init, (int p, int c, int r), { Module.um.init(p, c, r); });
EM_JS(void, js_put, (int p, int y, int x, int ch, int t, int u), { Module.um.put(p, y, x, ch, t, u); });
EM_JS(void, js_cursor, (int p, int y, int x), { Module.um.cursor(p, y, x); });
EM_JS(void, js_popup, (int r, int c), { Module.um.popup(r, c); });
EM_JS(void, js_flush, (int lvl, int town, int hy, int hx), { Module.um.flush(lvl, town, hy, hx); });
EM_JS(int, js_key, (void), { return Module.um.key(); });
EM_JS(int, js_want_save, (void), { return Module.um.wantSave(); });
EM_JS(void, js_sound, (const char *s), { Module.um.sound(UTF8ToString(s)); });
EM_JS(void, js_end, (int dead), { Module.um.end(dead); });

void be_init(int p, int cols, int rows) { js_init(p, cols, rows); }
void be_put(int p, int y, int x, chtype ch, int tile, int under) { js_put(p, y, x, (int) ch, tile, under); }
void be_cursor(int p, int y, int x) { js_cursor(p, y, x); }
void be_popup(int rows, int cols) { js_popup(rows, cols); }
void be_sound(const char *event) { js_sound(event); }
void be_flush() {
    bool play = game.character_generated && !game.character_is_dead;
    // player position in the Map pane (screen row 1, column 13 = pane 0,0):
    // the page scrolls a zoomed-in map to keep it in view
    js_flush(play ? dg.current_level : -1, play && dg.current_level == 0, py.pos.y - dg.panel.row_prt - 1,
             py.pos.x - dg.panel.col_prt - 13);
}

int be_getkey(int wait) {
    static double last;
    for (;;) {
        if (at_command_prompt && js_want_save()) autosaveGame();
        int k = js_key();
        if (k >= 0) return k;
        if (!wait) { // polling (explore, running, resting): let the page paint
            if (emscripten_get_now() - last > 50) {
                last = emscripten_get_now();
                emscripten_sleep(0);
            }
            return -1;
        }
        emscripten_sleep(10);
    }
}

void be_end() { js_end(game.character_is_dead ? 1 : 0); }
