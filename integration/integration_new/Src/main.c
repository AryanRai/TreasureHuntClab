#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "gpio.h"
#include "serial.h"
#include "stm32f303xc.h"
#include "structs.h"
#include "timer.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
#warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

// =================================== Global Variables ====================================
volatile int last_servo_selection  = -1;
volatile int last_servo_angle = -1;


// --- Game State Variables ---
GameState game = {
		.correct_servos = {1, 3, 4, 6}, //do not use servo number 0
		.items_found = 0,
		.items_left_to_find = 4,
		.digs_taken = 0,
		.digs_remaining = 4,
		.peeks_used = 0,
		.game_time_remaining = 2400,
		.game_over = 1,
		.total_items_to_find = 0
};

GameTriggers triggers = {
		.touchpad_pressed = -1,
		.magnet1_det = 0,
		.magnet2_det = 0,
		.servo_controlled = -1,
		.servo_angle = 0,
		.trimpot_value = 0,
		.peek_threshold = 10,
		.pending_peek = 0
};


// =================================== Callback Functions ===================================

// Each EXTI handler calls this with the corresponding pin number
void handle_touch(uint8_t pad, GameTriggers *trigger) {
	display_number(pad);
	trigger->touchpad_pressed = pad;
}

static void fn_a(const TimerSel sel, GameState *game) {
	game->game_time_remaining = game->game_time_remaining - 2;
    char buffer[64];
    sprintf(buffer, "TIME REMAINING:%d\r\n", game->game_time_remaining);
    serial_output_string(buffer, &USART1_PORT);
}

// Transmit callback
void output_callback() {
	return;
}

// Receive callback
void input_callback(char *data, uint32_t len) {

	serial_output_string((char *)"You typed: ", &USART1_PORT);
	serial_output_string(data, &USART1_PORT);					// Transmit back what was received
	serial_output_string((char *)"\r\n", &USART1_PORT);

	char compare[] = "game start";
	uint16_t test = strcmp(data, compare);
	if (!test) {
		start_game(&game);
	}
}

// =================================== Servo Functions ====================================
/*
// -------------------------------Most of these functions should be called from a header file
uint16_t read_magnet_signal() {
    // Dummy value (simulate metal nearby)
    return;
}

void led_feedback(uint16_t mag_val) {
    // Light up LEDs based on metal detection
    if (mag_val > 400) {
        // LED A
    } else {
        // Turn off
    }
}
*/

/*
float trimpot_control(uint8_t) {
    float idk = 6;
    return idk; // Simulate "dig"
}

uint8_t servo_peek(uint8_t pad_index) {
	serial_output_string((char *) "peaking\r\n", &USART1_PORT);

}

uint8_t servo_dig(uint8_t pad_index) {
	serial_output_string((char *) "digging\r\n", &USART1_PORT);
	//idk what will happen here
	uint8_t yes = 1;
	while (yes) {
		yes = trimpot_control(uint8_t)
	}

}

void evaluate_dig(uint8_t pad_index) {
    // Dummy logic to simulate hit/miss
    bool found_gold = (pad_index == 2); // fixed for demo

    if (found_gold) {
        treasures_remaining--;
        serial_output_string((char *) "Found gold!\r\n", &USART1_PORT);
    } else {
        serial_output_string((char *) "Missed!\r\n", &USART1_PORT);
    }

    digs_remaining--;
}


*/

void reset_all_servos() {
    serial_output_string((char *) "reset motors\r\n", &USART1_PORT);
}

// =================================== Game Functions ====================================
// Prints via UART game state
void transmit_game_state() {
    char buffer[64];
    sprintf(buffer, "DIGS REMAINING:%d TREASURES:%d\r\n", game.digs_remaining, game.items_left_to_find);
    serial_output_string(buffer, &USART1_PORT);
}

// --- Start Game Signal (from USART or button) ---
void start_game(GameState *game) {
    game->game_over = 0;
    game->game_time_remaining = 240;
    game->digs_remaining = 4;

    int count = 0;
    for (int i = 0; i < 6; i++) {
        if (game->correct_servos[i] != 0) {
            count++;
        }
    }
    game->total_items_to_find = count;

    reset_all_servos();

    timer_init();
    const TimerSel tim_a = TIMER_SEL_2;
    timer_prescaler_set(tim_a, 0xF00);
    timer_period_set(tim_a, 420);
    timer_silent_set(tim_a, false);
    timer_recur_set(tim_a, true);
    timer_callback_set(tim_a, &fn_a);
    timer_enable_set(tim_a, true);

    serial_output_string("Game Started\r\n", &USART1_PORT);

    transmit_game_state();

}

// Game variable update function
void update_game_state(uint8_t x, GameState *game, GameTriggers *triggers) {
	if (x == 1) {
		game->items_found++;
		game->digs_remaining--;
		game->digs_taken--;
		game->items_left_to_find--;
		transmit_game_state();
	} else {
		game->digs_remaining--;
		game->digs_taken--;
		transmit_game_state();
	}
}

// Main Game Loop
int main(void) {

	serial_initialise(BAUD_115200, &USART1_PORT, &output_callback, &input_callback);
	enable_interrupts(&USART1_PORT);

	initialise_touch();
	enable_touch_interrupts();
	touch_register_callback(&handle_touch);

    while (true) {
    	// Wait for game start
    	if (game.game_over) {
    		continue;
    	}

        // Game Over Conditions
        if (game.digs_remaining == 0 || game.game_time_remaining == 0 || game.items_left_to_find == 0) {

        	if (game.items_left_to_find == 0) {
        		serial_output_string((char *) "You Win!\n", &USART1_PORT);
        	}
        	else {
        		serial_output_string((char *) "Game Over\n", &USART1_PORT);
        	}

        	display_number(0);
            game.game_over = 1;

            //timer_disable();
            continue;
        }

        if (triggers.touchpad_pressed != -1) {
        	triggers.servo_controlled = triggers.touchpad_pressed;
            transmit_game_state();

            char buffer[64];
            sprintf(buffer, "touchpad %d chosen, door %d being controlled!\r\n", triggers.touchpad_pressed,  triggers.servo_controlled);
            serial_output_string(buffer, &USART1_PORT);


            if (triggers.servo_controlled != -1) {
            	if (triggers.servo_controlled != last_servo_selection) {

            		while (triggers.touchpad_pressed != -1) {
                		//just for testing unknown game logic for now
                		uint8_t x = 1;
                		if (x == 1) {
                			update_game_state(x, &game, &triggers);
                		}
                		else {
                			update_game_state(x, &game, &triggers);
                		}
                        //transmit_game_state();
                        triggers.touchpad_pressed = -1;
            		}

            		// im expecting the servo control function through trimpot to be in a while loop of a sort.
            		//Im not really sure how this is going to function with peak
            		//could try adding stevens code within the while loop

            	}
            }

        	triggers.touchpad_pressed = -1;
            last_servo_selection = triggers.servo_controlled;
        	triggers.servo_controlled = -1;


            char yes[64];
            sprintf(yes, "touchpad reset to %d, servo %d, previous servo %d\r\n", triggers.touchpad_pressed, triggers.servo_controlled, last_servo_selection);
            serial_output_string(yes, &USART1_PORT);

        }
    }
}


