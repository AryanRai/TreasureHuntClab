#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include "structs.h"

void initialise_touch();

void enable_touch_interrupts();

void display_number(uint8_t n);

void touch_register_callback(void (*callback)(uint8_t pad, GameTriggers *trigger));

#endif
