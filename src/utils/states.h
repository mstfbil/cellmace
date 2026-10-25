#ifndef STATES_H
#define STATES_H

#include <stdbool.h>

typedef struct
{
    void (*init)(void);
    void (*step)(void);
    void (*draw)(void);
    void (*quit)(void);
} game_state_t;

void states_Init(const game_state_t *initial_state);
void states_Step(void);
void states_EnterState(const game_state_t *new_state);
bool states_ShouldClose(void);

#endif