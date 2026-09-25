// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include "headers.h"

// RVIP 3c: a cursor over an item list whose entries are drawn on screen
// rows 1..n by `redraw`. 8/2 move, 4/6 are '/' (switch list), 0 and . are
// ESCAPE. Returns -1 when the entry under the cursor is chosen (5, Enter),
// else the key pressed.
template <class F> static int itemCursorKey(int &cur, int n, F redraw) {
    for (;;) {
        if (cur >= n) cur = n - 1;
        if (cur < 0) cur = 0;
        ui_highlight_row = n > 0 ? 1 + cur : -1;
        redraw();
        ui_highlight_row = -1;
        int k = getKeyInput();
        if (k == '8' && n > 0) {
            cur = (cur + n - 1) % n;
        } else if (k == '2' && n > 0) {
            cur = (cur + 1) % n;
        } else if (k == '4' || k == '6') {
            return '/';
        } else if (k == '0' || k == '.') {
            return ESCAPE;
        } else if ((k == '5' || k == '\r' || k == '\n') && n > 0) {
            redraw(); // without the highlight
            return -1;
        } else {
            redraw();
            return k;
        }
    }
}

static void inventoryItemWeightText(obj_desc_t text, int itemId) {
    int totalWeight = py.inventory[itemId].weight * py.inventory[itemId].items_count;
    int quotient = totalWeight / 10;
    int remainder = totalWeight % 10;

    (void) snprintf(text, MORIA_OBJ_DESC_SIZE, "%3d.%d lb", quotient, remainder);
}

// Displays inventory items from `item_id_start` to `item_id_end` -RAK-
// Designed to keep the display as far to the right as possible. -CJS-
// The parameter col gives a column at which to start, but if the display
// does not fit, it may be moved left.  The return value is the left edge
// used. If mask is non-zero, then only display those items which have a
// non-zero entry in the mask array.
int displayInventoryItems(int itemIdStart, int itemIdEnd, bool weighted, int column, const char *mask) {
    vtype_t descriptions[23];

    int len = 79 - column;

    int lim;
    if (weighted) {
        lim = 68;
    } else {
        lim = 76;
    }

    // Generate the descriptions text
    for (int i = itemIdStart; i <= itemIdEnd; i++) {
        if (mask != CNIL && (mask[i] == 0)) {
            continue;
        }

        obj_desc_t description = {'\0'};
        itemDescription(description, py.inventory[i], true);

        // Truncate if too long.
        description[lim] = 0;

        (void) snprintf(descriptions[i], MORIA_MESSAGE_SIZE, "%c) %s", 'a' + i, description);

        int l = (int) strlen(descriptions[i]) + 2;

        if (weighted) {
            l += 9;
        }

        if (l > len) {
            len = l;
        }
    }

    column = 79 - len;
    if (column < 0) {
        column = 0;
    }

    int currentLine = 1;

    // Print the descriptions
    for (int i = itemIdStart; i <= itemIdEnd; i++) {
        if (mask != CNIL && (mask[i] == 0)) {
            continue;
        }

        // don't need first two spaces if in first column
        if (column == 0) {
            putStringClearToEOL(descriptions[i], Coord_t{currentLine, column});
        } else {
            putString("  ", Coord_t{currentLine, column});
            putStringClearToEOL(descriptions[i], Coord_t{currentLine, column + 2});
        }

        if (weighted) {
            obj_desc_t text = {'\0'};
            inventoryItemWeightText(text, i);
            putStringClearToEOL(text, Coord_t{currentLine, 71});
        }

        currentLine++;
    }

    return column;
}

// Return a string describing how a given equipment item is carried. -CJS-
const char *playerItemWearingDescription(int body_location) {
    switch (body_location) {
        case PlayerEquipment::Wield:
            return "wielding";
        case PlayerEquipment::Head:
            return "wearing on your head";
        case PlayerEquipment::Neck:
            return "wearing around your neck";
        case PlayerEquipment::Body:
            return "wearing on your body";
        case PlayerEquipment::Arm:
            return "wearing on your arm";
        case PlayerEquipment::Hands:
            return "wearing on your hands";
        case PlayerEquipment::Right:
            return "wearing on your right hand";
        case PlayerEquipment::Left:
            return "wearing on your left hand";
        case PlayerEquipment::Feet:
            return "wearing on your feet";
        case PlayerEquipment::Outer:
            return "wearing about your body";
        case PlayerEquipment::Light:
            return "using to light the way";
        case PlayerEquipment::Auxiliary:
            return "holding ready by your side";
        default:
            return "carrying in your pack";
    }
}

static const char *equipmentPositionDescription(PlayerEquipment id, uint16_t weight) {
    switch (id) {
        case PlayerEquipment::Wield:
            if (py.stats.used[PlayerAttr::A_STR] * 15 < weight) {
                return "Just lifting";
            }
            return "Wielding";
        case PlayerEquipment::Head:
            return "On head";
        case PlayerEquipment::Neck:
            return "Around neck";
        case PlayerEquipment::Body:
            return "On body";
        case PlayerEquipment::Arm:
            return "On arm";
        case PlayerEquipment::Hands:
            return "On hands";
        case PlayerEquipment::Right:
            return "On right hand";
        case PlayerEquipment::Left:
            return "On left hand";
        case PlayerEquipment::Feet:
            return "On feet";
        case PlayerEquipment::Outer:
            return "About body";
        case PlayerEquipment::Light:
            return "Light source";
        case PlayerEquipment::Auxiliary:
            return "Spare weapon";
        default:
            return "Unknown equipment position ID";
    }
}

// Displays equipment items from r1 to end -RAK-
// Keep display as far right as possible. -CJS-
int displayEquipment(bool showWeights, int column) {
    vtype_t descriptions[PLAYER_INVENTORY_SIZE - PlayerEquipment::Wield];

    int len = 79 - column;

    int lim;
    if (showWeights) {
        lim = 52;
    } else {
        lim = 60;
    }

    // Range of equipment
    int line = 0;
    for (int i = PlayerEquipment::Wield; i < PLAYER_INVENTORY_SIZE; i++) {
        if (py.inventory[i].category_id == TV_NOTHING) {
            continue;
        }

        // Get position
        const char *equippedDescription = equipmentPositionDescription((PlayerEquipment) i, py.inventory[i].weight);

        obj_desc_t description = {'\0'};
        itemDescription(description, py.inventory[i], true);

        // Truncate if necessary
        description[lim] = 0;

        (void) snprintf(descriptions[line], MORIA_MESSAGE_SIZE, "%c) %-14s: %s", line + 'a', equippedDescription, description);

        int l = (int) strlen(descriptions[line]) + 2;

        if (showWeights) {
            l += 9;
        }

        if (l > len) {
            len = l;
        }

        line++;
    }

    column = 79 - len;
    if (column < 0) {
        column = 0;
    }

    // Range of equipment
    line = 0;
    for (int i = PlayerEquipment::Wield; i < PLAYER_INVENTORY_SIZE; i++) {
        if (py.inventory[i].category_id == TV_NOTHING) {
            continue;
        }

        // don't need first two spaces when using whole screen
        if (column == 0) {
            putStringClearToEOL(descriptions[line], Coord_t{line + 1, column});
        } else {
            putString("  ", Coord_t{line + 1, column});
            putStringClearToEOL(descriptions[line], Coord_t{line + 1, column + 2});
        }

        if (showWeights) {
            obj_desc_t text = {'\0'};
            inventoryItemWeightText(text, i);
            putStringClearToEOL(text, Coord_t{line + 1, 71});
        }

        line++;
    }
    eraseLine(Coord_t{line + 1, column});

    return column;
}

static int showEquipmentHelpMenu(int leftColumn) {
    if (leftColumn > 52) {
        leftColumn = 52;
    }

    putStringClearToEOL("  ESC: exit", Coord_t{1, leftColumn});
    putStringClearToEOL("  w  : wear or wield object", Coord_t{2, leftColumn});
    putStringClearToEOL("  t  : take off item", Coord_t{3, leftColumn});
    putStringClearToEOL("  d  : drop object", Coord_t{4, leftColumn});
    putStringClearToEOL("  x  : exchange weapons", Coord_t{5, leftColumn});
    putStringClearToEOL("  i  : inventory of pack", Coord_t{6, leftColumn});
    putStringClearToEOL("  e  : list used equipment", Coord_t{7, leftColumn});

    return 7; // current line position
}

// TODO: Most of the functions below should probably not be part of the UI,
// TODO: but have dependencies on "screen state" variables, so need to be here for now.

// All inventory commands (wear, exchange, take off, drop, inventory and
// equipment) are handled in an alternative command input mode, which accepts
// any of the inventory commands.
//
// It is intended that this function be called several times in succession,
// as some commands take up a turn, and the rest of moria must proceed in the
// interim. A global variable is provided, game.doing_inventory_command, which is normally
// zero; however if on return from inventoryExecuteCommand() it is expected that
// inventoryExecuteCommand() should be called *again*, (being still in inventory command
// input mode), then game.doing_inventory_command is set to the inventory command character
// which should be used in the next call to inventoryExecuteCommand().
//
// On return, the screen is restored, but not flushed. Provided no flush of
// the screen takes place before the next call to inventoryExecuteCommand(), the inventory
// command screen is silently redisplayed, and no actual output takes place at
// all. If the screen is flushed before a subsequent call, then the player is
// prompted to see if we should continue. This allows the player to see any
// changes that take place on the screen during inventory command input.
//
// The global variable, screen_has_changed, is cleared by inventoryExecuteCommand(), and set
// when the screen is flushed. This is the means by which inventoryExecuteCommand() tell
// if the screen has been flushed.
//
// The display of inventory items is kept to the right of the screen to
// minimize the work done to restore the screen afterwards. -CJS-

static void uiCommandSwitchScreen(Screen nextScreen) {
    if (nextScreen == game.screen.current_screen_id) {
        return;
    }
    game.screen.current_screen_id = nextScreen;

    int currentLinePos = 0;
    switch (nextScreen) {
        case Screen::Blank:
            break;
        case Screen::Help:
            currentLinePos = showEquipmentHelpMenu(game.screen.screen_left_pos);
            break;
        case Screen::Inventory:
            game.screen.screen_left_pos = displayInventoryItems(0, py.pack.unique_items - 1, config::options::show_inventory_weights, game.screen.screen_left_pos, CNIL);
            currentLinePos = py.pack.unique_items;
            break;
        case Screen::Wear:
            game.screen.screen_left_pos =
                displayInventoryItems(game.screen.wear_low_id, game.screen.wear_high_id, config::options::show_inventory_weights, game.screen.screen_left_pos, CNIL);
            currentLinePos = game.screen.wear_high_id - game.screen.wear_low_id + 1;
            break;
        case Screen::Equipment:
            game.screen.screen_left_pos = displayEquipment(config::options::show_inventory_weights, game.screen.screen_left_pos);
            currentLinePos = py.equipment_count;
            break;
        case Screen::Wrong:
        default:
            break;
    }

    if (currentLinePos >= game.screen.screen_bottom_pos) {
        game.screen.screen_bottom_pos = currentLinePos + 1;
        eraseLine(Coord_t{game.screen.screen_bottom_pos, game.screen.screen_left_pos});
        return;
    }
    currentLinePos++;

    while (currentLinePos <= game.screen.screen_bottom_pos) {
        eraseLine(Coord_t{currentLinePos, game.screen.screen_left_pos});
        currentLinePos++;
    }
}

// Used to verify if this really is the item we wish to wear or read. -CJS-
static bool verifyAction(const char *prompt, int item) {
    obj_desc_t description = {'\0'};
    itemDescription(description, py.inventory[item], true);

    // change the period to a question mark
    description[strlen(description) - 1] = '?';

    obj_desc_t msg = {'\0'};
    (void) snprintf(msg, MORIA_OBJ_DESC_SIZE, "%s %s", prompt, description);

    return getInputConfirmation(msg);
}

static void requestAndShowInventoryScreen(bool recoverScreen) {
    if (game.doing_inventory_command == 0) {
        game.screen.screen_left_pos = 50;
        game.screen.screen_bottom_pos = 0;
        game.screen.current_screen_id = Screen::Blank; // this forces exit of inventoryExecuteCommand() if selecting is not set true
        return;
    }

    // Take up where we left off after a previous inventory command. -CJS-

    // If the screen has been flushed, we need to redraw. If the command
    // is a simple ' ' to recover the screen, just quit. Otherwise, check
    // and see what the user wants.
    if (screen_has_changed) {
        if (recoverScreen || !getInputConfirmation("Continuing with inventory command?")) {
            game.doing_inventory_command = 0;
            return;
        }
        game.screen.screen_left_pos = 50;
        game.screen.screen_bottom_pos = 0;
    }

    Screen currentScreen = game.screen.current_screen_id;
    game.screen.current_screen_id = Screen::Wrong;
    uiCommandSwitchScreen(currentScreen);
}

static bool uiCommandInventoryTakeOffItem(bool selecting) {
    if (py.equipment_count == 0) {
        printMessage("You are not using any equipment.");
        // don't print message restarting inven command after taking off something, it is confusing
        return selecting;
    }

    if (py.pack.unique_items >= PlayerEquipment::Wield && (game.doing_inventory_command == 0)) {
        printMessage("You will have to drop something first.");
        return selecting;
    }

    if (game.screen.current_screen_id != Screen::Blank) {
        uiCommandSwitchScreen(Screen::Equipment);
    }

    return true;
}

static bool uiCommandInventoryDropItem(char &command, bool selecting) {
    if (py.pack.unique_items == 0 && py.equipment_count == 0) {
        printMessage("But you're not carrying anything.");
        return selecting;
    }

    if (dg.floor[py.pos.y][py.pos.x].treasure_id != 0) {
        printMessage("There's no room to drop anything here.");
        return selecting;
    }

    if ((game.screen.current_screen_id == Screen::Equipment && py.equipment_count > 0) || py.pack.unique_items == 0) {
        if (game.screen.current_screen_id != Screen::Blank) {
            uiCommandSwitchScreen(Screen::Equipment);
        }
        command = 'r'; // Remove - or take off and drop.
    } else if (game.screen.current_screen_id != Screen::Blank) {
        uiCommandSwitchScreen(Screen::Inventory);
    }

    return true;
}

static bool uiCommandInventoryWearWieldItem(bool selecting) {
    // Note: simple loop to get the global game.screen.wear_low_id value
    game.screen.wear_low_id = 0;
    while (game.screen.wear_low_id < py.pack.unique_items && py.inventory[game.screen.wear_low_id].category_id > TV_MAX_WEAR) {
        game.screen.wear_low_id++;
    }

    // Note: simple loop to get the global wear_high value
    game.screen.wear_high_id = game.screen.wear_low_id;
    while (game.screen.wear_high_id < py.pack.unique_items && py.inventory[game.screen.wear_high_id].category_id >= TV_MIN_WEAR) {
        game.screen.wear_high_id++;
    }
    game.screen.wear_high_id--;

    if (game.screen.wear_low_id > game.screen.wear_high_id) {
        printMessage("You have nothing to wear or wield.");
        return selecting;
    }

    if (game.screen.current_screen_id != Screen::Blank && game.screen.current_screen_id != Screen::Inventory) {
        uiCommandSwitchScreen(Screen::Wear);
    }

    return true;
}

static void uiCommandInventoryUnwieldItem() {
    if (!playerIsWieldingItem()) {
        printMessage("But you are wielding no weapons.");
        return;
    }

    if (playerWornItemIsCursed(PlayerEquipment::Wield)) {
        obj_desc_t description = {'\0'};
        itemDescription(description, py.inventory[PlayerEquipment::Wield], false);

        obj_desc_t msg = {'\0'};
        (void) snprintf(msg, MORIA_OBJ_DESC_SIZE, "The %s you are wielding appears to be cursed.", description);

        printMessage(msg);

        return;
    }

    game.player_free_turn = false;

    // swap auxiliary and wield items
    Inventory_t savedItem = py.inventory[PlayerEquipment::Auxiliary];
    py.inventory[PlayerEquipment::Auxiliary] = py.inventory[PlayerEquipment::Wield];
    py.inventory[PlayerEquipment::Wield] = savedItem;

    if (game.screen.current_screen_id == Screen::Equipment) {
        game.screen.screen_left_pos = displayEquipment(config::options::show_inventory_weights, game.screen.screen_left_pos);
    }

    playerAdjustBonusesForItem(py.inventory[PlayerEquipment::Auxiliary], -1); // Subtract bonuses
    playerAdjustBonusesForItem(py.inventory[PlayerEquipment::Wield], 1);      // Add bonuses

    if (py.inventory[PlayerEquipment::Wield].category_id != TV_NOTHING) {
        obj_desc_t label = {'\0'};
        (void) strcpy(label, "Primary weapon   : ");

        obj_desc_t description = {'\0'};
        itemDescription(description, py.inventory[PlayerEquipment::Wield], true);

        printMessage(strcat(label, description));
    } else {
        printMessage("No primary weapon.");
    }

    // this is a new weapon, so clear the heavy flag
    py.weapon_is_heavy = false;
    playerStrength();
}

// look for item whose inscription matches `which`
static int inventoryGetItemMatchingInscription(char which, char command, int from, int to) {
    int itemId;

    if (which >= '0' && which <= '9' && command != 'r' && command != 't') {
        // Note: simple loop to get id
        int m = from;
        while (m <= to && m < PLAYER_INVENTORY_SIZE && ((py.inventory[m].inscription[0] != which) || (py.inventory[m].inscription[1] != '\0'))) {
            m++;
        }

        if (m <= to) {
            itemId = m;
        } else {
            itemId = -1;
        }
    } else if (which >= 'A' && which <= 'Z') {
        itemId = which - 'A';
    } else {
        itemId = which - 'a';
    }

    return itemId;
}

static void buildCommandHeading(obj_desc_t str, int from, int to, const char *swap, char command, const char *prompt) {
    from += 'a';
    to += 'a';

    const char *list = "";
    if (game.screen.current_screen_id == Screen::Blank) {
        list = ", * to list";
    }

    const char *digits = "";
    if (command == 'w' || command == 'd') {
        digits = ", 0-9";
    }

    (void) snprintf(str, MORIA_OBJ_DESC_SIZE, "(%c-%c%s%s%s, space to break, ESC to exit) %s which one?", from, to, list, swap, digits, prompt);
}

static void changeScreenForCommand(char command) {
    if (command == 't' || command == 'r') {
        uiCommandSwitchScreen(Screen::Equipment);
    } else if (command == 'w' && game.screen.current_screen_id != Screen::Inventory) {
        uiCommandSwitchScreen(Screen::Wear);
    } else {
        uiCommandSwitchScreen(Screen::Inventory);
    }
}

static void flipInventoryEquipmentScreens() {
    if (game.screen.current_screen_id == Screen::Equipment) {
        uiCommandSwitchScreen(Screen::Inventory);
    } else if (game.screen.current_screen_id == Screen::Inventory) {
        uiCommandSwitchScreen(Screen::Equipment);
    }
}

static int requestPutRingOnWhichHand() {
    int hand = 0;

    // Rings. Give choice over where they go.
    do {
        char query;
        if (!getMenuItemId("Put ring on which hand (l/r/L/R)?", query)) {
            hand = -1;
        } else if (query == 'l') {
            hand = PlayerEquipment::Left;
        } else if (query == 'r') {
            hand = PlayerEquipment::Right;
        } else {
            if (query == 'L') {
                hand = PlayerEquipment::Left;
            } else if (query == 'R') {
                hand = PlayerEquipment::Right;
            } else {
                terminalBellSound();
            }
            if ((hand != 0) && !verifyAction("Replace", hand)) {
                hand = 0;
            }
        }
    } while (hand == 0);

    return hand;
}

static int inventoryGetSlotToWearEquipment(int categoryId) {
    int slot = -1;

    // Slot for equipment
    switch (categoryId) {
        case TV_SLING_AMMO:
        case TV_BOLT:
        case TV_ARROW:
        case TV_BOW:
        case TV_HAFTED:
        case TV_POLEARM:
        case TV_SWORD:
        case TV_DIGGING:
        case TV_SPIKE:
            slot = PlayerEquipment::Wield;
            break;
        case TV_LIGHT:
            slot = PlayerEquipment::Light;
            break;
        case TV_BOOTS:
            slot = PlayerEquipment::Feet;
            break;
        case TV_GLOVES:
            slot = PlayerEquipment::Hands;
            break;
        case TV_CLOAK:
            slot = PlayerEquipment::Outer;
            break;
        case TV_HELM:
            slot = PlayerEquipment::Head;
            break;
        case TV_SHIELD:
            slot = PlayerEquipment::Arm;
            break;
        case TV_HARD_ARMOR:
        case TV_SOFT_ARMOR:
            slot = PlayerEquipment::Body;
            break;
        case TV_AMULET:
            slot = PlayerEquipment::Neck;
            break;
        case TV_RING:
            if (playerRightHandRingEmpty()) {
                slot = PlayerEquipment::Right;
            } else if (playerLeftHandRingEmpty()) {
                slot = PlayerEquipment::Left;
            } else {
                slot = requestPutRingOnWhichHand();
            }
            break;
        default:
            printMessage("IMPOSSIBLE: I don't see how you can use that.");
            break;
    }

    return slot;
}

static void inventoryItemIsCursedMessage(int itemId) {
    obj_desc_t description = {'\0'};
    itemDescription(description, py.inventory[itemId], false);

    obj_desc_t msg = {'\0'};
    (void) snprintf(msg, MORIA_OBJ_DESC_SIZE, "The %s you are ", description);

    if (itemId == PlayerEquipment::Head) {
        (void) strcat(msg, "wielding ");
    } else {
        (void) strcat(msg, "wearing ");
    }

    printMessage(strcat(msg, "appears to be cursed."));
}

// Get its place in the equipment list.
static bool executeRemoveItemCommand(bool selecting, int itemId, char &command, const char *which, const char *prompt) {
    int itemIdToTakeOff = itemId;
    itemId = 21;

    do {
        itemId++;
        if (py.inventory[itemId].category_id != TV_NOTHING) {
            itemIdToTakeOff--;
        }
    } while (itemIdToTakeOff >= 0);

    if ((isupper((int) *which) != 0) && !verifyAction((char *) prompt, itemId)) {
        itemId = -1;
    } else if (inventoryItemIsCursed(py.inventory[itemId])) {
        itemId = -1;
        printMessage("Hmmm, it seems to be cursed.");
    } else if (command == 't' && !inventoryCanCarryItemCount(py.inventory[itemId])) {
        if (dg.floor[py.pos.y][py.pos.x].treasure_id != 0) {
            itemId = -1;
            printMessage("You can't carry it.");
        } else if (getInputConfirmation("You can't carry it.  Drop it?")) {
            command = 'r';
        } else {
            itemId = -1;
        }
    }

    if (itemId >= 0) {
        if (command == 'r') {
            inventoryDropItem(itemId, true);
            // As a safety measure, set the player's inven
            // weight to 0, when the last object is dropped.
            if (py.pack.unique_items == 0 && py.equipment_count == 0) {
                py.pack.weight = 0;
            }
        } else {
            playerTakeOff(itemId, inventoryCarryItem(py.inventory[itemId]));
        }

        playerStrength();
        game.player_free_turn = false;

        if (command == 'r') {
            selecting = false;
        }
    }

    return selecting;
}

// Wearing. Go to a bit of trouble over replacing existing equipment.
static void executeWearItemCommand(int itemId, const char *which, const char *prompt) {
    int slot = 0;

    if ((isupper((int) *which) != 0) && !verifyAction((char *) prompt, itemId)) {
        itemId = -1;
    } else {
        slot = inventoryGetSlotToWearEquipment(py.inventory[itemId].category_id);
        if (slot == -1) {
            itemId = -1;
        }
    }

    if (itemId >= 0 && py.inventory[slot].category_id != TV_NOTHING) {
        if (inventoryItemIsCursed(py.inventory[slot])) {
            inventoryItemIsCursedMessage(slot);
            itemId = -1;
        } else if (py.inventory[itemId].sub_category_id == ITEM_GROUP_MIN && py.inventory[itemId].items_count > 1 && !inventoryCanCarryItemCount(py.inventory[slot])) {
            // this can happen if try to wield a torch,
            // and have more than one in inventory
            printMessage("You will have to drop something first.");
            itemId = -1;
        }
    }

    if (itemId == -1) {
        return;
    }

    // OK. Wear it.
    game.player_free_turn = false;

    //
    // 1. remove new item from inventory
    //
    Inventory_t savedItem = py.inventory[itemId];
    Inventory_t *item = &savedItem;

    game.screen.wear_high_id--;

    // Fix for torches
    if (item->items_count > 1 && item->sub_category_id <= ITEM_SINGLE_STACK_MAX) {
        item->items_count = 1;
        game.screen.wear_high_id++;
    }

    py.pack.weight += item->weight * item->items_count;
    inventoryDestroyItem(itemId); // Subtracts weight

    //
    // 2. add old item to inv and remove from equipment list, if necessary.
    //
    item = &py.inventory[slot];
    if (item->category_id != TV_NOTHING) {
        int uniqItems = py.pack.unique_items;

        int id = inventoryCarryItem(*item);

        // If item removed did not stack with anything
        // in inventory, then increment wear_high.
        if (py.pack.unique_items != uniqItems) {
            game.screen.wear_high_id++;
        }

        playerTakeOff(slot, id);
    }

    //
    // 3. wear new item
    //
    *item = savedItem;
    py.equipment_count++;

    playerAdjustBonusesForItem(*item, 1);

    soundEvent("wield");
    const char *text = nullptr;
    if (slot == PlayerEquipment::Wield) {
        text = "You are wielding";
    } else if (slot == PlayerEquipment::Light) {
        text = "Your light source is";
    } else {
        text = "You are wearing";
    }

    obj_desc_t description = {'\0'};
    itemDescription(description, *item, true);

    // Get the right equipment letter.
    int itemIdToTakeOff = PlayerEquipment::Wield;
    itemId = 0;

    while (itemIdToTakeOff != slot) {
        if (py.inventory[itemIdToTakeOff++].category_id != TV_NOTHING) {
            itemId++;
        }
    }

    obj_desc_t msg = {'\0'};
    (void) snprintf(msg, MORIA_OBJ_DESC_SIZE, "%s %s (%c)", text, description, 'a' + itemId);
    printMessage(msg);

    // this is a new weapon, so clear heavy flag
    if (slot == PlayerEquipment::Wield) {
        py.weapon_is_heavy = false;
    }
    playerStrength();

    if (inventoryItemIsCursed(*item)) {
        printMessage("Oops! It feels deathly cold!");
        itemAppendToInscription(*item, config::identification::ID_DAMD);

        // To force a cost of 0, even if unidentified.
        item->cost = -1;
    }
}

static void executeDropItemCommand(int itemId, const char *which, const char *prompt) {
    int confirmed = -1;

    if (py.inventory[itemId].items_count > 1) {
        obj_desc_t description = {'\0'};
        itemDescription(description, py.inventory[itemId], true);
        description[strlen(description) - 1] = '?'; // replace period with question

        obj_desc_t msg = {'\0'};
        (void) snprintf(msg, MORIA_OBJ_DESC_SIZE, "Drop all %s", description);

        // request command from player
        confirmed = getInputConfirmationWithAbort(0, msg);
        // if aborted
        if (confirmed == -1) {
            itemId = -1;
        }
    } else if ((isupper((int) *which) != 0) && !verifyAction((char *) prompt, itemId)) {
        itemId = -1;
    }

    if (itemId >= 0) {
        game.player_free_turn = false;

        inventoryDropItem(itemId, confirmed == 1);
        playerStrength();
    }

    // As a safety measure, set the player's inven weight
    // to 0, when the last object is dropped.
    if (py.pack.unique_items == 0 && py.equipment_count == 0) {
        py.pack.weight = 0;
    }
}

static bool selectItemCommands(char *command, char *which, bool selecting) {
    int fromLine, toLine;
    const char *prompt = nullptr;
    const char *swap = nullptr;

    while (selecting && game.player_free_turn) {
        swap = "";

        if (*command == 'w') {
            fromLine = game.screen.wear_low_id;
            toLine = game.screen.wear_high_id;
            prompt = "Wear/Wield";
        } else {
            fromLine = 0;
            if (*command == 'd') {
                toLine = py.pack.unique_items - 1;
                prompt = "Drop";

                if (py.equipment_count > 0) {
                    swap = ", / for Equip";
                }
            } else {
                toLine = py.equipment_count - 1;

                if (*command == 't') {
                    prompt = "Take off";
                } else {
                    // command == 'r'

                    prompt = "Throw off";
                    if (py.pack.unique_items > 0) {
                        swap = ", / for Inven";
                    }
                }
            }
        }

        if (fromLine > toLine) {
            selecting = false;
            continue;
        }

        obj_desc_t headingMsg = {'\0'};
        changeScreenForCommand(*command);
        buildCommandHeading(headingMsg, fromLine, toLine, swap, *command, prompt);
        putStringClearToEOL(headingMsg, Coord_t{0, 0});

        static int cur = 0;
        char listCommand = *command;
        int k = itemCursorKey(cur, toLine - fromLine + 1, [&] {
            game.screen.current_screen_id = Screen::Wrong;
            changeScreenForCommand(listCommand);
        });
        *which = (char) (k < 0 ? 'a' + fromLine + cur : k);
        messageLineClear();

        // Abort everything.
        if (*which == ESCAPE) {
            *which = ESCAPE;
            selecting = false;
            continue;
        }

        // Draw the screen and maybe exit toLine main prompt.
        if (*which == ' ' || *which == '*') {
            changeScreenForCommand(*command);
            if (*which == ' ') {
                selecting = false;
            }
            continue;
        }

        // Swap screens (for drop)
        if (*which == '/' && (swap[0] != 0)) {
            if (*command == 'd') {
                *command = 'r';
            } else {
                *command = 'd';
            }
            flipInventoryEquipmentScreens();
            continue;
        }

        // look for item whose inscription matches "which"
        int itemId = inventoryGetItemMatchingInscription(*which, *command, fromLine, toLine);
        if (itemId < fromLine || itemId > toLine) {
            terminalBellSound();
            continue;
        }

        //
        // Found an item - do something with it!
        //
        if (*command == 'r' || *command == 't') {
            // Get its place in the equipment list.
            selecting = executeRemoveItemCommand(selecting, itemId, *command, which, prompt);
        } else if (*command == 'w') {
            executeWearItemCommand(itemId, which, prompt);
        } else {
            // command == 'd'
            executeDropItemCommand(itemId, which, prompt);
            selecting = false;
        }

        if (!game.player_free_turn && game.screen.current_screen_id == Screen::Blank) {
            selecting = false;
        }
    }

    return selecting;
}

// Put an appropriate header message (on message line).
static void inventoryDisplayAppropriateHeader() {
    if (game.screen.current_screen_id == Screen::Inventory) {
        obj_desc_t msg = {'\0'};
        int weightQuotient = py.pack.weight / 10;
        int weightRemainder = py.pack.weight % 10;

        if (!config::options::show_inventory_weights || py.pack.unique_items == 0) {
            (void) snprintf(msg, MORIA_OBJ_DESC_SIZE, "You are carrying %d.%d pounds. In your pack there is %s", //
                            weightQuotient, weightRemainder, (py.pack.unique_items == 0 ? "nothing." : "-"));
        } else {
            int capacityQuotient = playerCarryingLoadLimit() / 10;
            int capacityRemainder = playerCarryingLoadLimit() % 10;

            (void) snprintf(msg, MORIA_OBJ_DESC_SIZE,                                                          //
                            "You are carrying %d.%d pounds. Your capacity is %d.%d pounds. In your pack is -", //
                            weightQuotient, weightRemainder, capacityQuotient, capacityRemainder);
        }

        putStringClearToEOL(msg, Coord_t{0, 0});
    } else if (game.screen.current_screen_id == Screen::Wear) {
        if (game.screen.wear_high_id < game.screen.wear_low_id) {
            putStringClearToEOL("You have nothing you could wield.", Coord_t{0, 0});
        } else {
            putStringClearToEOL("You could wield -", Coord_t{0, 0});
        }
    } else if (game.screen.current_screen_id == Screen::Equipment) {
        if (py.equipment_count == 0) {
            putStringClearToEOL("You are not using anything.", Coord_t{0, 0});
        } else {
            putStringClearToEOL("You are using -", Coord_t{0, 0});
        }
    } else {
        putStringClearToEOL("Allowed commands:", Coord_t{0, 0});
    }

    eraseLine(Coord_t{game.screen.screen_bottom_pos, game.screen.screen_left_pos});
}

static void uiCommandDisplayInventory() {
    if (py.pack.unique_items == 0) {
        printMessage("You are not carrying anything.");
    } else {
        uiCommandSwitchScreen(Screen::Inventory);
    }
}

static void uiCommandDisplayEquipment() {
    if (py.equipment_count == 0) {
        printMessage("You are not using any equipment.");
    } else {
        uiCommandSwitchScreen(Screen::Equipment);
    }
}

// This does all the work.
void inventoryExecuteCommand(char command) {
    game.player_free_turn = true;

    terminalSaveScreen();

    bool recoverScreen = false;
    if (command == ' ') {
        recoverScreen = true;
    }
    requestAndShowInventoryScreen(recoverScreen);

    do {
        if (isupper((int) command) != 0) {
            command = (char) tolower((int) command);
        }

        // Simple command getting and screen selection.
        bool selecting = false;
        switch (command) {
            case 'i':
                uiCommandDisplayInventory();
                break;
            case 'e':
                uiCommandDisplayEquipment();
                break;
            case 't':
                selecting = uiCommandInventoryTakeOffItem(selecting);
                break;
            case 'd':
                selecting = uiCommandInventoryDropItem(command, selecting);
                break;
            case 'w':
                selecting = uiCommandInventoryWearWieldItem(selecting);
                break;
            case 'x':
                uiCommandInventoryUnwieldItem();
                break;
            case '?':
                uiCommandSwitchScreen(Screen::Help);
                break;
            case ' ':
                // Dummy command to return again to main prompt.
                break;
            default:
                // Nonsense command
                terminalBellSound();
                break;
        }

        // Clear the game.doing_inventory_command flag here, instead of at beginning, so that
        // can use it to control when messages above appear.
        game.doing_inventory_command = 0;

        // Keep looking for objects to drop/wear/take off/throw off
        char which = 'z';

        selecting = selectItemCommands(&command, &which, selecting);

        if (which == ESCAPE || game.screen.current_screen_id == Screen::Blank) {
            command = ESCAPE;
        } else if (!game.player_free_turn) {
            // Save state for recovery if they want to call us again next turn.
            // Otherwise, set a dummy command to recover screen.
            if (selecting) {
                game.doing_inventory_command = command;
            } else {
                game.doing_inventory_command = ' ';
            }

            // flush last message before clearing screen_has_changed and exiting
            printMessage(CNIL);

            // This lets us know if the world changes
            screen_has_changed = false;

            command = ESCAPE;
        } else {
            inventoryDisplayAppropriateHeader();

            putString("e/i/t/w/x/d/?/ESC:", Coord_t{game.screen.screen_bottom_pos, 60});
            command = getKeyInput();

            eraseLine(Coord_t{game.screen.screen_bottom_pos, game.screen.screen_left_pos});
        }
    } while (command != ESCAPE);

    if (game.screen.current_screen_id != Screen::Blank) {
        terminalRestoreScreen();
    }

    playerRecalculateBonuses();
}

enum class PackMenu {
    CloseMenu,
    Equipment,
    Inventory,
};

// Switch between Equipment/Inventory menu.
// Returns true when menu has changed
static bool inventorySwitchPackMenu(vtype_t &prompt, PackMenu &menu, bool menuActive, int &itemIdEnd) {
    if (menu == PackMenu::Inventory) {
        bool changed = false;

        if (py.equipment_count == 0) {
            putStringClearToEOL("But you're not using anything -more-", Coord_t{0, 0});
            (void) getKeyInput();
        } else {
            menu = PackMenu::Equipment;
            changed = true;

            if (menuActive) {
                itemIdEnd = py.equipment_count;

                while (itemIdEnd < py.pack.unique_items) {
                    itemIdEnd++;
                    eraseLine(Coord_t{itemIdEnd, 0});
                }
            }
            itemIdEnd = py.equipment_count - 1;
        }

        putStringClearToEOL(prompt, Coord_t{0, 0});
        return changed;
    }

    if (py.pack.unique_items == 0) {
        putStringClearToEOL("But you're not carrying anything -more-", Coord_t{0, 0});
        (void) getKeyInput();
        return false;
    }

    menu = PackMenu::Inventory;
    if (menuActive) {
        itemIdEnd = py.pack.unique_items;

        while (itemIdEnd < py.equipment_count) {
            itemIdEnd++;
            eraseLine(Coord_t{itemIdEnd, 0});
        }
    }
    itemIdEnd = py.pack.unique_items - 1;
    return true;
}

// Get the ID of an item and return the CTR value of it -RAK-
bool inventoryGetInputForItemId(int &commandKeyId, const char *prompt, int itemIdStart, int itemIdEnd, char *mask, const char *message) {
    PackMenu menu = PackMenu::Inventory;
    bool packFull = false;

    if (itemIdEnd > PlayerEquipment::Wield) {
        packFull = true;

        if (py.pack.unique_items == 0) {
            menu = PackMenu::Equipment;
            itemIdEnd = py.equipment_count - 1;
        } else {
            itemIdEnd = py.pack.unique_items - 1;
        }
    }

    if (py.pack.unique_items < 1 && (!packFull || py.equipment_count < 1)) {
        putStringClearToEOL("You are not carrying anything.", Coord_t{0, 0});
        return false;
    }

    commandKeyId = 0;

    bool itemFound = false;
    bool menuActive = true; // RVIP 3c: the list is always shown
    terminalSaveScreen();
    static int cur = 0;

    do {
        if (menuActive) {
            if (menu == PackMenu::Inventory) {
                (void) displayInventoryItems(itemIdStart, itemIdEnd, false, 80, mask);
            } else {
                (void) displayEquipment(false, 80);
            }
        }

        vtype_t description = {'\0'};

        if (packFull) {
            (void) snprintf(description, MORIA_MESSAGE_SIZE,                   //
                            "(%s: %c-%c,%s%s / for %s, or ESC) %s",            //
                            (menu == PackMenu::Inventory ? "Inven" : "Equip"), //
                            itemIdStart + 'a',                                 //
                            itemIdEnd + 'a',                                   //
                            (menu == PackMenu::Inventory ? " 0-9," : ""),      //
                            (menuActive ? "" : " * to see,"),                  //
                            (menu == PackMenu::Inventory ? "Equip" : "Inven"), //
                            prompt                                             //
            );
        } else {
            (void) snprintf(description, MORIA_MESSAGE_SIZE,              //
                            "(Items %c-%c,%s%s ESC to exit) %s",          //
                            itemIdStart + 'a',                            //
                            itemIdEnd + 'a',                              //
                            (menu == PackMenu::Inventory ? " 0-9," : ""), //
                            (menuActive ? "" : " * for inventory list,"), //
                            prompt                                        //
            );
        }

        putStringClearToEOL(description, Coord_t{0, 0});

        bool done = false;
        while (!done) {
            std::vector<int> rows; // item on each list row
            if (menu == PackMenu::Inventory) {
                for (int i = itemIdStart; i <= itemIdEnd; i++) {
                    if (mask == CNIL || mask[i] != 0) rows.push_back(i);
                }
            } else {
                for (int i = 0; i < py.equipment_count; i++) rows.push_back(i);
            }
            int k = itemCursorKey(cur, (int) rows.size(), [&] {
                if (menu == PackMenu::Inventory) {
                    (void) displayInventoryItems(itemIdStart, itemIdEnd, false, 80, mask);
                } else {
                    (void) displayEquipment(false, 80);
                }
            });
            char which = (char) (k < 0 ? 'a' + rows[cur] : k);

            switch (which) {
                case ESCAPE:
                    menu = PackMenu::CloseMenu;
                    done = true;
                    game.player_free_turn = true;
                    break;
                case '/':
                    done = inventorySwitchPackMenu(description, menu, menuActive, itemIdEnd);

                    break;
                case '*': // activate menu if required
                    if (!menuActive) {
                        done = true;
                        terminalSaveScreen();
                        menuActive = true;
                    }
                    break;
                default:
                    // look for item whose inscription matches "which"
                    if (which >= '0' && which <= '9' && menu != PackMenu::Equipment) {
                        int m;

                        // Note: loop to find the inventory item
                        for (m = itemIdStart; m < PlayerEquipment::Wield && (py.inventory[m].inscription[0] != which || py.inventory[m].inscription[1] != '\0'); m++)
                            ;

                        if (m < PlayerEquipment::Wield) {
                            commandKeyId = m;
                        } else {
                            commandKeyId = -1;
                        }
                    } else if (isupper((int) which) != 0) {
                        commandKeyId = which - 'A';
                    } else {
                        commandKeyId = which - 'a';
                    }

                    if (commandKeyId >= itemIdStart && commandKeyId <= itemIdEnd && (mask == CNIL || (mask[commandKeyId] != 0))) {
                        if (menu == PackMenu::Equipment) {
                            itemIdStart = 21;
                            itemIdEnd = commandKeyId;

                            do {
                                // Note: a simple loop to find first inventory item
                                itemIdStart++;
                                while (py.inventory[itemIdStart].category_id == TV_NOTHING) {
                                    itemIdStart++;
                                }

                                itemIdEnd--;
                            } while (itemIdEnd >= 0);

                            commandKeyId = itemIdStart;
                        }

                        if ((isupper((int) which) != 0) && !verifyAction("Try", commandKeyId)) {
                            menu = PackMenu::CloseMenu;
                            done = true;

                            game.player_free_turn = true;

                            break;
                        }

                        menu = PackMenu::CloseMenu;
                        done = true;

                        itemFound = true;
                    } else if (message != nullptr) {
                        printMessage(message);

                        // Set `done` to force redraw of the question.
                        done = true;
                    } else {
                        terminalBellSound();
                    }
                    break;
            }
        }
    } while (menu != PackMenu::CloseMenu);

    if (menuActive) {
        terminalRestoreScreen();
    }

    messageLineClear();

    return itemFound;
}

// ---- RVIP 3c: i / e with a cursor and item menus ----

char inventory_reopen = 0; // 'i' / 'e': reopen after the action, 0 = no

struct ItemAction {
    char key, rl_key; // original / roguelike command key
    bool item_letter; // the command then asks for the item
    const char *name;
};

static const ItemAction ACT_EAT{'E', 'E', true, "Eat"}, ACT_QUAFF{'q', 'q', true, "Quaff"}, ACT_READ{'r', 'r', true, "Read"},
    ACT_AIM{'a', 'z', true, "Aim"}, ACT_USE{'u', 'Z', true, "Use"}, ACT_CAST{'m', 'm', true, "Cast from"}, ACT_PRAY{'p', 'p', true, "Pray from"},
    ACT_BROWSE{'b', 'P', true, "Browse"}, ACT_WEAR{'w', 'w', true, "Wear/Wield"}, ACT_TAKEOFF{'t', 'T', true, "Take off"},
    ACT_FILL{'F', 'F', false, "Refuel lamp"}, ACT_THROW{'f', 't', true, "Throw/Fire"}, ACT_DROP{'d', 'd', true, "Drop"},
    ACT_INSCRIBE{'{', '{', true, "Inscribe"}, ACT_EXAMINE{0, 0, false, "Examine"};

static int equipmentSlot(int n) {
    for (int i = PlayerEquipment::Wield; i < PLAYER_INVENTORY_SIZE; i++) {
        if (py.inventory[i].category_id != TV_NOTHING && n-- == 0) return i;
    }
    return -1;
}

static std::vector<ItemAction> itemActions(bool equipment, int slot) {
    if (equipment) return {ACT_TAKEOFF, ACT_DROP, ACT_INSCRIBE, ACT_EXAMINE};
    std::vector<ItemAction> a;
    int tv = py.inventory[slot].category_id;
    switch (tv) {
        case TV_FOOD: a.push_back(ACT_EAT); break;
        case TV_POTION1: case TV_POTION2: a.push_back(ACT_QUAFF); break;
        case TV_SCROLL1: case TV_SCROLL2: a.push_back(ACT_READ); break;
        case TV_WAND: a.push_back(ACT_AIM); break;
        case TV_STAFF: a.push_back(ACT_USE); break;
        case TV_MAGIC_BOOK: a.push_back(ACT_CAST); a.push_back(ACT_BROWSE); break;
        case TV_PRAYER_BOOK: a.push_back(ACT_PRAY); a.push_back(ACT_BROWSE); break;
        case TV_FLASK: a.push_back(ACT_FILL); break;
        case TV_SLING_AMMO: case TV_BOLT: case TV_ARROW: case TV_SPIKE: a.push_back(ACT_THROW); break;
        default:
            if (tv >= TV_LIGHT && tv <= TV_MAX_WEAR) a.push_back(ACT_WEAR);
    }
    if (a.empty() || a[0].key != 'f') a.push_back(ACT_THROW);
    a.push_back(ACT_DROP);
    a.push_back(ACT_INSCRIBE);
    a.push_back(ACT_EXAMINE);
    return a;
}

// Runs `act` on item n of the list. Returns true when the screen closes.
static bool itemDoAction(ItemAction const &act, bool equipment, int n) {
    int slot = equipment ? equipmentSlot(n) : n;
    if (act.key == 0) {
        obj_desc_t d = {'\0'};
        itemDescription(d, py.inventory[slot], true);
        printMessage(d);
        return false;
    }
    std::string keys(1, config::options::use_roguelike_keys ? act.rl_key : act.key);
    if (act.item_letter) {
        if (equipment && act.key == 'd') keys += '/'; // drop: switch to equipment
        keys += (char) ('a' + n);
    }
    keyQueuePush(keys);
    inventory_reopen = equipment ? 'e' : 'i';
    return true;
}

// Floating menu of the actions for one item; true when one ran.
static bool itemActionMenu(bool equipment, int n) {
    int slot = equipment ? equipmentSlot(n) : n;
    std::vector<ItemAction> acts = itemActions(equipment, slot);
    obj_desc_t d = {'\0'};
    itemDescription(d, py.inventory[slot], true);
    size_t width = strlen(d);
    for (auto const &a : acts) width = std::max(width, strlen(a.name) + 4);

    // Umoria has one saved screen, owned by inventoryBrowse(): show the menu
    // alone over the map, the list is redrawn when it comes back.
    terminalRestoreScreen();
    terminalSaveScreen();
    int cur = 0, count = (int) acts.size();
    for (;;) {
        // box sized to its content: title + one line per action
        std::string title(d);
        title.resize(width, ' ');
        putString(title.c_str(), Coord_t{1, 13});
        for (int i = 0; i < count; i++) {
            char key = config::options::use_roguelike_keys ? acts[i].rl_key : acts[i].key;
            std::string s = std::string(" ") + (key != 0 ? key : '*') + "  " + acts[i].name;
            s.resize(width, ' ');
            ui_highlight_row = i == cur ? 2 + i : -1;
            putString(s.c_str(), Coord_t{2 + i, 13});
        }
        ui_highlight_row = -1;
        moveCursor(Coord_t{2 + cur, 13});
        int k = getKeyInput();
        int pick = -1;
        if (k == '8') cur = (cur + count - 1) % count;
        else if (k == '2') cur = (cur + 1) % count;
        else if (k == '5' || k == '6' || k == '\r' || k == '\n' || k == ' ') pick = cur;
        else if (k == '*') pick = count - 1;
        else if (k == ESCAPE || k == '4' || k == '0' || k == '.') break;
        else {
            for (int i = 0; i < count; i++) {
                char key = config::options::use_roguelike_keys ? acts[i].rl_key : acts[i].key;
                if (key == k) pick = i;
            }
        }
        if (pick >= 0) return itemDoAction(acts[pick], equipment, n);
    }
    return false;
}

void inventoryBrowse(bool equipment) {
    game.player_free_turn = true;
    inventory_reopen = 0;
    static int cur = 0;
    terminalSaveScreen();
    for (;;) {
        if (py.pack.unique_items == 0 && py.equipment_count == 0) {
            terminalRestoreScreen();
            printMessage("You are not carrying anything.");
            return;
        }
        if (!equipment && py.pack.unique_items == 0) equipment = true;
        if (equipment && py.equipment_count == 0) equipment = false;
        int n = equipment ? py.equipment_count : py.pack.unique_items;

        terminalRestoreScreen();
        terminalSaveScreen();
        putStringClearToEOL(equipment ? "Equipment: letter/+ takes off, Shift/- drops, Ctrl/* examines, Enter: menu, 4/6: inventory"
                                      : "Inventory: letter/+ uses, Shift/- drops, Ctrl/* examines, Enter: menu, 4/6: equipment",
                            Coord_t{0, 0});
        int k = itemCursorKey(cur, n, [&] {
            if (equipment) {
                (void) displayEquipment(config::options::show_inventory_weights, 80);
            } else {
                (void) displayInventoryItems(0, n - 1, config::options::show_inventory_weights, 80, CNIL);
            }
        });
        bool closes = false;
        int slot = equipment ? equipmentSlot(cur) : cur;
        if (k == ESCAPE) {
            break;
        } else if (k == '/') {
            equipment = !equipment;
            continue;
        } else if (k < 0) {
            closes = itemActionMenu(equipment, cur);
        } else if (k == '+') {
            closes = itemDoAction(itemActions(equipment, slot)[0], equipment, cur);
        } else if (k == '-') {
            closes = itemDoAction(ACT_DROP, equipment, cur);
        } else if (k == '*') {
            closes = itemDoAction(ACT_EXAMINE, equipment, cur);
        } else if (k >= 'a' && k < 'a' + n) {
            cur = k - 'a';
            slot = equipment ? equipmentSlot(cur) : cur;
            closes = itemDoAction(itemActions(equipment, slot)[0], equipment, cur);
        } else if (k >= 'A' && k < 'A' + n) {
            cur = k - 'A';
            closes = itemDoAction(ACT_DROP, equipment, cur);
        } else if (k >= 1 && k <= n) { // Ctrl+letter
            cur = k - 1;
            closes = itemDoAction(ACT_EXAMINE, equipment, cur);
        } else {
            keyQueuePush(std::string(1, (char) k)); // any other key: a normal command
            closes = true;
        }
        if (closes) break;
    }
    terminalRestoreScreen();
}
