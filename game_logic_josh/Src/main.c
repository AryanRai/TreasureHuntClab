#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "gpio.h"
#include "serial.h"
#include "stm32f303xc.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
#warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif


// --- Game State Variables ---
volatile uint8_t digs_remaining = 3;
volatile uint16_t time_remaining = 60;  // seconds
volatile uint8_t treasures_remaining = 3;
volatile bool game_running = false;
volatile bool touch_pressed = false;
volatile uint32_t tick_ms = 0;


// --- Start Game Signal (from USART or button) ---
void start_game() {
    game_running = true;
    time_remaining = 60;
    digs_remaining = 3;
    treasures_remaining = 3;

    reset_all_servos();
    usart_send("Game Started\n");
}

// Transmit callback
void output_callback() {
	// Delay
	for (volatile uint32_t i = 0; i < 0x9ffff; i++) {
	}
}


// Receive callback
void input_callback(char *data, uint32_t len) {

	serial_output_string((char *)"You typed: ", &USART1_PORT);
	serial_output_string(data, &USART1_PORT);					// Transmit back what was received
	serial_output_string((char *)"\r\n\n", &USART1_PORT);

	if (data == "game start") {
		start_game();
	}
}

// -------------------------------Most of these functions should be called from a header file
uint16_t read_magnet_signal() {
    // Dummy value (simulate metal nearby)
    return;
}

float read_trimpot() {
    // Return ADC voltage or mapped value
    return; // Simulate "dig"
}

void led_feedback(uint16_t mag_val) {
    // Light up LEDs based on metal detection
    if (mag_val > 400) {
        // LED A
    } else {
        // Turn off
    }
}

void servo_peek(uint8_t pad_index) {
    // Slightly open servo at given index
}

void servo_dig(uint8_t pad_index) {
    // Fully open trapdoor
}

void reset_all_servos() {
    // Reset trapdoors
}

void display_number(uint8_t n) {
    // Display `n` on PE8–PE11 (4-bit binary)
    GPIOE->ODR &= ~(0xF << 8);        // Clear PE8–11
    GPIOE->ODR |= ((n & 0xF) << 8);   // Set new value
}

// Each EXTI handler calls this with the corresponding pin number
void handle_touch(uint8_t pin) {
    display_number(pin); // Display the number of the pin touched (1–6)
    float control = read_trimpot();
     if (control < 0.5f) {
         servo_peek(pad_index);
     } else {
         servo_dig(pad_index);
         evaluate_dig(pad_index);
     }

}
// -------------------------------Most of these functions should be called from a header file


// -------------------------------Game Logic Modules

void evaluate_dig(uint8_t pad_index) {
    // Dummy logic to simulate hit/miss
    bool found_gold = (pad_index == 2); // fixed for demo

    if (found_gold) {
        treasures_remaining--;
        serial_output_string("Found gold!\n");
    } else {
        serial_output_string("Missed!\n");
    }

    digs_remaining--;
}

void update_game_state() {
    char buffer[64];
    sprintf(buffer, "TIME:%d DIGS:%d TREASURES:%d\n", time_remaining, digs_remaining, treasures_remaining);
    serial_output_string(buffer, &USART1_PORT);
}

// --- Can someone make a timer module sort of thing im very bad at timers ---
void timeidk(void) {

}

int main(void) {

	serial_initialise(BAUD_115200, &USART1_PORT, &output_callback, &input_callback);
	enable_interrupts(&USART1_PORT);

	initialise_touch();
	enable_touch_interrupts();

    while (1) {
        if (!game_running) {
            // Wait for USART command or button
            // if (ustart_game(); }
            continue;
        }

        // Game Over Conditions
        if (digs_remaining == 0 || time_remaining == 0 || treasures_remaining == 0) {

        	if (treasures_remaining == 0) {
        		serial_output_string((char *) "You Win!\n", &USART1_PORT);
        	}
        	else {
        		serial_output_string((char *) "Game Over\n", &USART1_PORT);
        	}

        	display_number(0);
            game_running = false;

            continue;
        }

        if (touch_pressed) {
        	touch_pressed = false;
        }
        update_game_state();
}




// Interrupt handlers - i know it looks messy but i cant fine another way to make it work :/
void EXTI1_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR1) {
        EXTI->PR |= EXTI_PR_PR1;
        touch_pressed = true;
        handle_touch(1);
    }
}
void EXTI2_TSC_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR2) {
        EXTI->PR |= EXTI_PR_PR2;
        touch_pressed = true;
        handle_touch(2);
    }
}
void EXTI3_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR3) {
        EXTI->PR |= EXTI_PR_PR3;
        touch_pressed = true;
        handle_touch(3);
    }
}
void EXTI4_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR4) {
        EXTI->PR |= EXTI_PR_PR4;
        touch_pressed = true;
        handle_touch(4);
    }
}

void EXTI9_5_IRQHandler(void) {
    for (int i = 5; i <= 6; i++) {
        if (EXTI->PR & (1 << i)) {
            EXTI->PR |= (1 << i);
            touch_pressed = true;
            handle_touch(i);
        }
    }
}

