#ifndef GPIO_UNIFIED_H
#define GPIO_UNIFIED_H

#include "stm32f303xc.h"
#include <stdint.h>
#include <stdlib.h>

// Port name enumeration
typedef enum {
    PORT_A = 0,
    PORT_B = 1,
    PORT_C = 2,
    PORT_D = 3,
    PORT_E = 4
} port_name_link;

// Port mode enumeration
typedef enum {
    INPUT = 0,
    OUTPUT = 1,
    ANALOG = 2
} port_mode;

// Trigger type enumeration
typedef enum {
    RISING_EDGE = 0,
    FALLING_EDGE = 1
} trigger_type;

// GPIO structure
typedef struct _GPIO {
    uint8_t PORT_IND;           // Index of port (A:0, B:1 ...)
    GPIO_TypeDef *PORT_ADR;     // Pointer to base of GPIOx
    uint8_t MODE;               // 0: Input, 1: output, 2: analog
    uint8_t PIN_LOWER;          // Range of pins to R/W (0-15)
    uint8_t PIN_UPPER;
} GPIO;

// Callback function types
typedef void (*gpio_interrupt_callback_t)(uint8_t pin_index);
typedef void (*touch_callback_t)(uint8_t pad, void *trigger_data);

// Core GPIO functions
GPIO *init_port(port_name_link name, port_mode mode, uint8_t pin_lower, uint8_t pin_upper);

uint32_t create_mask(uint8_t start, uint8_t end);

// Digital I/O functions
uint16_t read_pins(GPIO *port_pt);
uint8_t read_single_pin(GPIO *port_pt, uint8_t pin_index);
void write_pins(GPIO *port_pt, uint16_t data);
void write_single_pin(GPIO *port_pt, uint8_t single_bit, uint8_t pin_index);

// Analog functions
void read_pins_analog(GPIO *port_pt, uint16_t *dest_pt);

// Generic interrupt functions
void enable_interrupt(GPIO *port_pt, uint8_t pin_index, trigger_type trigger,
                     uint8_t priority, gpio_interrupt_callback_t handler);

// Touch-specific functions
void initialise_touch(void);
void enable_touch_interrupts(void);
void touch_register_callback(touch_callback_t callback, void *trigger_data);
void display_number(uint8_t n);

#endif // GPIO_UNIFIED_H
