#include <GPIO.h>


#include "stm32f303xc.h"




 const uint32_t adr_link[] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE};
 const uint32_t clock_mask_link[] = {RCC_AHBENR_GPIOAEN, RCC_AHBENR_GPIOBEN,
 								RCC_AHBENR_GPIOCEN, RCC_AHBENR_GPIODEN,
 								RCC_AHBENR_GPIOEEN};

 //Links pin-index with appropriate ADC config-reg (only Port A)
 //const uint32_t ADC_link[] = {ADC1, ADC2, ADC3, ADC4, ADC5, ADC6, ADC7};


 typedef struct _GPIO{

 	uint8_t PORT_IND; //Index of port (A:0, B:1 ...)
 	GPIO_TypeDef *PORT_ADR; //Pointer to base of GPIOx;
 	uint8_t MODE; //0: Input, 1: output
 	uint8_t PIN_LOWER; //Range of pins to R/W (0-15)
 	uint8_t PIN_UPPER;

 } GPIO;


 uint32_t create_mask(uint8_t start, uint8_t end) {
 	//Bitwise-logic trick to get 1's mask between start and end index
 	return ((1 << (end + 1)) - 1) ^ ((1 << start) - 1);                      // XOR to get the range
 }



 GPIO *init_port(port_name_link name, port_mode mode, uint8_t pin_lower, uint8_t pin_upper){

 	GPIO *port_pt = malloc(sizeof(GPIO)); //Pointer to Port

 	port_pt->PORT_IND = name;
 	port_pt->PORT_ADR = adr_link[name];
 	port_pt->MODE = mode;
 	port_pt->PIN_LOWER = pin_lower;
 	port_pt->PIN_UPPER = pin_upper;

 	//Enable clock for portX
 	uint32_t clock_en_mask = clock_mask_link[name];
 	RCC->AHBENR |= clock_en_mask;


 	uint32_t *port_mode_reg = ((uint32_t *) &(port_pt->PORT_ADR->MODER));

 	//Generate mode-mask

 	//First mask away the selected section (pin_range)
 	uint32_t temp_mask = create_mask(pin_lower*2, pin_upper*2 + 1);

 	//Sets the pin_range section to 0
 	*port_mode_reg &= ~temp_mask;


 	//Input-mode is enabled when each pin is 0b00 so no more masking is required

 	if (mode == OUTPUT){

 		//Similar idea to clear-mask, except shifting by 2 each time (0b01010101...)
 		uint32_t first_mask = (1 << pin_lower*2);

 		temp_mask = first_mask;
 		for (uint8_t i=pin_lower; i<pin_upper+1; i++){
 			temp_mask <<= 2;
 			temp_mask |= first_mask;
 		}

 		*port_mode_reg |=temp_mask;

 	} else if (mode == ANALOG){
 		//All 1s for analog mode (conveniently same as clear-mask)

 		*port_mode_reg |= temp_mask;

 		//ONLY AVAILABLE FOR PA0 - PA3 !!!!!!

 		// enable the clock for ADC1
		RCC->AHBENR |= RCC_AHBENR_ADC12EN;

		// set to synchronise the ADC with the clock
		ADC12_COMMON->CCR |= ADC12_CCR_CKMODE_0;

		// ADEN must be = 0 for configuration (is the default)
		ADC2->CR &= ~ADC_CR_ADVREGEN; // clear voltage regulator enable
		ADC2->CR |= ADC_CR_ADVREGEN_0; // set ADVREGEN TO 01
		ADC2->CR &= ~ADC_CR_ADCALDIF; // clear bit to enable Single-ended-input

		// calibrate the ADC (self calibration routine)
		ADC2->CR |= ADC_CR_ADCAL;
		while((ADC2->CR & ADC_CR_ADCAL) == ADC_CR_ADCAL); // Waiting for the calibration to finish




		//Clear channel-sequence
		ADC2->SQR1 = 0;

		//Add each pin/channel to the conversion-sequence
		uint8_t pin; //Index of pin to add in seq
		for (uint8_t i=0; i < pin_upper-pin_lower +1; i++){
			pin = pin_lower + i;
			ADC2->SQR1 |= (pin-3) << 6*(i+1); // set the request for channel x
		}


		ADC2->SQR1 |= (pin_upper-pin_lower) << ADC_SQR1_L_Pos; // set the number of channels to read (number of pins in range)



		// single shot mode
		ADC2->CFGR &= ~ADC_CFGR_CONT;

		// Enable the ADC
		ADC2->CR |= ADC_CR_ADEN;

		// Wait the ADC to be ready.
		while (!(ADC2->ISR & ADC_ISR_ADRDY));

 	}


 	//Port has been initialised!! - return the port-struct
 	return port_pt;
 }




 //Read pins on port (INPUT mode)
 uint16_t read_pins(GPIO *port_pt){
	if (port_pt->MODE == INPUT){
		//Load state of all pins (16-bit)
		uint16_t pin_states = port_pt->PORT_ADR->IDR;

		//Shift forwads then back to shave off non-selected pins
		pin_states <<= 15 - port_pt->PIN_UPPER;
		pin_states >>= 15 - port_pt->PIN_UPPER + port_pt->PIN_LOWER;

		return pin_states;

	}

	//Invalid mode for pin-reading (OUTPUT/ANALOG)
	return 0;

 }

 //Read analog pins (writes to pt)
void read_pins_analog(GPIO *port_pt, uint16_t *dest_pt){
	 if (port_pt->MODE == ANALOG){
		// request the process to start
		ADC2->CR |= ADC_CR_ADSTART;

		uint8_t i = 0;
		while (!(ADC2->ISR & ADC_ISR_EOS)) {
		    while (!(ADC2->ISR & ADC_ISR_EOC));  // Wait for conversion
		    dest_pt[i++] = ADC2->DR;               // Reading clears EOC
		}

		// Clear EOS if needed
		ADC2->ISR |= ADC_ISR_EOS;


	}

	 return;
 }


 //Read single bit of data from specified pin (INPUT mode)
 //Note: ADC is designed to convert entire sequence at once so single_pin-reading isn't available
 uint8_t read_single_pin(GPIO *port_pt, uint8_t pin_index){
 	//Check if targeted-idnex is within range
 	if (pin_index < port_pt->PIN_LOWER || port_pt->PIN_UPPER < pin_index){
 		return;
 	}
 	//Load state of all pins (16-bit)
 	uint16_t pin_states = port_pt->PORT_ADR->IDR;

 	//Return bit at pin-index
 	return pin_states & (1 << pin_index);
 }



 //Write to pins on port (OUTPUT mode)
 void write_pins(GPIO *port_pt, uint16_t data){
 	//Shift data in position of lower-pin
 	data <<= port_pt->PIN_LOWER;

 	//Load pointer to ODR for clarity
 	uint16_t *odr_pt = &port_pt->PORT_ADR->ODR;

 	//Clear pin-bits from lower-upper
 	*odr_pt &= ~create_mask(port_pt->PIN_LOWER, port_pt->PIN_UPPER);
 	//Load data
 	*odr_pt |= data;

 	return;
 }



 //Write a single bit of data - at specified index (OUTPUT mode)
 void write_single_pin(GPIO *port_pt, uint8_t single_bit, uint8_t pin_index){
 	//Check if targeted-idnex is within range
 	if (pin_index < port_pt->PIN_LOWER || port_pt->PIN_UPPER < pin_index){
 		return;
 	}

 	//Load pointer to ODR for clarity
 	uint16_t *odr_pt = &port_pt->PORT_ADR->ODR;

 	//Clear bit at index
 	*odr_pt &= ~(1 << pin_index);
 	//Write new-bit
 	*odr_pt  |= single_bit << pin_index;

 	return;
 }


 //Pointers to callback-funcitons (for each of 16 pins)
 void (*EXTI_Callbacks[16])(uint8_t pin_index) = {0x00};
 //Wish this wasn't necessary, but each handler function has to be defined seperately
 //Is there a better way to do this? There doesn't seem to be a generic IQRHandler..
 void EXTI0_IRQHandler(void) {EXTI->PR |= (1 << 0); if (EXTI_Callbacks[0]) EXTI_Callbacks[0](0);}
 void EXTI1_IRQHandler(void) {EXTI->PR |= (1 << 1); if (EXTI_Callbacks[1]) EXTI_Callbacks[1](1);}
 void EXTI2_IRQHandler(void) {EXTI->PR |= (1 << 2); if (EXTI_Callbacks[2]) EXTI_Callbacks[2](2);}
 void EXTI3_IRQHandler(void) {EXTI->PR |= (1 << 3); if (EXTI_Callbacks[3]) EXTI_Callbacks[3](3);}
 void EXTI4_IRQHandler(void) {EXTI->PR |= (1 << 4); if (EXTI_Callbacks[4]) EXTI_Callbacks[4](4);}
 //Must find which pin in 5-9 range triggered handler
 void EXTI9_5_IRQHandler(void) {
     for (uint8_t i = 5; i <= 9; i++) {
         if (EXTI->PR & (1 << i)) {
             EXTI->PR |= (1 << i);  // Clear pending flag
             if (EXTI_Callbacks[i]) EXTI_Callbacks[i](i);  // Call user-defined function
         }
     }
 }

 void EXTI15_10_IRQHandler(void) {
     for (uint8_t i = 10; i <= 15; i++) {
         if (EXTI->PR & (1 << i)) {
             EXTI->PR |= (1 << i);  // Clear pending flag
             if (EXTI_Callbacks[i]) EXTI_Callbacks[i](i);  // Call user-defined function
         }
     }
 }




 void enable_interupt(GPIO *port_pt, uint8_t pin_index,
 					trigger_type trigger, uint8_t priority,
 					void (*interupt_handler)(uint8_t pin_index)){

 	//Link interupt_handler function to appropriate EXTI_Callback
 	EXTI_Callbacks[pin_index] = interupt_handler;

 	// Disable the interrupts while messing around with the settings
 	//  otherwise can lead to strange behaviour
 	__disable_irq();

 	// Enable the system configuration controller (SYSCFG in RCC)
 	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

 	//Tell pin-0 multiplexer to target selected-port (PORT_IND)

 	//If pin 0-3 or 12-15 :

 	uint32_t exticr_shift = 4 * (pin_index % 4);
 	SYSCFG->EXTICR[pin_index / 4] &= ~(0xF << exticr_shift); // clear old bits
 	SYSCFG->EXTICR[pin_index / 4] |= (port_pt->PORT_IND << exticr_shift); // set new bits

 	//  Select interrupt on rising/falling edge (pins are consecutive bits)
 	if (trigger==RISING_EDGE){
 		//Set rising-edge
 		EXTI->RTSR |= (1 << pin_index);
 	} else{
 		//Set falling-edge
 		EXTI->FTSR |= (1 << pin_index);
 	}


 	// set the interrupt from EXTI line x as 'not masked' - as in, enable it.
 	EXTI->IMR |= (1 << pin_index);


 	// Tell the NVIC module that EXTIx interrupts should be handled

 	//Pins 0-4 have a seperate request-num (from 6-10),
 	//pins 5-9 are all at 23, and pins 10-15 are at 40
 	uint8_t req_num = (pin_index <= 4) ? (6 + pin_index) :
 					  (pin_index <= 9) ? 23 : 40;

 	NVIC_SetPriority(req_num, priority);  // Set Priority
 	NVIC_EnableIRQ(req_num);

 	// Re-enable all interrupts (now that we are finished)
 	__enable_irq();

 }
