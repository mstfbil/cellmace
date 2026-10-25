#ifndef __SPRITES_H__
#define __SPRITES_H__

#include "gfx.h"
#include <graphx.h>

#define CELL_WIDTH 16
#define CELL_HEIGHT 16

extern gfx_sprite_t *cells_sprites[cells_num_tiles][4];
extern gfx_sprite_t *cell_icons_sprites[cell_icons_num_tiles][4];

void initCellSprites(void);
void freeCellSprites(void);

#endif // __SPRITES_H__