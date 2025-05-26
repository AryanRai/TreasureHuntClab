/*
 * gamestate.c
 *
 *  Created on: May 18, 2025
 *      Author: 163910
 */

#include "gamestate.h"


int check_servo_choice(int correct_servos[], int servo_choice, int total_servos){
	for(int i = 0; i < total_servos; i++){
		if(correct_servos[i] == servo_choice){
	return(1);
        }
    }
    return 0;  // No match found after checking all
}
