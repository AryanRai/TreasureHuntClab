/*#include "gpio_new.h"
#include "structs.h"
#include "stm32f303xc.h"


// Address and clock mask lookup tables
const uint32_t adr_link[] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE};
const uint32_t clock_mask_link[] = {RCC_AHBENR_GPIOAEN, RCC_AHBENR_GPIOBEN,
                                   RCC_AHBENR_GPIOCEN, RCC_AHBENR_GPIODEN,
                                   RCC_AHBENR_GPIOEEN};

// Callback arrays for different interrupt types
static gpio_interrupt_callback_t gpio_callbacks[16] = {0};
static touch_callback_t touch_callback = NULL;
static void *touch_trigger_data = NULL;

// Utility function to create bit masks
uint32_t create_mask(uint8_t start, uint8_t end) {
    return ((1 << (end + 1)) - 1) ^ ((1 << start) - 1);
}

// Initialize a GPIO port
GPIO *init_port(port_name_link name, port_mode mode, uint8_t pin_lower, uint8_t pin_upper) {
    GPIO *port_pt = malloc(sizeof(GPIO));

    port_pt->PORT_IND = name;
    port_pt->PORT_ADR = (GPIO_TypeDef*)adr_link[name];
    port_pt->MODE = mode;
    port_pt->PIN_LOWER = pin_lower;
    port_pt->PIN_UPPER = pin_upper;

    // Enable clock for portX
    uint32_t clock_en_mask = clock_mask_link[name];
    RCC->AHBENR |= clock_en_mask;

    uint32_t *port_mode_reg = &(port_pt->PORT_ADR->MODER);

    // Generate mode-mask
    uint32_t temp_mask = create_mask(pin_lower * 2, pin_upper * 2 + 1);

    // Clear the selected section
    *port_mode_reg &= ~temp_mask;

    if (mode == OUTPUT) {
        // Set output mode (0b01 for each pin)
        uint32_t output_mask = 0;
        for (uint8_t i = pin_lower; i <= pin_upper; i++) {
            output_mask |= (1 << (i * 2));
        }
        *port_mode_reg |= output_mask;

    } else if (mode == ANALOG) {
        // Set analog mode (0b11 for each pin)
        *port_mode_reg |= temp_mask;

        // Configure ADC (only for Port A pins 0-7)
        if (name == PORT_A && pin_lower <= 7) {
            // Enable ADC clock
            RCC->AHBENR |= RCC_AHBENR_ADC12EN;

            // Synchronize ADC with clock
            ADC12_COMMON->CCR |= ADC12_CCR_CKMODE_0;

            // Configure voltage regulator
            ADC2->CR &= ~ADC_CR_ADVREGEN;
            ADC2->CR |= ADC_CR_ADVREGEN_0;
            ADC2->CR &= ~ADC_CR_ADCALDIF;

            // Calibrate ADC
            ADC2->CR |= ADC_CR_ADCAL;
            while((ADC2->CR & ADC_CR_ADCAL) == ADC_CR_ADCAL);

            // Configure conversion sequence
            ADC2->SQR1 = 0;

            // Add channels to sequence
            for (uint8_t i = 0; i < (pin_upper - pin_lower + 1); i++) {
                uint8_t pin = pin_lower + i;
                if (pin >= 3) { // Pins PA3+ map to ADC channels
                    ADC2->SQR1 |= (pin - 3) << (6 * (i + 1));
                }
            }

            // Set number of conversions
            ADC2->SQR1 |= (pin_upper - pin_lower) << ADC_SQR1_L_Pos;

            // Single shot mode
            ADC2->CFGR &= ~ADC_CFGR_CONT;

            // Enable ADC
            ADC2->CR |= ADC_CR_ADEN;
            while (!(ADC2->ISR & ADC_ISR_ADRDY));
        }
    }

    return port_pt;
}

// Read digital pins
uint16_t read_pins(GPIO *port_pt) {
    if (port_pt->MODE == INPUT) {
        uint16_t pin_states = port_pt->PORT_ADR->IDR;

        // Mask to get only the selected pins
        pin_states <<= (15 - port_pt->PIN_UPPER);
        pin_states >>= (15 - port_pt->PIN_UPPER + port_pt->PIN_LOWER);

        return pin_states;
    }
    return 0;
}

// Read single digital pin
uint8_t read_single_pin(GPIO *port_pt, uint8_t pin_index) {
    if (pin_index < port_pt->PIN_LOWER || pin_index > port_pt->PIN_UPPER) {
        return 0;
    }

    uint16_t pin_states = port_pt->PORT_ADR->IDR;
    return (pin_states & (1 << pin_index)) ? 1 : 0;
}

// Read analog pins
void read_pins_analog(GPIO *port_pt, uint16_t *dest_pt) {
    if (port_pt->MODE == ANALOG) {
        // Start conversion
        ADC2->CR |= ADC_CR_ADSTART;

        uint8_t i = 0;
        while (!(ADC2->ISR & ADC_ISR_EOS)) {
            while (!(ADC2->ISR & ADC_ISR_EOC));
            dest_pt[i++] = ADC2->DR;
        }

        // Clear end of sequence flag
        ADC2->ISR |= ADC_ISR_EOS;
    }
}

// Write to digital pins
void write_pins(GPIO *port_pt, uint16_t data) {
    if (port_pt->MODE == OUTPUT) {
        // Shift data to correct position
        data <<= port_pt->PIN_LOWER;

        uint16_t *odr_pt = &port_pt->PORT_ADR->ODR;

        // Clear target bits
        *odr_pt &= ~create_mask(port_pt->PIN_LOWER, port_pt->PIN_UPPER);

        // Set new data
        *odr_pt |= data;
    }
}

// Write to single digital pin
void write_single_pin(GPIO *port_pt, uint8_t single_bit, uint8_t pin_index) {
    if (pin_index < port_pt->PIN_LOWER || pin_index > port_pt->PIN_UPPER) {
        return;
    }

    if (port_pt->MODE == OUTPUT) {
        uint16_t *odr_pt = &port_pt->PORT_ADR->ODR;

        // Clear bit
        *odr_pt &= ~(1 << pin_index);

        // Set new bit
        *odr_pt |= (single_bit & 1) << pin_index;
    }
}

// Enable generic GPIO interrupt
void enable_interrupt(GPIO *port_pt, uint8_t pin_index, trigger_type trigger,
                     uint8_t priority, gpio_interrupt_callback_t handler) {

    // Register callback
    gpio_callbacks[pin_index] = handler;

    __disable_irq();

    // Enable SYSCFG clock
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // Configure EXTI source
    uint32_t exticr_shift = 4 * (pin_index % 4);
    SYSCFG->EXTICR[pin_index / 4] &= ~(0xF << exticr_shift);
    SYSCFG->EXTICR[pin_index / 4] |= (port_pt->PORT_IND << exticr_shift);

    // Set trigger edge
    if (trigger == RISING_EDGE) {
        EXTI->RTSR |= (1 << pin_index);
        EXTI->FTSR &= ~(1 << pin_index);
    } else {
        EXTI->FTSR |= (1 << pin_index);
        EXTI->RTSR &= ~(1 << pin_index);
    }

    // Enable interrupt
    EXTI->IMR |= (1 << pin_index);

    // Configure NVIC
    IRQn_Type irq_num;
    if (pin_index <= 4) {
        irq_num = (IRQn_Type)(EXTI0_IRQn + pin_index);
    } else if (pin_index <= 9) {
        irq_num = EXTI9_5_IRQn;
    } else {
        irq_num = EXTI15_10_IRQn;
    }

    NVIC_SetPriority(irq_num, priority);
    NVIC_EnableIRQ(irq_num);

    __enable_irq();
}

// Touch-specific initialization
void initialise_touch(void) {
    // Enable clocks
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOEEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // Configure PE8–PE11 as output for display
    uint32_t *pe_mode = &GPIOE->MODER;
    uint32_t pe_mask = create_mask(16, 23); // PE8-PE11 (bits 16-23)
    *pe_mode &= ~pe_mask;
    *pe_mode |= 0x00550000; // Set as outputs (01 pattern)

    // Set PA1–PA6 as inputs (default mode is input, so just clear any previous config)
    uint32_t *pa_mode = &GPIOA->MODER;
    uint32_t pa_mask = create_mask(2, 13); // PA1-PA6 (bits 2-13)
    *pa_mode &= ~pa_mask; // Clear to set as inputs
}

void enable_touch_interrupts(void) {
    __disable_irq();

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // Map EXTI lines to PA pins
    SYSCFG->EXTICR[0] = SYSCFG_EXTICR1_EXTI1_PA | SYSCFG_EXTICR1_EXTI2_PA | SYSCFG_EXTICR1_EXTI3_PA;
    SYSCFG->EXTICR[1] = SYSCFG_EXTICR2_EXTI4_PA | SYSCFG_EXTICR2_EXTI5_PA | SYSCFG_EXTICR2_EXTI6_PA;

    // Configure for rising edge triggers
    EXTI->RTSR |= EXTI_RTSR_TR1 | EXTI_RTSR_TR2 | EXTI_RTSR_TR3 |
                  EXTI_RTSR_TR4 | EXTI_RTSR_TR5 | EXTI_RTSR_TR6;
    EXTI->FTSR &= ~(EXTI_FTSR_TR1 | EXTI_FTSR_TR2 | EXTI_FTSR_TR3 |
                    EXTI_FTSR_TR4 | EXTI_FTSR_TR5 | EXTI_FTSR_TR6);

    // Unmask interrupt lines
    EXTI->IMR |= EXTI_IMR_MR1 | EXTI_IMR_MR2 | EXTI_IMR_MR3 |
                 EXTI_IMR_MR4 | EXTI_IMR_MR5 | EXTI_IMR_MR6;

    // Enable NVIC IRQs
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_TSC_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);

    __enable_irq();
}


void disable_touch_interrupts_global(void) {
    __disable_irq();

    // Disable NVIC IRQs for touch pins
    NVIC_DisableIRQ(EXTI1_IRQn);
    NVIC_DisableIRQ(EXTI2_TSC_IRQn);
    NVIC_DisableIRQ(EXTI3_IRQn);
    NVIC_DisableIRQ(EXTI4_IRQn);
    NVIC_DisableIRQ(EXTI9_5_IRQn);

    // Mask interrupt lines (disable interrupts)
    EXTI->IMR &= ~(EXTI_IMR_MR1 | EXTI_IMR_MR2 | EXTI_IMR_MR3 |
                   EXTI_IMR_MR4 | EXTI_IMR_MR5 | EXTI_IMR_MR6);

    // Clear any pending interrupt flags
    EXTI->PR |= EXTI_PR_PR1 | EXTI_PR_PR2 | EXTI_PR_PR3 |
                EXTI_PR_PR4 | EXTI_PR_PR5 | EXTI_PR_PR6;

    // Disable rising edge triggers
    EXTI->RTSR &= ~(EXTI_RTSR_TR1 | EXTI_RTSR_TR2 | EXTI_RTSR_TR3 |
                    EXTI_RTSR_TR4 | EXTI_RTSR_TR5 | EXTI_RTSR_TR6);

    // Disable falling edge triggers (in case they were set)
    EXTI->FTSR &= ~(EXTI_FTSR_TR1 | EXTI_FTSR_TR2 | EXTI_FTSR_TR3 |
                    EXTI_FTSR_TR4 | EXTI_FTSR_TR5 | EXTI_FTSR_TR6);

    __enable_irq();
}

void disable_touch_interrupts_specific(uint8_t n) {
    __disable_irq();
    if (n == 1) {
        NVIC_DisableIRQ(EXTI1_IRQn);
    } else if (n == 2) {

    }
    // Disable NVIC IRQs for touch pins
    NVIC_DisableIRQ(EXTI1_IRQn);
    NVIC_DisableIRQ(EXTI2_TSC_IRQn);
    NVIC_DisableIRQ(EXTI3_IRQn);
    NVIC_DisableIRQ(EXTI4_IRQn);
    NVIC_DisableIRQ(EXTI9_5_IRQn);

    // Mask interrupt lines (disable interrupts)
    EXTI->IMR &= ~(EXTI_IMR_MR1 | EXTI_IMR_MR2 | EXTI_IMR_MR3 |
                   EXTI_IMR_MR4 | EXTI_IMR_MR5 | EXTI_IMR_MR6);

    // Clear any pending interrupt flags
    EXTI->PR |= EXTI_PR_PR1 | EXTI_PR_PR2 | EXTI_PR_PR3 |
                EXTI_PR_PR4 | EXTI_PR_PR5 | EXTI_PR_PR6;

    // Disable rising edge triggers
    EXTI->RTSR &= ~(EXTI_RTSR_TR1 | EXTI_RTSR_TR2 | EXTI_RTSR_TR3 |
                    EXTI_RTSR_TR4 | EXTI_RTSR_TR5 | EXTI_RTSR_TR6);

    // Disable falling edge triggers (in case they were set)
    EXTI->FTSR &= ~(EXTI_FTSR_TR1 | EXTI_FTSR_TR2 | EXTI_FTSR_TR3 |
                    EXTI_FTSR_TR4 | EXTI_FTSR_TR5 | EXTI_FTSR_TR6);

    __enable_irq();
}
void touch_register_callback(touch_callback_t callback, void *trigger_data) {
    touch_callback = callback;
    touch_trigger_data = trigger_data;
}

void display_number(uint8_t n) {
    // Display n on PE8–PE11 (4-bit binary)
    GPIOE->ODR &= ~(0xF << 8); // Clear PE8–11
    GPIOE->ODR |= ((n & 0xF) << 8); // Set new value
}

// Unified interrupt handlers
void EXTI0_IRQHandler(void) {
    EXTI->PR |= EXTI_PR_PR0;
    if (gpio_callbacks[0]) gpio_callbacks[0](0);
}

void EXTI1_IRQHandler(void) {
    EXTI->PR |= EXTI_PR_PR1;
    if (touch_callback) {
        touch_callback(1, touch_trigger_data);
    } else if (gpio_callbacks[1]) {
        gpio_callbacks[1](1);
    }
}

void EXTI2_TSC_IRQHandler(void) {
    EXTI->PR |= EXTI_PR_PR2;
    if (touch_callback) {
        touch_callback(2, touch_trigger_data);
    } else if (gpio_callbacks[2]) {
        gpio_callbacks[2](2);
    }
}

void EXTI3_IRQHandler(void) {
    EXTI->PR |= EXTI_PR_PR3;
    if (touch_callback) {
        touch_callback(3, touch_trigger_data);
    } else if (gpio_callbacks[3]) {
        gpio_callbacks[3](3);
    }
}

void EXTI4_IRQHandler(void) {
    EXTI->PR |= EXTI_PR_PR4;
    if (touch_callback) {
        touch_callback(4, touch_trigger_data);
    } else if (gpio_callbacks[4]) {
        gpio_callbacks[4](4);
    }
}

void EXTI9_5_IRQHandler(void) {
    for (uint8_t i = 5; i <= 9; i++) {
        if (EXTI->PR & (1 << i)) {
            EXTI->PR |= (1 << i);

            // Handle touch callbacks for pins 5-6
            if (i <= 6 && touch_callback) {
                touch_callback(i, touch_trigger_data);
            } else if (gpio_callbacks[i]) {
                gpio_callbacks[i](i);
            }
        }
    }
}

void EXTI15_10_IRQHandler(void) {
    for (uint8_t i = 10; i <= 15; i++) {
        if (EXTI->PR & (1 << i)) {
            EXTI->PR |= (1 << i);
            if (gpio_callbacks[i]) gpio_callbacks[i](i);
        }
    }
}
*/
