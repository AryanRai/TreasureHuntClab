# Technical Documentation: Treasure Hunt Game - STM32F303xC

## 1. Project Overview

This document provides detailed technical information about the STM32F303xC Treasure Hunt game. The game involves players using touch sensors and a potentiometer to control servo-operated "treasure boxes," aiming to find hidden treasures within set constraints of digs and time. It primarily uses `main.c` for game logic, with helper C libraries for serial communication, timer management, and GPIO/EXTI handling.

## 2. File Structure

The main project code is located within the `integration/Integration_final/MainBoard/` directory.

*   **`Core/Src/main.c`**: Central file containing the main game loop, state machine, hardware initialization, and core game logic functions.
*   **`Core/Inc/main.h`**: Header file for `main.c`, including necessary includes and function prototypes if not defined elsewhere.
*   **`Core/Inc/structs.h`**: Defines the core data structures `GameState` and `GameTriggers` used throughout the application to manage game state and player inputs.
*   **`Core/Src/serial_josh.c` & `Core/Inc/serial_josh.h`**: Custom library for USART1 serial communication. Handles initialization, transmission of strings/characters, and interrupt-driven reception of commands.
*   **`Core/Src/timer_josh.c` & `Core/Inc/timer_josh.h`**: Custom library for managing hardware timers (e.g., TIM7 for the main game countdown). Provides functions for timer initialization, setting periods, prescalers, and callbacks.
*   **`Core/Src/GPIO.c` & `Core/Inc/GPIO.h`**: Custom library for GPIO pin configuration (input, output, analog, EXTI) and EXTI interrupt management.
*   **`startup/`**: STM32 startup files (e.g., `startup_stm32f303xc.s`).
*   **`Drivers/STM32F3xx_HAL_Driver/`**: STM32 HAL (Hardware Abstraction Layer) drivers provided by STMicroelectronics.
*   **`Drivers/CMSIS/`**: ARM Cortex Microcontroller Software Interface Standard files.

## 3. Hardware Connection Details

(Based on configurations in `main.c` `MX_GPIO_Init()` and peripheral initialization blocks)

*   **Touch Sensors (Input, Pull-down configuration in `MX_GPIO_Init`, EXTI on Rising Edge via `enable_interrupt` in `GPIO.c` called from `main.c`):**
    *   Sensor linked to `touch_to_servo_map[0]` (Default: Servo 1): `PB7` (EXTI line 7, handled by `EXTI9_5_IRQHandler`)
    *   Sensor linked to `touch_to_servo_map[1]` (Default: Servo 2): `PB6` (EXTI line 6, handled by `EXTI9_5_IRQHandler`)
    *   Sensor linked to `touch_to_servo_map[2]` (Default: Servo 3): `PB5` (EXTI line 5, handled by `EXTI9_5_IRQHandler`)
    *   Sensor linked to `touch_to_servo_map[3]` (Default: Servo 4): `PB4` (EXTI line 4, handled by `EXTI4_IRQHandler`)
    *   Sensor linked to `touch_to_servo_map[4]` (Default: Servo 5): `PB3` (EXTI line 3, handled by `EXTI3_IRQHandler`)
    *   Sensor linked to `touch_to_servo_map[5]` (Default: Servo 6): `PB13` (EXTI line 13, handled by `EXTI15_10_IRQHandler`)
        *Mapping note: `main.c` uses `static const uint8_t touch_pins[6] = {7, 6, 5, 4, 3, 13};` for internal indexing in `handle_touch` and `disable_touchpad`.*
*   **Servo Motors (PWM Output)**:
    *   Servo 1: `PE2` (TIM3_CH1)
    *   Servo 2: `PE3` (TIM3_CH2)
    *   Servo 3: `PA0` (TIM2_CH1)
    *   Servo 4: `PA1` (TIM2_CH2)
    *   Servo 5: `PD12` (TIM4_CH1)
    *   Servo 6: `PD13` (TIM4_CH2)
*   **Trimpot (Analog Input)**:
    *   `PA4` (Configured via `GPIO *trim_pot = init_port(A, ANALOG, 4, 4);`. The `GPIO.c` library likely configures this for ADC2_INP4 for `read_pins_analog`.)
*   **Serial UART (USART1 via `serial_josh.c`)**:
    *   TX: `PC4` (GPIO_AF7_USART1)
    *   RX: `PC5` (GPIO_AF7_USART1)

## 4. Core Data Structures (`structs.h`)

*   **`GameState`**: Holds all persistent variables related to the current game instance.
    ```c
    typedef struct {
        volatile int correct_servos[6];  // Defines which servo (0-5) holds which treasure type/value (0 = no treasure)
        volatile int items_found;        // Number of treasures found so far
        volatile int items_left_to_find; // Number of treasures remaining
        volatile int digs_taken;         // Number of digs performed
        volatile int digs_remaining;     // Number of digs available
        volatile int peeks_used;         // Number of peeks performed
        volatile int game_time_remaining;// Seconds left in the game
        volatile int game_over;          // Flag: 0 = game active, 1 = game over
        volatile int total_items_to_find;// Initial number of treasures at game start
        volatile int current_score;      // Player's current score
    } GameState;
    ```
*   **`GameTriggers`**: Holds transient flags for events. In `main.c`, `triggers.touchpad_pressed` is the primary member used from this struct to signal a touch event from an ISR to the main loop.
    ```c
    typedef struct{
        volatile int touchpad_pressed; // Stores the pin number (e.g., 7 for PB7) of the last pressed touchpad, -1 if none pending.
        // Other fields (magnet1_det, servo_controlled, etc.) are defined but not central to the current game flow in main.c
    } GameTriggers;
    ```

## 5. Key Game Constants (Defined in `main.c` `/* USER CODE BEGIN PD */` and within main loop logic)

*   `PWM_PERIOD_TICKS`: `2000` (Timer period for PWM, e.g., for 20ms at a 100kHz timer clock derived from 48MHz system clock / 480 prescaler).
*   `PWM_MIN_PULSE`: `50` (Pulse width for servo 0 degrees, e.g., 0.5ms for a 100kHz timer clock).
*   `PWM_MAX_PULSE`: `250` (Pulse width for servo 180 degrees, e.g., 2.5ms for a 100kHz timer clock).
*   `POT_ACTIVE_THRESHOLD_RAW`: `50` (Raw ADC value from trimpot (0-4095 range) to activate an armed servo).
*   `SERVO_TARGET_OPEN_ANGLE`: `90` (Target angle in degrees for a fully open box during a dig).
*   `SERVO_TARGET_CLOSED_ANGLE`: `0` (Target angle in degrees for a fully closed box).
*   `SERVO_ANGLE_TOLERANCE`: `3` (Degrees tolerance for detecting if servo has reached open/close targets).
*   `PEEK_MAX_ANGLE`: `20` (Maximum angle in degrees for a peek before committing to a dig).
*   `POT_TO_COMMIT_DIG_THRESHOLD_RAW`: `950` (Alternative raw ADC value threshold. If the potentiometer value, when mapped to the full servo range, would exceed `PEEK_MAX_ANGLE`, it can indicate a dig commitment. This offers a more direct ADC check rather than converting to angle first for this specific transition logic. Calculated as `(PEEK_MAX_ANGLE / SERVO_TARGET_OPEN_ANGLE) * 0xFFF` would be `(20/90)*4095 approx 910`. `950` provides some leeway.)

## 6. Core Function Descriptions (from `main.c`)

Standard HAL/CubeMX generated functions like `SystemClock_Config`, `MX_GPIO_Init`, `MX_TIMx_Init`, `MX_USART1_UART_Init` provide initial peripheral setup, though custom libraries (`serial_josh.c`, `timer_josh.c`, `GPIO.c`) and direct register manipulation in `main.c` often tailor or supersede these for specific game needs.

*   **`void SetServoAngle(uint8_t servoId, uint16_t angle)`**: 
    *   Inputs: `servoId` (1-6), `angle` (0-180 degrees).
    *   Calculates the required PWM pulse width based on `PWM_MIN_PULSE`, `PWM_MAX_PULSE`, and the desired `angle`.
    *   Sets the `Compare` register of the appropriate Timer (TIM2, TIM3, or TIM4) and Channel (1 or 2) corresponding to the `servoId` to generate the PWM signal.
    *   Sends a serial message logging the action.
*   **`void handle_touch(uint8_t pin_index)`**: 
    *   Inputs: `pin_index` (the GPIO pin number that triggered the EXTI, e.g., 7 for PB7).
    *   This function is intended to be called from an EXTI ISR.
    *   Maps the `pin_index` to a `touchpad_index` (0-5).
    *   Checks if `touchpad_used[touchpad_index]` is `true`. If so, logs that the touchpad was already used and returns.
    *   If the touchpad is available and `touch_enabled` is true, it sets `triggers.touchpad_pressed = pin_index` to signal the main loop about the new touch event.
*   **`void dig_used(uint8_t servoId)`**: 
    *   Inputs: `servoId` (1-6) of the servo being dug.
    *   Called when a servo is considered fully opened during a dig.
    *   Returns immediately if `game.game_over` is true or `game.digs_remaining == 0`.
    *   Increments `game.digs_taken`, decrements `game.digs_remaining`.
    *   Checks `game.correct_servos[servoId - 1]` for a treasure. 
    *   If treasure found (value > 0): sets `success = true`, stores `treasure_value`, increments `game.items_found`, decrements `game.items_left_to_find`, adds `treasure_value` to `game.current_score`, and sets `game.correct_servos[servoId - 1] = 0`.
    *   Sends a serial message indicating DIG SUCCESS (with treasure value) or DIG FAIL.
    *   Calls `transmit_game_state()` and `check_game_over()`.
*   **`void display_start_menu(void)`**: Prints an ASCII art welcome menu and "Send 'game start'..." instruction to the serial console.
*   **`void display_final_scoreboard(GameState *game_param, const char* end_reason_msg)`**: Prints a formatted game over message, the reason for game end, and final statistics (Score, Treasures Found/Total, Digs Used, Peeks Used).
*   **`void start_game(GameState *game_param, const uint8_t map[6], int chances, int time_limit)`**: 
    *   Resets all fields of the global `game` struct (`game_param` points to this) to initial values based on `map`, `chances`, and `time_limit` arguments.
    *   Calculates `game.total_items_to_find` and `game.items_left_to_find` from the input `map`.
    *   Sets all servos to 0 degrees using `SetServoAngle()`.
    *   Calls `reset_touchpads()` to re-enable all touch sensors.
    *   Initializes and starts a hardware timer (TIM7 via `timer_josh.c`) to call `fn_a` every second.
    *   Sends "Game Started" message and calls `transmit_game_state()`.
*   **`void input_callback(char *data, uint32_t len)`**: 
    *   Called by `serial_josh.c` when a complete line (terminated by `\r`) is received via USART1.
    *   Makes a mutable copy of the input `data`.
    *   Trims leading whitespace and control characters from this copy (`command_to_parse`).
    *   If `command_to_parse` starts with "game start":
        *   Extracts parameters (e.g., "map=...", "chances=...", "time=...") from the rest of the string using `parse_game_config()`.
        *   Calls `start_game()` with the parsed values or defaults if parameters are absent.
    *   Logs unknown commands.
*   **`void parse_game_config(char* params_str, uint8_t* out_map, int* out_chances, int* out_time)`**: 
    *   Helper function using `strtok_r` to parse space-separated key-value pairs (e.g., "map=1,2,3", "chances=5") from the `params_str`.
    *   Updates the output variables `out_map`, `out_chances`, `out_time` with found values. Defaults (pre-set in `input_callback` before calling this) are used if a parameter is not found.
*   **`static void fn_a(TimerSel sel)`**: 
    *   Timer interrupt callback function (typically configured for 1Hz via `timer_josh.c`).
    *   Decrements `game.game_time_remaining` if it's greater than 0.
    *   Sends a serial message: `TIME REMAINING:%d\r\n`.
    *   Calls `transmit_game_state()` to send the full game state update.
*   **`uint8_t check_game_over(GameState *game_param)`**: 
    *   Checks for game end conditions:
        *   Win: `game.items_left_to_find == 0`.
        *   Lose (No Digs): `game.digs_remaining == 0`.
        *   Lose (Time Up): `game.game_time_remaining <= 0` (specifically checks `<=1` and sets to 0 if 1 for display consistency).
    *   If any condition is met: stops the game timer, sets `game.game_over = 1`, calls `display_final_scoreboard()`, then `display_start_menu()`. Returns `1`.
    *   Otherwise, returns `0`.
*   **`void reset_touchpads(void)`**: 
    *   Iterates through `touchpad_used` array, setting all to `false`.
    *   Re-enables EXTI interrupts for all 6 touch sensor pins by manipulating `EXTI->IMR` and clearing pending flags `EXTI->PR`.
    *   Sends a confirmation message via serial.
*   **`void disable_touchpad(uint8_t touchpad_index)`**: 
    *   Inputs: `touchpad_index` (0-5).
    *   Sets `touchpad_used[touchpad_index] = true`.
    *   Maps `touchpad_index` to the actual GPIO pin number (e.g., index 0 maps to pin 7 for PB7).
    *   Disables the EXTI interrupt for this specific pin by manipulating `EXTI->IMR` and clearing its pending flag `EXTI->PR`.
    *   Sends a confirmation message via serial.

## 7. Code Flow and State Machine (Main Loop in `main.c`)

The core game logic is managed by a state machine within the `while(1)` loop of `main()`.
Key static variables in `main()` control the state: `isActiveMode`, `activeServoId`, `servoFullyOpened`, `servoFullyClosed`, `activeTouchpadPin`, `armed_touchpad_pin`, `inDigCommitPhase`, `hasPeekMovementOccurred`. The global `touch_enabled` flag controls if `handle_touch` processes new touches.

1.  **Initial State / Game Over State (`game.game_over == 1`)**:
    *   The loop first checks if `game.game_over` is true.
    *   If so, it resets interaction flags: `isActiveMode = false`, `activeServoId = 0`, `servoFullyOpened = false`, `servoFullyClosed = false`, `activeTouchpadPin = 0`, `armed_touchpad_pin = 0`.
    *   `touch_enabled` is set to `true` (to allow `handle_touch` to register touches for a new game, though game logic prevents actions until started).
    *   `triggers.touchpad_pressed` is cleared.
    *   The loop effectively pauses here by `continue`-ing, waiting for a serial command to start a new game (which will call `start_game()` and set `game.game_over = 0`).

2.  **Active Game - Pre-Interaction Checks (`game.game_over == 0`)**:
    *   `check_game_over(&game)` is called. If it returns `1` (game has ended), the loop `continue`s, effectively transitioning back to the Game Over state.

3.  **Idle / Arming State (`!isActiveMode`)**:
    *   `touch_enabled` is set to `true`, allowing `handle_touch` (called by EXTI ISRs) to set `triggers.touchpad_pressed` if an unused touchpad is pressed.
    *   **Touch Reception**: If `triggers.touchpad_pressed != -1`:
        *   `armed_touchpad_pin` is set to the pin number from `triggers.touchpad_pressed`.
        *   Serial message: "Touchpad PB%d armed..."
        *   `triggers.touchpad_pressed` is cleared.
    *   **Activation**: If `armed_touchpad_pin != 0` (a pad is armed) AND `pot_raw_value > POT_ACTIVE_THRESHOLD_RAW` (potentiometer moved past threshold):
        *   Transition to Active Mode: `isActiveMode = true`.
        *   Reset sub-state flags: `inDigCommitPhase = false`, `hasPeekMovementOccurred = false`, `servoFullyOpened = false`, `servoFullyClosed = false`.
        *   `activeTouchpadPin` (the pin for current interaction) is set from `armed_touchpad_pin`.
        *   `armed_touchpad_pin` is cleared (disarmed).
        *   `activeTouchpadPin` is mapped to `activeServoId` (1-6).
        *   Serial message: "Activated: Armed PB%d (Servo %d)..."
        *   `touch_enabled = false` (ignore other touches while controlling a servo).

4.  **Active Servo Control Mode (`isActiveMode`)**:
    *   `touch_enabled` remains `false`.
    *   `pot_raw_value` is read via `read_pins_analog()`.
    *   `current_pot_angle_full_range` (0-90 degrees) is calculated from `pot_raw_value`.
    *   **Peek Control Phase (`!inDigCommitPhase`)**: 
        *   `peek_control_angle` is `current_pot_angle_full_range` but capped at `PEEK_MAX_ANGLE`.
        *   `SetServoAngle(activeServoId, peek_control_angle)` is called.
        *   If `peek_control_angle > SERVO_ANGLE_TOLERANCE`, `hasPeekMovementOccurred` is set `true`.
        *   **Transition to Dig**: If `current_pot_angle_full_range > PEEK_MAX_ANGLE + SERVO_ANGLE_TOLERANCE`:
            *   `inDigCommitPhase = true`.
            *   `hasPeekMovementOccurred = false` (reset for dig phase).
            *   Serial message: "PEEK converted to DIG commitment..."
        *   **Peek Action Completion**: If `hasPeekMovementOccurred == true` AND `peek_control_angle <= SERVO_ANGLE_TOLERANCE` (servo closed after peeking):
            *   `game.peeks_used++`.
            *   Serial message: "PEEK used on Servo %d..."
            *   `transmit_game_state()`.
            *   Reset to Idle State: `isActiveMode = false`, `activeServoId = 0`, `activeTouchpadPin = 0`.
            *   `touch_enabled = true`.
    *   **Dig Control Phase (`inDigCommitPhase`)**: 
        *   `SetServoAngle(activeServoId, current_pot_angle_full_range)` is called (0-90 degree range).
        *   **If `!servoFullyOpened`** (servo not yet fully opened for dig):
            *   If `current_pot_angle_full_range >= (SERVO_TARGET_OPEN_ANGLE - SERVO_ANGLE_TOLERANCE)`:
                *   `servoFullyOpened = true`.
                *   `dig_used(activeServoId)` is called (updates game state, checks for treasure, etc.).
        *   **Else if `!servoFullyClosed`** (`servoFullyOpened` is true, waiting for full closure):
            *   If `current_pot_angle_full_range <= (SERVO_TARGET_CLOSED_ANGLE + SERVO_ANGLE_TOLERANCE)`:
                *   `servoFullyClosed = true`.
                *   Serial message: "Servo %d fully closed after DIG..."
                *   The touchpad index corresponding to `activeTouchpadPin` is determined, and `disable_touchpad()` is called for that index.
                *   Reset to Idle State: `isActiveMode = false`, `activeServoId = 0`, `activeTouchpadPin = 0`, `servoFullyOpened = false`, `servoFullyClosed = false`.
                *   `touch_enabled = true`.

Visualized State Flow (Simplified for clarity):
```mermaid
graph TD
    A[Game Over / Start Menu] -- "game start" cmd --> B{Idle / Wait Touch};
    B -- Touchpad Press (Pin X) --> C{Pad X Armed};
    C -- Pot > Threshold --> D{Active Mode (Servo for Pad X)};
    D -- Pot Angle <= PEEK_MAX --> E[Peek Phase: Servo 0-PEEK_MAX_ANGLE];
    E -- Pot Angle > PEEK_MAX+Tol --> F{Dig Commit Phase};
    F -- Pot Angle controls Servo 0-OPEN_ANGLE --> G[Dig Phase: Servo 0-OPEN_ANGLE];
    E -- Servo Moved & Closed from Peek --> H[Peek Complete: peeks_used++, isActiveMode=false];
    H --> B;
    G -- Servo @ OPEN_ANGLE --> I[Dig Registered: dig_used() called];
    I -- Servo @ CLOSED_ANGLE --> J[Dig Cycle Done: Touchpad X Disabled, isActiveMode=false];
    J --> B;
    B -- Win/Lose/Timeup in check_game_over() --> A;
    subgraph Active Mode Operation
        D
        E
        F
        G
        I
    end
```

## 8. Serial Commands

Commands are sent via USART1 (typically at 115200 baud) and must be terminated with a single Carriage Return (`\r`). Input is case-sensitive.

*   **`game start`**: 
    *   Starts a new game with default settings:
        *   Default Map: `game.correct_servos = {4, 8, 0, 0, 0, 0}` (Treasure value 4 in box 1, 8 in box 2, others empty).
        *   Default Digs: 4.
        *   Default Time: 240 seconds.

*   **`game start [param1=value1] [param2=value2] ...`**: 
    *   Starts a new game with specified parameters. Parameters are space-separated and can be in any order.
    *   **`map=v0,v1,v2,v3,v4,v5`**: Sets the treasure map. `v0` to `v5` are comma-separated integer values for servos 1 through 6, respectively (index 0 of array for servo 1). `0` means no treasure. 
        *   Example: `map=10,0,20,0,15,5`
    *   **`chances=X`**: Sets the number of digs allowed to integer `X`.
        *   Example: `chances=3`
    *   **`time=Y`**: Sets the game duration to integer `Y` seconds.
        *   Example: `time=180`

    **Full Command Examples:**
    *   `game start map=5,10,0,15,0,20 chances=5 time=300`
    *   `game start chances=2 time=120` (uses default map)
    *   `game start map=0,0,0,0,0,100` (uses default chances and time)

## 9. Game State Monitoring (Serial Output)

The game provides real-time feedback via the serial console (USART1).

*   **Periodic Game State Update (from `transmit_game_state()`):** 
    This message is sent by the timer callback `fn_a` every second and after key game actions (like `dig_used` or peek completion).
    ```
    GAME STATE: Score: <score> | Digs Left: <digs_rem>, Digs Taken: <digs_taken> | Treasures Left: <items_left>, Treasures Found: <items_found> | Peeks Used: <peeks_used> | Time: <time_rem>
    ```
    *   `<score>`: Current player score.
    *   `<digs_rem>`: Digs remaining.
    *   `<digs_taken>`: Digs already used.
    *   `<items_left>`: Treasures yet to be found.
    *   `<items_found>`: Treasures already found.
    *   `<peeks_used>`: Number of peeks used.
    *   `<time_rem>`: Game time remaining in seconds.

*   **Direct Time Remaining Update (from `fn_a`):**
    This is sent every second by the timer callback, just before `transmit_game_state()`.
    ```
    TIME REMAINING:<time_rem>
    ```

*   **Other Key Informational Messages:**
    *   On boot / after game end: ASCII Start Menu (`display_start_menu()`) or Final Scoreboard (`display_final_scoreboard()`).
    *   `Game Started
`
    *   `Touchpad PB%d armed. Waiting for pot threshold (>%d).
` (e.g., `Touchpad PB7 armed. Waiting for pot threshold (>50).`)
    *   `Activated: Armed PB%d (Servo %d), Pot (%d) > Thresh (%d). Controlling Servo %d.
`
    *   `Setting Servo %d to %d°
`
    *   `PEEK converted to DIG commitment. Open fully, then close fully.
`
    *   `PEEK used on Servo %d. Total Peeks: %d. Choose next action.
`
    *   `DIG SUCCESS at Servo %d! Found Treasure (Value: %d). Digs left: %d. Treasures left: %d
`
    *   `DIG FAIL at Servo %d. No treasure. Digs left: %d. Treasures left: %d
`
    *   `Servo %d fully closed after DIG. Cycle complete for touch PB%d.
`
    *   `Touchpad %d (PB%d) disabled - already used
` (when trying to arm an already used pad)
    *   `Touchpad %d (PB%d) disabled - already used
` (after a successful dig, referring to the pin used in `disable_touchpad` via its index)
    *   `All touchpads re-enabled
` (from `reset_touchpads()`)
    *   `DEBUG: input_callback fired!
`
    *   `DEBUG: Received (len %lu, actual str len %lu): %s
` (shows raw received data)
    *   `DEBUG: Command after trimming leading chars: '%s'
` (shows command after sanitization)
    *   `DEBUG: Unknown command received (after trim): '%s'
`
    *   Error messages like "Error: Mapped to invalid Servo ID..." or "Error: In active mode with invalid activeServoId..."

## 9.1. Magnetometer Sub-System (QMC5883L)

This section details the functionality of the magnetometer module, primarily implemented in `MagnetometerLedBuzzer/Core/Src/main.c`. It uses a QMC5883L magnetometer sensor connected via I2C1.

### 9.1.1. Overview

The magnetometer sub-system is designed to detect magnetic fields, control an LED and a buzzer based on field strength, predict the type of magnet, and transmit data over serial connections.

### 9.1.2. Key Functionalities

*   **Initialization (`QMC5883L_Init`)**: Configures the QMC5883L sensor for continuous measurement mode, with settings for output data rate (ODR), range (RNG), and oversampling rate (OSR).
*   **Data Reading (`QMC5883L_ReadXYZ`)**: Reads the raw X, Y, and Z magnetic field data from the sensor. It waits for the data ready (DRDY) status bit before reading.
*   **Magnitude Calculation**: Computes the overall magnetic field strength using the formula: `magnitude = sqrt(x*x + y*y + z*z)`.
*   **LED and Buzzer Control**: 
    *   The duty cycle for PWM signals controlling an LED (TIM1_CH3) and a buzzer (TIM2_CH3) is adjusted based on the `magnitude`.
    *   An inverse relationship is implemented: stronger magnetic fields (lower magnitude values, assuming the sensor is close to a strong magnet that it is calibrated for) result in a lower duty cycle (dimmer LED, quieter buzzer), while weaker fields result in a higher duty cycle.
    *   Thresholds `MAG_MIN_THRESHOLD` (e.g., 3000) and `MAG_MAX_THRESHOLD` (e.g., 10000) define the range for this mapping. Magnitudes below `MAG_MIN_THRESHOLD` result in maximum duty cycle, and above `MAG_MAX_THRESHOLD` result in minimum (0) duty cycle.
*   **Magnet Type Prediction**: 
    *   Predicts the probability of the detected magnet being one of two types (e.g., "Small" or "Big") based on its magnitude.
    *   The prediction uses defined ranges:
        *   Small Magnet Range: `MAG_SMALL_LOW_F` (e.g., 2000.0f) to `MAG_SMALL_HIGH_F` (e.g., 6000.0f)
        *   Big Magnet Range: `MAG_BIG_LOW_F` (e.g., 6000.0f) to `MAG_BIG_HIGH_F` (e.g., 20000.0f)
    *   Probabilities are calculated to provide a smooth transition between types, with a 50/50 prediction at the boundary (e.g., 6000.0f).
*   **Serial Data Transmission**: 
    *   **USART2**: Transmits a custom binary packet containing raw X, Y, Z data. 
    *   **USART1**: Transmits human-readable debug messages periodically.

### 9.1.3. Key Defines (from `MagnetometerLedBuzzer/Core/Src/main.c`)

*   `QMC5883L_ADDR`: `0x0D << 1` (I2C address)
*   `QMC5883L_REG_X_LSB`: `0x00` (Start register for XYZ data)
*   `QMC5883L_REG_CTRL1`: `0x09` (Control Register 1 for mode settings)
*   `MAG_MIN_THRESHOLD`: e.g., `3000` (Magnitude for max LED/Buzzer intensity)
*   `MAG_MAX_THRESHOLD`: e.g., `10000` (Magnitude for min LED/Buzzer intensity)
*   `MAG_SMALL_LOW_F`: e.g., `2000.0f`
*   `MAG_SMALL_HIGH_F`: e.g., `6000.0f` (Boundary for small magnet max / big magnet transition)
*   `MAG_BIG_LOW_F`: e.g., `6000.0f` (Boundary for big magnet min / small magnet transition)
*   `MAG_BIG_HIGH_F`: e.g., `20000.0f`
*   `UART_TX_PERIOD_MS`: e.g., `1000` (Period for USART1 debug messages)

### 9.1.4. USART2 Custom Packet Protocol

The magnetometer data (X, Y, Z) is transmitted over USART2 using a simple custom packet structure:

*   **Size**: 11 bytes
*   **Structure**:
    *   `packet[0]`: `PROTOCOL_START_MARKER` (0x7E)
    *   `packet[1]`: Length (e.g., 0x0A = 10 bytes for sensor type + sensor ID + payload + checksum)
    *   `packet[2]`: `PROTOCOL_SENSOR_TYPE_MAG` (0x01)
    *   `packet[3]`: `PROTOCOL_SENSOR_ID_QMC` (0x01)
    *   `packet[4-5]`: X-axis data (int16_t, little-endian: LSB, MSB)
    *   `packet[6-7]`: Y-axis data (int16_t, little-endian: LSB, MSB)
    *   `packet[8-9]`: Z-axis data (int16_t, little-endian: LSB, MSB)
    *   `packet[10]`: Checksum (sum of bytes from `packet[1]` to `packet[9]`, truncated to 8 bits)

### 9.1.5. USART1 Debug Output

Periodically (e.g., every `UART_TX_PERIOD_MS`), a debug message is sent via USART1:

`DEBUG: Mag: <magnitude>, Duty: <duty_cycle_percent>%, Pred: <prob_M1>% M1, <prob_M2>% M2\r\n`

*   `<magnitude>`: Floating-point value of the calculated magnetic field strength.
*   `<duty_cycle_percent>`: Integer percentage of the PWM duty cycle for LED/Buzzer.
*   `<prob_M1>`: Integer percentage probability of it being Magnet 1 (Small).
*   `<prob_M2>`: Integer percentage probability of it being Magnet 2 (Big).

Example: `DEBUG: Mag: 5500.25, Duty: 60%, Pred: 75% M1, 25% M2\r\n`

## 9.2 Servo Control Sub-System

The Servo Control Sub-System is responsible for managing the six servo motors that operate the "treasure box" lids in the game. Control is achieved through Pulse Width Modulation (PWM) signals generated by dedicated hardware timers.

### 9.2.1. Hardware and Connections

Six servo motors are used, each connected to a specific GPIO pin configured for alternate function PWM output. Three timers (TIM2, TIM3, TIM4) are utilized, with each timer controlling two servos on different channels:

*   **Servo 1**: `PE2` (TIM3_CH1)
*   **Servo 2**: `PE3` (TIM3_CH2)
*   **Servo 3**: `PA0` (TIM2_CH1)
*   **Servo 4**: `PA1` (TIM2_CH2)
*   **Servo 5**: `PD12` (TIM4_CH1)
*   **Servo 6**: `PD13` (TIM4_CH2)

The necessary GPIO pins are initialized to their respective alternate functions, and the timers are configured for PWM generation in `MX_GPIO_Init()` and `MX_TIMx_Init()` functions, further tailored by custom logic where applicable.

### 9.2.2. Key Constants and Configuration

The precision of servo movement is governed by several key constants defined in `main.c`:

*   `PWM_PERIOD_TICKS`: `2000` - Defines the timer period for PWM, establishing the PWM frequency (e.g., 50Hz for a 20ms period, common for servos).
*   `PWM_MIN_PULSE`: `50` - Represents the pulse width in timer ticks for the servo's 0-degree position (e.g., 0.5ms).
*   `PWM_MAX_PULSE`: `250` - Represents the pulse width in timer ticks for the servo's 180-degree position (e.g., 2.5ms).
*   `SERVO_TARGET_OPEN_ANGLE`: `90` - The target angle (in degrees) for a treasure box to be considered fully open during a "dig".
*   `SERVO_TARGET_CLOSED_ANGLE`: `0` - The target angle (in degrees) for a treasure box to be considered fully closed.
*   `SERVO_ANGLE_TOLERANCE`: `3` - A small tolerance (in degrees) used when checking if a servo has reached its target open or closed position.
*   `PEEK_MAX_ANGLE`: `20` - The maximum angle (in degrees) a box can be opened for a "peek" action.

These values are critical for calibrating the servo movement to the physical constraints of the treasure boxes and desired game mechanics.

### 9.2.3. Core Functionality: `SetServoAngle`

The primary function for controlling the servos is:
`void SetServoAngle(uint8_t servoId, uint16_t angle)`

*   **Inputs**:
    *   `servoId`: An identifier from 1 to 6, specifying which servo to control.
    *   `angle`: The desired angle for the servo, typically within a 0 to 180-degree range, though the game logic often caps this at `SERVO_TARGET_OPEN_ANGLE` (90 degrees).
*   **Operation**:
    1.  Calculates the required PWM pulse width (Compare value) based on the input `angle`, linearly interpolating between `PWM_MIN_PULSE` and `PWM_MAX_PULSE`.
    2.  Identifies the correct timer instance (TIM2, TIM3, or TIM4) and channel (TIM_CHANNEL_1 or TIM_CHANNEL_2) associated with the given `servoId`.
    3.  Updates the timer's Capture Compare Register (CCR) with the calculated pulse width, thereby changing the servo's position.
    4.  Typically logs the action via a serial message for debugging.

### 9.2.4. Role in Game Logic (Peek/Dig Mechanics)

Servos are central to the core gameplay, representing the treasure boxes that players interact with using the touch sensors and potentiometer:

*   **Activation**: When a touchpad is armed and the potentiometer is moved beyond `POT_ACTIVE_THRESHOLD_RAW`, the corresponding servo becomes active.
*   **Peek Mode**: If the player manipulates the potentiometer to open the servo to an angle less than or equal to `PEEK_MAX_ANGLE`, they are in "peek" mode. Closing the servo from this state consumes a "peek".
*   **Dig Mode**: If the servo is opened beyond `PEEK_MAX_ANGLE`, the player commits to a "dig".
    *   The servo must be opened to at least `SERVO_TARGET_OPEN_ANGLE - SERVO_ANGLE_TOLERANCE`.
    *   Once fully opened, the `dig_used()` function is called to process the dig (check for treasure, update game state).
    *   The servo must then be closed back to `SERVO_TARGET_CLOSED_ANGLE + SERVO_ANGLE_TOLERANCE` to complete the dig cycle.
*   The main game loop in `main.c` continuously reads the potentiometer, calculates the desired servo angle based on the current game state (idle, peek, dig commit), and calls `SetServoAngle()` to update the active servo's position.

This tight integration of servo control with player input and game state transitions forms the heart of the treasure hunt experience.

## 10. Troubleshooting & Debugging Guide

*   **No Serial Output / Garbled Output:**
    *   **Action:** Verify serial terminal settings (115200 baud, 8N1). Check COM port and USB connection. Confirm STM32 UART pins (PC4 TX, PC5 RX for USART1) are correctly connected if using an external adapter.
*   **Commands Not Recognized / Game Not Starting:**
    *   **Action:** Ensure commands are terminated with a single Carriage Return (`\r`). Check for exact command spelling (e.g., `game start`). Observe `DEBUG:` messages from `input_callback` in `main.c` to see what the STM32 is receiving and parsing.
*   **Touch Sensors Unresponsive:**
    *   **Action:** Check physical connections to PB3, PB4, PB5, PB6, PB7, PB13. Confirm pull-down resistors are effective if needed. Verify EXTI configuration in `GPIO.c` and `main.c` (rising edge). Check `touch_enabled` flag in `main.c` logic – it should be true when waiting for a touch.
*   **Servos Not Moving / Incorrect Movement / Jitter:**
    *   **Action:** **POWER!** Servos need stable 5V and sufficient current; an external power supply is strongly recommended, with its ground common to the STM32. Verify PWM settings (`PWM_MIN_PULSE`, `PWM_MAX_PULSE` in `main.c`) against your servo datasheet. Check PA4 trimpot connection and ADC readings. Trace the `current_pot_angle_full_range` calculation.
*   **Peek/Dig Logic Issues:**
    *   **Action:** Monitor serial output for state transitions ("PEEK converted to DIG...", "PEEK used...", "DIG SUCCESS/FAIL"). Closely examine the `isActiveMode`, `inDigCommitPhase`, `hasPeekMovementOccurred`, `servoFullyOpened`, `servoFullyClosed` flags in the debugger or via temporary serial prints.
*   **Game State Incorrect (Digs, Score, Time):**
    *   **Action:** Review logic in `dig_used()`, `start_game()`, `check_game_over()`, and the timer callback `fn_a`. Pay attention to how and when these game state variables are modified. Use the `GAME STATE:` serial output as your ground truth.
*   **Touchpads Not Disabling After Dig:**
    *   **Action:** Confirm `disable_touchpad()` is called with the correct `touchpad_index` (0-5) after a dig cycle. Verify its logic for masking the correct EXTI line.

By enabling and observing the various `serial_output_string` debug messages already present in `main.c`, many issues can be diagnosed effectively.
