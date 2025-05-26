/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "GPIO.h"
#include "serial_josh.h"
#include "structs.h"
#include "timer_josh.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PWM_PERIOD_TICKS 2000  // 20 ms at 100 kHz
#define PWM_MIN_PULSE 50       // 0.5 ms (0°)
#define PWM_MAX_PULSE 250      // 2.5 ms (180°)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
// Touch-to-servo mapping: index = touch sensor (0-5), value = servo ID (1-6)
uint8_t touch_to_servo_map[6] = {1, 2, 3, 4, 5, 6}; // Default: 1:1 mapping
// Track servo states (0° or 90°)
uint8_t servo_states[6] = {0, 0, 0, 0, 0, 0}; // 0 = closed (0°), 1 = open (90°)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void SetServoAngle(uint8_t servoId, uint16_t angle);
void touch_pad_handler(uint8_t pin_index);
void dig_used(uint8_t servoId); // Declaration for new function
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief Set angle for a specific servo
  * @param servoId: 1–6
  * @param angle: 0–180
  * @retval None
  */


// =================================== Global Variables ====================================
volatile int last_servo_selection = -1;
volatile int last_servo_angle = -1;

// --- Game State Variables ---
GameState game = {
    .correct_servos = {7, 6, 5, 4, 3, 13}, // do not use servo number 0
    .items_found = 0,
    .items_left_to_find = 4,
    .digs_taken = 0,
    .digs_remaining = 4,
    .peeks_used = 0,
    .game_time_remaining = 240,
    .game_over = 1,
    .total_items_to_find = 0
};

GameTriggers triggers = {
    .touchpad_pressed = -1,
    .magnet1_det = 0,
    .magnet2_det = 0,
    .servo_controlled = -1,
    .servo_angle = 0,
    .trimpot_value = 0,
    .peek_threshold = 10,
    .pending_peek = 0
};

// Track which touchpads have been used (true = used, false = available)
bool touchpad_used[6] = {false, false, false, false, false, false};
bool touch_enabled = true;
// =================================== Game Functions ====================================
// Prints via UART game state
void transmit_game_state() {
    char buffer[64];
    sprintf(buffer, "DIGS REMAINING:%d TREASURES:%d\r\n\n", game.digs_remaining, game.items_left_to_find);
    serial_output_string(buffer, &USART1_PORT);
}

// Timer callback
static void fn_a(const TimerSel sel, GameState *game) {
	game->game_time_remaining = game->game_time_remaining - 1;
    char buffer[64];
    sprintf(buffer, "TIME REMAINING:%d\r\n", game->game_time_remaining);
    serial_output_string(buffer, &USART1_PORT);
}

// Add this function to reset all touchpads (for game start)
void reset_touchpads(void) {
    // Reset tracking array
    for (int i = 0; i < 6; i++) {
        touchpad_used[i] = false;
    }

    // Re-enable all touchpad interrupts
    static const uint8_t touch_pins[6] = {7, 6, 5, 4, 3, 13};
    for (int i = 0; i < 6; i++) {
        uint8_t pin_num = touch_pins[i];
        EXTI->IMR |= (1 << pin_num);   // Unmask (enable) the interrupt
        EXTI->PR |= (1 << pin_num);    // Clear any pending interrupt
    }

    serial_output_string("All touchpads re-enabled\r\n", &USART1_PORT);
}

// Add this function to disable a specific touchpad
void disable_touchpad(uint8_t touchpad_index) {
    if (touchpad_index < 6) {
        touchpad_used[touchpad_index] = true;

        // Map touchpad index to actual pin numbers
        static const uint8_t touch_pins[6] = {7, 6, 5, 4, 3, 13}; // PB7, PB6, PB5, PB4, PB3, PB13
        uint8_t pin_num = touch_pins[touchpad_index];

        // Disable the interrupt for this specific pin
        EXTI->IMR &= ~(1 << pin_num);  // Mask (disable) the interrupt
        EXTI->PR |= (1 << pin_num);    // Clear any pending interrupt

        char buffer[64];
        sprintf(buffer, "Touchpad %d (PB%d) disabled - already used\r\n", touch_pins[touchpad_index], pin_num);
        serial_output_string(buffer, &USART1_PORT);
    }
}

// --- Start Game Signal (from USART or button) ---
void start_game(GameState *game) {
	// Restart game state
    game->game_over = 0;
    game->game_time_remaining = 240;
    game->digs_remaining = 4;

    int count = 0;
    for (int i = 0; i < 6; i++) {
        if (game->correct_servos[i] != 0) {
            count++;
        }
    }
    game->total_items_to_find = count;
    game->items_left_to_find = game->total_items_to_find; // Initialize based on actual treasures

    // Calibrate: Set all servos to 0°
    for (uint8_t servoId = 1; servoId <= 6; servoId++)
    {
      SetServoAngle(servoId, 0);
    }

    reset_touchpads();

    // Init game timer
    timer_init();
    const TimerSel tim_a = TIMER_SEL_7;
    timer_prescaler_set(tim_a, 11999);
    timer_period_set(tim_a, 3999);
    timer_silent_set(tim_a, false);
    timer_recur_set(tim_a, true);
    timer_callback_set(tim_a, &fn_a);
    timer_enable_set(tim_a, true);

    serial_output_string("Game Started\r\n\n", &USART1_PORT);

    transmit_game_state();

}

//Check for game over conditions
uint8_t check_game_over(GameState *game) {
    if (game->digs_remaining == 0 || game->game_time_remaining == 1 || game->items_left_to_find == 0) {
  	    const TimerSel tim_a = TIMER_SEL_7;

  	    if (game->items_left_to_find == 0) {
      	  	timer_enable_set(tim_a, false);

      		serial_output_string((char *) "You Win!\n", &USART1_PORT);

      	}
      	else {
      	  	timer_enable_set(tim_a, false);

      		serial_output_string((char *) "Game Over\n", &USART1_PORT);
      	}

        game->game_over = 1;
        return 1;
    }
    return 0;
}


// =================================== Callback Functions ===================================

// Each EXTI handler calls this with the corresponding pin number
void handle_touch(uint8_t pad) {
	if (touch_enabled){
		// Map pin number to touchpad index
		static const uint8_t touch_pins[6] = {7, 6, 5, 4, 3, 13}; // PB7, PB6, PB5, PB4, PB3, PB13
		uint8_t touchpad_index = 255; // Invalid index

		for (uint8_t i = 0; i < 6; i++) {
			if (pad == touch_pins[i]) {
				touchpad_index = i;
				break;
			}
		}

		// Check if this touchpad has already been used
		if (touchpad_index < 6 && touchpad_used[touchpad_index]) {
			char buffer[64];
			sprintf(buffer, "Touchpad %d already used - ignoring\r\n", touchpad_index);
			serial_output_string(buffer, &USART1_PORT);
			return; // Ignore this touch
		}

		// If we get here, the touchpad is valid and hasn't been used
		triggers.touchpad_pressed = pad;
	}
}

// Transmit callback
void output_callback() {
	return;
}

// Receive callback
void input_callback(char *data, uint32_t len) {
	// Check for game start input
	char compare[] = "game start";
	uint16_t test = strcmp(data, compare);
	if (!test) {
		start_game(&game);
	}
}

void SetServoAngle(uint8_t servoId, uint16_t angle)
{
  if (angle > 180) angle = 180;
  uint32_t pulse = PWM_MIN_PULSE + (angle * (PWM_MAX_PULSE - PWM_MIN_PULSE) / 180);

  switch (servoId)
  {
    case 1: __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse); break; // PE2
    case 2: __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse); break; // PE3
    case 3: __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse); break; // PA0
    case 4: __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pulse); break; // PA1
    case 5: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, pulse); break; // PD12
    case 6: __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, pulse); break; // PD13
    default: break;
  }

  // Log action
  char txBuffer[64];
  sprintf(txBuffer, "Setting Servo %d to %d°\r\n", servoId, angle);
  serial_output_string(txBuffer, &USART1_PORT);
}

/**
  * @brief Handle touch sensor interrupt
  * @param pin_index: EXTI line (0-15)
  * @retval None
  */


void get_servo(uint8_t pin_index)
{
  // Map EXTI line to touch sensor index (0-5)
  static const uint8_t touch_pins[6] = {7, 6, 5, 4, 3, 13}; // PB7, PB6, PB5, PB4, PB3, PB13
  static const uint8_t touch_ports[6] = {1, 1, 1, 1, 1, 1}; // GPIOB=1
  uint8_t touch_index = 255;

  for (uint8_t i = 0; i < 6; i++)
  {
    if (pin_index == touch_pins[i] && touch_ports[i] == 1) // All on GPIOB
    {
      touch_index = i;
      break;
    }
  }

  if (touch_index < 6)
  {
    uint8_t servoId = touch_to_servo_map[touch_index];
    if (servoId >= 1 && servoId <= 6)
    {
      // Toggle servo state
      uint8_t servoIndex = servoId - 1; // Array index (0-5)
      servo_states[servoIndex] = !servo_states[servoIndex];
      uint16_t angle = servo_states[servoIndex] ? 90 : 0;

      char txBuffer[64];
      sprintf(txBuffer, "Touch detected on PB%d, toggling Servo %d to %d°\r\n\n",
              touch_pins[touch_index], servoId, angle);
      serial_output_string(txBuffer, &USART1_PORT);


      SetServoAngle(servoId, angle);
    }
  }
}

void dig_used(uint8_t servoId) {
    // This is a scaffolding function.
    // It's called when the active servo is fully opened via potentiometer control.
    char buffer[128];

    if (game.game_over || game.digs_remaining == 0) {
        sprintf(buffer, "Dig attempt on servo %d, but game is over or no digs left.\r\n", servoId);
        serial_output_string(buffer, &USART1_PORT);
        return;
    }

    // A dig is always consumed when this function is called
    game.digs_taken++;
    game.digs_remaining--;

    bool success = false;
    uint8_t treasure_value = 0; // To store the value/type of treasure if found

    // Check for treasure - servoId is 1-6, array is 0-5
    if (servoId >= 1 && servoId <= 6) {
        if (game.correct_servos[servoId - 1] != 0) {
            success = true;
            treasure_value = game.correct_servos[servoId - 1];
            game.items_found++;
            game.items_left_to_find--;

            // Mark this treasure as found by setting its spot to 0 (or another indicator if needed)
            // For now, let's assume a treasure once found cannot be "found" again for points.
            // game.correct_servos[servoId - 1] = 0; // Optional: Prevent re-finding the same treasure
        }
    }

    if (success) {
        sprintf(buffer, "DIG SUCCESS at Servo %d! Found Treasure (Value: %d). Digs left: %d. Treasures left: %d\r\n",
                servoId, treasure_value, game.digs_remaining, game.items_left_to_find);
    } else {
        sprintf(buffer, "DIG FAIL at Servo %d. No treasure. Digs left: %d. Treasures left: %d\r\n",
                servoId, game.digs_remaining, game.items_left_to_find);
    }
    serial_output_string(buffer, &USART1_PORT);

    transmit_game_state(); // Send updated Digs Remaining / Treasures Left
    check_game_over(&game); // Check if this dig ended the game

    // The old update_game_state function might become redundant or be refactored.
    // For now, its core logic (decrementing digs, updating items) is handled here.
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  //MX_USART1_UART_Init();


  /* USER CODE BEGIN 2 */

  //HAL_UART_Receive_IT(&huart1, &rx_data, 1);

  // Initialize touch sensors
  GPIO *touch_pads_pb = init_port(B, INPUT, 3, 13); // PB3-PB7, PB13
  GPIO *trim_pot = init_port(A, ANALOG, 4, 4); // PA4 for trimpot

  // Enable interrupts for touch sensors
  enable_interupt(touch_pads_pb, 3, RISING_EDGE, 0, &handle_touch); // PB3
  enable_interupt(touch_pads_pb, 4, RISING_EDGE, 0, &handle_touch); // PB4
  enable_interupt(touch_pads_pb, 5, RISING_EDGE, 0, &handle_touch); // PB5
  enable_interupt(touch_pads_pb, 6, RISING_EDGE, 0, &handle_touch); // PB6
  enable_interupt(touch_pads_pb, 7, RISING_EDGE, 0, &handle_touch); // PB7
  enable_interupt(touch_pads_pb, 13, RISING_EDGE, 0, &handle_touch); // PB13

  // Initialize servos: Start PWM
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // Servo 1: PE2
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); // Servo 2: PE3
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); // Servo 3: PA0
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2); // Servo 4: PA1
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1); // Servo 5: PD12
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2); // Servo 6: PD13

  // Serial Init
  serial_initialise(115200, &USART1_PORT, &output_callback, &input_callback);

  enable_interrupts(&USART1_PORT);

  // New state variables for trimpot-touch interaction logic
  static bool isActiveMode = false;
  static uint8_t activeServoId = 0; // Will be 1-6
  static bool servoFullyOpened = false;
  static bool servoFullyClosed = false;
  static uint8_t activeTouchpadPin = 0; // Stores the pin number of the triggering touch (e.g. 7, 6, ..)
  static uint8_t armed_touchpad_pin = 0; // Pin number of the touch sensor that is "armed" for activation. 0 if none.

  #define POT_ACTIVE_THRESHOLD_RAW 50     // Raw ADC value from trimpot (e.g., ~1.2% of 4095)
  #define SERVO_TARGET_OPEN_ANGLE 90      // Target angle for "fully open"
  #define SERVO_TARGET_CLOSED_ANGLE 0     // Target angle for "fully closed"
  #define SERVO_ANGLE_TOLERANCE 3         // Degrees tolerance for open/close detection

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // Potentiometer reading - moved up for general access if needed, specifically for new logic
    uint16_t pot_raw_value;
    read_pins_analog(trim_pot, &pot_raw_value);

	  // Wait for game start
	  if (game.game_over) {
        // Reset active mode state if game ends/restarts
        isActiveMode = false;
        activeServoId = 0;
        servoFullyOpened = false;
        servoFullyClosed = false;
        activeTouchpadPin = 0;
        armed_touchpad_pin = 0; // Reset armed pin
        touch_enabled = true; // Ensure touches are re-enabled
		  continue;
	  }

	  int check = check_game_over(&game);
	  if (check == 1) {
        // Reset active mode state if game ends due to conditions
        isActiveMode = false;
        activeServoId = 0;
        servoFullyOpened = false;
        servoFullyClosed = false;
        activeTouchpadPin = 0;
        armed_touchpad_pin = 0; // Reset armed pin
        touch_enabled = true;
		  continue;
	  }

    // --- New Trimpot and Touch Interaction Logic ---
    if (!isActiveMode) {
        touch_enabled = true; // Ensure touches can be registered by ISR

        if (triggers.touchpad_pressed != -1) { // A new touch event from ISR
            // A touch sensor was pressed. Arm it.
            // This will override any previously armed touchpad if not yet activated.
            armed_touchpad_pin = triggers.touchpad_pressed;
            char log_buf[80]; // Increased buffer size
            sprintf(log_buf, "Touchpad PB%d armed. Waiting for pot threshold (>%d).", armed_touchpad_pin, POT_ACTIVE_THRESHOLD_RAW);
            serial_output_string(log_buf, &USART1_PORT);
            triggers.touchpad_pressed = -1; // Consume the ISR flag
        }

        if (armed_touchpad_pin != 0 && pot_raw_value > POT_ACTIVE_THRESHOLD_RAW) {
            // An armed touchpad exists AND pot is above threshold. Activate!
            isActiveMode = true;
            servoFullyOpened = false;
            servoFullyClosed = false; // Reset these for the new cycle
            activeTouchpadPin = armed_touchpad_pin; // This is the pin that triggered the mode
            armed_touchpad_pin = 0; // Disarm, as it's now active and being processed.

            // Map the pressed pin number to a servo ID (1-6)
            uint8_t touch_index_for_servo_map = 255; // 0-5 index for touch_to_servo_map
            static const uint8_t touch_pins_lookup_table[6] = {7, 6, 5, 4, 3, 13}; // PB7, PB6, PB5, PB4, PB3, PB13
            for (uint8_t i = 0; i < 6; i++) {
                if (activeTouchpadPin == touch_pins_lookup_table[i]) {
                    touch_index_for_servo_map = i;
                    break;
                }
            }

            if (touch_index_for_servo_map < 6) {
                activeServoId = touch_to_servo_map[touch_index_for_servo_map]; // Get Servo ID 1-6
                if (activeServoId >= 1 && activeServoId <= 6) {
                    char log_buffer[128];
                    sprintf(log_buffer, "Activated: Armed PB%d (Servo %d), Pot (%d) > Thresh (%d). Controlling Servo %d.\r\n",
                            activeTouchpadPin, activeServoId, pot_raw_value, POT_ACTIVE_THRESHOLD_RAW, activeServoId);
                    serial_output_string(log_buffer, &USART1_PORT);
                    touch_enabled = false; // Disable further touch processing by ISR while in active mode
                } else {
                    serial_output_string("Error: Mapped to invalid Servo ID. Deactivating armed touch.\r\n", &USART1_PORT);
                    isActiveMode = false; // Abort activation
                    activeTouchpadPin = 0; // Clear, was from armed_touchpad_pin which is now 0
                    // touch_enabled = true; // Will be set true at the start of the next !isActiveMode block
                }
            } else {
                char log_buffer[128]; // Increased buffer size
                sprintf(log_buffer, "Error: Armed touch pin PB%d not found in map. Deactivating armed touch.\r\n", activeTouchpadPin);
                serial_output_string(log_buffer, &USART1_PORT);
                isActiveMode = false; // Abort activation
                activeTouchpadPin = 0;
                // touch_enabled = true;
            }
        }
    } else { // isActiveMode == true
        touch_enabled = false; // Keep touch input disabled for ISR

        if (activeServoId >= 1 && activeServoId <= 6) {
            // Map potentiometer value to servo angle (0 to SERVO_TARGET_OPEN_ANGLE)
            uint16_t current_target_angle = (uint16_t)(((float)pot_raw_value * (float)SERVO_TARGET_OPEN_ANGLE) / 0xFFF);
            if (current_target_angle > SERVO_TARGET_OPEN_ANGLE) current_target_angle = SERVO_TARGET_OPEN_ANGLE;
            // if (current_target_angle < SERVO_TARGET_CLOSED_ANGLE) current_target_angle = SERVO_TARGET_CLOSED_ANGLE; // Pot raw is uint

            SetServoAngle(activeServoId, current_target_angle); // Continuously update servo position

            if (!servoFullyOpened) {
                // Check if servo is considered fully open
                if (current_target_angle >= (SERVO_TARGET_OPEN_ANGLE - SERVO_ANGLE_TOLERANCE)) {
                    servoFullyOpened = true;
                    dig_used(activeServoId); // Call the scaffolding function for "dig"

                    char log_buffer[128];
                    sprintf(log_buffer, "Servo %d fully opened (Angle: %d). Pot-control active. Close fully to finish.\r\n",
                            activeServoId, current_target_angle);
                    serial_output_string(log_buffer, &USART1_PORT);
                }
            } else if (!servoFullyClosed) { // servoFullyOpened is true, now waiting for full closure
                // Check if servo is considered fully closed
                if (current_target_angle <= (SERVO_TARGET_CLOSED_ANGLE + SERVO_ANGLE_TOLERANCE)) {
                    servoFullyClosed = true;
                    // Cycle for this servo is complete
                    char log_buffer[128];
                    sprintf(log_buffer, "Servo %d fully closed (Angle: %d). Cycle complete for touch PB%d.\r\n",
                            activeServoId, current_target_angle, activeTouchpadPin);
                    serial_output_string(log_buffer, &USART1_PORT);

                    // Disable the touchpad that was used for this cycle
                    uint8_t touchpad_index_to_disable = 255; // 0-5 index for disable_touchpad
                    static const uint8_t touch_pins_disable_lookup_table[6] = {7, 6, 5, 4, 3, 13};
                    for (uint8_t i = 0; i < 6; i++) {
                        if (activeTouchpadPin == touch_pins_disable_lookup_table[i]) {
                            touchpad_index_to_disable = i;
                            break;
                        }
                    }
                    if (touchpad_index_to_disable < 6) {
                        disable_touchpad(touchpad_index_to_disable); // disable_touchpad expects 0-5 index
                    }

                    // Reset state for the next interaction
                    isActiveMode = false;
                    activeServoId = 0; // Clear active servo
                    activeTouchpadPin = 0; // Clear active pin
                    // servoFullyOpened and servoFullyClosed will be reset at the start of the next activation
                    touch_enabled = true; // Re-enable touch processing by ISR
                }
            }
        } else {
            // This case should ideally not be reached if logic is sound
            serial_output_string("Error: In active mode with invalid activeServoId. Deactivating.\r\n", &USART1_PORT);
            isActiveMode = false;
            touch_enabled = true; // Ensure touches are re-enabled
        }
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  /* USER CODE BEGIN TIM2_Init 0 */
  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */
  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 479; // 48 MHz / 480 = 100 kHz
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = PWM_PERIOD_TICKS - 1; // 20 ms
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */
  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
  /* USER CODE BEGIN TIM3_Init 0 */
  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */
  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 479; // 48 MHz / 480 = 100 kHz
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = PWM_PERIOD_TICKS - 1; // 20 ms
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */
  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);
}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{
  /* USER CODE BEGIN TIM4_Init 0 */
  /* USER CODE END TIM4_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */
  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 479; // 48 MHz / 480 = 100 kHz
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = PWM_PERIOD_TICKS - 1; // 20 ms
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */
  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  /* USER CODE BEGIN USART1_Init 0 */
  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */
  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  /* USER CODE END USART1_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /* Configure PB3-PB7, PB13 for touch sensors */
  GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN; // Ensure low when not touched
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Configure PC4 (TX), PC5 (RX) for USART1 */
  GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Configure PA0, PA1 for TIM2_CH1-2 (Servos 3-4) */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Configure PE2, PE3 for TIM3_CH1-2 (Servos 1-2) */
  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* Configure PD12, PD13 for TIM4_CH1-2 (Servos 5-6) */
  GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
