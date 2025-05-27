#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdint.h>

// Initialize LED GPIO
void led_init(void);

// Toggle LED
void led_toggle(void);

// Set LED state (0 = off, 1 = on)
void led_set(uint8_t pattern);

// Get LED state (returns 0 or 1)
uint8_t led_get(void);

void convertStringToBinary(char *input, uint8_t *output);

int isBinary(char *input);

#endif
