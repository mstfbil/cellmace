#include "utils/input.h"
#include "state_editor.h"
#include "state_menu.h"

#define CURSOR_DELAY (32768 / 10)
#define TICK_DELAY (32768 / 5)

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

static const int8_t DIR_X[] = {1, 0, -1, 0};
static const int8_t DIR_Y[] = {0, 1, 0, -1};

uint8_t grid[GRID_HEIGHT][GRID_WIDTH];
static uint8_t grid_initial[GRID_HEIGHT][GRID_WIDTH];
char save_var_name[9];

static bool is_playing, is_reset;
static int cursor_x, cursor_y;
static uint8_t picked_cell_type, picked_cell_rot;
static uint32_t last_cursor_time, last_tick_time;

static int playing_bar_x = 0;
static bool playing_bar_to_right = true;
#define PLAYING_BAR_WIDTH (GFX_LCD_WIDTH / 10)
#define PLAYING_BAR_SPEED 4

void load_save_from_appvar(void)
{
    sprintf(save_var_name, SAVE_VAR_NAME_FORMAT, save_slot);
    uint8_t slot = ti_Open(save_var_name, "r");
    if (slot == 0)
        memset(grid, CELL_EMPTY, sizeof(grid));

    ti_Read(&grid, sizeof(grid), 1, slot);
    ti_Close(slot);
}

static void save_grid_to_appvar(void)
{
    bool is_empty = true;
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            if (grid[y][x] != CELL_EMPTY)
            {
                is_empty = false;
                break;
            }
    if (is_empty)
        ti_Delete(save_var_name);
    else
    {
        uint8_t slot = ti_Open(save_var_name, "w");
        if (slot == 0)
            return;

        ti_Write(&grid, sizeof(grid), 1, slot);
        ti_Close(slot);
    }
}

static void drawCellNoClip(uint8_t cell, int x, int y)
{
    gfx_Sprite_NoClip(cells_sprites[GET_CELL_TYPE(cell)][GET_CELL_ROT(cell)], x, y);
}

static inline bool isInBounds(uint8_t x, uint8_t y)
{
    return (x < GRID_WIDTH) && (y < GRID_HEIGHT);
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

    if (!GET_CELL_UPDATED(cell) && GET_CELL_TYPE(cell) == CELL_GENERATOR && GET_CELL_ROT(cell) == rot)
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

    if (!GET_CELL_UPDATED(cell) && GET_CELL_TYPE(cell) == type)
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

    if (!GET_CELL_UPDATED(cell) && GET_CELL_TYPE(cell) == CELL_MOVER && GET_CELL_ROT(cell) == rot)
    {
        if (pushCellRecursive(x, y, rot))
        {
            SET_UPDATED(grid[y + DIR_Y[rot]][x + DIR_X[rot]]);
        }
    }
}

// i really wanted to make this work but it's too slow at runtime so whatever
/* static void sweepGrid(void (*func)(int, int, uint8_t))
{
    typedef struct
    {
        int start_x, end_x, step_x;
        int start_y, end_y, step_y;
        bool is_y_outer_loop;
        uint8_t rot;
    } SweepConfig_t;

    static const SweepConfig_t sweep_config[4] =
        {
            {GRID_WIDTH - 1, -1, -1, GRID_HEIGHT - 1, -1, -1, 0, ROT_RIGHT},
            {0, GRID_WIDTH, 1, 0, GRID_HEIGHT, 1, 0, ROT_LEFT},
            {0, GRID_WIDTH, 1, 0, GRID_HEIGHT, 1, 1, ROT_UP},
            {GRID_WIDTH - 1, -1, -1, GRID_HEIGHT - 1, -1, -1, 1, ROT_DOWN}};

    for (int p = 0; p < 4; ++p)
    {
        const SweepConfig_t *config = &sweep_config[p];

        if (config->is_y_outer_loop)
        {
            for (int y = config->start_y; y != config->end_y; y += config->step_y)
                for (int x = config->start_x; x != config->end_x; x += config->step_x)
                    func(x, y, config->rot);
        }
        else
        {
            for (int x = config->start_x; x != config->end_x; x += config->step_x)
                for (int y = config->start_y; y != config->end_y; y += config->step_y)
                    func(x, y, config->rot);
        }
    }
} */

static void tick(void)
{
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

    is_playing = false;
    is_reset = true;

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

        // update indicator
        if (playing_bar_to_right)
        {
            playing_bar_x += PLAYING_BAR_SPEED;
            if (playing_bar_x + PLAYING_BAR_WIDTH >= GFX_LCD_WIDTH)
            {
                playing_bar_x = GFX_LCD_WIDTH - PLAYING_BAR_WIDTH;
                playing_bar_to_right = false;
            }
        }
        else
        {
            playing_bar_x -= PLAYING_BAR_SPEED;
            if (playing_bar_x <= 0)
            {
                playing_bar_x = 0;
                playing_bar_to_right = true;
            }
        }
    }
    else
    {
        if (isKeyHeld(kb_Key2nd))
            grid[cursor_y][cursor_x] = MAKE_CELL(picked_cell_type, picked_cell_rot);
        if (isKeyHeld(kb_KeyAlpha))
            grid[cursor_y][cursor_x] = CELL_EMPTY;
        if (isKeyPressed(kb_KeyZoom))
        {
            if (is_reset)
                memcpy(grid_initial, grid, sizeof(grid));
            is_reset = false;
            tick();
        }
        if (isKeyPressed(kb_KeyClear) && is_reset)
            states_EnterState(&STATE_MENU);
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

        if (isKeyPressed(kb_Key1))
            picked_cell_type = CELL_PUSH;
        if (isKeyPressed(kb_Key2))
            picked_cell_type = CELL_SLIDE;
        if (isKeyPressed(kb_Key3))
            picked_cell_type = CELL_MOVER;
        if (isKeyPressed(kb_Key4))
            picked_cell_type = CELL_CW_ROTATOR;
        if (isKeyPressed(kb_Key5))
            picked_cell_type = CELL_CCW_ROTATOR;
        if (isKeyPressed(kb_Key6))
            picked_cell_type = CELL_GENERATOR;
        if (isKeyPressed(kb_Key7))
            picked_cell_type = CELL_ENEMY;
        if (isKeyPressed(kb_Key8))
            picked_cell_type = CELL_TRASH;
        if (isKeyPressed(kb_Key9))
            picked_cell_type = CELL_WALL;
        if (isKeyPressed(kb_KeyAdd))
            picked_cell_rot = (picked_cell_rot + 1) % ROT_COUNT;
        if (isKeyPressed(kb_KeySub))
            picked_cell_rot = (picked_cell_rot + ROT_COUNT - 1) % ROT_COUNT;
    }
    if (isKeyPressed(kb_KeyYequ))
    {
        if (is_reset)
        {
            memcpy(grid_initial, grid, sizeof(grid));
            playing_bar_x = 0;
        }
        is_reset = false;
        is_playing = !is_playing;
        last_tick_time = current_time;
    }
    if (isKeyPressed(kb_KeyWindow) && !is_reset)
    {
        memcpy(grid, grid_initial, sizeof(grid));
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
        if (picked_cell_type > 1)
        {
            gfx_TransparentSprite(cell_icons_sprites[picked_cell_type - 2][picked_cell_rot], cursor_x * 16, cursor_y * 16);
        }
    }

    // draw busy indicators
    if (!is_reset)
    {
        gfx_SetColor(1);
        if (is_playing) // actively playing
        {
            gfx_HorizLine(playing_bar_x, GFX_LCD_HEIGHT - 1, PLAYING_BAR_WIDTH);
        }
        else // paused, step mode
        {
            gfx_Rectangle(0, 0, GFX_LCD_WIDTH, GFX_LCD_HEIGHT);
        }
    }

    gfx_SwapDraw();
}

static void quit(void)
{
    save_grid_to_appvar();
    timer_Disable(1);
}

const game_state_t STATE_EDITOR = {init, step, draw, quit};