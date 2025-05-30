
#include <stdint.h>
#include "stm32f303xc.h"
#include <ctype.h>
#include "gamestate.h"
extern GameState game;

#define PRESCALER 0x3E7      // 999 + 1 = 1000
#define DELAY 0x1F40      // 8000 System block is 8mhz
#define DELAY2 0x7d0	// 2000
#define LED_FLASH_POS 3
volatile uint8_t* lights = ((uint8_t*)&(GPIOE->ODR)) + 1;
//extern volatile uint8_t* lights; //declared elsewhere


//static function pointer that stores a function
static void (*timer2_callback)(void) = 0; //set as 0 until we run timer_function_callback

// allow us to store a function into compare callback
void timer2_set_callback(void (*cb)(void)) {
	timer2_callback = cb;
}

static uint32_t timer2_period = 0; // Set base timer period

void set_timer2(uint32_t value) {
	timer2_period = value; }

uint32_t get_timer2(void) {
	return timer2_period; }


void enableTimer2()
{
	__disable_irq();

	// Enable the clock for Timer 2
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN_Msk;

	// Set Timer 2 Channel 1 to output compare
	TIM2->CCER |= TIM_CCER_CC1E_Msk;

	// Set output pin to toggle on successful output compare
	TIM2->CCMR1 |= TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1;

	// Value to be compared against
	TIM2->PSC = PRESCALER; // divide by 10001? Would make it 8000 hz
	TIM2->ARR = 0xFFFFFFFF; //count restarts at 4294967295 counts

	//Setting the CCR1 to 8000 means that the compare flags when CNT gets to 8000 which is 1 second with prescaler
	TIM2->CCR1 = timer2_period; // delay is 8000 so 8000 is 1 second.
	// Force update event to apply PSC and ARR. Because prescaler only takes effect an event
	TIM2->EGR |= TIM_EGR_UG; // Forces update event, immediately loads prescaler and ARR values

	//Interupts =================================================================================

	// Request an interrupt upon successful output compare
	TIM2->DIER |= TIM_DIER_CC1IE_Msk;

	// Set priority and enable interrupts
	NVIC_SetPriority(TIM2_IRQn, 1);
	NVIC_EnableIRQ(TIM2_IRQn);

	// Start the counter
	TIM2->CR1 |= TIM_CR1_CEN_Msk;

	__enable_irq();
}

void TIM2_IRQHandler()
{
	// Output compare Interrupt SR = status register of timer 2 and a bitmask checking the Capture/Compare 1 Interrupt Flag.
	if (TIM2->SR & TIM_SR_CC1IF_Msk)
	{
		// The next output compare should occur in the next 8000 clock cycles
		TIM2->CCR1 = TIM2->CNT + timer2_period; // changes CCR1 to the current count + the delay
		TIM2->SR = 0x00; // clear all status flags in interupt register
		//*lights = *lights ^ 1UL << LED_POS; //toggle LED on or off
		if(timer2_callback) {
			timer2_callback();
		}
		}
	}

// Pre-scaler updates only upon "events" so this triggers an overflow
// This is how it's done in the lectures. But I use the EGR register instead


void LEDflash(){
	//*lights = *lights ^ 1UL << LED_FLASH_POS; //toggle LED on or off
	static uint8_t state = 0;
		if (state) {
			*lights = 0x55;  // Turn 0b01010101 on
		} else {
			*lights = 0xAA;  // 0b10101010: On-Off-On-Off...
		}
		state ^= 1;  // Flip between 0 and 1
		game.game_time_remaining -= 1;
}

void disableTimer2() {
    TIM2->CR1 &= ~TIM_CR1_CEN_Msk;        // stop the timer
    TIM2->DIER &= ~TIM_DIER_CC1IE_Msk;    // disable interrupt
    TIM2->CNT = 0;  					// reset counter
    NVIC_DisableIRQ(TIM2_IRQn);           // disable NVIC interrupt, stops going to TIM2_IRQ handler
}

/*int valid_period_check(char *input){
    // Check for an empty string
    if (input == NULL || strlen(input) == 0) {
        return 0;
    }

    // Confirm every character is a digit
    for (int i = 0; input[i] != '\0'; i++) {
        if (!isdigit((unsigned char)input[i])) {
            return 0;
        }
    }
    return 1;
}*/
