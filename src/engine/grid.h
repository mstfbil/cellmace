#ifndef __GRID_H__
#define __GRID_H__

#include <stdbool.h>
#include <stdint.h>

#define GRID_WIDTH 20
#define GRID_HEIGHT 15

#define CELL_TYPE_MASK 0xF
#define CELL_ROT_MASK 0x30
#define CELL_UPDATED_MASK 0x80

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

bool isInGridBounds(int x, int y);
void saveGridToInitial(void);
void resetGridToInitial(void);

#endif // __GRID_H__
