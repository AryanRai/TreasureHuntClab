
#ifndef TIMER2_MODULE_H //Define initialise only if it hasnt been already
#define TIMER2_MODULE_H

#include <stdint.h> // allow fixed width integer types uint8_t, uint16_t and uint32
#include "stm32f303xc.h"

void enableTimer3(void);
void enableInterrupts(void);
void TIM2_IRQHandler(void);
void timer3_set_callback(void (*cb)(void));
//void set_timer_period3(uint32_t new_period);
void set_timer3(uint32_t val);
void TIM3_IRQHandler(void);
void one_shot_LED(void);
void disableTimer2();

//what else do I need?


#endif /* TIMER2_MODULE_H */
