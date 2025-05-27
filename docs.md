 \
# Treasure Hunt Game - STM32F303xC

## Project Overview

This project implements a "Treasure Hunt" game on an STM32F303xC microcontroller. Players interact with the game using touch sensors and a potentiometer to control servo-operated "treasure boxes." The objective is to find hidden treasures within a limited number of digs and a set time. Game state, scores, and instructions are communicated via a serial interface.

## Hardware Requirements

*   STM32F303xC-based development board (e.g., Nucleo or custom board).
*   Up to 6 Touch Sensors connected to GPIO pins configured as EXTI interrupt sources (PB3-PB7, PB13 used in `main.c`).
*   Up to 6 Servo Motors, controlled by PWM signals from TIM2, TIM3, and TIM4 (PA0, PA1, PE2, PE3, PD12, PD13 used in `main.c`).
*   1 Potentiometer connected to an ADC-capable pin (PA4 used in `main.c`) for controlling servo movement and game interactions.
*   Serial communication interface (e.g., ST-Link VCP, USB-to-UART adapter) connected to USART1 (PC4 TX, PC5 RX used in `main.c`).

## Software Structure

The project is primarily contained within `integration/Integration_final/MainBoard/`.

### Core Files:

*   **`Core/Src/main.c`**:
    *   Contains the main game loop, state machine, and logic.
    *   Initializes hardware (GPIO, Timers for PWM, USART, ADC for potentiometer).
    *   Handles touch sensor interrupts (`handle_touch`).
    *   Manages servo control (`SetServoAngle`, `get_servo`).
    *   Implements game rules: starting, ending, dig counting, treasure finding, scorekeeping, peeking.
    *   Parses serial commands for starting and configuring the game.
    *   Outputs game state and menus to the serial console.
*   **`Core/Inc/main.h`**: Header for `main.c`.
*   **`Core/Inc/structs.h`**:
    *   Defines core data structures: `GameState` and `GameTriggers`.
        *   `GameState`: Tracks all persistent game variables (score, digs, time, treasure locations, etc.).
        *   `GameTriggers`: Holds transient event flags (touch pressed, etc.).
*   **`Core/Src/serial_josh.c` / `Core/Inc/serial_josh.h`**: (Assumed) Custom library for USART serial communication, including interrupt-driven reception.
*   **`Core/Src/timer_josh.c` / `Core/Inc/timer_josh.h`**: (Assumed) Custom library for timer management, used for the main game countdown.
*   **`Core/Src/GPIO.c` / `Core/Inc/GPIO.h`**: (Assumed) Custom library for GPIO and EXTI interrupt configuration.
*   **`startup/` & `Drivers/`**: Standard STM32CubeIDE generated files for MCU startup, HAL drivers, and CMSIS.

### Key Game Logic in `main.c`:

*   **Game States:**
    *   **Menu/Idle State (`game.game_over == 1`):** Waits for "game start" command. Displays start menu or final scoreboard.
    *   **Active Game State (`game.game_over == 0`):** Main game loop runs.
        *   **Touchpad Arming:** Pressing a touchpad "arms" it.
        *   **Activation Mode (`isActiveMode`):** If an armed touchpad exists and the potentiometer is moved above a small threshold, this mode activates for the corresponding servo.
            *   **Peek Phase (`!inDigCommitPhase`):** Player can move the trimpot to control the servo up to `PEEK_MAX_ANGLE` (e.g., 20 degrees).
                *   If closed back to 0 after peeking: Counts as a "peek used". Player can select another touchpad. Touchpad is *not* disabled.
                *   If trimpot attempts to open beyond `PEEK_MAX_ANGLE`: Transitions to Dig Commit Phase.
            *   **Dig Commit Phase (`inDigCommitPhase`):** Player has committed to a full dig. Trimpot controls servo 0-90 degrees.
                *   When servo fully opens (90 degrees): `dig_used()` is called, consuming a dig chance and checking for treasure.
                *   Player must then fully close the servo (0 degrees) to complete the cycle.
                *   The touchpad used for this dig is then disabled for the rest of the game.
*   **Serial Commands:**
    *   `game start [map=v1,v2..] [chances=X] [time=Y]`: Starts a new game. Parameters are optional.
        *   `map`: 6 comma-separated integers for treasure values in servos 1-6 (0 for no treasure). Default: `4,8,0,0,0,0`.
        *   `chances`: Number of digs. Default: `4`.
        *   `time`: Game duration in seconds. Default: `240`.
*   **Scoring:** Treasure values found are added to `current_score`.
*   **Win/Lose Conditions:**
    *   Win: All treasures found (`items_left_to_find == 0`).
    *   Lose: No digs remaining or time runs out.

## How to Run / Play

1.  **Build and Flash:**
    *   Open the project in STM32CubeIDE (or your preferred environment).
    *   Ensure the correct target (STM32F303xC) is selected.
    *   Build the project.
    *   Flash the compiled binary to the microcontroller.
2.  **Serial Connection:**
    *   Connect to the STM32's USART1 via a serial terminal program (e.g., PuTTY, Tera Term, STM32CubeIDE Serial Monitor).
    *   Baud Rate: `115200`
    *   Data Bits: 8
    *   Parity: None
    *   Stop Bits: 1
    *   Flow Control: None
    *   Ensure your terminal sends a Carriage Return (`\\r`, ASCII 13) when you press Enter, as this is the `TERMINATE` character used by `serial_josh.c`.
3.  **Gameplay:**
    *   On startup, an ASCII art start menu will appear:
        ```
        =====================================
         T R E A S U R E   H U N T ! ! !
        =====================================
        Send 'game start' via serial to begin.
        -------------------------------------
        ```
    *   **Starting a Game:** Type `game start` and press Enter.
        *   Optionally, add parameters: `game start map=1,2,3,0,0,0 chances=3 time=180`
    *   The game will begin, and the serial console will periodically display:
        `GAME STATE: Score: X | Digs Left: Y, Digs Taken: Z | Treasures Left: A, Treasures Found: B | Peeks Used: C | Time: TTT`
    *   **Interacting:**
        1.  **Select a Box:** Press one of the 6 touch sensors. The console will show "Touchpad PBX armed. Waiting for pot threshold...".
        2.  **Activate Potentiometer:** Slightly turn the potentiometer. The console will show "Activated: Armed PBX (Servo Y)...".
        3.  **Peek or Dig:**
            *   **Peeking:** Control the potentiometer to open the servo lid slightly (up to `PEEK_MAX_ANGLE`, ~20 degrees). You can open and close it within this range. To complete a "peek action," close the lid fully (back to 0 degrees). This uses one "peek" and allows you to choose another box. The console will log "PEEK used...".
            *   **Committing to a Dig:** From the peek phase, if you continue to turn the potentiometer to open the lid *beyond* `PEEK_MAX_ANGLE`, it becomes a committed dig. The console will log "PEEK converted to DIG commitment...". You must now open the lid fully (90 degrees). This will trigger `dig_used()`, consume a "dig chance", and check for treasure. Then, you must close the lid fully (0 degrees) to complete the dig cycle. The touchpad used for this dig will then be disabled.
    *   **Game End:** The game ends if you run out of digs, time, or find all treasures. A final scoreboard is displayed, followed by the start menu to play again.

## Code Structure - Key User Sections in `main.c`

*   **`/* USER CODE BEGIN Includes */`**: For custom `#include` directives.
*   **`/* USER CODE BEGIN PD */`**: For `#define` constants related to game parameters (e.g., PWM values, peek angles).
*   **`/* USER CODE BEGIN PV */`**: For global game variables (e.g., `GameState game`, `GameTriggers triggers`).
*   **`/* USER CODE BEGIN PFP */`**: For private function prototypes specific to `main.c`.
*   **`/* USER CODE BEGIN 0 */`**: For user-defined functions (e.g., `display_start_menu`, `display_final_scoreboard`, `dig_used`, `input_callback`, `parse_game_config`, `handle_touch`, `get_servo`, `SetServoAngle`).
*   **`main()` function `/* USER CODE BEGIN 2 */`**: Hardware initialization calls and definitions for `main`'s local static state variables.
*   **`main()` function `while(1)` loop `/* USER CODE BEGIN WHILE */`**: The core game logic and state machine.

## Future Enhancements (from `gamedetails.txt` or potential)

*   **Advanced Peek Logic:** Implement trimpot speed detection for peek vs. dig.
*   **Power Meter/Gauge:** Add a skill-based mini-game for digging using the trimpot.
*   **LED Module:** Add LEDs for visual feedback (e.g., treasure proximity, game events).
*   **Magnet Detector Module:** Implement actual treasure detection if physical magnets are used.
*   **Physical Start Button:** Add an alternative to the serial command for starting the game.
*   More sophisticated error handling for serial command parsing.
*   Persistent high scores (if storage like EEPROM is available/used).

This `docs.md` should provide a good starting point.
