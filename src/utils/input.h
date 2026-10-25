#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <keypadc.h>

void updateInput(void);
bool isKeyHeld(kb_lkey_t key);
bool isKeyPressed(kb_lkey_t key);

#endif