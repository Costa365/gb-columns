#include "game.h"

#include <string.h>

#define FLASH_FRAMES 18
#define CASCADE_FRAMES 3
#define LOCK_FRAMES 22
#define LEVEL_SPAN 18

const Difficulty difficulties[DIFFICULTY_COUNT] = {
    {"EASY",   5, 58, 18},
    {"NORMAL", 6, 38, 12},
    {"HARD",   6, 22,  6}
};

static const int8_t directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {-1, 1}};

static uint16_t random_word(Game *game) {
    uint16_t value = game->random;
    value ^= value << 7;
    value ^= value >> 9;
    value ^= value << 8;
    game->random = value;
    return value;
}

static uint8_t draw_gem(Game *game) {
    uint8_t i;
    uint8_t j;
    uint8_t gem;
    if (!game->bag_left) {
        uint8_t types = difficulties[game->difficulty].types;
        game->bag_left = types * 2;
        for (i = 0; i < types; ++i) {
            game->bag[i] = i + 1;
            game->bag[i + types] = i + 1;
        }
        for (i = game->bag_left - 1; i > 0; --i) {
            j = (uint8_t)random_word(game) % (i + 1);
            gem = game->bag[i];
            game->bag[i] = game->bag[j];
            game->bag[j] = gem;
        }
    }
    return game->bag[--game->bag_left];
}

uint8_t game_fits(const Game *game, int8_t x, int8_t y) {
    int8_t bottom = y + TRIO_SIZE - 1;
    if (x < 0 || x >= WELL_WIDTH || bottom >= WELL_HEIGHT) {
        return 0;
    }
    for (; y <= bottom; ++y) {
        if (y >= 0 && game->board[y][x]) {
            return 0;
        }
    }
    return 1;
}

static void spawn(Game *game) {
    uint8_t i;
    for (i = 0; i < TRIO_SIZE; ++i) {
        game->piece[i] = game->next[i];
        game->next[i] = draw_gem(game);
    }
    game->x = WELL_WIDTH / 2;
    game->y = 1 - TRIO_SIZE;
    game->timer = 0;
    game->lock_timer = 0;
    game->phase = game_fits(game, game->x, game->y) ? FALLING : TOPPED_OUT;
}

void game_start(Game *game, uint8_t difficulty, uint16_t seed) {
    uint8_t i;
    memset(game, 0, sizeof(*game));
    game->random = seed ? seed : 1;
    game->difficulty = difficulty;
    game->interval = difficulties[difficulty].starting_interval;
    game->level = 1;
    game->next_level = LEVEL_SPAN;
    for (i = 0; i < TRIO_SIZE; ++i) {
        game->next[i] = draw_gem(game);
    }
    spawn(game);
}

uint8_t game_move(Game *game, int8_t dx) {
    if (!game_fits(game, game->x + dx, game->y)) {
        return 0;
    }
    game->x += dx;
    return 1;
}

void game_cycle(Game *game, uint8_t reverse) {
    uint8_t first = game->piece[0];
    uint8_t last = game->piece[2];
    if (reverse) {
        game->piece[0] = game->piece[1];
        game->piece[1] = last;
        game->piece[2] = first;
    } else {
        game->piece[2] = game->piece[1];
        game->piece[1] = first;
        game->piece[0] = last;
    }
}

int8_t game_landing_row(const Game *game) {
    int8_t row = game->y + TRIO_SIZE;
    while (row < WELL_HEIGHT && !game->board[row][game->x]) {
        ++row;
    }
    return row - TRIO_SIZE;
}

static uint8_t in_well(int8_t x, int8_t y) {
    return x >= 0 && x < WELL_WIDTH && y >= 0 && y < WELL_HEIGHT;
}

static uint8_t find_matches(Game *game) {
    int8_t x;
    int8_t y;
    uint8_t count = 0;
    memset(game->marked, 0, sizeof(game->marked));
    for (y = 0; y < WELL_HEIGHT; ++y) {
        for (x = 0; x < WELL_WIDTH; ++x) {
            uint8_t gem = game->board[y][x];
            uint8_t direction;
            if (!gem) {
                continue;
            }
            for (direction = 0; direction < 4; ++direction) {
                int8_t dx = directions[direction][0];
                int8_t dy = directions[direction][1];
                int8_t end_x = x + 2 * dx;
                int8_t end_y = y + 2 * dy;
                int8_t scan_x = x;
                int8_t scan_y = y;
                if (!in_well(end_x, end_y) || game->board[end_y][end_x] != gem ||
                    game->board[y + dy][x + dx] != gem) {
                    continue;
                }
                if (in_well(x - dx, y - dy) && game->board[y - dy][x - dx] == gem) {
                    continue;
                }
                do {
                    if (!game->marked[scan_y][scan_x]) {
                        game->marked[scan_y][scan_x] = 1;
                        ++count;
                    }
                    scan_x += dx;
                    scan_y += dy;
                } while (in_well(scan_x, scan_y) && game->board[scan_y][scan_x] == gem);
            }
        }
    }
    return count;
}

static void settle(Game *game) {
    uint8_t count = find_matches(game);
    if (!count) {
        spawn(game);
        return;
    }

    ++game->chain;
    game->score += 10ul * count * game->chain * (game->difficulty + 1);
    if (game->score > 999999ul) {
        game->score = 999999ul;
    }
    if (game->cleared <= 9999u - count) {
        game->cleared += count;
    } else {
        game->cleared = 9999u;
    }
    while (game->level < 99 && game->cleared >= game->next_level) {
        ++game->level;
        game->next_level += LEVEL_SPAN;
        if (game->interval > difficulties[game->difficulty].minimum_interval) {
            game->interval -= 2;
        }
    }
    game->phase = FLASHING;
    game->timer = FLASH_FRAMES;
}

static void lock_piece(Game *game) {
    uint8_t i;
    if (game->y < 0) {
        game->phase = TOPPED_OUT;
        return;
    }
    for (i = 0; i < TRIO_SIZE; ++i) {
        game->board[game->y + i][game->x] = game->piece[i];
    }
    game->chain = 0;
    settle(game);
}

void game_drop(Game *game) {
    game->y = game_landing_row(game);
    lock_piece(game);
}

static void clear_matches(Game *game) {
    uint8_t x;
    uint8_t y;
    for (y = 0; y < WELL_HEIGHT; ++y) {
        for (x = 0; x < WELL_WIDTH; ++x) {
            if (game->marked[y][x]) {
                game->board[y][x] = 0;
            }
        }
    }
    game->phase = CASCADING;
    game->timer = CASCADE_FRAMES;
}

static void cascade(Game *game) {
    uint8_t moved = 0;
    int8_t y;
    uint8_t x;
    for (y = WELL_HEIGHT - 1; y > 0; --y) {
        for (x = 0; x < WELL_WIDTH; ++x) {
            if (!game->board[y][x] && game->board[y - 1][x]) {
                game->board[y][x] = game->board[y - 1][x];
                game->board[y - 1][x] = 0;
                moved = 1;
            }
        }
    }
    if (!moved) {
        settle(game);
    } else {
        game->timer = CASCADE_FRAMES;
    }
}

void game_tick(Game *game, uint8_t soft_drop, uint8_t down_pressed) {
    switch (game->phase) {
    case FALLING: {
        int8_t below = game->y + TRIO_SIZE;
        if (below == WELL_HEIGHT || game->board[below][game->x]) {
            if (++game->lock_timer >= (soft_drop ? 5 : LOCK_FRAMES)) {
                lock_piece(game);
            }
            return;
        }
        game->lock_timer = 0;
        if (++game->timer >= (soft_drop ? 2 : game->interval) || down_pressed) {
            ++game->y;
            game->timer = 0;
        }
        break;
    }
    case FLASHING:
        if (--game->timer == 0) {
            clear_matches(game);
        }
        break;
    case CASCADING:
        if (--game->timer == 0) {
            cascade(game);
        }
        break;
    case TOPPED_OUT:
        break;
    }
}
