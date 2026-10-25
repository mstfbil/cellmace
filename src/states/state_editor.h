#ifndef STATE_EDITOR_H
#define STATE_EDITOR_H

#include "utils/states.h"
extern const game_state_t STATE_EDITOR;

#define GET_CELL_TYPE(cell) ((cell) & 0xF)
#define GET_CELL_UPDATED(cell) ((cell) & 0x80)
#define GET_CELL_ROT(cell) (((cell) >> 4) & 3)
#define SET_UPDATED(cell) ((cell) |= 0x80)
#define CLEAR_UPDATED(cell) ((cell) &= 0x7F)
#define MAKE_CELL(type, rot) (type | (rot << 4))

#endif