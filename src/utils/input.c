#include "input.h"

static uint8_t old_kb_Data[8];

void updateInput(void)
{
    for (int i = 0; i < 8; i++)
        old_kb_Data[i] = kb_Data[i];

    kb_Scan();
}

bool isKeyHeld(kb_lkey_t key)
{
    uint8_t group = (uint8_t)(key >> 8);
    uint8_t mask = (uint8_t)(key & 0xFF);
    return (kb_Data[group] & mask);
}

bool isKeyPressed(kb_lkey_t key)
{
    uint8_t group = (uint8_t)(key >> 8);
    uint8_t mask = (uint8_t)(key & 0xFF);
    return (kb_Data[group] & mask) && !(old_kb_Data[group] & mask);
}