#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


#include <stm32f303xc.h>

#include "structs.h"
// ===== TYPES =====


typedef enum TimerSel {
    // TIMER_SEL_1,
    TIMER_SEL_2,
    TIMER_SEL_3,
    TIMER_SEL_4,
    TIMER_SEL_6,
    TIMER_SEL_7,
    // TIMER_SEL_8,
    TIMER_SEL_15,
    TIMER_SEL_16,
    TIMER_SEL_17,

    /// Don't use, used to calculate how many timers we have
    _TIMER_COUNT,
} TimerSel;


typedef uint16_t TimerPeriod;
typedef uint16_t TimerPrescale;


/// Function pointer for a timer callback
/// # Params:
/// - `sel`: Which timer fired, to call this callback
typedef void (TimerCallbackFn)(TimerSel sel);


// ===== INITIALISATION =====


/// Initialise all the timer peripherals
void timer_init(void);


// ===== CONTROL =====


/// Enable or disable the given timer
void timer_enable_set(TimerSel sel, bool enable);
bool timer_enable_get(TimerSel sel);

/// Silence a timer to prevent the callback from being called
/// when the timer fires
void timer_silent_set(TimerSel sel, bool silent);
bool timer_silent_get(TimerSel sel);


// ===== PERIOD =====


/// Set time period between fires
void timer_period_set(TimerSel sel, TimerPeriod period);
TimerPeriod timer_period_get(TimerSel sel);

/// Set the prescaler for the timer's clock
void timer_prescaler_set(TimerSel sel, TimerPrescale scale);
TimerPrescale timer_prescaler_get(TimerSel sel);

/// Control whether the timer will call the callback repeatedly
/// If set to `false`, then the timer will silence itself on the next event.
/// The timer can be un-silenced inside the callback.
void timer_recur_set(TimerSel sel, bool recur);
bool timer_recur_get(TimerSel sel);

/// Reset the timer's counter, so it starts counting from this instant
void timer_counter_reset(TimerSel sel);


// ===== CALLBACKS =====


/// Set the callback to be called when the timer fires
void timer_callback_set(TimerSel sel, TimerCallbackFn *callback);
/// Get the callback for the given timer
TimerCallbackFn *timer_callback_get(TimerSel sel, GameState *game);
