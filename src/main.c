#include "common.h"
#include "utils/states.h"
#include "utils/input.h"
#include "states/state_menu.h"
#include "states/state_editor.h" // temporary!
#include "gfx/sprites.h"

int save_slot; // temporary!

int main(void)
{
    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    gfx_SetTransparentColor(0);
    initCellSprites();

    save_slot = 1;
    load_save_from_appvar();

    states_Init(&STATE_MENU);

    while (!states_ShouldClose())
    {
        updateInput();
        states_Step();
    }

    freeCellSprites();
    gfx_End();
    return 0;
}