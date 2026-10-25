#include "engine/grid.h"
#include "engine/save.h"
#include "utils/states.h"
#include "utils/input.h"
#include "states/state_menu.h"
#include "gfx/sprites.h"

int main(void)
{
    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    gfx_SetTransparentColor(0);
    initCellSprites();

    save_slot = 1;
    loadSaveFromAppvar(grid);

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