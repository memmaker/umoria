// Map cell -> Shockbolt tile (port/tilemap.h), and the Inventory pane.
// Looks at the game's own data for what is at that grid; the character on
// screen must still match, so hallucination/blindness stay text.
#include <cstring>
#include "../src/headers.h"
#include "wcurses.h"
#include "tilemap.h"

// What a grid of the real level is, for autotiling (RVIP-Finetuning:
// DawnLike floors are autotiles). Secret doors count as wall.
enum { K_NONE, K_ROOM, K_CORR, K_TOWN, K_DOOR, K_WALL };

static bool inside(int y, int x) { return y >= 0 && x >= 0 && y < dg.height && x < dg.width; }

static int kind(int y, int x) {
    if (!inside(y, x)) return K_NONE;
    Tile_t const &t = dg.floor[y][x];
    if (t.feature_id >= MIN_CAVE_WALL) return K_WALL;
    if (t.treasure_id != 0 && game.treasure.list[t.treasure_id].category_id == TV_SECRET_DOOR) return K_WALL;
    if (t.feature_id == TILE_NULL_WALL) return K_NONE;
    if (dg.current_level == 0) return K_TOWN;
    if (t.feature_id == TILE_BLOCKED_FLOOR) return K_DOOR;   // door or rubble: joins any floor
    return t.feature_id <= MAX_CAVE_ROOM ? K_ROOM : K_CORR;
}

// a wall that borders open ground (the face you see); walls connect only to
// these, so solid rock does not become a lattice
static bool edge_wall(int y, int x) {
    if (kind(y, x) != K_WALL) return false;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int k = kind(y + dy, x + dx);
            if (k != K_WALL && k != K_NONE) return true;
        }
    }
    return false;
}

static const int DY[] = {-1, 1, 0, 0}, DX[] = {0, 0, -1, 1};   // n s w e -> mask 8 4 2 1

static int floor_tile(int y, int x) {
    Tile_t const &t = dg.floor[y][x];
    int i = t.permanent_light || t.temporary_light ? 0 : 1;
    int k = kind(y, x), m = 0;
    if (k == K_WALL) {
        int w = t.feature_id == TILE_MAGMA_WALL ? 1 : t.feature_id == TILE_QUARTZ_WALL ? 2
              : t.feature_id == TILE_BOUNDARY_WALL ? 3 : 0;
        for (int d = 0; d < 4 && edge_wall(y, x); d++) {   // inner rock stays plain
            if (edge_wall(y + DY[d], x + DX[d])) m |= 8 >> d;
        }
        return T_AUTO_WALL[w][i] + m;
    }
    if (k == K_DOOR) k = K_CORR;     // the floor drawn under a door or rubble
    for (int d = 0; d < 4; d++) {
        int n = kind(y + DY[d], x + DX[d]);
        if (n != k && n != K_DOOR) m |= 8 >> d;
    }
    return T_AUTO_FLOOR[k == K_ROOM ? 0 : k == K_CORR ? 1 : 2][i] + m;
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

int wc_itemtile(Inventory_t const &it) { return object_tile(it); }

int wc_objtile(int i) {
    Inventory_t const &it = py.inventory[i];
    return it.category_id == TV_NOTHING ? -1 : object_tile(it);
}

int tile_for(int y, int x, int ch, int *under) {
    *under = -1;
    if (ch == ' ') return -1;
    Coord_t c{y + dg.panel.row_prt, x + dg.panel.col_prt};
    if (c.y < 0 || c.x < 0 || c.y >= dg.height || c.x >= dg.width) return -1;
    Tile_t const &t = dg.floor[c.y][c.x];
    int fl = floor_tile(c.y, c.x);
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

// Angband's colour for an item's tval (RVIP W0: colours come from the game)
const char *wc_css(int tval) {
    switch (tval) {
    case TV_SLING_AMMO: case TV_BOLT: case TV_ARROW: case TV_SPIKE: return "#909098";
    case TV_LIGHT: return "#ffff90";
    case TV_BOW: case TV_HAFTED: case TV_POLEARM: case TV_SWORD: return "#b0b0b8";
    case TV_DIGGING: return "#c0c0c0";
    case TV_BOOTS: case TV_GLOVES: case TV_CLOAK: case TV_HELM: case TV_SHIELD:
    case TV_HARD_ARMOR: case TV_SOFT_ARMOR: return "#a07040";
    case TV_AMULET: return "#ff9000";
    case TV_RING: return "#ff4040";
    case TV_STAFF: return "#d09050";
    case TV_WAND: return "#40d040";
    case TV_SCROLL1: case TV_SCROLL2: return "#ffffff";
    case TV_POTION1: case TV_POTION2: case TV_FLASK: return "#40a0ff";
    case TV_FOOD: return "#d09050";
    case TV_MAGIC_BOOK: case TV_PRAYER_BOOK: return "#60e0e0";
    case TV_GOLD: return "#ffe040";
    }
    return "";
}

// Inventory pane: pack then equipment, one line each. With a tile set the
// row is "a)   name" (JS draws the icon over cols 2-4), else "a) ! name".
void wc_inv(WINDOW *w) {
    obj_desc_t d;
    int y = 0;
    bool icons = be_icons() != 0;
    auto item = [&](char *buf, size_t n, char letter, int i) {
        itemDescription(d, py.inventory[i], true);
        if (icons) snprintf(buf, n, "%c)   %s", letter, d);
        else snprintf(buf, n, "%c) %c %s", letter, py.inventory[i].sprite, d);
    };
    auto line = [&](const char *s, const char *css = "", int tile = -1) {
        be_invfg(y, css, icons ? tile : -1);
        wmove(w, y++, 0);
        waddstr(w, s);
        wclrtoeol(w);
    };
    line("Inventory");
    for (int i = 0; i < py.pack.unique_items && y < w->maxy - 1; i++) {
        char buf[200];
        item(buf, sizeof buf, (char) ('a' + i), i);
        buf[w->maxx - 1] = 0;
        line(buf, wc_css(py.inventory[i].category_id), wc_objtile(i));
    }
    line("");
    line("Equipment");
    for (int i = PlayerEquipment::Wield; i < PLAYER_INVENTORY_SIZE && y < w->maxy; i++) {
        if (py.inventory[i].category_id == TV_NOTHING) continue;
        char buf[200];
        item(buf, sizeof buf, (char) ('a' + i - PlayerEquipment::Wield), i);
        buf[w->maxx - 1] = 0;
        line(buf, wc_css(py.inventory[i].category_id), wc_objtile(i));
    }
    while (y < w->maxy) line("");
}
