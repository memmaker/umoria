// Floating command menu on Enter (RVIP 3b), built from the game's own help
// page, so it lists exactly the commands of the current keyset. Returns the
// chosen key as if typed, or ESCAPE.

#include <fstream> // before headers.h, which redefines fopen
#include <vector>
#include "headers.h"
#include "curses.h"

struct MenuEntry {
    char key;
    std::string text;
};

static void parseHalf(std::string h, std::vector<MenuEntry> &out) {
    if (h.size() < 3) return;
    h = h.substr(2); // "@ " count marker
    size_t sp = h.find(' ');
    std::string key = h.substr(0, sp);
    std::string desc = sp == std::string::npos ? "" : h.substr(sp);
    desc.erase(0, desc.find_first_not_of(" ~"));
    desc.erase(desc.find_last_not_of(' ') + 1);
    char k;
    if (key.size() == 1 && key != "~") {
        k = key[0];
    } else if (key.size() == 6 && key.compare(0, 5, "CTRL-") == 0) {
        k = CTRL_KEY(key[5]);
    } else {
        return; // "CTRL ~", "SHIFT ~", "Enter": not a single command key
    }
    out.push_back({k, desc});
}

static std::vector<MenuEntry> menuEntries() {
    std::vector<MenuEntry> left, right;
    std::ifstream f(config::options::use_roguelike_keys ? config::files::help_roguelike : config::files::help);
    std::string line;
    std::getline(f, line); // title
    while (std::getline(f, line) && line.compare(0, 11, "Directions:") != 0) {
        size_t bar = line.find('|');
        parseHalf(line.substr(0, bar), left);
        if (bar != std::string::npos) parseHalf(line.substr(bar + 2), right);
    }
    left.insert(left.end(), right.begin(), right.end());
    return left;
}

static std::string keyName(char k) {
    if (k > 0 && k < ' ') return std::string("^") + (char) (k + '@');
    return std::string(1, k);
}

char commandMenu() {
    std::vector<MenuEntry> items = menuEntries();
    if (items.empty()) return ESCAPE;
    int n = (int) items.size(), width = 0;
    for (auto const &e : items) width = std::max(width, 4 + (int) e.text.size());
    int rows = std::min(n, 21), cur = 0, top = 0;

    terminalSaveScreen();
    char result = ' '; // nothing chosen: the "do nothing" command
    for (;;) {
        if (cur < top) top = cur;
        if (cur >= top + rows) top = cur - rows + 1;
        for (int r = 0; r < rows; r++) {
            MenuEntry const &e = items[top + r];
            std::string s = " " + keyName(e.key);
            s.resize(4, ' ');
            s += e.text;
            s.resize(width, ' ');
            chtype attr = top + r == cur ? A_STANDOUT : 0;
            move(1 + r, 13);
            for (char c : s) addch((unsigned char) c | attr);
        }
        move(1 + cur - top, 13);
        int k = getKeyInput();
        bool listed = false;
        for (auto const &e : items) listed = listed || e.key == k;
        if (listed) { // a command's own key runs it
            result = (char) k;
            break;
        }
        if (k == ESCAPE || k == '0') break;
        if (k == '8') cur = (cur + n - 1) % n;
        else if (k == '2') cur = (cur + 1) % n;
        else if (k == '9') cur = std::max(0, cur - rows);
        else if (k == '3') cur = std::min(n - 1, cur + rows);
        else if (k == '\r' || k == '\n' || k == '5' || k == ' ') {
            result = items[cur].key;
            break;
        }
    }
    terminalRestoreScreen();
    return result;
}
