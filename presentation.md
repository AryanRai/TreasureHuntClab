# Treasure Hunt: An STM32 Adventure - Presentation Outline

## Slide 1: Title - Treasure Hunt: An STM32 Adventure

*   **Project Name:** Treasure Hunt on STM32F303
*   **Team Context:** MTRX2700 Mechatronics Project (University of Sydney)
*   **Core Goal:** To design and build an immersive embedded game that combines physical player interaction (touch sensors, potentiometer) with strategic gameplay (peek vs. dig, resource management) on an STM32 microcontroller.
*   **Elevator Pitch:** It's a race against time and resources to find hidden treasures, using touch to select and a dial to dig, with a crucial peek-before-you-commit mechanic!

---

## Slide 2: Gameplay - The Thrill of the Hunt

*   **Player Objective:**
    *   Find all hidden treasures concealed within 6 servo-controlled boxes.
    *   Achieve this before running out of time or your limited number of "digs."
*   **Core Interaction Loop:**
    1.  **Touch to Arm:** Player touches one of 6 capacitive sensor pads. This "arms" the corresponding treasure box.
    2.  **Potentiometer to Activate & Control:** Rotating a potentiometer above a small threshold "activates" the armed box. Further rotation directly controls the box's lid (servo angle).
*   **Strategic Mechanics - Peek vs. Dig:**
    *   **PEEK Mode (Cautious Exploration):**
        *   Rotate potentiometer to open lid slightly (e.g., up to 20 degrees).
        *   Player can *peek* inside.
        *   Closing the lid from a peek consumes one "peek resource."
        *   The touchpad remains active for another try or a different action.
    *   **DIG Mode (Full Commitment):**
        *   Rotate potentiometer to open lid *beyond* the peek threshold (e.g., > 20 degrees). This commits to a full "dig."
        *   Lid must be opened fully (e.g., 90 degrees) – this is when treasure is checked, and a "dig resource" is consumed.
        *   Lid must then be fully closed (0 degrees) to complete the cycle.
        *   After a successful dig cycle, the touchpad for that box is *disabled* for the rest of the game.
*   **Resource Management is Key:**
    *   **Time:** A configurable countdown (e.g., 240 seconds).
    *   **Digs:** A limited number of full dig attempts (e.g., 4).
    *   **Peeks:** A separate count of peeks available.
    *   **Score:** Treasures have point values, contributing to a final score.

---

## Slide 3: System Overview - Hardware & Software

*   **Key Hardware Components:**
    *   **Brain:** STM32F303xC Microcontroller.
    *   **Player Input (Selection):** 6x Capacitive Touch Sensors (connected to GPIO pins with EXTI).
    *   **Player Input (Action):** 1x Potentiometer (connected to ADC).
    *   **Actuators (Box Lids):** 6x Servo Motors (controlled by PWM signals from STM32 Timers).
    *   **Communication:** USB-Serial link for PC interface.
*   **Software Architecture (STM32 - C Language):**
    *   **Main Loop (`main.c`):** Central hub orchestrating game flow based on a state machine.
        *   States include: Game Over/Menu, Idle (Waiting for Touch), Armed (Touch registered, waiting for Pot activation), Active Mode (controlling servo), Peek Phase, Dig Phase.
    *   **Interrupt-Driven Input:**
        *   Touch sensor presses trigger EXTI interrupts (`handle_touch`).
        *   Game timer uses a Hardware Timer interrupt (`fn_a` via `timer_josh.c`).
        *   Serial commands via USART interrupt (`input_callback` via `serial_josh.c`).
    *   **Analog Sensing:** ADC reads potentiometer value for servo control.
    *   **Game Logic Module:**
        *   Manages `GameState` struct (score, time, digs, peeks, treasures found/remaining).
        *   Handles `start_game`, `check_game_over`, `dig_used` (treasure checking, score update).
    *   **Actuator Control:** `SetServoAngle` function precisely positions servos using PWM.
    *   **Serial Communication:** Custom library (`serial_josh.c`) for sending game state updates and receiving commands (e.g., "game start map=... chances=... time=...").
*   **Python UI (Conceptual/In-Progress):** A Tkinter-based GUI for a more user-friendly way to send commands and view game state, communicating over serial.

---

## Slide 4: Technical Deep Dive - Key Challenges & Solutions

*   **Challenge 1: Implementing nuanced Peek vs. Dig behavior with a single Potentiometer.**
    *   **Problem:** How to allow partial opening (peek) without penalty, versus full opening (dig) with resource consumption, using one analog input.
    *   **Solution: State-Driven Control Logic in "Active Mode"**
        1.  **Arm & Activate:** Touch arms a servo; pot rotation above threshold activates it.
        2.  **Peek Phase (`!inDigCommitPhase`):** Servo movement initially capped at `PEEK_MAX_ANGLE` (e.g., 20°). Potentiometer directly controls angle within this peek range.
        3.  **Transition to Dig:** If pot pushes angle beyond `PEEK_MAX_ANGLE` + tolerance, state transitions to `inDigCommitPhase = true`.
        4.  **Peek Completion:** If servo moved during peek phase and then closed (angle ≤ tolerance) *without* committing to dig: `peeks_used` increments, player can choose another action.
        5.  **Dig Phase (`inDigCommitPhase`):** Potentiometer now controls servo up to `SERVO_TARGET_OPEN_ANGLE` (e.g., 90°).
            *   Reaching full open: `dig_used()` is called (consumes dig resource, checks for treasure, updates score).
            *   Must then be fully closed: Completes cycle, disables that touchpad.
*   **Challenge 2: Ensuring Robust Serial Command Handling for Game Restart and Configuration.**
    *   **Problem:** Initial attempts at restarting the game or sending configuration commands via serial were unreliable, often failing due to extra characters or inconsistent line endings from terminals.
    *   **Solution: Input Sanitization and Standardized Termination.**
        1.  **STM32 `input_callback` Enhancement:** Implemented trimming of leading/trailing whitespace and control characters from the raw serial buffer before command parsing (`strncmp`).
        2.  **Standardized Terminator:** The STM32 code expects a single Carriage Return (`\r`) to signify end-of-command. Python UI was updated to send `\r` instead of `\r\n`.
        3.  **Result:** Reliable parsing of "game start" and its optional parameters (`map=`, `chances=`, `time=`), leading to stable game restarts and custom configurations.

---

## Slide 5: Outcomes & Future Directions

*   **Project Accomplishments:**
    *   **Functional Embedded Game:** A complete Treasure Hunt game with defined rules, win/lose conditions, and interactive physical controls.
    *   **Hardware Integration:** Successfully interfaced and managed STM32F303 with touch sensors, servo motors, and a potentiometer.
    *   **Complex Game Mechanics:** Implemented nuanced peek-and-dig logic, resource management (time, digs, peeks), and a scoring system.
    *   **Configurable Gameplay:** Game parameters (treasure map, digs, time) can be customized via serial commands.
    *   **Real-Time Feedback:** Continuous game state updates provided over serial communication.
*   **Key Learning Outcomes:**
    *   Practical application of state machine design for managing complex game flow.
    *   Interrupt handling for responsive input (touch, serial).
    *   Precise actuator control using PWM and ADC.
    *   Debugging and problem-solving in an embedded C environment.
    *   Importance of robust communication protocols.
*   **Potential Future Enhancements:**
    *   **Advanced Python UI:** Complete the Tkinter GUI for enhanced visual display of game state, graphical controls, and easier game configuration.
    *   **Sound Effects/Music:** Add audio feedback for events like finding treasure, time running low, game over.
    *   **Visual Indicators:** LEDs per box to show status (e.g., armed, dug, contains treasure after a peek - if a "peek reveals" mechanic is added).
    *   **More Diverse Content:** Introduce "traps" or "tools" in boxes, different treasure tiers with varying score values.
    *   **EEPROM Storage:** Save high scores or player preferences. 