
#ifndef TIMER_MODULE_H //Define initialise only if it hasnt been already
#define TIMER_MODULE_H

#include <stdint.h> // allow fixed width integer types uint8_t, uint16_t and uint32
#include "stm32f303xc.h"
#include "gamestate.h"


void enableTimer2();
void TIM2_IRQHandler(void);
uint32_t get_timer(void);;
//void set_timer_period(uint32_t new_period);
void set_timer2(uint32_t val);
//void reset_timer(void);
void use_alternate_timer_period(void);
void LEDflash();
int valid_period_check();

//what else do I need?


#endif /* TIMER_MODULE_H */
