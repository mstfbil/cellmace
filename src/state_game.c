#include "state_game.h"
#include "input.h"

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

static uint8_t grid[GRID_HEIGHT][GRID_WIDTH];

static void init(void)
{
    grid[3][3] = CELL_ENEMY;
}

static void step(void)
{
    if (isKeyPressed(kb_KeyClear))
    {
        enterState(NULL);
    }
}

static void draw(void)
{
    for (uint8_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < GRID_WIDTH; x++)
        {
            gfx_Sprite_NoClip(cells_tiles[grid[y][x]], x * 16, y * 16);
        }
    }
    gfx_SwapDraw();
}

const game_state_t STATE_GAME = {init, step, draw};