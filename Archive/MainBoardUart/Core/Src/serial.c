#include "stm32f303xc.h"
#include <stddef.h>
#include <stdlib.h> // Added for itoa
#include "gamestate.h"
#include "serial.h"

#define ALTFUNCTION 0xA00
#define RXTX 0x770000
#define HIGHSPEED 0xF00
#define BAUDRATE 0x46
#define BUFFER 256
#define LED_OUTPUT 0x5555

int i;
unsigned char string[BUFFER];
extern int message_complete;

void reset_input_buffer() {
    memset(string, 0, BUFFER);
    i = 0;
}

void enableUSART1()
{
    // Enable GPIO C and USART1's clocks
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN_Msk;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN_Msk;

    // Set GPIO C to use UART as alternate function
    GPIOC->MODER = ALTFUNCTION;
    GPIOC->AFR[0] = RXTX;
    GPIOC->OSPEEDR = HIGHSPEED;

    // Set the baud rate and ready USART 1 for both receive and transmit
    USART1->BRR = BAUDRATE;                   // Baud rate = 115200
    USART1->CR1 |= USART_CR1_RE_Msk;
    USART1->CR1 |= USART_CR1_TE_Msk;
    USART1->CR1 |= USART_CR1_UE_Msk;
}

void enableLEDs()
{
    // Enable clock for Port E (LEDs)
    RCC->AHBENR |= RCC_AHBENR_GPIOEEN;

    // Get the most significant 16 bits of port mode register as that is where the mode for the LEDs are defined
    uint16_t* portMode = ((uint16_t*)&(GPIOE->MODER))+1;

    // Set the mode of the port pins to output since they are LEDs
    *portMode = LED_OUTPUT;
}

void enableUARTInterrupts()
{
    __disable_irq();

    // Generate an interrupt upon receiving data
    USART1->CR1 |= USART_CR1_RXNEIE_Msk;

    // Set priority and enable interrupts
    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_EnableIRQ(USART1_IRQn);

    __enable_irq();
}

void USART1_EXTI25_IRQHandler()
{
    // Check for overrun or frame errors
    if ((USART1->ISR & USART_ISR_FE_Msk) && (USART1->ISR & USART_ISR_ORE_Msk))
    {
        return;
    }

    // If we have stored the maximum amount, stop
    if (i == BUFFER)
    {
        return;
    }

    if (USART1->ISR & USART_ISR_RXNE_Msk)
    {
        // Read data
        unsigned char data = (uint8_t) USART1->RDR;
        USART1->TDR = data;  // Echo back

        if (data != '\r')
        {
        // Store the read data
        string[i] = data;
        i++;
        //USART1->TDR = '\n';


        } else {
        message_complete = 1;

        }
    }
}

void send_string(const char *msg) {
    while (*msg) {
        while (!(USART1->ISR & USART_ISR_TXE_Msk));
        USART1->TDR = *msg++;
    }
}

void send_string_buffer(int struct_data) {
    char buffer[12];
    itoa(struct_data, buffer, 10);
    char *msg = buffer; // Create a pointer to walk through the buffer
    while (*msg) {
        while (!(USART1->ISR & USART_ISR_TXE_Msk));
        USART1->TDR = *msg++;
    }
}

void print_game_state(GameState game) {
    send_string("correct_servos: ");
    for(int i = 0; i < game.total_items_to_find; i++){
        send_string(" ");
        send_string_buffer(game.correct_servos[i]);
    }
    send_string("\r\n");

    send_string("items_found: ");
    send_string_buffer(game.items_found);
    send_string("\r\n");

    send_string("items_left_to_find: ");
    send_string_buffer(game.items_left_to_find);
    send_string("\r\n");

    send_string("digs_taken: ");
    send_string_buffer(game.digs_taken);
    send_string("\r\n");

    send_string("digs_remaining: ");
    send_string_buffer(game.digs_remaining);
    send_string("\r\n");

    send_string("peeks_used: ");
    send_string_buffer(game.peeks_used);
    send_string("\r\n");

    send_string("total_items_to_find: ");
    send_string_buffer(game.total_items_to_find);
    send_string("\r\n");

    send_string("game_time_remaining: ");
    send_string_buffer(game.game_time_remaining);
    send_string("\r\n");

    send_string("game_over: ");
    send_string(game.game_over ? "true\n" : "false\n");
    send_string("\r\n\r\n");
}

void print_game_triggers(GameTriggers triggers) {
    send_string("touchpad_pressed: ");
    send_string_buffer(triggers.touchpad_pressed);
    send_string("\r\n");

    send_string("magnet1_det: ");
    send_string_buffer(triggers.magnet1_det);
    send_string("\r\n");

    send_string("magnet2_det: ");
    send_string_buffer(triggers.magnet2_det);
    send_string("\r\n");

    send_string("servo_controlled: ");
    send_string_buffer(triggers.servo_controlled);
    send_string("\r\n");

    send_string("servo_angle: ");
    send_string_buffer(triggers.servo_angle);
    send_string("\r\n");

    send_string("trimpot_value: ");
    send_string_buffer(triggers.trimpot_value);
    send_string("\r\n");

    send_string("peek_threshold: ");
    send_string_buffer(triggers.peek_threshold);
    send_string("\r\n\r\n");
}

void clear_screen(void) {
    send_string("\x1B[2J\x1B[H");
}

//For Testing
int valid_period_check(char *input){
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
}
