#include "state_editor.h"
#include "state_menu.h"
#include "engine/grid.h"
#include "engine/sim.h"
#include "engine/save.h"
#include "utils/input.h"
#include "gfx/sprites.h"
#include <graphx.h>
#include <sys/timers.h>
#include <string.h>

#define CURSOR_DELAY (32768 / 10)
#define TICK_DELAY (32768 / 5)

static bool is_playing, is_reset;
static int cursor_x, cursor_y;
static uint8_t picked_cell_type, picked_cell_rot;
static uint32_t last_cursor_time, last_tick_time;

static int playing_bar_x = 0;
static bool playing_bar_to_right = true;
#define PLAYING_BAR_WIDTH (GFX_LCD_WIDTH / 10)
#define PLAYING_BAR_SPEED 4

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
            stepSimulation(grid);
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
            grid[cursor_y][cursor_x] = (picked_cell_type | (picked_cell_rot) << 4);
        if (isKeyHeld(kb_KeyAlpha))
            grid[cursor_y][cursor_x] = CELL_EMPTY;
        if (isKeyPressed(kb_KeyZoom))
        {
            if (is_reset)
                saveGridToInitial();
            is_reset = false;
            stepSimulation(grid);
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
            saveGridToInitial();
            playing_bar_x = 0;
        }
        is_reset = false;
        is_playing = !is_playing;
        last_tick_time = current_time;
    }
    if (isKeyPressed(kb_KeyWindow) && !is_reset)
    {
        resetGridToInitial();
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
            uint8_t cell = grid[y][x];
            gfx_Sprite_NoClip(cells_sprites[cell & CELL_TYPE_MASK][(cell & CELL_ROT_MASK) >> 4], x * 16, y * 16);
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
    saveGridToAppvar(grid);
    timer_Disable(1);
}

const game_state_t STATE_EDITOR = {init, step, draw, quit};