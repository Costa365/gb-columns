#include <gb/gb.h>

#include <stdint.h>
#include <string.h>

#include "game.h"
#include "tiles.h"

#define SCREEN_WIDTH 20
#define SCREEN_HEIGHT 18
#define WELL_LEFT 2
#define WELL_TOP 2
#define HORIZONTAL_KEYS (J_LEFT | J_RIGHT)
#define TILE_ZERO (TILE_FONT + '0' - ' ')

typedef enum {
    MENU,
    HELP,
    PLAYING,
    PAUSED,
    GAME_OVER
} Screen;

typedef enum {
    NO_REDRAW,
    REDRAW_FRAME,
    REDRAW_TRIO
} Redraw;

static Game game;
static Screen screen = MENU;
static uint8_t selected = 1;
static uint16_t entropy = 0x39b5;
static uint8_t horizontal_key;
static uint8_t repeat_timer;
static uint8_t image[SCREEN_HEIGHT][SCREEN_WIDTH];
static uint8_t displayed[SCREEN_HEIGHT][SCREEN_WIDTH];
static uint8_t well_dirty[WELL_HEIGHT];
static uint8_t drawn_x;
static int8_t drawn_y;
static int8_t drawn_landing;

static void rectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t tile) {
    while (height--) {
        memset(&image[y++][x], tile, width);
    }
}

static void text(uint8_t x, uint8_t y, const char *message, uint8_t inverted) {
    while (*message) {
        uint8_t code = *message++ - ' ';
        image[y][x++] = inverted ? inverted_font[code] : TILE_FONT + code;
    }
}

static void number(uint8_t x, uint8_t y, uint8_t width, uint32_t value, uint8_t zero_tile) {
    static const uint32_t places[] = {1, 10, 100, 1000, 10000, 100000};
    uint8_t *output = &image[y][x];
    while (width) {
        uint32_t place = places[--width];
        uint8_t digit = zero_tile;
        while (value >= place) {
            value -= place;
            ++digit;
        }
        *output++ = digit;
    }
}

static void draw_menu(void) {
    static const char *const descriptions[] = {
        "5 GEMS / SLOW", "6 GEMS / MEDIUM", "6 GEMS / FAST"
    };
    uint8_t x;
    uint8_t y;
    memcpy(image[0], menu_header, sizeof(menu_header));
    for (y = 0; y < 5; ++y) {
        for (x = 0; x < 16; ++x) {
            image[2 + y][2 + x] = TILE_BANNER + y * 16 + x;
        }
    }
    for (x = 0; x < difficulties[selected].types; ++x) {
        image[7][4 + x * 2] = TILE_DIAMOND + x;
    }
    rectangle(3, 8, 14, 1, TILE_RULE);
    for (y = 0; y < DIFFICULTY_COUNT; ++y) {
        uint8_t current = y == selected;
        if (current) {
            rectangle(3, 9 + y, 14, 1, TILE_DARK);
            text(4, 9 + y, ">", 1);
        }
        text(6, 9 + y, difficulties[y].name, current);
    }
    text(3, 13, descriptions[selected], 0);
    memcpy(image[16], menu_footer, sizeof(menu_footer));
}

static void draw_help(void) {
    uint8_t i;
    memcpy(image[0], help_header, sizeof(help_header));
    text(1, 3, "MATCH 3 OR MORE", 0);
    text(1, 4, "OF THE SAME SHAPE", 0);
    text(1, 5, "ROWS/COLS/DIAGONALS", 0);
    for (i = 0; i < 3; ++i) {
        image[7][2 + i] = TILE_DIAMOND;
        image[6 + i][9] = TILE_RING;
        image[8 - i][14 + i] = TILE_CROSS;
    }
    rectangle(1, 9, 18, 1, TILE_RULE);
    text(1, 10, "LEFT/RIGHT  MOVE", 0);
    text(1, 11, "A/B   CYCLE TRIO", 0);
    text(1, 12, "DOWN  SOFT DROP", 0);
    text(1, 13, "UP    HARD DROP", 0);
    text(1, 14, "START PAUSE", 0);
    memcpy(image[16], help_footer, sizeof(help_footer));
}

static void mark_cell(uint8_t x, uint8_t y, uint8_t tile) {
    image[WELL_TOP + y][WELL_LEFT + x] = tile;
    well_dirty[y] = 1;
}

static void draw_trio(void) {
    uint8_t i;
    drawn_x = game.x;
    drawn_y = game.y;
    drawn_landing = game_landing_row(&game);
    for (i = 0; i < TRIO_SIZE; ++i) {
        int8_t ghost_row = drawn_landing + i;
        int8_t gem_row = drawn_y + i;
        if (ghost_row >= 0 && ghost_row >= drawn_y + TRIO_SIZE) {
            mark_cell(drawn_x, ghost_row, TILE_GHOST);
        }
        if (gem_row >= 0) {
            mark_cell(drawn_x, gem_row, game.piece[i]);
        }
    }
}

static void draw_well(void) {
    uint8_t x;
    uint8_t y;
    uint8_t flash = game.phase == FLASHING && (game.timer & 4);
    rectangle(0, WELL_TOP, 1, WELL_HEIGHT, TILE_TEXTURE);
    rectangle(1, WELL_TOP, 1, WELL_HEIGHT, TILE_RAIL_L);
    rectangle(9, WELL_TOP, 1, WELL_HEIGHT, TILE_RAIL_R);
    rectangle(10, WELL_TOP, 1, WELL_HEIGHT, TILE_PANEL);
    for (y = 0; y < WELL_HEIGHT; ++y) {
        uint8_t *row = &image[WELL_TOP + y][WELL_LEFT];
        memcpy(row, game.board[y], WELL_WIDTH);
        if (flash) {
            for (x = 0; x < WELL_WIDTH; ++x) {
                if (game.marked[y][x]) {
                    row[x] = TILE_FLASH;
                }
            }
        }
    }
    if (game.phase == FALLING) {
        draw_trio();
    }
}

static void draw_sidebar(void) {
    uint8_t i;
    for (i = 0; i < 5; ++i) {
        image[3][12 + i] = TILE_SCORE_LABEL + i;
    }
    number(12, 4, 6, game.score, TILE_SCORE_DIGITS);
    rectangle(11, 5, 9, 1, TILE_RULE);
    text(12, 7, "NEXT", 0);
    for (i = 0; i < TRIO_SIZE; ++i) {
        image[6 + i][17] = game.next[i];
    }
    rectangle(11, 9, 9, 1, TILE_RULE);
    text(12, 10, "LEVEL", 0);
    number(12, 11, 2, game.level, TILE_ZERO);
    rectangle(11, 12, 9, 1, TILE_RULE);
    if (game.chain > 1) {
        text(12, 13, "CHAIN", 0);
        text(12, 14, "X", 0);
        number(13, 14, 2, game.chain, TILE_ZERO);
    } else {
        text(12, 13, "CLEARED", 0);
        number(12, 14, 4, game.cleared, TILE_ZERO);
    }
}

static void draw_dialog(void) {
    rectangle(2, 5, 16, 8, TILE_BLANK);
    rectangle(2, 5, 16, 1, TILE_RIM);
    rectangle(2, 6, 1, 6, TILE_DIALOG_L);
    rectangle(17, 6, 1, 6, TILE_DIALOG_R);
    rectangle(2, 12, 16, 1, TILE_RIM);
    if (screen == PAUSED) {
        text(7, 7, "PAUSED", 0);
        text(4, 9, "START RESUME", 0);
        text(4, 10, "SELECT MENU", 0);
    } else {
        text(6, 6, "GAME OVER", 0);
        text(4, 8, "SCORE", 0);
        number(10, 8, 6, game.score, TILE_ZERO);
        text(4, 10, "A RETRY", 0);
        text(4, 11, "B MENU", 0);
    }
}

static void draw_game(void) {
    uint8_t difficulty_tile = TILE_DIFFICULTY_LABELS + selected * 4;
    uint8_t x;
    memcpy(image[0], game_header, sizeof(game_header));
    for (x = 15; x < 19; ++x) {
        image[0][x] = difficulty_tile++;
    }
    memcpy(image[16], game_footer, sizeof(game_footer));
    draw_well();
    draw_sidebar();
    if (screen == PAUSED || screen == GAME_OVER) {
        draw_dialog();
    }
}

static void draw(void) {
    memset(image, TILE_BLANK, sizeof(image));
    if (screen == MENU) {
        draw_menu();
    } else if (screen == HELP) {
        draw_help();
    } else {
        draw_game();
    }
}

static void present_row(uint8_t y) {
    uint8_t left = 0;
    uint8_t right = SCREEN_WIDTH - 1;
    if (memcmp(image[y], displayed[y], SCREEN_WIDTH) == 0) {
        return;
    }
    while (image[y][left] == displayed[y][left]) {
        ++left;
    }
    while (image[y][right] == displayed[y][right]) {
        --right;
    }
    set_bkg_tiles(left, y, right - left + 1, 1, &image[y][left]);
    memcpy(displayed[y], image[y], SCREEN_WIDTH);
}

static void present(Redraw mode, uint8_t transition) {
    uint8_t y;
    if (mode == REDRAW_TRIO) {
        memset(well_dirty, 0, sizeof(well_dirty));
        for (y = 0; y < TRIO_SIZE; ++y) {
            int8_t row = drawn_y + y;
            int8_t ghost = drawn_landing + y;
            if (row >= 0) {
                mark_cell(drawn_x, row, TILE_EMPTY);
            }
            if (ghost >= 0) {
                mark_cell(drawn_x, ghost, TILE_EMPTY);
            }
        }
        draw_trio();
        for (y = 0; y < WELL_HEIGHT; ++y) {
            if (well_dirty[y]) {
                present_row(WELL_TOP + y);
            }
        }
        return;
    }
    draw();
    if (transition) {
        DISPLAY_OFF;
        set_bkg_tiles(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, &image[0][0]);
        memcpy(displayed, image, sizeof(image));
        DISPLAY_ON;
        return;
    }
    for (y = 0; y < SCREEN_HEIGHT; ++y) {
        present_row(y);
    }
}

static void sound(uint16_t pitch, uint8_t length, uint8_t volume) {
    NR11_REG = 0x80 | (64 - length);
    NR12_REG = (volume << 4) | 2;
    NR13_REG = (uint8_t)pitch;
    NR14_REG = 0xc0 | (pitch >> 8);
}

static void start_game(void) {
    game_start(&game, selected, entropy);
    horizontal_key = 0;
    repeat_timer = 0;
    screen = PLAYING;
    sound(1673, 12, 6);
}

static Redraw update_menu(uint8_t pressed) {
    uint8_t before = selected;
    if (pressed & (J_UP | J_LEFT)) {
        selected = selected == 0 ? DIFFICULTY_COUNT - 1 : selected - 1;
    } else if (pressed & (J_DOWN | J_RIGHT | J_SELECT)) {
        selected = selected == DIFFICULTY_COUNT - 1 ? 0 : selected + 1;
    }
    if (pressed & (J_A | J_START)) {
        start_game();
    } else if (pressed & J_B) {
        screen = HELP;
        sound(1547, 5, 4);
    } else if (before != selected) {
        sound(1673, 4, 4);
    }
    return before != selected ? REDRAW_FRAME : NO_REDRAW;
}

static void handle_horizontal(uint8_t held, uint8_t pressed) {
    uint8_t direction = pressed & HORIZONTAL_KEYS;
    if (!direction && !(held & horizontal_key)) {
        direction = held & HORIZONTAL_KEYS;
        horizontal_key = 0;
    }
    if (direction) {
        horizontal_key = (direction & J_LEFT) ? J_LEFT : J_RIGHT;
        repeat_timer = 12;
    } else if (horizontal_key) {
        if (--repeat_timer == 0) {
            direction = horizontal_key;
            repeat_timer = 4;
        }
    }
    if (direction) {
        game_move(&game, horizontal_key == J_LEFT ? -1 : 1);
    }
}

static Redraw update_game(uint8_t held, uint8_t pressed) {
    Phase old_phase = game.phase;
    int8_t old_x = game.x;
    int8_t old_y = game.y;
    uint8_t old_timer = game.timer;
    uint8_t cycled = 0;
    if (pressed & J_START) {
        screen = PAUSED;
        sound(1515, 8, 5);
        return REDRAW_FRAME;
    }
    if (game.phase == FALLING) {
        handle_horizontal(held, pressed);
        if (pressed & (J_A | J_B)) {
            game_cycle(&game, (pressed & J_B) != 0 && (pressed & J_A) == 0);
            sound(1750, 5, 4);
            cycled = 1;
        }
        if (pressed & J_UP) {
            game_drop(&game);
            sound(1000, 8, 5);
        } else {
            game_tick(&game, held & J_DOWN, pressed & J_DOWN);
        }
    } else {
        game_tick(&game, 0, 0);
    }
    if (game.phase == TOPPED_OUT) {
        screen = GAME_OVER;
        sound(1220, 24, 7);
        return REDRAW_FRAME;
    }
    if (game.phase != old_phase) {
        horizontal_key = 0;
        repeat_timer = 0;
        if (game.phase == FLASHING) {
            uint8_t pitch_step = game.chain > 7 ? 7 : game.chain;
            sound(1490 + 55 * pitch_step, 18, 9);
        }
        return REDRAW_FRAME;
    }
    if ((pressed & J_UP) || game.y < old_y ||
        (game.phase == FLASHING && ((old_timer ^ game.timer) & 4)) ||
        (game.phase == CASCADING && old_timer == 1)) {
        return REDRAW_FRAME;
    }
    if (cycled || game.x != old_x || game.y != old_y) {
        return REDRAW_TRIO;
    }
    return NO_REDRAW;
}

static Redraw update(uint8_t held, uint8_t pressed) {
    switch (screen) {
    case MENU:
        return update_menu(pressed);
    case HELP:
        if (pressed & (J_A | J_B | J_START)) {
            screen = MENU;
        }
        break;
    case PLAYING:
        return update_game(held, pressed);
    case PAUSED:
        if (pressed & J_START) {
            horizontal_key = 0;
            screen = PLAYING;
            sound(1710, 7, 4);
        } else if (pressed & J_SELECT) {
            screen = MENU;
        }
        break;
    case GAME_OVER:
        if (pressed & (J_A | J_START)) {
            start_game();
        } else if (pressed & (J_B | J_SELECT)) {
            screen = MENU;
        }
        break;
    }
    return NO_REDRAW;
}

void main(void) {
    uint8_t previous = 0;
    DISPLAY_OFF;
    SHOW_BKG;
    HIDE_SPRITES;
    HIDE_WIN;
    BGP_REG = 0xe4;
    set_bkg_data(0, TILE_COUNT, tile_data);
    NR52_REG = 0x80;
    NR50_REG = 0x55;
    NR51_REG = 0x11;
    NR10_REG = 0;
    present(REDRAW_FRAME, 1);

    while (1) {
        uint8_t held;
        Redraw redraw;
        Screen old_screen = screen;
        vsync();
        held = joypad();
        entropy = entropy * 109u + 89u + DIV_REG + held;
        redraw = update(held, held & (uint8_t)~previous);
        previous = held;
        if (screen != old_screen) {
            present(REDRAW_FRAME, 1);
        } else if (redraw != NO_REDRAW) {
            present(redraw, 0);
        }
    }
}
