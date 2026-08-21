#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <string.h>
#include <graphx.h>
#include "gfx/gfx.h"

extern gfx_sprite_t *cell_sprites[cells_0_num_tiles][4];
void cleanupRotatedSprites(void);

#endif
