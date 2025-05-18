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
	int items_found;
	int items_to_find;
	int digs_taken;
	int digs_remaining;
	int peeks_used;
	int game_time_remaining;
	bool game_over;
} GameState;

typedef struct{
	int touchpad_pressed;
	int magnet1_det;
	int magnet2_det;
	int servo_controlled;
} GameTriggers;


#endif /* GAMESTATE_H_ */
