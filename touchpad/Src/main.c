#include <stdint.h>
#include "touch.h"
#include "stm32f303xc.h"

void (*on_touch_detected)() = 0x00;

void chase_led(){
	uint8_t *led_register = ((uint8_t*)&(GPIOE->ODR)) + 1;

	*led_register <<= 1;
	if (*led_register == 0) {
		*led_register = 1;
	}
}

void initialise_board() {
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOEEN;
	// Set PE8–15 as outputs (LEDs)
	uint16_t *led_output_registers = ((uint16_t *)&(GPIOE->MODER)) + 1;
	*led_output_registers = 0x5555;

	// Ensure PA1 is configured as input (optional: it's default after reset)
	uint16_t *touch_registers = ((uint16_t *)&(GPIOA->MODER));
	*touch_registers = 0x0000;
}

void enable_touch_interrupt() {
	__disable_irq();

	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

	// Map EXTI1 to PA1
	SYSCFG->EXTICR[0] = SYSCFG_EXTICR1_EXTI1_PA;

	// Trigger on rising edge
	EXTI->RTSR |= EXTI_RTSR_TR1;
	EXTI->FTSR &= ~EXTI_FTSR_TR1;       // Optional: ensure falling edge not set

	// Unmask EXTI line 1
	EXTI->IMR |= EXTI_IMR_MR1;

	// Enable interrupt in NVIC
	NVIC_SetPriority(EXTI1_IRQn, 1);
	NVIC_EnableIRQ(EXTI1_IRQn);

	__enable_irq();
}

void EXTI1_IRQHandler(void)
{
	if (on_touch_detected != 0x00) {
		on_touch_detected();
	}
	// Clear the interrupt flag
	EXTI->PR |= EXTI_PR_PR1;
}

int main(void)
{
	initialise_board();

	on_touch_detected = &chase_led;

	enable_touch_interrupt();

	while (1) {}
}

