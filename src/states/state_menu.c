#include "utils/input.h"
#include "state_menu.h"
#include "state_editor.h"

#define PREVIEW_X 200
#define PREVIEW_Y 82
#define PREVIEW_CELL_SIZE 5

static const uint8_t CELL_COLORS[] = {
    4,  // CELL_EMPTY
    6,  // CELL_WALL
    19, // CELL_PUSH
    17, // CELL_SLIDE
    8,  // CELL_MOVER
    18, // CELL_CW_ROTATOR
    13, // CELL_CCW_ROTATOR
    14, // CELL_GENERATOR
    16, // CELL_ENEMY
    15  // CELL_TRASH
};

static bool at_delete_confirmation = false;

static void init(void)
{
    at_delete_confirmation = false;
}

static void step(void)
{
    if (at_delete_confirmation)
    {
        if (isKeyPressed(kb_KeyClear))
            at_delete_confirmation = false;
        if (isKeyPressed(kb_KeyDel))
        {
            ti_Delete(save_var_name);
            load_save_from_appvar();
            at_delete_confirmation = false;
        }
    }
    else
    {

        if (isKeyPressed(kb_KeyClear))
            states_EnterState(NULL);
        if (isKeyPressed(kb_KeyEnter))
            states_EnterState(&STATE_EDITOR);
        if (isKeyPressed(kb_KeyDel))
        {
            at_delete_confirmation = true;
        }
        if (isKeyPressed(kb_KeyRight))
        {
            save_slot++;
            if (save_slot > 9)
                save_slot = 1;
            load_save_from_appvar();
            at_delete_confirmation = false;
        }
        if (isKeyPressed(kb_KeyLeft))
        {
            save_slot--;
            if (save_slot < 1)
                save_slot = 9;
            load_save_from_appvar();
            at_delete_confirmation = false;
        }
    }
}

static void draw(void)
{
    gfx_FillScreen(1);

    gfx_SetColor(0);
    gfx_Sprite(cells_sprites[4][0], 10, 80);
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("cellmaCE", 30, 80);
    gfx_SetTextScale(1, 1);
    gfx_PrintString(" v1.0");

    if (at_delete_confirmation)
    {
        gfx_PrintStringXY("<del> confirm delete", 10, 140);
        gfx_PrintStringXY("<clear> cancel", 10, 150);
    }
    else
    {
        gfx_PrintStringXY("Selected save: <-", 10, 110);
        gfx_PrintUInt(save_slot, 1);
        gfx_PrintString("->");
        gfx_PrintStringXY("<enter> enter editor", 10, 130);

        gfx_PrintStringXY("<del> delete save", 10, 140);
        gfx_PrintStringXY("<clear> exit", 10, 150);
    }

    gfx_PrintStringXY("cellmaCE - made with <3 by voltie_dev - 2026", 10, 230);

    // draw grid preview
    gfx_Rectangle(PREVIEW_X - 2, PREVIEW_Y - 2, PREVIEW_CELL_SIZE * GRID_WIDTH + 4, PREVIEW_CELL_SIZE * GRID_HEIGHT + 4);
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            uint8_t type = GET_CELL_TYPE(grid[y][x]);
            gfx_SetColor(CELL_COLORS[type]);
            gfx_FillRectangle_NoClip(PREVIEW_X + PREVIEW_CELL_SIZE * x, PREVIEW_Y + PREVIEW_CELL_SIZE * y, PREVIEW_CELL_SIZE, PREVIEW_CELL_SIZE);
        }

    gfx_SwapDraw();
}

const game_state_t STATE_MENU = {init, step, draw, NULL};