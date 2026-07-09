#include "state_game.h"
#include "common.h"
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
static uint8_t picked_cell_type, picked_cell_rot;
static uint32_t last_cursor_time;
static bool is_cell_picker_open;

static void drawCellNoClip(uint8_t cell, int x, int y)
{
    const uint8_t cell_type = cell & 0xF;
    const uint8_t cell_rot = (cell >> 4) & 0xF;
    gfx_Sprite_NoClip(cell_sprites[cell_type][cell_rot], x, y);
}

static void init(void)
{
    cursor_x = 0;
    cursor_y = 0;
    picked_cell_type = 1;
    picked_cell_rot = 0;

    is_cell_picker_open = false;

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

    if (is_cell_picker_open)
    {
        if (isKeyPressed(kb_KeyLeft) && picked_cell_type > 1)
            picked_cell_type--;
        if (isKeyPressed(kb_KeyRight) && picked_cell_type < cells_0_num_tiles - 1)
            picked_cell_type++;
        if (isKeyPressed(kb_KeyDown))
            picked_cell_rot = (picked_cell_rot + 1) & 3;
        if (isKeyPressed(kb_KeyUp))
            picked_cell_rot = (picked_cell_rot - 1) & 3;
    }
    else
    {
        if (isKeyHeld(kb_Key2nd))
            grid[cursor_y][cursor_x] = picked_cell_type | (picked_cell_rot << 4);
        if (isKeyHeld(kb_KeyAlpha))
            grid[cursor_y][cursor_x] = CELL_EMPTY;
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
    }

    if (isKeyPressed(kb_KeyClear))
        enterState(NULL);
    if (isKeyPressed(kb_KeyMode))
        is_cell_picker_open = !is_cell_picker_open;
}

static void draw(void)
{
    // draw grid
    for (uint8_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < GRID_WIDTH; x++)
        {
            drawCellNoClip(grid[y][x], x * 16, y * 16);
        }
    }

    // draw cursor
    gfx_SetColor(1);
    gfx_Rectangle(cursor_x * 16, cursor_y * 16, 16, 16);

    // draw cell picker
    if (is_cell_picker_open)
    {
        gfx_SetColor(1);
        gfx_FillRectangle_NoClip(0, 208, GFX_LCD_WIDTH, 32);
        for (uint8_t i = 0; i < cells_0_num_tiles - 1; i++)
        {
            // lower 4 bytes are type, higher 4 are rotation
            // x and y values are so to center the picker lol
            drawCellNoClip(((i + 1) | (picked_cell_rot << 4)), 36 * i + 8, 216);
        }
        gfx_SetColor(0);
        gfx_Rectangle_NoClip(36 * (picked_cell_type - 1) + 4, 212, 24, 24);
    }

    gfx_SwapDraw();
}

static void quit(void)
{
    timer_Disable(1);
}

const game_state_t STATE_GAME = {init, step, draw, quit};