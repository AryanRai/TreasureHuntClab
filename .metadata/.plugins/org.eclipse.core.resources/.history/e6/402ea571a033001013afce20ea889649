/*
 * serial.c
 *
 *  Created on: Apr 14, 2025
 *      Author: 163910
 */

#ifndef SERIAL_C_
#define SERIAL_C_

#ifndef BUFFER
#define BUFFER 256
#endif

#include "gamestate.h"

void reset_input_buffer();
void enableUSART1();
void enableLEDs();
void enableUARTInterrupts();
void print_game_state(GameState game);
void print_game_triggers(GameTriggers triggers);
extern unsigned char string[BUFFER];
void send_string_buffer();
void send_string();
void print_game_state();


#endif /* SERIAL_C_ */
