#include "serial_josh.h"
#include "stm32f303xc.h"
#include <string.h>

// Global variable defines
#define TERMINATE '\r'
#define BUFFER_SIZE 64

static char DOUBLE_INPUT_BUFFER[2][BUFFER_SIZE];
static uint32_t COUNTER = 0;
static uint8_t ACTIVE_RX_BUFFER = 0;


// Struct for GPIO and UART to be used
struct _SerialPort {
	USART_TypeDef *UART;
	GPIO_TypeDef *GPIO;
	volatile uint32_t MaskAPB2ENR;	// mask to enable RCC APB2 bus registers
	volatile uint32_t MaskAPB1ENR;	// mask to enable RCC APB1 bus registers
	volatile uint32_t MaskAHBENR;	// mask to enable RCC AHB bus registers
	volatile uint32_t SerialPinModeValue;
	volatile uint32_t SerialPinSpeedValue;
	volatile uint32_t SerialPinAlternatePinValueLow;
	volatile uint32_t SerialPinAlternatePinValueHigh;
	void (*output_callback)(void);
	void (*receive_callback)(char *, uint32_t);
};


// Instantiate the serial port parameters
SerialPort USART1_PORT = {
		USART1,						// USART1 for USB function
		GPIOC,						// Using GPIOC
		RCC_APB2ENR_USART1EN, 		// bit to enable for APB2 bus
		0x00,						// bit to enable for APB1 bus
		RCC_AHBENR_GPIOCEN, 		// bit to enable for AHB bus
		0xA00,						// GPIO mode
		0xF00,						// GPIO speed
		0x770000,  					// for USART1 PC10 and 11, this is in the AFR low register
		0x00, 						// no change to the high alternate function register
		0x00 						// default function pointer is NULL
};


// InitialiseSerial - Initialise the serial port // Input: baud_rate is from an enumerated set
void serial_initialise(uint32_t baud_rate, SerialPort *serial_port, void (*output_callback_function)(void), void (*input_callback_function)(char *, uint32_t)) {

	serial_port->output_callback = output_callback_function;
	serial_port->receive_callback = input_callback_function;

	// Enable clock power, system configuration clock and GPIOC
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

	// Enable the GPIO which is on the AHB bus
	RCC->AHBENR |= serial_port->MaskAHBENR;

	// Set pin mode to alternate function for the specific GPIO pins
	serial_port->GPIO->MODER = serial_port->SerialPinModeValue;

	// Enable high speed clock for specific GPIO pins
	serial_port->GPIO->OSPEEDR = serial_port->SerialPinSpeedValue;

	// Set alternate function to enable USART to external pins
	serial_port->GPIO->AFR[0] |= serial_port->SerialPinAlternatePinValueLow;
	serial_port->GPIO->AFR[1] |= serial_port->SerialPinAlternatePinValueHigh;

	// Enable the device based on the bits defined in the serial port definition
	RCC->APB1ENR |= serial_port->MaskAPB1ENR;
	RCC->APB2ENR |= serial_port->MaskAPB2ENR;

	// Get a pointer to the 16 bits of the BRR register that we want to change
	uint16_t *baud_rate_config = (uint16_t*)&serial_port->UART->BRR; // only 16 bits used!

	// Baud rate calculation
	switch(baud_rate){
	case BAUD_9600:
		*baud_rate_config = 0x342;
		break;
	case BAUD_19200:
		*baud_rate_config = 0x1A1;
		break;
	case BAUD_38400:
		*baud_rate_config = 0xD1;
		break;
	case BAUD_57600:
		*baud_rate_config = 0x8B;
		break;
	case BAUD_115200:
		*baud_rate_config = 0x46;
		break;
	}

	// Enable serial port for tx and rx
	serial_port->UART->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}


// Output char using polling
void serial_output_char(char data, SerialPort *serial_port) {

	while((serial_port->UART->ISR & USART_ISR_TXE) == 0){
	}

	serial_port->UART->TDR = data;
}


// Output string using polling
void serial_output_string(char *string, SerialPort *serial_port) {

	uint32_t count = 0;
	while(*string) {
		serial_output_char(*string, serial_port);
		count++;
		string++;
	}

	// Callback function pointer call
	if (serial_port->output_callback != NULL)
		serial_port->output_callback();
}


// Enable interrupts needed for UART
void enable_interrupts(SerialPort *serial_port) {
	__disable_irq();

	// Interrupt upon receiving data
	serial_port->UART->CR1 |= USART_CR1_RXNEIE_Msk;
	//serial_port->UART->CR1 |= USART_CR1_TXEIE_MSK;

	// Set priority and enable interrupts
	NVIC_SetPriority(USART1_IRQn, 1);
	NVIC_EnableIRQ(USART1_IRQn);

	__enable_irq();
}


// Function executed when interrupt called
// Double buffer implementation
void USART1_EXTI25_IRQHandler() {
	// Check and handle overrun or frame errors
	if ((USART1_PORT.UART->ISR & USART_ISR_FE_Msk) || (USART1_PORT.UART->ISR & USART_ISR_ORE_Msk)) {

		USART1_PORT.UART->ICR = USART_ICR_ORECF | USART_ICR_FECF;

		return;
	}

	// Check and handle for full buffer
	if (COUNTER == BUFFER_SIZE) {
		COUNTER = 0;

		memset(DOUBLE_INPUT_BUFFER[ACTIVE_RX_BUFFER], '\0', BUFFER_SIZE);

		return;
	}

	if (USART1_PORT.UART->ISR & USART_ISR_RXNE_Msk) {
		char received = USART1_PORT.UART->RDR;

		// Store char
		DOUBLE_INPUT_BUFFER[ACTIVE_RX_BUFFER][COUNTER] = received;
		COUNTER++;

		// If termination character, NULL append and exit
		if (received == TERMINATE) {
			DOUBLE_INPUT_BUFFER[ACTIVE_RX_BUFFER][COUNTER - 1] = '\0';

			// Swap buffer
			uint8_t current = ACTIVE_RX_BUFFER;
			ACTIVE_RX_BUFFER ^= 1;

			if (USART1_PORT.receive_callback != NULL) {
				// Callback function pointer call
				USART1_PORT.receive_callback(DOUBLE_INPUT_BUFFER[current], COUNTER);
			}

			// Reset counter and buffer after input finish
			COUNTER = 0;
			memset(DOUBLE_INPUT_BUFFER[current], '\0', BUFFER_SIZE);
		}
		return;
	}
	/*
    // Handle Transmit (TXE)
    if ((USART1_PORT.UART->ISR & USART_ISR_TXE_Msk) && (USART1_PORT.UART->CR1 & USART_CR1_TXEIE_Msk)) {
        // Send next byte in buffer if data left
        if (USART1_PORT.tx_buffer_pos < USART1_PORT.tx_buffer_len) {
            USART1_PORT.UART->TDR = USART1_PORT.tx_buffer[USART1_PORT.tx_buffer_pos++];

        } else {
            // Done transmitting
            USART1_PORT.UART->CR1 &= ~USART_CR1_TXEIE_Msk;  // Disable TXE interrupt

            // Call optional transmit callback
            if (USART1_PORT.output_callback != NULL) {
                USART1_PORT.output_callback();
            }

            // Reset position for next transmission
            USART1_PORT.tx_buffer_pos = 0;
            USART1_PORT.tx_buffer_len = 0;
        }

        return;
    }
    */
}


