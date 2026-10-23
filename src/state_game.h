#ifndef STATE_GAME_H
#define STATE_GAME_H

#include "common.h"
#include "states.h"
extern const game_state_t STATE_GAME;

#define GRID_WIDTH 20
#define GRID_HEIGHT 15

#define GET_CELL_TYPE(cell) ((cell) & 0xF)
#define GET_CELL_UPDATED(cell) ((cell) & 0x80)
#define GET_CELL_ROT(cell) (((cell) >> 4) & 3)
#define SET_UPDATED(cell) ((cell) |= 0x80)
#define CLEAR_UPDATED(cell) ((cell) &= 0x7F)
#define MAKE_CELL(type, rot) (type | (rot << 4))

void load_save_from_appvar(void);
extern uint8_t grid[GRID_HEIGHT][GRID_WIDTH];
extern char save_var_name[9];

#endif