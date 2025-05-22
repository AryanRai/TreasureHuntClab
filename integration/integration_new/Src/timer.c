#include "timer.h"


// ===== INTERNALS =====


/// Holds info about the clock enable for a timer
typedef struct {
    /// Pointer to the register
    volatile uint32_t *const reg;
    /// Position of the enable bit inside `.reg`
    const uint8_t pos;
    /// Mask inside `.reg`. Should have 1 bit set, at `.pos` bits left
    const uint32_t mask;
} TimerClockEnable;
static const TimerClockEnable CLOCK_ENABLES[_TIMER_COUNT] = {
    // [TIMER_SEL_1]  = { .reg = &RCC->APB2ENR, .pos = RCC_APB2ENR_TIM1EN_Pos,  .mask = RCC_APB2ENR_TIM1EN_Msk  },
    [TIMER_SEL_2]  = { .reg = &RCC->APB1ENR, .pos = RCC_APB1ENR_TIM2EN_Pos,  .mask = RCC_APB1ENR_TIM2EN_Msk  },
    [TIMER_SEL_3]  = { .reg = &RCC->APB1ENR, .pos = RCC_APB1ENR_TIM3EN_Pos,  .mask = RCC_APB1ENR_TIM3EN_Msk  },
    [TIMER_SEL_4]  = { .reg = &RCC->APB1ENR, .pos = RCC_APB1ENR_TIM4EN_Pos,  .mask = RCC_APB1ENR_TIM4EN_Msk  },
    [TIMER_SEL_6]  = { .reg = &RCC->APB1ENR, .pos = RCC_APB1ENR_TIM6EN_Pos,  .mask = RCC_APB1ENR_TIM6EN_Msk  },
    [TIMER_SEL_7]  = { .reg = &RCC->APB1ENR, .pos = RCC_APB1ENR_TIM7EN_Pos,  .mask = RCC_APB1ENR_TIM7EN_Msk  },
    // [TIMER_SEL_8]  = { .reg = &RCC->APB2ENR, .pos = RCC_APB2ENR_TIM8EN_Pos,  .mask = RCC_APB2ENR_TIM8EN_Msk  },
    [TIMER_SEL_15] = { .reg = &RCC->APB2ENR, .pos = RCC_APB2ENR_TIM15EN_Pos, .mask = RCC_APB2ENR_TIM15EN_Msk },
    [TIMER_SEL_16] = { .reg = &RCC->APB2ENR, .pos = RCC_APB2ENR_TIM16EN_Pos, .mask = RCC_APB2ENR_TIM16EN_Msk },
    [TIMER_SEL_17] = { .reg = &RCC->APB2ENR, .pos = RCC_APB2ENR_TIM17EN_Pos, .mask = RCC_APB2ENR_TIM17EN_Msk },
};


/// Direct access to the raw timer peripheral
typedef volatile TIM_TypeDef TimerRaw;
static TimerRaw *RAWS[_TIMER_COUNT] = {
    // [TIMER_SEL_1]  = TIM1,
    [TIMER_SEL_2]  = TIM2,
    [TIMER_SEL_3]  = TIM3,
    [TIMER_SEL_4]  = TIM4,
    [TIMER_SEL_6]  = TIM6,
    [TIMER_SEL_7]  = TIM7,
    // [TIMER_SEL_8]  = TIM8,
    [TIMER_SEL_15] = TIM15,
    [TIMER_SEL_16] = TIM16,
    [TIMER_SEL_17] = TIM17,
};

static IRQn_Type IRQ_NUMS[_TIMER_COUNT] = {
    // [TIMER_SEL_1]  = TIM1_IRQn,
    [TIMER_SEL_2]  = TIM2_IRQn,
    [TIMER_SEL_3]  = TIM3_IRQn,
    [TIMER_SEL_4]  = TIM4_IRQn,
    [TIMER_SEL_6]  = TIM6_DAC_IRQn,
    [TIMER_SEL_7]  = TIM7_IRQn,
    // [TIMER_SEL_8]  = TIM8_IRQn,
    [TIMER_SEL_15] = TIM1_BRK_TIM15_IRQn,
    [TIMER_SEL_16] = TIM1_UP_TIM16_IRQn,
    [TIMER_SEL_17] = TIM1_TRG_COM_TIM17_IRQn,
};


/// Internal state that we use to keep track of custom timer state
typedef struct {
    /// Is the timer enabled (controls `CEN`)
    bool enable;
    /// Don't call the callback when the timer fires
    bool silent;
    /// Should the timer keep itself firing multiple times.
    /// If `false`, `silent` is set, so future firings are skipped
    bool recur;
    /// How long between timer events
    TimerPeriod period;
    /// Prescaler applied to the hardware
    TimerPrescale prescale;
    /// Callback to call when timer fires
    TimerCallbackFn *callback;
} TimerState;
static TimerState STATES[_TIMER_COUNT] = {
    [0 ... _TIMER_COUNT-1] = (TimerState) {
        .enable = false,
        .silent = true,
        .recur = false,
        .period = 0,
        .prescale = 0,
        .callback = NULL,
    }
};


// ===== INITIALISATION =====


void timer_init(void) {
    __disable_irq();
    
    // enable all clocks
    for (uintmax_t i = 0; i < _TIMER_COUNT; i++) {
        const TimerClockEnable en = CLOCK_ENABLES[i];
        *en.reg |=  en.mask;
    }

    // enable irq events
    for (uintmax_t i = 0; i < _TIMER_COUNT; i++) {
        RAWS[i]->DIER |= TIM_DIER_UIE;
        NVIC_EnableIRQ(IRQ_NUMS[i]);
    }

    __enable_irq();
}


// ===== CONTROL =====


void timer_enable_set(const TimerSel sel, const bool enable) {
    TimerRaw *raw = RAWS[sel];

    // clear interrupt flag in case event was already waiting
    raw->SR &= ~TIM_SR_UIF;

    timer_counter_reset(sel);

    /* After writing PSC, ARR & CNT there is a Bug where 
    the UIF-Bit gets set immediatly after setting the CEN-Bit in CR1. 
    By manually generate an interrupt and clearing it by deleting 
    the UIF-Bit in SR, the problem gets solved. */

    // HACK: After writes to `PSC`, `ARR`, and `CNT,
    // there seems to be a hardware bug that causes the `UIF` flag
    // to be set immediately.
    // 
    // To avoid this, manually generate the interrupt, and then clear it
    raw->EGR |=  TIM_EGR_UG;
    raw->SR  &= ~TIM_SR_UIF;

    // Need to set `.enable` after the irq has fired
    // So if we are enabling, it dummy fires while `enable=false`,
    // which is ignored by the handler

    STATES[sel].enable = enable;
    if (enable)
        raw->CR1 |=  TIM_CR1_CEN;
    else
        raw->CR1 &= ~TIM_CR1_CEN;

}
bool timer_enable_get(const TimerSel sel) {
    // detect if the bit is masked on
    const TimerClockEnable en = CLOCK_ENABLES[sel];
    return *en.reg & en.mask;
}

void timer_silent_set(const TimerSel sel, const bool silent) {
    STATES[sel].silent = silent;
}
bool timer_silent_get(const TimerSel sel) {
    return STATES[sel].silent;
}


// ===== PERIOD =====


void timer_period_set(const TimerSel sel, const TimerPeriod period) {
    STATES[sel].period = period;
    TimerRaw *raw = RAWS[sel];
    raw->ARR = period;
}

TimerPeriod timer_period_get(const TimerSel sel) {
    return STATES[sel].period;
}


void timer_prescaler_set(const TimerSel sel, const TimerPrescale scale) {
    STATES[sel].prescale = scale;
    TimerRaw *const raw = RAWS[sel];
    raw->PSC = scale;

    // changing the prescaler will not take effect until counter overflow
    // so set reload to `1` and counter to `0`, to immediately overflow
    // and apply changes
    const uint32_t arr = raw->ARR;
    raw->ARR = 1;
    raw->CNT = 0;
    // spin to give time for changes to take affect
    for (uintmax_t i = 0; i < 8; i++)
        asm("NOP");
    raw->ARR = arr;
}

TimerPrescale timer_prescale_get(const TimerSel sel) {
    return STATES[sel].prescale;
}

// TODO: See TIM->CR1->OPM for oneshot mode
void timer_recur_set(const TimerSel sel, const bool recur) {
    STATES[sel].recur = recur;
}

bool timer_recur_get(const TimerSel sel) {
    return STATES[sel].recur;
}


void timer_counter_reset(const TimerSel sel) {
    RAWS[sel]->CNT = 0;
}


// ===== CALLBACKS =====


/// The callback handler called by our IRQ handlers
static void _timer_interrupt_handler(const TimerSel sel) {
    TimerRaw *raw = RAWS[sel];

    // check it was definitely this timer that fired
    // since multiple timers can share one event
    if (!(raw->SR & TIM_SR_UIF))
        return;

    // clear interrupt flag so it doesn't get called again
    raw->SR &= ~TIM_SR_UIF;

    TimerState *const state = &STATES[sel];

    // We may get a dummy fire when enabling the timer, see `timer_enable_set()`
    if (!state->enable)
        return;

    // update silent for future firings
    const bool was_silent = state->silent;
    state->silent = !state->recur;

    // call callback if enabled and has been set
    // do this after setting silent, so the callback
    // can un-silence itself
    if (!was_silent && state->callback != NULL)
        state->callback(sel);

    // TODO: should not be using this, use auto reload
    // reset counter
    timer_counter_reset(sel);
}

void timer_callback_set(const TimerSel sel, TimerCallbackFn *const callback) {
    STATES[sel].callback = callback;
}

TimerCallbackFn *timer_callback_get(const TimerSel sel) {
    return STATES[sel].callback;
}


// ===== IRQ HANDLERS =====


// NOTE: IRQ handlers seem to be added by overriding the weakly linked
// default handler. Some are also shared for multiple events


// TIM2 global interrupt
void TIM2_IRQHandler(void) {
    _timer_interrupt_handler(TIMER_SEL_2);
}
// TIM3 global interrupt
void TIM3_IRQHandler(void) {
    _timer_interrupt_handler(TIMER_SEL_3);
}
// TIM4 global interrupt
void TIM4_IRQHandler(void) {
    _timer_interrupt_handler(TIMER_SEL_4);
}
// TIM6 global and DAC12 underrun interrupts
void TIM6_DACUNDER_IRQHandler(void) {
    _timer_interrupt_handler(TIMER_SEL_6);
}
// TIM7 global interrupt
void TIM7_IRQHandler(void) {
    _timer_interrupt_handler(TIMER_SEL_7);
}
// TIM1 Break/TIM15 global interrupts
void TIM1_BRK_TIM15_IRQHandler(void) {
    _timer_interrupt_handler(TIMER_SEL_15);
}
// TIM1 Update/TIM16 global interrupts
void TIM1_UP_TIM16_IRQHandler(void) {
    _timer_interrupt_handler(TIMER_SEL_16);
}
// TIM1 trigger and commutation/TIM17 interrupts
void TIM1_TRG_COM_TIM17_IRQHandler(void) {
    _timer_interrupt_handler(TIMER_SEL_17);
}
