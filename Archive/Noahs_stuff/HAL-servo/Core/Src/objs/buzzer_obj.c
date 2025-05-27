#include "buzzer_obj.h"






typedef struct {
	uint16_t TEMPO; //Tempo of note-seq
	uint32_t CLOCK_SPEED; //Speed of Tim2 clock after prescaler
	uint8_t BASE_FREQ; //Base-freq that note-feqs are calculated from
	uint8_t BASE_NOTE; //Associated midi-note for the base-freq

	MidiNoteEvent *MIDI_SEQ; //Pointer to midi-sequence
	uint16_t EVENT_INDEX;

} buzzerObj;


//Initialize buzzer-obj
//Note: Assumes that pwm has been initialized on PA14
buzzerObj *init_buzzer(uint16_t tempo, float base_freq, uint8_t base_note){
	buzzerObj *buzzer_pt = malloc(sizeof(buzzerObj));

	//This is the frequency that other notes are calculated from (note is a semi-tone offset)
	//Typically A4 (400.0Hz note 69 in MIDI)
	buzzer_pt->BASE_FREQ = base_freq;
	buzzer_pt->BASE_NOTE = base_note;

	//Tempo for note-seq
	buzzer_pt->TEMPO = tempo;

	return buzzer_pt;
}


//Set a note (offset from the base-frequency
void buzzer_set_note(buzzerObj *buzzer_pt, int8_t note){
	//Calculate the semi-tone offset from base-freq
	uint8_t semi_off = note - buzzer_pt->BASE_NOTE;

	//Calculate the frequency (an exponential projection from the base-freq)
	buzzer_set_freq(buzzer_pt, buzzer_pt->BASE_FREQ * pow(2.0f, semi_off/12.0f));
	return;
}


//Update the pwm-output to produce a specified frequency
void buzzer_set_freq(buzzerObj *buzzer_pt, uint16_t freq){
	uint16_t period = buzzer_pt->CLOCK_SPEED/freq - 1;

	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
	htim2.Instance->ARR = period;
	htim2.Instance->CCR1 = period / 2;  // 50% duty cycle
	htim2.Instance->EGR = TIM_EGR_UG;  // Force update

	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	return;
}


//Turn the buzzer off
void buzzer_mute(buzzerObj *buzzer_pt){
	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
	return;
}




//Warning overrides TIM3 and uses its update-interupt for note-seq
//Inits a midi-note seq (with option of looping)
void buzzer_init_midi_seq(buzzerObj *buzzer_pt, MidiNoteEvent *midi_seq, uint16_t seq_len, uint8_t loop_flag){
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->PSC = 47999;    // 48 MHz / (47999+1) = 1 kHz
    TIM3->EGR |= TIM_EGR_UG;
    TIM3->ARR = 1;      // Instantly call interupt handler to start note sequence

    TIM3->DIER |= TIM_DIER_UIE;
    TIM3->CR1 |= TIM_CR1_CEN;

    NVIC_SetPriority(TIM3_IRQn, 1);
    NVIC_EnableIRQ(TIM3_IRQn);
}



//
void TIM3_IRQHandler(void) {
    if (TIM3->SR & TIM_SR_UIF) {
        TIM3->SR &= ~TIM_SR_UIF;  // Clear the flag

        if (EVENT_INDEX%2){
        	//Pause event
        	pause_buzzer()
        } else{
        	//Start note event
        	play_note(note[EVENT_INDEX/2]);
        }

        TIM3->ARR = how_long_till_next_event;      //
        TIM3->CNT = 0;

        EVENT_INDEX++;
    }
}

