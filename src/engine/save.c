#include "save.h"
#include "grid.h"
#include <fileioc.h>
#include <string.h>

int save_slot;

void deleteSave(void)
{
    char save_var_name[9];
    sprintf(save_var_name, SAVE_VAR_NAME_FORMAT, save_slot);
    ti_Delete(save_var_name);
}

void loadSaveFromAppvar(game_grid_t grid)
{
    char save_var_name[9];
    sprintf(save_var_name, SAVE_VAR_NAME_FORMAT, save_slot);

    uint8_t slot = ti_Open(save_var_name, "r");
    if (slot == 0)
    {
        memset(grid, CELL_EMPTY, sizeof(game_grid_t));
        return;
    }

    ti_Read(grid, sizeof(game_grid_t), 1, slot);
    ti_Close(slot);
}

void saveGridToAppvar(game_grid_t grid)
{
    char save_var_name[9];
    sprintf(save_var_name, SAVE_VAR_NAME_FORMAT, save_slot);

    bool is_empty = true;
    for (int y = 0; y < GRID_HEIGHT; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
            if (grid[y][x] != CELL_EMPTY)
            {
                is_empty = false;
                break;;
            }
        if (!is_empty)
            break;;
    }

    if (is_empty)
    {
        deleteSave();
        return;
    }

    uint8_t slot = ti_Open(save_var_name, "w");
    if (slot == 0)
        return;

    ti_Write(grid, sizeof(game_grid_t), 1, slot);
    ti_Close(slot);
}