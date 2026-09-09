#ifndef BLOCKS_H
#define BLOCKS_H

#include "../game.h"
#include "../prng.h"

enum { BLK_COLS = 10, BLK_ROWS = 20, BLK_CELL = 10, BLK_WELL_X0 = 110, BLK_WELL_Y0 = 24,
       BLK_DAS_DELAY = 16, BLK_DAS_REPEAT = 6, BLK_SOFT_DROP_TICKS = 2, BLK_FLASH_TICKS = 20 };
typedef enum { BLK_I = 0, BLK_O, BLK_T, BLK_S, BLK_Z, BLK_J, BLK_L, BLK_PIECE_COUNT } blk_piece_t;
typedef enum { BLK_ST_TITLE, BLK_ST_PLAY, BLK_ST_FLASH, BLK_ST_GAMEOVER } blk_state_t;
typedef struct {
    blk_state_t state;
    uint8_t cells[BLK_ROWS][BLK_COLS];      /* 0 empty, else piece + 1 (for color) */
    blk_piece_t piece, next;
    int rot, px, py;                        /* current piece rotation and 4x4 box origin (px column, py row) */
    uint8_t bag[BLK_PIECE_COUNT]; int bag_left;
    int score, lines, level, high_score;
    int gravity_ticks;                      /* ticks until the next gravity step */
    int das_dir, das_ticks;                 /* horizontal auto-repeat state */
    int soft_ticks;
    int flash_ticks; uint8_t flash_rows[4]; int flash_count;
    prng_t rng;
    sfx_queue_t sfx;
    uint32_t ticks;
    int drop_armed;                         /* hard drop edge: re-arm when dpad_y != +1 and stick_y < 64 */
} blocks_t;

extern const int8_t BLK_SHAPES[BLK_PIECE_COUNT][4][4][2];   /* [piece][rotation][cell][x,y] within a 4x4 box, y down */
extern const uint8_t BLK_GRAVITY[20];                         /* ticks per row by level-1; level 20+ uses index 19 */
uint32_t blocks_piece_color(blk_piece_t p);
bool blocks_fits(const blocks_t *g, blk_piece_t p, int rot, int px, int py);
blk_piece_t blocks_bag_next(blocks_t *g);                     /* 7-bag draw (tests) */
void blocks_spawn(blocks_t *g);                               /* tests: force a spawn of g->next */
void blocks_init(blocks_t *g); void blocks_start(blocks_t *g, uint32_t seed);
void blocks_update(blocks_t *g, const input_t in[GAME_MAX_PLAYERS]);
void blocks_render(const blocks_t *g, const draw_t *d);
void blocks_autoplay(const blocks_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_BLOCKS;

#endif
