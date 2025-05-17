#include <stdint.h>
#include "stm32f303xc.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

void display_number(uint8_t n) {
    // Display `n` on PE8–PE11 (4-bit binary)
    GPIOE->ODR &= ~(0xF << 8);        // Clear PE8–11
    GPIOE->ODR |= ((n & 0xF) << 8);   // Set new value
}

void initialise_board() {
    // Enable clocks
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOEEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // Configure PE8–PE11 as output
	uint16_t *led_output_registers = ((uint16_t *)&(GPIOE->MODER)) + 1;
	*led_output_registers = 0x5555;

    // Set PA1–PA6 as inputs
	uint16_t *touch_registers = ((uint16_t *)&(GPIOA->MODER));
	*touch_registers = 0x0000;
}

void enable_touch_interrupts() {
	__disable_irq();

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // Map EXTI lines 0–3 to PA1–PA3
    SYSCFG->EXTICR[0] = SYSCFG_EXTICR1_EXTI1_PA | SYSCFG_EXTICR1_EXTI2_PA | SYSCFG_EXTICR1_EXTI3_PA;
    // Map EXTI lines 4–7 to PA4–PA6
    SYSCFG->EXTICR[1] = SYSCFG_EXTICR2_EXTI4_PA | SYSCFG_EXTICR2_EXTI5_PA | SYSCFG_EXTICR2_EXTI6_PA;
    //SYSCFG->EXTICR[1] &= ~((0xF << 0) | (0xF << 4) | (0xF << 8)); // PA4–6 => EXTI4–6


	// Trigger on rising edge and stop falling edge
	EXTI->RTSR |= EXTI_RTSR_TR1 | EXTI_RTSR_TR2 | EXTI_RTSR_TR3 | EXTI_RTSR_TR4 | EXTI_RTSR_TR5 | EXTI_RTSR_TR6;
	EXTI->FTSR &= ~(EXTI_FTSR_TR1 | EXTI_FTSR_TR2 | EXTI_FTSR_TR3 |
	                EXTI_FTSR_TR4 | EXTI_FTSR_TR5 | EXTI_FTSR_TR6);      

	// Unmask EXTI line 1
	EXTI->IMR |= EXTI_IMR_MR1 | EXTI_IMR_MR2 | EXTI_IMR_MR3 | EXTI_IMR_MR4 | EXTI_IMR_MR5 | EXTI_IMR_MR6;

    // Enable NVIC IRQs for EXTI1–EXTI3 and EXTI4–9 (shared)
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_TSC_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
	__enable_irq();
}

// Each EXTI handler calls this with the corresponding pin number
void handle_touch(uint8_t pin) {
    display_number(pin); // Display the number of the pin touched (1–6)
}

// Interrupt handlers
void EXTI1_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR1) {
        EXTI->PR |= EXTI_PR_PR1;
        handle_touch(1);
    }
}
void EXTI2_TSC_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR2) {
        EXTI->PR |= EXTI_PR_PR2;
        handle_touch(2);
    }
}
void EXTI3_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR3) {
        EXTI->PR |= EXTI_PR_PR3;
        handle_touch(3);
    }
}
void EXTI4_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR4) {
        EXTI->PR |= EXTI_PR_PR4;
        handle_touch(4);
    }
}

void EXTI9_5_IRQHandler(void) {
    for (int i = 5; i <= 6; i++) {
        if (EXTI->PR & (1 << i)) {
            EXTI->PR |= (1 << i);
            handle_touch(i);
        }
    }
}

int main(void) {
    initialise_board();
    enable_touch_interrupts();

    while (1) {
        // Main loop does nothing; interrupt-driven
    }
}
