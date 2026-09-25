// Map cell -> Shockbolt tile (port/tilemap.h), and the Inventory pane.
// Looks at the game's own data for what is at that grid; the character on
// screen must still match, so hallucination/blindness stay text.
#include <cstring>
#include "../src/headers.h"
#include "wcurses.h"
#include "tilemap.h"

static int floor_tile(Tile_t const &t) {
    bool lit = t.permanent_light || t.temporary_light;
    int i = lit ? 0 : 1;
    switch (t.feature_id) {
        case TILE_GRANITE_WALL: return T_GRANITE[i];
        case TILE_MAGMA_WALL: return T_MAGMA[i];
        case TILE_QUARTZ_WALL: return T_QUARTZ[i];
        case TILE_BOUNDARY_WALL: return T_PERM[i];
        default: return T_FLOOR[i];
    }
}

template <class F> static int flavour(F const &tab, int n, const char *name) {
    for (int i = 0; i < n; i++) {
        if (strcmp(tab[i].name, name) == 0) return tab[i].tile;
    }
    return -1;
}
#define FLV(tab, name) flavour(tab, (int) (sizeof tab / sizeof tab[0]), name)

static int object_tile(Inventory_t const &it) {
    int k = it.sub_category_id & (ITEM_SINGLE_STACK_MIN - 1);
    int t = -1;
    switch (it.category_id) {
        case TV_POTION1: case TV_POTION2: t = FLV(flv_potion, colors[k]); break;
        case TV_RING: t = FLV(flv_ring, rocks[k]); break;
        case TV_AMULET: t = FLV(flv_amulet, amulets[k]); break;
        case TV_WAND: t = FLV(flv_wand, metals[k]); break;
        case TV_STAFF: t = FLV(flv_staff, woods[k]); break;
        case TV_SCROLL1: case TV_SCROLL2:
            t = scroll_tiles[k % (int) (sizeof scroll_tiles / sizeof scroll_tiles[0])];
            break;
        case TV_FOOD:
            if (k <= 20) t = FLV(flv_mushroom, mushrooms[k]);
            break;
    }
    if (t < 0 && it.id < MAX_OBJECTS_IN_GAME) t = obj_tile[it.id];
    return t;
}

int tile_for(int y, int x, int ch, int *under) {
    *under = -1;
    if (ch == ' ') return -1;
    Coord_t c{y + dg.panel.row_prt, x + dg.panel.col_prt};
    if (c.y < 0 || c.x < 0 || c.y >= dg.height || c.x >= dg.width) return -1;
    Tile_t const &t = dg.floor[c.y][c.x];
    int fl = floor_tile(t);
    if (ch == '@' && t.creature_id == 1) {
        *under = fl;
        return player_tiles[py.misc.race_id % 8][py.misc.class_id % 6][py.misc.gender ? 1 : 0];
    }
    if (t.creature_id > 1) {
        Monster_t const &m = monsters[t.creature_id];
        if (m.lit && creatures_list[m.creature_id].sprite == ch) {
            *under = fl;
            return mon_tile[m.creature_id];
        }
    }
    if (t.treasure_id != 0) {
        Inventory_t const &it = game.treasure.list[t.treasure_id];
        if (it.sprite == ch) {
            int o = object_tile(it);
            if (o >= 0) {
                *under = fl;
                return o;
            }
        }
    }
    if (ch == '.' || ch == '#' || ch == '%') return fl;
    return -1;
}

// Inventory pane: equipment then pack, one line each.
void wc_inv(WINDOW *w) {
    obj_desc_t d;
    int y = 0;
    auto line = [&](const char *s) {
        wmove(w, y++, 0);
        waddstr(w, s);
        wclrtoeol(w);
    };
    line("Inventory");
    for (int i = 0; i < py.pack.unique_items && y < w->maxy - 1; i++) {
        char buf[200];
        itemDescription(d, py.inventory[i], true);
        snprintf(buf, sizeof buf, "%c) %s", 'a' + i, d);
        buf[w->maxx - 1] = 0;
        line(buf);
    }
    line("");
    line("Equipment");
    for (int i = PlayerEquipment::Wield; i < PLAYER_INVENTORY_SIZE && y < w->maxy; i++) {
        if (py.inventory[i].category_id == TV_NOTHING) continue;
        char buf[200];
        itemDescription(d, py.inventory[i], true);
        snprintf(buf, sizeof buf, "%c) %s", 'a' + i - PlayerEquipment::Wield, d);
        buf[w->maxx - 1] = 0;
        line(buf);
    }
    while (y < w->maxy) line("");
}
