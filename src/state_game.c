#include "state_game.h"
#include "input.h"
#include <sys/timers.h>

#define GRID_WIDTH 20
#define GRID_HEIGHT 15
#define CURSOR_DELAY (32768 / 8)

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
static uint8_t cursor_x, cursor_y;
static uint32_t last_cursor_time;

static void init(void)
{
    cursor_x = 0;
    cursor_y = 0;

    for (uint8_t y = 0; y < GRID_HEIGHT; y++)
        for (uint8_t x = 0; x < GRID_WIDTH; x++)
            grid[y][x] = 0;

    timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP);
    uint32_t current_time = timer_Get(1);
    last_cursor_time = current_time - CURSOR_DELAY;
}

static void step(void)
{
    uint32_t current_time = timer_Get(1);

    if (current_time - last_cursor_time >= CURSOR_DELAY)
    {
        bool moved = false;
        if (isKeyHeld(kb_KeyLeft) && cursor_x > 0)
        {
            cursor_x--;
            moved = true;
        }
        if (isKeyHeld(kb_KeyRight) && cursor_x < GRID_WIDTH - 1)
        {
            cursor_x++;
            moved = true;
        }
        if (isKeyHeld(kb_KeyUp) && cursor_y > 0)
        {
            cursor_y--;
            moved = true;
        }
        if (isKeyHeld(kb_KeyDown) && cursor_y < GRID_HEIGHT - 1)
        {
            cursor_y++;
            moved = true;
        }
        if (moved)
            last_cursor_time = current_time;
    }

    if (isKeyPressed(kb_KeyClear))
    {
        enterState(NULL);
    }
}

static void draw(void)
{
    // draw grid
    for (uint8_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < GRID_WIDTH; x++)
        {
            gfx_Sprite_NoClip(cells_tiles[grid[y][x]], x * 16, y * 16);
        }
    }

    // draw cursor
    gfx_SetColor(1);
    gfx_Rectangle(cursor_x * 16, cursor_y * 16, 16, 16);

    gfx_SwapDraw();
}

static void quit(void)
{
    timer_Disable(1);
}

const game_state_t STATE_GAME = {init, step, draw, quit};