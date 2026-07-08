#include "common.h"
#include "input.h"
#include "state_game.h"

game_state_t *current_state = NULL;

void enterState(const game_state_t *new_state)
{
    current_state = (game_state_t *)new_state;

    if (current_state != NULL && current_state->init != NULL)
        current_state->init();
}

int main(void)
{
    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    enterState(&STATE_GAME);

    while (current_state != NULL)
    {
        updateInput();
        current_state->step();

        if (current_state != NULL)
        {
            current_state->draw();
        }
    }

    gfx_End();
    return 0;
}