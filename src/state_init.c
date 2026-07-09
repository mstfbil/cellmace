#include "state_init.h"
#include "state_game.h"

gfx_sprite_t *cell_sprites[cells_0_num_tiles][4];

void init(void)
{
    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);
    gfx_PrintStringXY("please wait... cellmace indev by voltie_dev", 10, 10);
    gfx_SwapDraw();

    for (uint8_t i = 0; i < cells_0_num_tiles; i++)
    {
        gfx_sprite_t *base = cells_0_tiles[i];

        cell_sprites[i][0] = base;

        cell_sprites[i][1] = gfx_MallocSprite(base->width, base->height);
        gfx_RotateSpriteC(base, cell_sprites[i][1]);

        cell_sprites[i][2] = gfx_MallocSprite(base->width, base->height);
        gfx_RotateSpriteHalf(base, cell_sprites[i][2]);

        cell_sprites[i][3] = gfx_MallocSprite(base->width, base->height);
        gfx_RotateSpriteCC(base, cell_sprites[i][3]);
    }

    enterState(&STATE_GAME);
}

void cleanupRotatedSprites(void)
{
    for (uint8_t i = 0; i < cells_0_num_tiles; i++)
    {
        free(cell_sprites[i][1]);
        free(cell_sprites[i][2]);
        free(cell_sprites[i][3]);
    }
}

const game_state_t STATE_INIT = {init, NULL, NULL, NULL};