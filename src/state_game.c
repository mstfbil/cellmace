#include "state_game.h"
#include "state_menu.h"
#include "common.h"
#include "input.h"
#include <sys/timers.h>

#define GRID_WIDTH 20
#define GRID_HEIGHT 15
#define CURSOR_DELAY (32768 / 10)
#define TICK_DELAY (32768 / 5)

#define GET_CELL_TYPE(cell) ((cell) & 0xF)
#define GET_CELL_ROT(cell) (((cell) >> 4) & 3)
#define SET_UPDATED(cell) ((cell) |= 0x80)
#define CLEAR_UPDATED(cell) ((cell) &= 0x7F)
#define IS_UPDATED(cell) ((cell) & 0x80)

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
    ROT_UP
};

static uint8_t grid[GRID_HEIGHT][GRID_WIDTH];
static uint8_t grid_initial[GRID_HEIGHT][GRID_WIDTH];
static int cursor_x, cursor_y;
static uint8_t picked_cell_type, picked_cell_rot;
static uint32_t last_cursor_time, last_tick_time;
static bool is_cell_picker_open, is_playing, is_reset;
static const int8_t DIR_X[] = {1, 0, -1, 0};
static const int8_t DIR_Y[] = {0, 1, 0, -1};

// static uint8_t UPDATE_ROT_ORDER[] = {ROT_RIGHT, ROT_LEFT, ROT_UP, ROT_DOWN};
// WILL I EVER BE ABLE TO USE THIS STUPID THING?!

static void drawCellNoClip(uint8_t cell, int x, int y)
{
    gfx_Sprite_NoClip(cell_sprites[GET_CELL_TYPE(cell)][GET_CELL_ROT(cell)], x, y);
}

static bool isInBounds(int x, int y)
{
    return x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT;
}

static bool pushCellRecursive(int x, int y, uint8_t dir)
{
    if (!isInBounds(x, y))
        return false;

    uint8_t cell = grid[y][x];
    uint8_t type = GET_CELL_TYPE(cell);

    if (type == CELL_EMPTY)
        return true;
    else if (type == CELL_WALL)
        return false;
    else if (type == CELL_SLIDE)
    {
        uint8_t rot = GET_CELL_ROT(cell);
        if ((rot == ROT_LEFT || rot == ROT_RIGHT) != (dir == ROT_LEFT || dir == ROT_RIGHT))
            return false;
    }
    else if (type == CELL_ENEMY)
    {
        grid[y][x] = CELL_EMPTY;
        return false;
    }

    int next_x = x + DIR_X[dir];
    int next_y = y + DIR_Y[dir];
    if (!isInBounds(next_x, next_y))
        return false;

    uint8_t target_cell = grid[next_y][next_x];
    uint8_t target_type = GET_CELL_TYPE(target_cell);

    if (target_type == CELL_TRASH)
    {
        grid[y][x] = CELL_EMPTY;
        return true;
    }
    if (target_type == CELL_ENEMY)
    {
        grid[next_y][next_x] = CELL_EMPTY;
        grid[y][x] = CELL_EMPTY;
        return true;
    }

    if (pushCellRecursive(next_x, next_y, dir))
    {
        grid[next_y][next_x] = cell;
        grid[y][x] = CELL_EMPTY;
        return true;
    }

    return false;
}

static void doGenerator(int x, int y, uint8_t rot)
{
    uint8_t cell = grid[y][x];

    if (!IS_UPDATED(cell) && GET_CELL_TYPE(cell) == CELL_GENERATOR && GET_CELL_ROT(cell) == rot)
    {
        int back_x = x - DIR_X[rot];
        int back_y = y - DIR_Y[rot];

        if (isInBounds(back_x, back_y))
        {
            uint8_t source = grid[back_y][back_x];
            if (GET_CELL_TYPE(source) != CELL_EMPTY)
            {
                int front_x = x + DIR_X[rot];
                int front_y = y + DIR_Y[rot];

                if (pushCellRecursive(front_x, front_y, rot))
                {
                    grid[front_y][front_x] = source & 0x7F;
                }
                SET_UPDATED(grid[y][x]);
            }
        }
    }
}

static void doRotator(int x, int y, uint8_t type, int8_t rot_dir)
{
    uint8_t cell = grid[y][x];

    if (!IS_UPDATED(cell) && GET_CELL_TYPE(cell) == type)
    {
        for (int i = 0; i < 4; i++)
        {
            int target_x = x + DIR_X[i];
            int target_y = y + DIR_Y[i];

            if (isInBounds(target_x, target_y))
            {
                uint8_t neighbor = grid[target_y][target_x];
                uint8_t neighbor_type = GET_CELL_TYPE(neighbor);

                if (neighbor_type != CELL_EMPTY)
                {
                    uint8_t neighbor_rot = GET_CELL_ROT(neighbor);
                    uint8_t new_rot = (neighbor_rot + rot_dir) & 3;

                    grid[target_y][target_x] = (neighbor & 0xCF) | (new_rot << 4);
                }
            }
        }

        SET_UPDATED(grid[y][x]);
    }
}

static void doMover(int x, int y, uint8_t rot)
{
    uint8_t cell = grid[y][x];

    if (!IS_UPDATED(cell) && GET_CELL_TYPE(cell) == CELL_MOVER && GET_CELL_ROT(cell) == rot)
    {
        if (pushCellRecursive(x, y, rot))
        {
            SET_UPDATED(grid[y + DIR_Y[rot]][x + DIR_X[rot]]);
        }
    }
}

static void tick(void)
{
    // very crude and repetitive implementation, will improve later

    // GENERATORS
    // right
    for (int x = GRID_WIDTH - 1; x >= 0; x--)
        for (int y = GRID_HEIGHT - 1; y >= 0; y--)
            doGenerator(x, y, ROT_RIGHT);
    // left
    for (int x = 0; x < GRID_WIDTH; x++)
        for (int y = 0; y < GRID_HEIGHT; y++)
            doGenerator(x, y, ROT_LEFT);
    // up
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            doGenerator(x, y, ROT_UP);
    // down
    for (int y = GRID_HEIGHT - 1; y >= 0; y--)
        for (int x = GRID_WIDTH - 1; x >= 0; x--)
            doGenerator(x, y, ROT_DOWN);

    // ROTATORS
    // cw
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            doRotator(x, y, CELL_CW_ROTATOR, 1);
    // ccw
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            doRotator(x, y, CELL_CCW_ROTATOR, -1);

    // MOVERS
    // right
    for (int x = GRID_WIDTH - 1; x >= 0; x--)
        for (int y = GRID_HEIGHT - 1; y >= 0; y--)
            doMover(x, y, ROT_RIGHT);
    // left
    for (int x = 0; x < GRID_WIDTH; x++)
        for (int y = 0; y < GRID_HEIGHT; y++)
            doMover(x, y, ROT_LEFT);
    // up
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            doMover(x, y, ROT_UP);
    // down
    for (int y = GRID_HEIGHT - 1; y >= 0; y--)
        for (int x = GRID_WIDTH - 1; x >= 0; x--)
            doMover(x, y, ROT_DOWN);

    // CLEANUP OF UPDATED FLAG
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            CLEAR_UPDATED(grid[y][x]);
}

static void init(void)
{
    cursor_x = 0;
    cursor_y = 0;
    picked_cell_type = 1;
    picked_cell_rot = 0;

    is_cell_picker_open = false;
    is_playing = false;
    is_reset = true;

    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            grid[y][x] = CELL_EMPTY;

    timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP);
    uint32_t current_time = timer_Get(1);
    last_cursor_time = current_time - CURSOR_DELAY;
    last_tick_time = current_time;
}

static void step(void)
{
    uint32_t current_time = timer_Get(1);

    if (is_playing)
    {
        if (current_time - last_tick_time >= TICK_DELAY)
        {
            tick();
            last_tick_time = current_time;
        }
    }

    if (is_cell_picker_open)
    {
        if (isKeyPressed(kb_KeyLeft))
        {
            picked_cell_type--;
            if (picked_cell_type < 1)
                picked_cell_type = cells_0_num_tiles - 1;
        }
        if (isKeyPressed(kb_KeyRight))
        {
            picked_cell_type++;
            if (picked_cell_type > cells_0_num_tiles - 1)
                picked_cell_type = 1;
        }
        if (isKeyPressed(kb_KeyDown))
            picked_cell_rot = (picked_cell_rot + 1) & 3;
        if (isKeyPressed(kb_KeyUp))
            picked_cell_rot = (picked_cell_rot - 1) & 3;
    }
    else
    {
        if (!is_playing)
        {
            if (isKeyHeld(kb_Key2nd))
                grid[cursor_y][cursor_x] = picked_cell_type | (picked_cell_rot << 4);
            if (isKeyPressed(kb_KeyZoom))
            {
                if (is_reset)
                    for (int y = 0; y < GRID_HEIGHT; y++)
                        for (int x = 0; x < GRID_WIDTH; x++)
                            grid_initial[y][x] = grid[y][x];
                is_reset = false;
                tick();
            }
            if (isKeyHeld(kb_KeyAlpha))
                grid[cursor_y][cursor_x] = CELL_EMPTY;
            if (current_time - last_cursor_time >= CURSOR_DELAY)
            {
                bool moved = false;
                if (isKeyHeld(kb_KeyLeft))
                {
                    cursor_x--;
                    if (cursor_x < 0)
                        cursor_x = GRID_WIDTH - 1;
                    moved = true;
                }
                if (isKeyHeld(kb_KeyRight))
                {
                    cursor_x++;
                    if (cursor_x > GRID_WIDTH - 1)
                        cursor_x = 0;
                    moved = true;
                }
                if (isKeyHeld(kb_KeyUp))
                {
                    cursor_y--;
                    if (cursor_y < 0)
                        cursor_y = GRID_HEIGHT - 1;
                    moved = true;
                }
                if (isKeyHeld(kb_KeyDown))
                {
                    cursor_y++;
                    if (cursor_y > GRID_HEIGHT - 1)
                        cursor_y = 0;
                    moved = true;
                }
                if (moved)
                    last_cursor_time = current_time;
            }
        }
    }

    if (isKeyPressed(kb_KeyClear))
        enterState(&STATE_MENU);
    if (isKeyPressed(kb_KeyMode) && !is_playing)
        is_cell_picker_open = !is_cell_picker_open;
    if (isKeyPressed(kb_KeyYequ))
    {
        if (is_reset)
            for (int y = 0; y < GRID_HEIGHT; y++)
                for (int x = 0; x < GRID_WIDTH; x++)
                    grid_initial[y][x] = grid[y][x];
        is_reset = false;
        is_playing = !is_playing;
        last_tick_time = current_time;
    }
    if (isKeyPressed(kb_KeyWindow) && !is_reset)
    {
        for (int y = 0; y < GRID_HEIGHT; y++)
            for (int x = 0; x < GRID_WIDTH; x++)
                grid[y][x] = grid_initial[y][x];
        is_playing = false;
        is_reset = true;
    }
}

static void draw(void)
{
    // draw grid
    for (int y = 0; y < GRID_HEIGHT; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            drawCellNoClip(grid[y][x], x * 16, y * 16);
        }
    }

    // draw cursor
    if (!is_playing)
    {
        gfx_SetColor(1);
        gfx_Rectangle(cursor_x * 16, cursor_y * 16, 16, 16);
    }

    // draw cell picker
    if (is_cell_picker_open)
    {
        gfx_SetColor(1);
        gfx_FillRectangle_NoClip(0, 208, GFX_LCD_WIDTH, 32);
        for (uint8_t i = 0; i < cells_0_num_tiles - 1; i++)
        {
            // lower 4 bits are type, next 2 are rotation, next is updated flag, msb is unused
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