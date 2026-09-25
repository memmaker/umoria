// Auto-explore (g) and walking to the nearest known staircase (< >). RVIP.
//
// One step per game turn: executeInputCommands() repeats `py_auto` as the
// command until playerDisturb(), a message, a visible monster or a key.
// Umoria forgets lamp-lit floor once you leave it, so what the player has
// seen is kept here, per level.

#include "headers.h"

char py_auto = 0; // 'g' explore, '<' / '>' walk to stairs, 0 = off

static bool seen[MAX_HEIGHT][MAX_WIDTH];
static bool visited[MAX_HEIGHT][MAX_WIDTH]; // stood on
static bool bad[MAX_HEIGHT][MAX_WIDTH];     // locked/stuck door: skip

void playerExploreNewLevel() {
    py_auto = 0;
    memset(seen, 0, sizeof seen);
    memset(visited, 0, sizeof visited);
    memset(bad, 0, sizeof bad);
}

static int tval(Tile_t const &t) {
    return t.treasure_id != 0 ? game.treasure.list[t.treasure_id].category_id : TV_NOTHING;
}

static bool passable(int y, int x) {
    Tile_t const &t = dg.floor[y][x];
    if (!seen[y][x] || bad[y][x] || t.feature_id > MAX_CAVE_FLOOR) return false;
    int v = tval(t);
    return v != TV_RUBBLE && v != TV_VIS_TRAP && v != TV_SECRET_DOOR;
}

static bool target(char mode, int y, int x) {
    Tile_t const &t = dg.floor[y][x];
    int v = tval(t);
    if (mode == '<') return v == TV_UP_STAIR;
    if (mode == '>') return v == TV_DOWN_STAIR;
    if (visited[y][x]) return false;
    if (v != TV_NOTHING && v <= TV_MAX_PICK_UP) return true; // an item not yet looked at
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int ny = y + dy, nx = x + dx;
            if (ny >= 0 && nx >= 0 && ny < dg.height && nx < dg.width && !seen[ny][nx]) return true;
        }
    }
    return false;
}

bool playerMonsterInView() {
    for (int i = config::monsters::MON_MIN_INDEX_ID; i < next_free_monster_id; i++) {
        // town: townspeople are always in view, only near ones count
        int near = dg.current_level == 0 ? 5 : 255;
        if (monsters[i].hp > 0 && monsters[i].lit && monsters[i].distance_from_player <= near) return true;
    }
    return false;
}

// Direction (keypad 1-9) of the first step towards the nearest target, 0 = none.
static int firstStep(char mode) {
    static Coord_t from[MAX_HEIGHT][MAX_WIDTH];
    static bool done[MAX_HEIGHT][MAX_WIDTH];
    static Coord_t queue[MAX_HEIGHT * MAX_WIDTH];
    memset(done, 0, sizeof done);
    int head = 0, tail = 0;
    queue[tail++] = py.pos;
    done[py.pos.y][py.pos.x] = true;
    while (head < tail) {
        Coord_t c = queue[head++];
        if (!(c.y == py.pos.y && c.x == py.pos.x) && target(mode, c.y, c.x)) {
            while (!(from[c.y][c.x].y == py.pos.y && from[c.y][c.x].x == py.pos.x)) c = from[c.y][c.x];
            return 5 + (c.x - py.pos.x) - 3 * (c.y - py.pos.y);
        }
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int ny = c.y + dy, nx = c.x + dx;
                if (ny < 0 || nx < 0 || ny >= dg.height || nx >= dg.width || done[ny][nx] || !passable(ny, nx)) continue;
                done[ny][nx] = true;
                from[ny][nx] = c;
                queue[tail++] = Coord_t{ny, nx};
            }
        }
    }
    return 0;
}

// One step of `mode`. Returns false (and stops) when there is nothing to do.
void playerAutoStep(char mode) {
    game.player_free_turn = true;
    py_auto = 0;

    for (int y = 0; y < dg.height; y++) {
        for (int x = 0; x < dg.width; x++) {
            Tile_t const &t = dg.floor[y][x];
            if (t.permanent_light || t.temporary_light || t.field_mark) seen[y][x] = true;
        }
    }
    seen[py.pos.y][py.pos.x] = visited[py.pos.y][py.pos.x] = true;

    if (py.flags.blind > 0 || py.flags.confused > 0 || py.flags.image > 0) {
        printMessage("You are in no state to explore.");
        return;
    }
    if (playerMonsterInView()) {
        printMessage("Not with a monster in view.");
        return;
    }
    int dir = firstStep(mode);
    if (dir == 0) {
        printMessage(mode == 'g' ? "Nothing left to explore." : mode == '<' ? "You know of no up staircase." : "You know of no down staircase.");
        return;
    }

    Coord_t c = py.pos;
    (void) playerMovePosition(dir, c);
    Tile_t &t = dg.floor[c.y][c.x];
    if (tval(t) == TV_CLOSED_DOOR) {
        Inventory_t &door = game.treasure.list[t.treasure_id];
        if (door.misc_use != 0) {
            // never pick locks: skip this door from now on
            bad[c.y][c.x] = true;
            printMessage(door.misc_use > 0 ? "The door is locked." : "The door is stuck.");
            return;
        }
        inventoryItemCopyTo(config::dungeon::objects::OBJ_OPEN_DOOR, door);
        t.feature_id = TILE_CORR_FLOOR;
        dungeonLiteSpot(c);
        game.player_free_turn = false;
        py_auto = mode;
        return;
    }

    game.player_free_turn = false;
    py_auto = mode; // printMessage() or playerDisturb() during the move clears it
    playerMove(dir, true);
    visited[py.pos.y][py.pos.x] = true;
}
