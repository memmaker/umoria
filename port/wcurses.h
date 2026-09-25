// Minimal curses for Umoria (only ui_io.cpp uses curses): one in-memory
// screen, drawn by an X11 frontend as Angband-style panes. Adapted from
// ~/Games/xrogue/port. Only what Umoria uses.
#pragma once
#include <cstdio>

typedef unsigned int chtype;
#define ERR (-1)
#define OK 0
#define A_CHARTEXT 0xff
#define A_STANDOUT 0x100

struct WINDOW {
    int maxy, maxx, cury, curx;
    short *first, *last; // changed range per line, -1 = none
    chtype *c;
};

extern WINDOW *stdscr, *curscr;
extern int LINES, COLS;

WINDOW *initscr();
int endwin();
WINDOW *newwin(int, int, int, int);
int wmove(WINDOW *, int, int);
int waddch(WINDOW *, chtype);
int waddstr(WINDOW *, const char *);
int wclear(WINDOW *);
int wclrtoeol(WINDOW *);
int wclrtobot(WINDOW *);
int touchwin(WINDOW *);
int overwrite(WINDOW *, WINDOW *);
int wrefresh(WINDOW *);
int wgetch(WINDOW *);
int wc_kbhit();     // a key is waiting (running / auto-explore stop on it)
void wc_dungeon();  // the game draws the map: back from full-screen text

#define getyx(w, y, x) ((y) = (w)->cury, (x) = (w)->curx)
#define mvaddch(y, x, ch) (wmove(stdscr, y, x) == ERR ? ERR : waddch(stdscr, ch))
#define mvaddstr(y, x, s) (wmove(stdscr, y, x) == ERR ? ERR : waddstr(stdscr, s))
#define move(y, x) wmove(stdscr, y, x)
#define addch(ch) waddch(stdscr, ch)
#define addstr(s) waddstr(stdscr, s)
#define clear() wclear(stdscr)
#define clrtoeol() wclrtoeol(stdscr)
#define clrtobot() wclrtobot(stdscr)
#define refresh() wrefresh(stdscr)
#define getch() wgetch(stdscr)
#define keypad(w, b) OK
#define noecho() OK
#define raw() OK
#define nonl() OK
#define mvcur(a, b, c, d) OK
#define set_escdelay(n) OK
#define timeout(n) ((void) 0)

// Frontend: panes. Only the map is tiled; text goes to text panes and a
// pop-up box sized to its content.
enum { P_MAP, P_STATUS, P_MSG, P_INV, P_POP, NPANES };
void be_init(int pane, int cols, int rows);
void be_put(int pane, int y, int x, chtype ch, int tile, int under);
void be_cursor(int pane, int y, int x); // pane -1: no cursor
void be_popup(int rows, int cols);      // 0: close
void be_flush();
int be_getkey(int wait); // -1 when !wait and nothing queued
void be_sound(const char *event); // web: play a sound event
void be_end();                   // web: the game ended
int tile_for(int y, int x, int ch, int *under); // tiles.cpp: -1 = text
void wc_inv(WINDOW *);                          // tiles.cpp: Inventory pane
