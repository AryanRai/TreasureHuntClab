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


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "math.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Noah's files
#include "stm32f303xc.h"
#include "midi_note_data.h"
#include "servo_driver_obj.h"
#include "door_obj.h"
#include "door_manager_obj.h"
#include "utils.h"
#include "GPIO.h"

// Josh's files
#include "gpio_josh.h"
#include "serial_josh.h"
#include "structs.h"
#include "timer_josh.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

PCD_HandleTypeDef hpcd_USB_FS;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USB_PCD_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// =================================== Global Variables ====================================
volatile int last_servo_selection = -1;
volatile int last_servo_angle = -1;

// --- Game State Variables ---
GameState game = {
    .correct_servos = {1, 3, 4, 6}, // do not use servo number 0
    .items_found = 0,
    .items_left_to_find = 4,
    .digs_taken = 0,
    .digs_remaining = 4,
    .peeks_used = 0,
    .game_time_remaining = 2400,
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

doorManagerObj *manager;

// =================================== Game Functions ====================================
// Prints via UART game state
void transmit_game_state() {
    char buffer[64];
    sprintf(buffer, "DIGS REMAINING:%d TREASURES:%d\r\n", game.digs_remaining, game.items_left_to_find);
    serial_output_string(buffer, &USART1_PORT);
}

// Timer callback
static void fn_a(const TimerSel sel, GameState *game) {
	game->game_time_remaining = game->game_time_remaining - 2;
    char buffer[64];
    sprintf(buffer, "TIME REMAINING:%d\r\n", game->game_time_remaining);
    serial_output_string(buffer, &USART1_PORT);
}

// --- Start Game Signal (from USART or button) ---
void start_game(GameState *game) {
	// Restart game state
    game->game_over = 0;
    game->game_time_remaining = 2400;
    game->digs_remaining = 4;

    int count = 0;
    for (int i = 0; i < 6; i++) {
        if (game->correct_servos[i] != 0) {
            count++;
        }
    }
    game->total_items_to_find = count;

    // Reset all doors
    door_manager_reset(manager);

    // Init game timer
    timer_init();
    const TimerSel tim_a = TIMER_SEL_3;
    timer_prescaler_set(tim_a, 0xF00);
    timer_period_set(tim_a, 420);
    timer_silent_set(tim_a, false);
    timer_recur_set(tim_a, true);
    timer_callback_set(tim_a, &fn_a);
    timer_enable_set(tim_a, true);

    serial_output_string("Game Started\r\n", &USART1_PORT);

    transmit_game_state();

}

// Game variable update function
void update_game_state(uint8_t result, GameState *game, GameTriggers *triggers) {
	// Update game state depending if successful dig
	if (result == 1) {
		game->items_found++;
		game->digs_remaining--;
		game->digs_taken--;
		game->items_left_to_find--;
		transmit_game_state();
	} else {
		game->digs_remaining--;
		game->digs_taken--;
		transmit_game_state();
	}
}

//Check for game over conditions
uint8_t check_game_over(GameState *game) {
    if (game->digs_remaining == 0 || game->game_time_remaining == 0 || game->items_left_to_find == 0) {

      	if (game->items_left_to_find == 0) {
      		serial_output_string((char *) "You Win!\n", &USART1_PORT);
      	}
      	else {
      		serial_output_string((char *) "Game Over\n", &USART1_PORT);
      	}

      	display_number(0);
        game->game_over = 1;
        //timer_disable();
        return 1;
    }
    return 0;
}
/*
bool pad_dug[6] = { false };

...

if (pad_dug[triggers.servo_controlled]) {
    serial_output_string("Pad already dug\r\n", &USART1_PORT);
    return;
}

pad_dug[triggers.servo_controlled] = true;
*/
// =================================== Callback Functions ===================================

// Each EXTI handler calls this with the corresponding pin number
void handle_touch(uint8_t pad, GameTriggers *trigger) {
	display_number(pad);
	trigger->touchpad_pressed = pad;
}

// Transmit callback
void output_callback() {
	return;
}

// Receive callback
void input_callback(char *data, uint32_t len) {

	serial_output_string((char *)"You typed: ", &USART1_PORT);
	serial_output_string(data, &USART1_PORT);					// Transmit back what was received
	serial_output_string((char *)"\r\n", &USART1_PORT);

	// Check for game start input
	char compare[] = "game start";
	uint16_t test = strcmp(data, compare);
	if (!test) {
		start_game(&game);
	}
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
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USB_PCD_Init();
  MX_TIM2_Init();


  /* USER CODE BEGIN 2 */

  // =================================== Init ====================================
  // Serial Init
  serial_initialise(BAUD_115200, &USART1_PORT, &output_callback, &input_callback);
  enable_interrupts(&USART1_PORT);

  // Touch Init
  initialise_touch();
  enable_touch_interrupts();
  touch_register_callback(&handle_touch);

  GPIO *pot = init_port(A, ANALOG, 4, 6);
  uint16_t analog_out[2];

  //Init servo-driver
  servoDriverObj *driver = init_servo_driver(0x40);

  float master_angle = 0.0f;
  //manager was initialized eariler (for touch_pad handler to reference)
  manager = init_door_manager(8, &master_angle, driver, 8);

  //Servo ROM test
  door_manager_set_angle(manager, 0, 80.0f);
  door_manager_set_angle(manager, 1, 80.0f);
  door_manager_update(manager);
  HAL_Delay(500);

  door_manager_set_angle(manager, 0, 0.0f);
  door_manager_set_angle(manager, 1, 0.0f);
  door_manager_update(manager);
  HAL_Delay(1000);

  const float SPEED = 1.2;
  const int PAUSE = 20;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  // =================================== Game Loop ====================================
  while (1)
 {
  	// Wait for game start
  	if (game.game_over) {
  		continue;
  	}

  	int check = check_game_over(&game);
  	if (check == 1) {
  		continue;
  	}

    if (triggers.touchpad_pressed != -1) {
    	triggers.servo_controlled = triggers.touchpad_pressed;
        transmit_game_state();

        char buffer[64];
        sprintf(buffer, "touchpad %d chosen, door %d being controlled!\r\n", triggers.touchpad_pressed,  triggers.servo_controlled);
        serial_output_string(buffer, &USART1_PORT);

        if (triggers.servo_controlled != -1 && triggers.servo_controlled != last_servo_selection) {

        	// Run peek loop for short time
        	uint32_t peek_start = HAL_GetTick();
        	bool committed_dig = false;

        	while (HAL_GetTick() - peek_start < 2000) {
        		read_pins_analog(pot, analog_out);

        	    master_angle = map_range((float)analog_out[1], 0.0f, 4095.0f, 0.0f, 80.0f);

        	    door_manager_update(manager);

        	    float trimpot = map_range((float)analog_out[0], 0.0f, 4095.0f, 0.0f, 100.0f);

        	    if (trimpot >= triggers.peek_threshold) {
        	    	 committed_dig = true;
        	         break;
        	    }
          	}
        	// Now process peek or dig
        	if (committed_dig) {
        		// Dig
        	    bool success = false;
        	    for (int i = 0; i < 6; i++) {
        	    	if (game.correct_servos[i] == triggers.servo_controlled) {
        	    		success = true;
        	            break;
        	        }
        	    }

        	    update_game_state(success ? 1 : 0, &game, &triggers);
        	    char idk[64];
        	    sprintf(idk, "DIG %s at pad %d\r\n", success ? "SUCCESS" : "FAIL", triggers.servo_controlled);
        	    serial_output_string(idk, &USART1_PORT);

             } else {
            	 game.peeks_used++;
           		 serial_output_string((char *) "PEEK ONLY\r\n", &USART1_PORT);

             }

        	 triggers.touchpad_pressed = -1;
        	 last_servo_selection = triggers.servo_controlled;
        	 triggers.servo_controlled = -1;

        	 char yes[64];
        	 sprintf(yes, "touchpad reset to %d, servo %d, previous servo %d\r\n", triggers.touchpad_pressed, triggers.servo_controlled, last_servo_selection);
        	 serial_output_string(yes, &USART1_PORT);
         }
     }
	  /* USER CODE END WHILE */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00201D2B;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_4BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  htim2.Init.Prescaler = 23;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */

  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */

  /* USER CODE END USB_Init 1 */
  hpcd_USB_FS.Instance = USB;
  hpcd_USB_FS.Init.dev_endpoints = 8;
  hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */

  /* USER CODE END USB_Init 2 */

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, CS_I2C_SPI_Pin|LD4_Pin|LD3_Pin|LD5_Pin
                          |LD7_Pin|LD9_Pin|LD10_Pin|LD8_Pin
                          |LD6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : DRDY_Pin MEMS_INT3_Pin MEMS_INT4_Pin MEMS_INT1_Pin
                           MEMS_INT2_Pin */
  GPIO_InitStruct.Pin = DRDY_Pin|MEMS_INT3_Pin|MEMS_INT4_Pin|MEMS_INT1_Pin
                          |MEMS_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_I2C_SPI_Pin LD4_Pin LD3_Pin LD5_Pin
                           LD7_Pin LD9_Pin LD10_Pin LD8_Pin
                           LD6_Pin */
  GPIO_InitStruct.Pin = CS_I2C_SPI_Pin|LD4_Pin|LD3_Pin|LD5_Pin
                          |LD7_Pin|LD9_Pin|LD10_Pin|LD8_Pin
                          |LD6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

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
