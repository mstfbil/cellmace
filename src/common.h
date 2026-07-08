#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <graphx.h>
#include "gfx/gfx.h"

typedef struct
{
    void (*init)(void);
    void (*step)(void);
    void (*draw)(void);
} game_state_t;

extern game_state_t *current_state;
void enterState(const game_state_t *new_state);

#endif