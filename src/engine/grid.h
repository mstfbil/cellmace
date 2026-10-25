#ifndef __GRID_H__
#define __GRID_H__

#include <stdint.h>

#define GRID_WIDTH 20
#define GRID_HEIGHT 15

enum CellType
{
    CELL_EMPTY,
    CELL_WALL,
    CELL_PUSH,
    CELL_SLIDE,
    CELL_MOVER,
    CELL_CW_ROTATOR,
    CELL_CCW_ROTATOR,
    CELL_GENERATOR,
    CELL_ENEMY,
    CELL_TRASH
};

enum CellRot
{
    ROT_RIGHT,
    ROT_DOWN,
    ROT_LEFT,
    ROT_UP,
    ROT_COUNT
};

typedef uint8_t game_grid_t[GRID_HEIGHT][GRID_WIDTH];

extern game_grid_t grid;

#endif // __GRID_H__
