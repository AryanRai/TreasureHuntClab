
#ifndef POWERDIG_H_
#define POWERDIG_H_

#define BONUS_TIMER TIMER3  // or whatever your timer ID is


int GenerateRandVoltage( int min, int max);

static void bonus_timer_callback(const TimerSel sel, GameTriggers *triggers);

int power_dig_bonuspts(GameTriggers *triggers);

//static void bonus_timer_callback(TimerSel sel);


#endif /* POWERDIG_H_ */
