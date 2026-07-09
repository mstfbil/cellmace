#include "state_game.h"
#include "common.h"
#include "input.h"
#include <sys/timers.h>

#define GRID_WIDTH 20
#define GRID_HEIGHT 15
#define CURSOR_DELAY (32768 / 8)
#define TICK_DELAY (32768 / 4)

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
static uint8_t cursor_x, cursor_y;
static uint8_t picked_cell_type, picked_cell_rot;
static uint32_t last_cursor_time, last_tick_time;
static bool is_cell_picker_open, is_playing, is_reset;
static const int8_t DIR_X[] = {1, 0, -1, 0};
static const int8_t DIR_Y[] = {0, 1, 0, -1};

static void drawCellNoClip(uint8_t cell, int x, int y)
{
    const uint8_t cell_type = cell & 0xF;
    const uint8_t cell_rot = (cell >> 4) & 3;
    gfx_Sprite_NoClip(cell_sprites[cell_type][cell_rot], x, y);
}

static bool pushCellRecursive(int x, int y, uint8_t dir)
{
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT)
        return false;

    uint8_t cell = grid[y][x];
    uint8_t type = cell & 0xF;

    if (type == CELL_EMPTY)
        return true;
    else if (type == CELL_WALL)
        return false;
    else if (type == CELL_SLIDE)
    {
        uint8_t rot = (cell >> 4) & 3;
        if (rot == ROT_RIGHT || rot == ROT_LEFT)
        {
            if (dir == ROT_UP || dir == ROT_DOWN)
                return false;
        }
        else if (dir == ROT_RIGHT || dir == ROT_LEFT)
            return false;
    }

    int next_x = x + DIR_X[dir];
    int next_y = y + DIR_Y[dir];

    if (next_x < 0 || next_x >= GRID_WIDTH || next_y < 0 || next_y >= GRID_HEIGHT)
        return false;

    uint8_t target_cell = grid[next_y][next_x];
    uint8_t target_type = target_cell & 0xF;

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

/* static uint8_t update_rot_order[] = {ROT_RIGHT, ROT_LEFT, ROT_UP, ROT_DOWN}; */
// will use the above later!!
static void tick()
{
    // very crude and repetitive implementation, will improve later

    // GENERATORS
    // right
    for (int x = GRID_WIDTH - 1; x >= 0; x--)
        for (int y = GRID_HEIGHT - 1; y >= 0; y--)
        {
            uint8_t cell = grid[y][x];

            if (!IS_UPDATED(cell) && (cell & 0xF) == CELL_GENERATOR && ((cell >> 4) & 3) == ROT_RIGHT)
            {
                int back_x = x - 1;
                int back_y = y;

                if (back_x >= 0)
                {
                    uint8_t source = grid[back_y][back_x];
                    if ((source & 0xF) != CELL_EMPTY)
                    {
                        int front_x = x + 1;
                        int front_y = y;

                        if (pushCellRecursive(front_x, front_y, ROT_RIGHT))
                        {
                            grid[front_y][front_x] = source & 0x7F;
                        }
                        SET_UPDATED(grid[y][x]);
                    }
                }
            }
        }
    // left
    for (int x = 0; x < GRID_WIDTH; x++)
        for (int y = 0; y < GRID_HEIGHT; y++)
        {
            uint8_t cell = grid[y][x];

            if (!IS_UPDATED(cell) && (cell & 0xF) == CELL_GENERATOR && ((cell >> 4) & 3) == ROT_LEFT)
            {
                int back_x = x + 1;
                int back_y = y;

                if (back_x < GRID_WIDTH)
                {
                    uint8_t source = grid[back_y][back_x];
                    if ((source & 0xF) != CELL_EMPTY)
                    {
                        int front_x = x - 1;
                        int front_y = y;

                        if (pushCellRecursive(front_x, front_y, ROT_LEFT))
                        {
                            grid[front_y][front_x] = source & 0x7F;
                        }
                        SET_UPDATED(grid[y][x]);
                    }
                }
            }
        }
    // up
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            uint8_t cell = grid[y][x];

            if (!IS_UPDATED(cell) && (cell & 0xF) == CELL_GENERATOR && ((cell >> 4) & 3) == ROT_UP)
            {
                int back_x = x;
                int back_y = y + 1;

                if (back_y < GRID_HEIGHT)
                {
                    uint8_t source = grid[back_y][back_x];
                    if ((source & 0xF) != CELL_EMPTY)
                    {
                        int front_x = x;
                        int front_y = y - 1;

                        if (pushCellRecursive(front_x, front_y, ROT_UP))
                        {
                            grid[front_y][front_x] = source & 0x7F;
                        }
                        SET_UPDATED(grid[y][x]);
                    }
                }
            }
        }
    // down
    for (int y = GRID_HEIGHT - 1; y >= 0; y--)
        for (int x = GRID_WIDTH - 1; x >= 0; x--)
        {
            uint8_t cell = grid[y][x];

            if (!IS_UPDATED(cell) && (cell & 0xF) == CELL_GENERATOR && ((cell >> 4) & 3) == ROT_DOWN)
            {
                int back_x = x;
                int back_y = y - 1;

                if (back_y >= 0)
                {
                    uint8_t source = grid[back_y][back_x];
                    if ((source & 0xF) != CELL_EMPTY)
                    {
                        int front_x = x;
                        int front_y = y + 1;

                        if (pushCellRecursive(front_x, front_y, ROT_DOWN))
                        {
                            grid[front_y][front_x] = source & 0x7F;
                        }
                        SET_UPDATED(grid[y][x]);
                    }
                }
            }
        }

    // ROTATORS
    // cw
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            uint8_t cell = grid[y][x];

            if (!IS_UPDATED(cell) && (cell & 0xF) == CELL_CW_ROTATOR)
            {
                for (int i = 0; i < 4; i++)
                {
                    int target_x = x + DIR_X[i];
                    int target_y = y + DIR_Y[i];

                    if (target_x >= 0 && target_x < GRID_WIDTH && target_y >= 0 && target_y < GRID_HEIGHT)
                    {
                        uint8_t neighbor = grid[target_y][target_x];

                        if ((neighbor & 0xF) != CELL_EMPTY)
                        {
                            uint8_t type = neighbor & 0x8F;
                            uint8_t rot = (neighbor >> 4) & 3;

                            uint8_t new_rot = (rot + 1) & 3;

                            grid[target_y][target_x] = type | (new_rot << 4);
                        }
                    }
                }

                SET_UPDATED(grid[y][x]);
            }
        }
    // ccw
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            uint8_t cell = grid[y][x];

            if (!IS_UPDATED(cell) && (cell & 0xF) == CELL_CCW_ROTATOR)
            {
                for (int i = 0; i < 4; i++)
                {
                    int target_x = x + DIR_X[i];
                    int target_y = y + DIR_Y[i];

                    if (target_x >= 0 && target_x < GRID_WIDTH && target_y >= 0 && target_y < GRID_HEIGHT)
                    {
                        uint8_t neighbor = grid[target_y][target_x];

                        if ((neighbor & 0xF) != CELL_EMPTY)
                        {
                            uint8_t type = neighbor & 0x8F;
                            uint8_t rot = (neighbor >> 4) & 3;

                            uint8_t new_rot = (rot - 1) & 3;

                            grid[target_y][target_x] = type | (new_rot << 4);
                        }
                    }
                }

                SET_UPDATED(grid[y][x]);
            }
        }

    // MOVERS
    // right
    for (int x = GRID_WIDTH - 1; x >= 0; x--)
        for (int y = GRID_HEIGHT - 1; y >= 0; y--)
        {
            uint8_t cell = grid[y][x];
            if (!IS_UPDATED(cell) && (cell & 0xF) == CELL_MOVER && ((cell >> 4) & 3) == ROT_RIGHT)
            {
                if (pushCellRecursive(x, y, ROT_RIGHT))
                {
                    SET_UPDATED(grid[y][x + 1]);
                }
            }
        }
    // left
    for (int x = 0; x < GRID_WIDTH; x++)
        for (int y = 0; y < GRID_HEIGHT; y++)
        {
            uint8_t cell = grid[y][x];
            if (!IS_UPDATED(cell) && (cell & 0xF) == CELL_MOVER && ((cell >> 4) & 3) == ROT_LEFT)
            {
                if (pushCellRecursive(x, y, ROT_LEFT))
                {
                    SET_UPDATED(grid[y][x - 1]);
                }
            }
        }
    // up
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            uint8_t cell = grid[y][x];
            if (!IS_UPDATED(cell) && (cell & 0xF) == CELL_MOVER && ((cell >> 4) & 3) == ROT_UP)
            {
                if (pushCellRecursive(x, y, ROT_UP))
                {
                    SET_UPDATED(grid[y - 1][x]);
                }
            }
        }
    // down
    for (int y = GRID_HEIGHT - 1; y >= 0; y--)
        for (int x = GRID_WIDTH - 1; x >= 0; x--)
        {
            uint8_t cell = grid[y][x];
            if (!IS_UPDATED(cell) && (cell & 0xF) == CELL_MOVER && ((cell >> 4) & 3) == ROT_DOWN)
            {
                if (pushCellRecursive(x, y, ROT_DOWN))
                {
                    SET_UPDATED(grid[y + 1][x]);
                }
            }
        }

    for (int y = 0; y < GRID_HEIGHT; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            CLEAR_UPDATED(grid[y][x]);
        }
    }
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

    for (uint8_t y = 0; y < GRID_HEIGHT; y++)
        for (uint8_t x = 0; x < GRID_WIDTH; x++)
            grid[y][x] = 0;

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
    }

    if (isKeyPressed(kb_KeyClear))
        enterState(NULL);
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
    if (isKeyPressed(kb_KeyWindow))
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
    for (uint8_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < GRID_WIDTH; x++)
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