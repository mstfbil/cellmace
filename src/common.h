#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <string.h>
#include <graphx.h>
#include <compression.h>
#include "gfx/gfx.h"

#define TILE_WIDTH 16
#define TILE_HEIGHT 16

extern gfx_sprite_t *cell_sprites[cells_num_tiles][4];
extern gfx_sprite_t *cell_icon_sprites[cell_icons_num_tiles][4];
void cleanupSprites(void);

#endif
