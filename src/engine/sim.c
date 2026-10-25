#include "sim.h"
#include "grid.h"
#include <stdbool.h>

static const int8_t DIR_X[] = {1, 0, -1, 0};
static const int8_t DIR_Y[] = {0, 1, 0, -1};

static bool push_cell_recursive(int x, int y, uint8_t dir)
{
    if (!isInGridBounds(x, y))
        return false;

    uint8_t cell = grid[y][x];
    uint8_t type = cell & CELL_TYPE_MASK;

    if (type == CELL_EMPTY)
        return true;
    else if (type == CELL_WALL)
        return false;
    else if (type == CELL_SLIDE)
    {
        uint8_t rot = (cell & CELL_ROT_MASK) >> 4;
        if ((rot == ROT_LEFT || rot == ROT_RIGHT) != (dir == ROT_LEFT || dir == ROT_RIGHT))
            return false;
    }
    else if (type == CELL_ENEMY)
    {
        grid[y][x] = CELL_EMPTY;
        return false;
    }

    int next_x = x + DIR_X[dir];
    int next_y = y + DIR_Y[dir];
    if (!isInGridBounds(next_x, next_y))
        return false;

    uint8_t target_cell = grid[next_y][next_x];
    uint8_t target_type = target_cell & CELL_TYPE_MASK;

    if (target_type == CELL_TRASH)
    {
        grid[y][x] = CELL_EMPTY;
        return true;
    }
    if (target_type == CELL_ENEMY)
    {
        grid[next_y][next_x] = CELL_EMPTY;
        grid[y][x] = CELL_EMPTY;
        return true;
    }

    if (push_cell_recursive(next_x, next_y, dir))
    {
        grid[next_y][next_x] = cell;
        grid[y][x] = CELL_EMPTY;
        return true;
    }

    return false;
}

static void do_generator(int x, int y, uint8_t rot)
{
    uint8_t cell = grid[y][x];

    if (!(cell & CELL_UPDATED_MASK) && (cell & CELL_TYPE_MASK) == CELL_GENERATOR && ((cell & CELL_ROT_MASK) >> 4) == rot)
    {
        int back_x = x - DIR_X[rot];
        int back_y = y - DIR_Y[rot];

        if (isInGridBounds(back_x, back_y))
        {
            uint8_t source = grid[back_y][back_x];
            if ((source & CELL_TYPE_MASK) != CELL_EMPTY)
            {
                int front_x = x + DIR_X[rot];
                int front_y = y + DIR_Y[rot];

                if (push_cell_recursive(front_x, front_y, rot))
                {
                    grid[front_y][front_x] = source & 0x7F;
                }
                grid[y][x] |= CELL_UPDATED_MASK;
            }
        }
    }
}

static void do_rotator(int x, int y, uint8_t type, int8_t rot_dir)
{
    uint8_t cell = grid[y][x];

    if (!(cell & CELL_UPDATED_MASK) && (cell & CELL_TYPE_MASK) == type)
    {
        for (int i = 0; i < 4; i++)
        {
            int target_x = x + DIR_X[i];
            int target_y = y + DIR_Y[i];

            if (isInGridBounds(target_x, target_y))
            {
                uint8_t neighbor = grid[target_y][target_x];
                uint8_t neighbor_type = (neighbor & CELL_TYPE_MASK);

                if (neighbor_type != CELL_EMPTY)
                {
                    uint8_t neighbor_rot = (neighbor & CELL_ROT_MASK) >> 4;
                    uint8_t new_rot = (neighbor_rot + rot_dir) & 3;

                    grid[target_y][target_x] = (neighbor & 0xCF) | (new_rot << 4);
                }
            }
        }

        grid[y][x] |= CELL_UPDATED_MASK;
    }
}

static void do_mover(int x, int y, uint8_t rot)
{
    uint8_t cell = grid[y][x];

    if (!(cell & CELL_UPDATED_MASK) && (cell & CELL_TYPE_MASK) == CELL_MOVER && ((cell & CELL_ROT_MASK) >> 4) == rot)
    {
        if (push_cell_recursive(x, y, rot))
        {
            grid[y + DIR_Y[rot]][x + DIR_X[rot]] |= CELL_UPDATED_MASK;
        }
    }
}

void stepSimulation(game_grid_t grid)
{
    // GENERATORS
    // right
     for (int x = GRID_WIDTH - 1; x >= 0; x--)
        for (int y = GRID_HEIGHT - 1; y >= 0; y--)
            do_generator(x, y, ROT_RIGHT);
    // left
    for (int x = 0; x < GRID_WIDTH; x++)
        for (int y = 0; y < GRID_HEIGHT; y++)
            do_generator(x, y, ROT_LEFT);
    // up
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            do_generator(x, y, ROT_UP);
    // down
    for (int y = GRID_HEIGHT - 1; y >= 0; y--)
        for (int x = GRID_WIDTH - 1; x >= 0; x--)
            do_generator(x, y, ROT_DOWN);
    
    // ROTATORS
    // cw
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            do_rotator(x, y, CELL_CW_ROTATOR, 1);
    // ccw
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            do_rotator(x, y, CELL_CCW_ROTATOR, -1);

    // MOVERS
    // right
    for (int x = GRID_WIDTH - 1; x >= 0; x--)
        for (int y = GRID_HEIGHT - 1; y >= 0; y--)
            do_mover(x, y, ROT_RIGHT);
    // left
    for (int x = 0; x < GRID_WIDTH; x++)
        for (int y = 0; y < GRID_HEIGHT; y++)
            do_mover(x, y, ROT_LEFT);
    // up
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            do_mover(x, y, ROT_UP);
    // down
    for (int y = GRID_HEIGHT - 1; y >= 0; y--)
        for (int x = GRID_WIDTH - 1; x >= 0; x--)
            do_mover(x, y, ROT_DOWN);

    // CLEANUP OF UPDATED FLAG
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            grid[y][x] &= ~CELL_UPDATED_MASK;
}