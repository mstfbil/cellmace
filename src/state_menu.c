#include "input.h"
#include "state_menu.h"
#include "state_game.h"

static int selected_menu_item;

static void init(void)
{
    selected_menu_item = 0;
}

static void step(void)
{
    if (isKeyPressed(kb_KeyClear))
        states_EnterState(NULL);
    else if (isKeyPressed(kb_KeyDown) && selected_menu_item < 1)
        selected_menu_item++;
    else if (isKeyPressed(kb_KeyUp) && selected_menu_item > 0)
        selected_menu_item--;
    else if (isKeyPressed(kb_KeyEnter) || isKeyPressed(kb_Key2nd))
    {
        switch (selected_menu_item)
        {
        case 0:
            states_EnterState(&STATE_GAME);
            break;
        case 1:
            states_EnterState(NULL);
            break;
        }
    }
}

static void draw(void)
{
    // very dirty menu, will improve later
    gfx_FillScreen(1);

    gfx_SetTextScale(3, 3);
    gfx_PrintStringXY("cellmaCE", 10, 10);
    gfx_SetTextScale(1, 1);
    gfx_PrintStringXY("v0.0.1-indev", 10, 40);

    gfx_PrintStringXY("Enter editor", 20, 80);
    gfx_PrintStringXY("Exit", 20, 100);

    gfx_PrintStringXY(">", 10, 80 + 20 * selected_menu_item);

    gfx_SwapDraw();
}

const game_state_t STATE_MENU = {init, step, draw, NULL};