/*
 * gamestate.h
 *
 *  Created on: May 18, 2025
 *      Author: 163910
 */

#ifndef GAMESTATE_H_
#define GAMESTATE_H_

#include <stdbool.h>


typedef struct{
	int correct_servos[6];
	int items_found;
	int items_left_to_find;
	int digs_taken;
	int digs_remaining;
	int peeks_used;
	int game_time_remaining;
	int game_over;
	int total_items_to_find;
} GameState;


typedef struct{
	volatile int touchpad_pressed;
	volatile int magnet1_det;
	volatile int magnet2_det;
	volatile int servo_controlled;
	volatile int servo_angle;
	volatile int trimpot_value;
	volatile int peek_threshold;
	volatile int pending_peek;
} GameTriggers;

int check_servo_choice();


#endif /* GAMESTATE_H_ */
