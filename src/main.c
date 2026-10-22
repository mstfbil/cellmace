#include "common.h"
#include "states.h"
#include "input.h"
#include "state_init.h"

int main(void)
{
    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);

    states_Init(&STATE_INIT);

    while (!states_ShouldClose())
    {
        updateInput();
        states_Step();
    }

    cleanupSprites();
    gfx_End();
    return 0;
}