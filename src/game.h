#ifndef COLUMNS_GAME_H
#define COLUMNS_GAME_H

#include <stdint.h>

#define WELL_WIDTH 7
#define WELL_HEIGHT 14
#define TRIO_SIZE 3
#define GEM_COUNT 6
#define DIFFICULTY_COUNT 3

typedef enum {
    FALLING,
    FLASHING,
    CASCADING,
    TOPPED_OUT
} Phase;

typedef struct {
    const char *name;
    uint8_t types;
    uint8_t starting_interval;
    uint8_t minimum_interval;
} Difficulty;

typedef struct {
    uint8_t board[WELL_HEIGHT][WELL_WIDTH];
    uint8_t marked[WELL_HEIGHT][WELL_WIDTH];
    uint8_t piece[TRIO_SIZE];
    uint8_t next[TRIO_SIZE];
    uint8_t bag[2 * GEM_COUNT];
    uint8_t bag_left;
    uint8_t difficulty;
    uint8_t level;
    uint8_t chain;
    uint8_t interval;
    uint8_t timer;
    uint8_t lock_timer;
    int8_t x;
    int8_t y;
    Phase phase;
    uint16_t random;
    uint16_t cleared;
    uint16_t next_level;
    uint32_t score;
} Game;

extern const Difficulty difficulties[DIFFICULTY_COUNT];

void game_start(Game *game, uint8_t difficulty, uint16_t seed);
uint8_t game_fits(const Game *game, int8_t x, int8_t y);
uint8_t game_move(Game *game, int8_t dx);
void game_cycle(Game *game, uint8_t reverse);
int8_t game_landing_row(const Game *game);
void game_drop(Game *game);
void game_tick(Game *game, uint8_t soft_drop, uint8_t down_pressed);

#endif
