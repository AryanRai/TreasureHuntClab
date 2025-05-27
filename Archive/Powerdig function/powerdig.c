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
#include "powerdig.h"
#include "main.h"
#include "powerdig.h"


int GenerateRandVoltage( int min, int max){
	return (rand()%(max-min+1)) + min;
}

static int Seek_volt = 0;

static bool bonus_timer_done = false;

int final_trimpot_value = 0;




static void bonus_timer_callback(const TimerSel, GameTriggers *triggers) {
    final_trimpot_value = triggers->trimpot_value; // Capture exact value at timeout
    bonus_timer_done = true;
    timer_enable_set(BONUS_TIMER, false); // Disable the timer

}


int power_dig_bonuspts(GameTriggers *triggers) {
    Seek_volt = GenerateRandVoltage(0.1, 2.9);

    bonus_timer_done = false;

    //This timer needs to be set properly or deleted and updated timer code integrated
    // Start a one-shot timer for the bonus period
    timer_prescaler_set(BONUS_TIMER, 23976); // Approximately half a second
    timer_period_set(BONUS_TIMER, 1000);
    timer_silent_set(BONUS_TIMER, false);
    timer_recur_set(BONUS_TIMER, false);          // One-shot
    timer_callback_set(BONUS_TIMER, &bonus_timer_callback);
    timer_enable_set(BONUS_TIMER, true);

    while (!bonus_timer_done) {
    	//waste time
    }

    int diff = abs(final_trimpot_value - Seek_volt);

    return diff;
}





