#include "gpio.h"
#include "stm32f303xc.h"

void initialise_touch(void (*_callback_function)) {
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



