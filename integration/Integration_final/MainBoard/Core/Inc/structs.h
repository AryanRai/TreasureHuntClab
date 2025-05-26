#ifndef GAMESTATE_H_
#define GAMESTATE_H_

#include <stdbool.h>


typedef struct{
	volatile int correct_servos[6];
	volatile int items_found;
	volatile int items_left_to_find;
	volatile int digs_taken;
	volatile int digs_remaining;
	volatile int peeks_used;
	volatile int game_time_remaining;
	volatile int game_over;
	volatile int total_items_to_find;
	volatile int current_score;
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


#endif /* GAMESTATE_H_ */
