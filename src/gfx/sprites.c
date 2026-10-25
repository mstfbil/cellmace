#include "sprites.h"
#include "gfx.h"
#include <graphx.h>
#include <compression.h>

gfx_sprite_t *cells_sprites[cells_num_tiles][4];
gfx_sprite_t *cell_icons_sprites[cell_icons_num_tiles][4];

static void generate_rotated_sprite(gfx_sprite_t *dest[4], gfx_sprite_t *base, int tile_width, int tile_height)
{
    dest[0] = base;

    dest[1] = gfx_MallocSprite(tile_height, tile_width);
    gfx_RotateSpriteC(base, dest[1]);

    dest[2] = gfx_MallocSprite(tile_width, tile_height);
    gfx_RotateSpriteHalf(base, dest[2]);

    dest[3] = gfx_MallocSprite(tile_height, tile_width);
    gfx_RotateSpriteCC(base, dest[3]);
}

void initCellSprites(void)
{
    for (int i = 0; i < cells_num_tiles; i++)
    {
        gfx_sprite_t *base = gfx_MallocSprite(CELL_WIDTH, CELL_HEIGHT);
        zx0_Decompress(base, cells_tiles_compressed[i]);
        generate_rotated_sprite(cells_sprites[i], base, CELL_WIDTH, CELL_HEIGHT);
    }
    for (int i = 0; i < cell_icons_num_tiles; i++)
    {
        gfx_sprite_t *base = gfx_MallocSprite(CELL_WIDTH, CELL_HEIGHT);
        zx0_Decompress(base, cell_icons_tiles_compressed[i]);
        generate_rotated_sprite(cell_icons_sprites[i], base, CELL_WIDTH, CELL_HEIGHT);
    }
}

void freeCellSprites(void)
{
    for (int i = 0; i < cells_num_tiles; i++)
        for (int j = 0; j < 4; j++)
            free(cells_sprites[i][j]);
    for (int i = 0; i < cell_icons_num_tiles; i++)
        for (int j = 0; j < 4; j++)
            free(cell_icons_sprites[i][j]);
}