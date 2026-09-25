// In-memory curses for Umoria, routed to Angband-style panes.
//
// Umoria draws everything on one 80x24 screen: message line (row 0),
// stats column (cols 0-12), map (rows 1-22, cols 13-78), status line
// (row 23). The frontend shows these as separate windows:
//   Map      stdscr map area, tiled (tile_for)
//   Status   stats column + the words of the status line
//   Messages row 0 live + history
//   Inventory built from the pack (wc_inv)
//   Pop-up   anything else: inventory lists, help, stores, character
//            sheet... sized to its content.
// Mode comes from what the game does to the screen, no game changes:
//   overwrite(stdscr, save)  -> overlay: pop-up = cells that differ from save
//   overwrite(save, stdscr)  -> back to the mode before
//   clear()                  -> full screen text: pop-up = all non-blank text
//   wc_dungeon() (panelPutTile) -> the game draws the map again
#include <cstdlib>
#include <cstring>
#include "wcurses.h"

WINDOW *stdscr, *curscr;
int LINES = 24, COLS = 80;

enum { M_DUNGEON, M_OVERLAY, M_FULL };
static int mode = M_FULL, saved_mode = M_FULL;
static WINDOW *base;          // the save_screen copy while in overlay mode
static WINDOW *pn[NPANES];
static chtype shown[24 * 80]; // what the Map pane has, per cell
static int shown_tile[24 * 80];

constexpr int MAP_Y = 1, MAP_X = 13, MAP_H = 22, MAP_W = 66;
constexpr int HIST = 21;                  // message history rows
constexpr int ST_W = 14, ST_H = 32;       // Status pane
constexpr int INV_W = 78, INV_H = 22;     // Inventory pane

WINDOW *newwin(int rows, int cols, int, int) {
    auto *w = (WINDOW *) calloc(1, sizeof(WINDOW));
    if (rows == 0) rows = LINES;
    if (cols == 0) cols = COLS;
    w->maxy = rows;
    w->maxx = cols;
    w->c = (chtype *) malloc(sizeof(chtype) * rows * cols);
    w->first = (short *) malloc(sizeof(short) * rows);
    w->last = (short *) malloc(sizeof(short) * rows);
    for (int i = 0; i < rows * cols; i++) w->c[i] = ' ';
    touchwin(w);
    return w;
}

WINDOW *initscr() {
    if (curscr == nullptr) {
        curscr = newwin(LINES, COLS, 0, 0);
        stdscr = newwin(LINES, COLS, 0, 0);
        memset(shown, 0xff, sizeof shown);
        pn[P_STATUS] = newwin(ST_H, ST_W, 0, 0);
        pn[P_MSG] = newwin(HIST + 1, COLS, 0, 0);
        pn[P_INV] = newwin(INV_H, INV_W, 0, 0);
        be_init(P_MAP, MAP_W, MAP_H);
        be_init(P_STATUS, ST_W, ST_H);
        be_init(P_MSG, COLS, HIST + 1);
        be_init(P_INV, INV_W, INV_H);
    }
    return stdscr;
}

int endwin() { return OK; }

static void touch(WINDOW *w, int y, int x) {
    if (w->first[y] < 0 || x < w->first[y]) w->first[y] = (short) x;
    if (x > w->last[y]) w->last[y] = (short) x;
}

static void untouch(WINDOW *w) {
    for (int y = 0; y < w->maxy; y++) w->first[y] = w->last[y] = -1;
}

int touchwin(WINDOW *w) {
    for (int y = 0; y < w->maxy; y++) {
        w->first[y] = 0;
        w->last[y] = (short) (w->maxx - 1);
    }
    return OK;
}

int wmove(WINDOW *w, int y, int x) {
    if (y < 0 || x < 0 || y >= w->maxy || x >= w->maxx) return ERR;
    w->cury = y;
    w->curx = x;
    return OK;
}

static void set(WINDOW *w, int y, int x, chtype ch) {
    if (y < 0 || x < 0 || y >= w->maxy || x >= w->maxx || w->c[y * w->maxx + x] == ch) return;
    w->c[y * w->maxx + x] = ch;
    touch(w, y, x);
}

int waddch(WINDOW *w, chtype ch) {
    int c = (int) (ch & A_CHARTEXT);
    if (c == '\n') {
        wclrtoeol(w);
        if (w->cury + 1 < w->maxy) {
            w->cury++;
            w->curx = 0;
        }
        return OK;
    }
    set(w, w->cury, w->curx, (ch & ~(chtype) A_CHARTEXT) | (chtype) c);
    if (++w->curx >= w->maxx) {
        // like curses: the bottom right corner can't wrap
        if (w->cury + 1 >= w->maxy) {
            w->curx = w->maxx - 1;
            return ERR;
        }
        w->cury++;
        w->curx = 0;
    }
    return OK;
}

int waddstr(WINDOW *w, const char *s) {
    while (*s != 0) {
        if (waddch(w, (unsigned char) *s++) == ERR) return ERR;
    }
    return OK;
}

int wclrtoeol(WINDOW *w) {
    for (int x = w->curx; x < w->maxx; x++) set(w, w->cury, x, ' ');
    return OK;
}

int wclrtobot(WINDOW *w) {
    int cy = w->cury, cx = w->curx;
    wclrtoeol(w);
    for (int y = cy + 1; y < w->maxy; y++) {
        for (int x = 0; x < w->maxx; x++) set(w, y, x, ' ');
    }
    w->cury = cy;
    w->curx = cx;
    return OK;
}

int wclear(WINDOW *w) {
    w->cury = w->curx = 0;
    wclrtobot(w);
    if (w == stdscr) mode = M_FULL;
    return OK;
}

int overwrite(WINDOW *s, WINDOW *d) {
    for (int y = 0; y < s->maxy && y < d->maxy; y++) {
        for (int x = 0; x < s->maxx && x < d->maxx; x++) set(d, y, x, s->c[y * s->maxx + x]);
    }
    if (s == stdscr) { // terminalSaveScreen()
        saved_mode = mode;
        mode = M_OVERLAY;
        base = d;
    } else if (d == stdscr) { // terminalRestoreScreen()
        mode = saved_mode;
    }
    return OK;
}

void wc_dungeon() {
    if (mode == M_FULL) mode = M_DUNGEON;
}

// ---- panes ----

static int pop_h, pop_w;

static void pflush(int i) {
    WINDOW *p = pn[i];
    for (int y = 0; p != nullptr && y < p->maxy; y++) {
        if (p->first[y] < 0) continue;
        for (int x = p->first[y]; x <= p->last[y]; x++) be_put(i, y, x, p->c[y * p->maxx + x], -1, -1);
        p->first[y] = p->last[y] = -1;
    }
}

static void close_popup() {
    if (pop_h != 0) be_popup(0, 0);
    pop_h = pop_w = 0;
}

static chtype at(WINDOW *w, int y, int x) { return w->c[y * w->maxx + x]; }

static void map_refresh() {
    // every cell: monsters/objects under unchanged characters may have changed
    for (int y = MAP_Y; y < MAP_Y + MAP_H; y++) {
        for (int x = MAP_X; x < MAP_X + MAP_W; x++) {
            int i = y * COLS + x, under = -1;
            chtype ch = at(stdscr, y, x);
            int t = tile_for(y, x, (int) (ch & A_CHARTEXT), &under);
            int key = t < 0 ? -1 : (t | (under + 1) << 16);
            if (shown[i] == ch && shown_tile[i] == key) continue;
            shown[i] = ch;
            shown_tile[i] = key;
            be_put(P_MAP, y - MAP_Y, x - MAP_X, ch, t, t < 0 ? -1 : under);
        }
    }
}

// Stats column (rows 2-22), then each word group of the status line.
static void status_refresh() {
    WINDOW *p = pn[P_STATUS];
    int y = 0;
    for (int sy = 2; sy <= 22; sy++, y++) {
        for (int x = 0; x < ST_W; x++) set(p, y, x, x < MAP_X ? at(stdscr, sy, x) : ' ');
    }
    y++;
    char row[81];
    for (int x = 0; x < 80; x++) row[x] = (char) (at(stdscr, 23, x) & A_CHARTEXT);
    row[80] = 0;
    for (int x = 0; x < 80 && y < ST_H;) {
        if (row[x] == ' ') {
            x++;
            continue;
        }
        int n = 0;
        char word[ST_W + 1];
        // a group ends at two spaces ("Rest 12" is one group)
        while (x < 80 && !(row[x] == ' ' && (x + 1 >= 80 || row[x + 1] == ' '))) {
            if (n < ST_W) word[n++] = row[x];
            x++;
        }
        for (int i = 0; i < ST_W; i++) set(p, y, i, i < n ? (unsigned char) word[i] : ' ');
        y++;
    }
    for (; y < ST_H; y++) {
        for (int x = 0; x < ST_W; x++) set(p, y, x, ' ');
    }
}

static char last0[128]; // message line as last seen

// a repeat of the newest history line becomes "line (xN)" in its row
static void hist(const char *s) {
    static char prev[128];
    static int reps;
    char buf[160];
    WINDOW *p = pn[P_MSG];
    if (*prev != 0 && strcmp(s, prev) == 0) {
        snprintf(buf, sizeof buf, "%s (x%d)", s, ++reps);
    } else {
        reps = 1;
        snprintf(prev, sizeof prev, "%s", s);
        snprintf(buf, sizeof buf, "%s", s);
        for (int y = 0; y < HIST - 1; y++) {
            for (int x = 0; x < p->maxx; x++) set(p, y, x, at(p, y + 1, x));
        }
    }
    int n = (int) strlen(buf);
    for (int x = 0; x < p->maxx; x++) set(p, HIST - 1, x, x < n ? (unsigned char) buf[x] : ' ');
}

static void msg_refresh() {
    char r[128];
    int n;
    for (n = 0; n < COLS; n++) r[n] = (char) (at(stdscr, 0, n) & A_CHARTEXT);
    r[n] = 0;
    char *m = strstr(r, " -more-");
    if (m != nullptr) *m = 0;
    for (n = (int) strlen(r); n > 0 && r[n - 1] == ' ';) r[--n] = 0;
    // a message went away (not just grew): into the history
    if (*last0 != 0 && strncmp(r, last0, strlen(last0)) != 0) hist(last0);
    strcpy(last0, r);
    for (int x = 0; x < COLS; x++) set(pn[P_MSG], HIST, x, at(stdscr, 0, x));
}

// Pop-up: bounding box of the text that isn't the game screen underneath.
static void pop_refresh() {
    int y0 = 99, y1 = -1, x0 = 99, x1 = -1;
    for (int y = 1; y < LINES; y++) {
        for (int x = 0; x < COLS; x++) {
            chtype ch = at(stdscr, y, x);
            if ((ch & A_CHARTEXT) == ' ') continue;
            if (mode == M_OVERLAY && ch == at(base, y, x)) continue;
            if (y < y0) y0 = y;
            if (y > y1) y1 = y;
            if (x < x0) x0 = x;
            if (x > x1) x1 = x;
        }
    }
    int cy = stdscr->cury, cx = stdscr->curx;
    if (y1 < 0) {
        close_popup();
        return;
    }
    // room for the cursor of a prompt ("Which one? _")
    if (cy >= y0 && cy <= y1 && cx > x1) x1 = cx;
    if (y1 - y0 + 1 != pop_h || x1 - x0 + 1 != pop_w) {
        pop_h = y1 - y0 + 1;
        pop_w = x1 - x0 + 1;
        be_popup(pop_h, pop_w);
        if (pn[P_POP] != nullptr) {
            free(pn[P_POP]->c);
            free(pn[P_POP]->first);
            free(pn[P_POP]->last);
            free(pn[P_POP]);
        }
        pn[P_POP] = newwin(pop_h, pop_w, 0, 0);
    }
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) set(pn[P_POP], y - y0, x - x0, at(stdscr, y, x));
    }
    if (cy >= y0 && cy <= y1 && cx >= x0 && cx <= x1) be_cursor(P_POP, cy - y0, cx - x0);
}

static void dump(FILE *f, const char *name, WINDOW *p) {
    fprintf(f, "== %s\n", name);
    for (int y = 0; p != nullptr && y < p->maxy; y++) {
        for (int x = 0; x < p->maxx; x++) fputc((int) (at(p, y, x) & A_CHARTEXT), f);
        fputc('\n', f);
    }
}

int wrefresh(WINDOW *w) {
    if (w != stdscr) return OK; // wrefresh(curscr) = ^R redraw: nothing to do
    int cy = stdscr->cury, cx = stdscr->curx;
    be_cursor(-1, 0, 0);
    msg_refresh();
    if (mode == M_DUNGEON) {
        close_popup();
        map_refresh();
        status_refresh();
        wc_inv(pn[P_INV]);
        if (cy >= MAP_Y && cy < MAP_Y + MAP_H && cx >= MAP_X) be_cursor(P_MAP, cy - MAP_Y, cx - MAP_X);
    } else {
        pop_refresh();
    }
    if (cy == 0) be_cursor(P_MSG, HIST, cx);
    untouch(stdscr);
    for (int i = P_STATUS; i < NPANES; i++) {
        if (i != P_POP || pop_h != 0) pflush(i);
    }
    be_flush();
    const char *d = getenv("UMORIA_DUMP"); // testing: panes as text
    FILE *f = d != nullptr ? fopen(d, "w") : nullptr;
    if (f != nullptr) {
        fprintf(f, "mode %d cursor %d,%d\n", mode, cy, cx);
        dump(f, "SCREEN", stdscr);
        dump(f, "STATUS", pn[P_STATUS]);
        dump(f, "MSG", pn[P_MSG]);
        dump(f, "INV", pn[P_INV]);
        dump(f, "POP", pop_h != 0 ? pn[P_POP] : nullptr);
        fclose(f);
    }
    return OK;
}

static int pushback = -1;

int wgetch(WINDOW *w) {
    wrefresh(w);
    int k = pushback;
    pushback = -1;
    return k >= 0 ? k : be_getkey(1);
}

int wc_kbhit() {
    if (pushback < 0) pushback = be_getkey(0);
    return pushback >= 0;
}
