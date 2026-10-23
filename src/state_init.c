#include "state_init.h"
#include "state_menu.h"

gfx_sprite_t *cell_sprites[cells_num_tiles][4];
gfx_sprite_t *cell_icon_sprites[cell_icons_num_tiles][4];

int save_slot;

static void init(void)
{
    save_slot = 1;
    gfx_SetTransparentColor(0);

    for (int i = 0; i < cells_num_tiles; i++)
    {
        gfx_sprite_t *base = gfx_MallocSprite(TILE_WIDTH, TILE_HEIGHT);
        zx0_Decompress(base, cells_tiles_compressed[i]);

        cell_sprites[i][0] = base;

        cell_sprites[i][1] = gfx_MallocSprite(TILE_HEIGHT, TILE_WIDTH);
        gfx_RotateSpriteC(base, cell_sprites[i][1]);

        cell_sprites[i][2] = gfx_MallocSprite(TILE_WIDTH, TILE_HEIGHT);
        gfx_RotateSpriteHalf(base, cell_sprites[i][2]);

        cell_sprites[i][3] = gfx_MallocSprite(TILE_HEIGHT, TILE_WIDTH);
        gfx_RotateSpriteCC(base, cell_sprites[i][3]);
    }

    for (int i = 0; i < cell_icons_num_tiles; i++)
    {
        gfx_sprite_t *base = gfx_MallocSprite(TILE_WIDTH, TILE_HEIGHT);
        zx0_Decompress(base, cell_icons_tiles_compressed[i]);
        
        cell_icon_sprites[i][0] = base;

        cell_icon_sprites[i][1] = gfx_MallocSprite(TILE_HEIGHT, TILE_WIDTH);
        gfx_RotateSpriteC(base, cell_icon_sprites[i][1]);

        cell_icon_sprites[i][2] = gfx_MallocSprite(TILE_WIDTH, TILE_HEIGHT);
        gfx_RotateSpriteHalf(base, cell_icon_sprites[i][2]);

        cell_icon_sprites[i][3] = gfx_MallocSprite(TILE_HEIGHT, TILE_WIDTH);
        gfx_RotateSpriteCC(base, cell_icon_sprites[i][3]);
    }

    states_EnterState(&STATE_MENU);
}

void cleanupSprites(void)
{
    for (int i = 0; i < cells_num_tiles; i++)
    {
        free(cell_sprites[i][0]);
        free(cell_sprites[i][1]);
        free(cell_sprites[i][2]);
        free(cell_sprites[i][3]);
    }

    for (int i = 0; i < cell_icons_num_tiles; i++)
    {
        free(cell_icon_sprites[i][0]);
        free(cell_icon_sprites[i][1]);
        free(cell_icon_sprites[i][2]);
        free(cell_icon_sprites[i][3]);
    }
}

const game_state_t STATE_INIT = {init, NULL, NULL, NULL};