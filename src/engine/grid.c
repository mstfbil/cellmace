#include "grid.h"
#include <stdbool.h>
#include <string.h>

game_grid_t grid;
game_grid_t grid_initial;

bool isInGridBounds(int x, int y)
{
    return (x >= 0) && (y >= 0) && (x < GRID_WIDTH) && (y < GRID_HEIGHT);
}

void saveGridToInitial(void)
{
    memcpy(grid_initial, grid, sizeof(game_grid_t));
}

void resetGridToInitial(void)
{
    memcpy(grid, grid_initial, sizeof(game_grid_t));
}