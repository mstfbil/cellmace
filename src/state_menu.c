#include "input.h"
#include "state_menu.h"
#include "state_game.h"

static void step(void)
{
    if (isKeyPressed(kb_KeyClear))
        states_EnterState(NULL);
    if (isKeyPressed(kb_KeyEnter))
        states_EnterState(&STATE_GAME);
    if (isKeyPressed(kb_KeyRight))
    {
        save_slot++;
        if (save_slot > 9)
            save_slot = 1;
    }
    if (isKeyPressed(kb_KeyLeft))
    {
        save_slot--;
        if (save_slot < 1)
            save_slot = 9;
    }
}

static void draw(void)
{
    gfx_FillScreen(1);

    gfx_Sprite(cell_sprites[4][0], 10, 80);
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("cellmaCE", 30, 80);
    gfx_SetTextScale(1, 1);
    gfx_PrintString(" v1.0");

    gfx_PrintStringXY("Selected save: ", 10, 110);
    gfx_PrintUInt(save_slot, 1);
    gfx_PrintStringXY("<left> and <right> change save", 10, 130);
    gfx_PrintStringXY("<enter> load save and enter editor", 10, 140);
    gfx_PrintStringXY("<clear> exit", 10, 150);

    gfx_SwapDraw();
}

const game_state_t STATE_MENU = {NULL, step, draw, NULL};