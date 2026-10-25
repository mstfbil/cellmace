#ifndef __SAVE_H__
#define __SAVE_H__

#include "grid.h"

#define SAVE_VAR_NAME_FORMAT "CELLMAC%d"

extern int save_slot;

void deleteSave(void);
void loadSaveFromAppvar(game_grid_t grid);
void saveGridToAppvar(game_grid_t grid);

#endif // __SAVE_H__
