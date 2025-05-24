#include <serial.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "stm32f303xc.h"
#include "gamestate.h"
#include "gpio.h"
// #include <timer_module.h>

/*
#include <serial module header>
        variables
                send_serial_1
                receive_serial_1
#include <scoreboard display module header>
        variables

#include <LED module header>
#include <magnet detector module>
#include <servo module header>
#include <touchpad module header>
#include <trimpot module header>
*/

/*typedef struct{
        int items_found;
        int items_to_find;
        int digs_taken;
        int digs_remaining;
        int peeks_used;
        int game_time_remaining;
        bool game_over;
} GameState;*/
int message_complete = 0;
int touchpad_interrupt = 1;
int last_servo_angle = -1;
int last_servo_selection = -1;

// For Testing ========
char feedback_string[BUFFER];

// For Testing ========

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


#if !defined(__SOFT_FP__) && defined(__ARM_FP)
#warning \
    "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif
void display_number(uint8_t n) {
    // Display `n` on PE8–PE11 (4-bit binary)
    GPIOE->ODR &= ~(0xF << 8);        // Clear PE8–11
    GPIOE->ODR |= ((n & 0xF) << 8);   // Set new value
}
// Each EXTI handler calls this with the corresponding pin number
void handle_touch(uint8_t pad, GameTriggers *trigger) {
	display_number(pad);
	trigger->touchpad_pressed = pad;
}



int main(void) {
  // For Testing =======
  char* command = NULL;
  char* follow = NULL;
  // For Testing =======

  enableUSART1();
  enableUARTInterrupts();
  clear_screen();

  send_string("hello world");
  clear_screen();

  initialise_touch();
  enable_touch_interrupts();

  GameState game = {
			.correct_servos = {1, 3, 4, 6}, //do not use servo number 0
			.items_found = 0,
			.items_left_to_find = 4,
			.digs_taken = 0,
			.digs_remaining = 4,
			.peeks_used = 0,
			.game_time_remaining = 240,
			.game_over = 0,
			.total_items_to_find = 0
  };

  //Count item numbers
  int count = 0;
  for (int i = 0; i < 6; i++) {
      if (game.correct_servos[i] != 0) {
          count++;
      }
  }
  game.total_items_to_find = count;



  send_string("Game Initialised\r\n");
  // print_game_state(game);
  // print_game_triggers(triggers);

  // triggers.touchpad_pressed = touchpad_interrupt;

  // Loop forever
  while (game.game_over == 0) {
    if (message_complete == 1) {
      command = strtok((char*)string, " ");  // load commmand
      follow = strtok(NULL, "");             // load
      //		      message_complete = 0;  // need to reset this for
      //next message
      clear_screen();
    }
    if (command != NULL && message_complete == 1) {
      if (strcmp(command, "touchpad") == 0 && valid_period_check(follow) != 0) {

        sprintf(feedback_string, "Touchpad set to: %s \r\n", follow);
        send_string(feedback_string);
        triggers.touchpad_pressed = atoi(follow);
        memset(feedback_string, 0, BUFFER);
        send_string("Touchpad set to ");
        send_string_buffer(triggers.touchpad_pressed);
        send_string("\r\n");
        //		          print_game_triggers(triggers);
        message_complete = 0;

      } else if (strcmp(command, "servo") == 0 &&
                 valid_period_check(follow) != 0) {

        sprintf(feedback_string, "Servo angle set to: %s \r\n", follow);
        send_string(feedback_string);
        triggers.servo_angle = atoi(follow);
        triggers.trimpot_value = atoi(follow);  // Remove this later
        memset(feedback_string, 0, BUFFER);
        send_string("Servo set to ");
        send_string_buffer(triggers.servo_angle);
        send_string("\r\n");
        // print_game_triggers(triggers);
        message_complete = 0;

      } else if (strcmp(command, "trimpot") == 0 &&
                 valid_period_check(follow) != 0) {

        sprintf(feedback_string, "Trimpot Value set to: %s \r\n", follow);
        send_string(feedback_string);
        triggers.trimpot_value = atoi(follow);
        memset(feedback_string, 0, BUFFER);
        send_string("Trimpot set to ");
        send_string_buffer(triggers.trimpot_value);
        send_string("\r\n");

        // print_game_triggers(triggers);
        message_complete = 0;

      } else if (strcmp(command, "triggers") == 0){
    	  print_game_triggers(triggers);
    	  message_complete = 0;
      } else if (strcmp(command, "game") == 0){
    	  print_game_state(game);
    	  message_complete = 0;
      } else {
        send_string("Unknown command\r\n");
        message_complete = 0;
      }
      reset_input_buffer();
      command = NULL;
      follow = NULL;
      message_complete = 0;
      memset(feedback_string, 0, BUFFER);
    }

    // Need an interrupt to change triggers.touchpad_pressed = -1 when touchpad
    // is pressed
    if (triggers.touchpad_pressed != -1) {
      // send_string("\n\ntouchpad pressed\r\n");
      triggers.servo_controlled = triggers.touchpad_pressed;
      /*send_string("Touchpad Pressed trigged, Servo being controlled ");
      send_string_buffer(triggers.servo_controlled);
      send_string("\r\n");
      touctriggers.touchpad_pressed = -1;
      */
      // print_game_triggers(triggers);

      if (triggers.servo_controlled != -1) {
    	  if (triggers.servo_controlled != last_servo_selection) {
    	          if(triggers.pending_peek == 1){
    	        	game.peeks_used += 1;
					send_string("New Peek Used\r\n");
					print_game_state(game);
					triggers.pending_peek = 0;
    	          } // Reset peek
    	          triggers.servo_angle = 0;
    	          last_servo_selection = triggers.servo_controlled;
    	      }


          if (triggers.servo_angle != last_servo_angle) {

              // Respond to angle
              if (triggers.servo_angle == 0) {
                  send_string("Door Closed\r\n");


                  if (triggers.pending_peek == 1 && last_servo_selection == triggers.servo_controlled) { // only change peek if dig is not passed
                      game.peeks_used += 1;
                      send_string("New Peek Used\r\n");
                      print_game_state(game);
                      triggers.pending_peek = 0; // Reset peek
                  }
              } else if (triggers.servo_angle > 0 && triggers.servo_angle < triggers.peek_threshold) {
                  send_string("Below Dig Threshold. But above 0. Peek in Progress.\r\n");

                  // Only increment peek count if coming *from* a closed or dig state
                  if (last_servo_angle == 0 && last_servo_selection == triggers.servo_controlled) {
                	  //game.peeks_used += 1;
                      //send_string("New Peek Used\r\n");
                	  //print_game_state(game);
                	  triggers.pending_peek = 1;
                  }

              } else if (triggers.servo_angle >= triggers.peek_threshold) {
                  send_string("Dig Threshold Passed.\r\n");


                  // Only increment dig count if last angle was not already a dig
                  if (last_servo_angle < triggers.peek_threshold && last_servo_selection == triggers.servo_controlled) {
                      game.digs_taken += 1;
                      if (check_servo_choice(game.correct_servos, triggers.servo_controlled, game.total_items_to_find) == 1) {
                              game.items_found++;
                              game.items_left_to_find--;
                          }
                      send_string("New Dig Used\r\n");
                      print_game_state(game);

                  }
                  triggers.pending_peek = 0;
              }

              // Update the last angle
              last_servo_angle = triggers.servo_angle;
              last_servo_selection =triggers.servo_controlled;
          }
      }
    }
}
  return 0;
}

// Each EXTI handler calls this with the corresponding pin nu
// Interrupt handlers
void EXTI1_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR1) {
        EXTI->PR |= EXTI_PR_PR1;
        handle_touch(1, &triggers);
    }
}
void EXTI2_TSC_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR2) {
        EXTI->PR |= EXTI_PR_PR2;
        handle_touch(2, &triggers);
    }
}
void EXTI3_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR3) {
        EXTI->PR |= EXTI_PR_PR3;
        handle_touch(3, &triggers);
    }
}
void EXTI4_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR4) {
        EXTI->PR |= EXTI_PR_PR4;
        handle_touch(4, &triggers);
    }
}

void EXTI9_5_IRQHandler(void) {
    for (int i = 5; i <= 6; i++) {
        if (EXTI->PR & (1 << i)) {
            EXTI->PR |= (1 << i);
            handle_touch(i, &triggers);
        }
    }
}
