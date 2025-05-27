#ifndef GPIO_H
#define GPIO_H


#include <stdint.h>

void initialise_board();

void enable_touch_interrupts();

void display_number(uint8_t n);

void handle_touch(uint8_t pin);

#endif
