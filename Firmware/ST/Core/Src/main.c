/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "oled.h"
#include "ina238.h"
#include "banana.h"
#include "boot.h"
#include "meter.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NOTE_C4   262
#define NOTE_D4   294
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_G4   392
#define NOTE_A4   440
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_D5   587
#define NOTE_E5   659
#define NOTE_F5   698
#define NOTE_G5   784
#define NOTE_REST 0

#define Q   300
#define H   600
#define DQ  450
#define E8  150
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
void Buzzer_Tone(uint32_t freq_hz, uint32_t duration_ms);
void Buzzer_Off(void);
void Play_Song1(void);
void Play_Song2(void);
void Play_Song3(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_I2C2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(OLED_RESET_GPIO_Port, OLED_RESET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(OLED_POWER_EN_GPIO_Port, OLED_POWER_EN_Pin, GPIO_PIN_SET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(OLED_RESET_GPIO_Port, OLED_RESET_Pin, GPIO_PIN_SET);
  HAL_Delay(10);
  OLED_Init();
  if (INA238_Init() != HAL_OK)
  {
      OLED_WriteString(3, 0, "INA238 init fail");
      HAL_Delay(2000);
      OLED_Clear();
  }
  BOOT_Run();   /* one-shot boot screen with graph animation */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
  METER_Run();   /* handles meter + graph screens + Easter-egg dance; never returns */
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00503D58;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

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
  htim3.Init.Prescaler = 15;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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
  sConfigOC.Pulse = 499;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, OLED_POWER_EN_Pin|OLED_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BUCK_PG_Pin */
  GPIO_InitStruct.Pin = BUCK_PG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BUCK_PG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_RESET_Pin BTN_AUX_Pin BTN_PAGE_Pin */
  GPIO_InitStruct.Pin = BTN_RESET_Pin|BTN_AUX_Pin|BTN_PAGE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : INA_ALERT_Pin */
  GPIO_InitStruct.Pin = INA_ALERT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(INA_ALERT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : OLED_POWER_EN_Pin OLED_RESET_Pin */
  GPIO_InitStruct.Pin = OLED_POWER_EN_Pin|OLED_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


void Buzzer_Off(void)
{
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
}

void Buzzer_Tone(uint32_t freq_hz, uint32_t duration_ms)
{
  if (freq_hz == NOTE_REST)
  {
    Buzzer_Off();
    HAL_Delay(duration_ms);
    return;
  }
  uint32_t arr = (1000000UL / freq_hz) - 1;
  __HAL_TIM_SET_AUTORELOAD(&htim3, arr);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, arr / 2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  HAL_Delay(duration_ms - 40);
  Buzzer_Off();
  HAL_Delay(40);
}

void Play_Song1(void)
{
  /* Twinkle Twinkle Little Star — BTN_RESET */
  static const uint32_t melody[][2] = {
    {NOTE_C4, Q}, {NOTE_C4, Q}, {NOTE_G4, Q}, {NOTE_G4, Q},
    {NOTE_A4, Q}, {NOTE_A4, Q}, {NOTE_G4, H},
    {NOTE_F4, Q}, {NOTE_F4, Q}, {NOTE_E4, Q}, {NOTE_E4, Q},
    {NOTE_D4, Q}, {NOTE_D4, Q}, {NOTE_C4, H},
    {NOTE_G4, Q}, {NOTE_G4, Q}, {NOTE_F4, Q}, {NOTE_F4, Q},
    {NOTE_E4, Q}, {NOTE_E4, Q}, {NOTE_D4, H},
    {NOTE_G4, Q}, {NOTE_G4, Q}, {NOTE_F4, Q}, {NOTE_F4, Q},
    {NOTE_E4, Q}, {NOTE_E4, Q}, {NOTE_D4, H},
    {NOTE_C4, Q}, {NOTE_C4, Q}, {NOTE_G4, Q}, {NOTE_G4, Q},
    {NOTE_A4, Q}, {NOTE_A4, Q}, {NOTE_G4, H},
    {NOTE_F4, Q}, {NOTE_F4, Q}, {NOTE_E4, Q}, {NOTE_E4, Q},
    {NOTE_D4, Q}, {NOTE_D4, Q}, {NOTE_C4, H},
  };
  for (uint32_t i = 0; i < sizeof(melody) / sizeof(melody[0]); i++)
    Buzzer_Tone(melody[i][0], melody[i][1]);
}

void Play_Song2(void)
{
  /* Happy Birthday — BTN_AUX */
  static const uint32_t melody[][2] = {
    {NOTE_G4, DQ}, {NOTE_G4, E8}, {NOTE_A4, Q}, {NOTE_G4, Q}, {NOTE_C5, Q}, {NOTE_B4, H}, {NOTE_REST, Q},
    {NOTE_G4, DQ}, {NOTE_G4, E8}, {NOTE_A4, Q}, {NOTE_G4, Q}, {NOTE_D5, Q}, {NOTE_C5, H}, {NOTE_REST, Q},
    {NOTE_G4, DQ}, {NOTE_G4, E8}, {NOTE_G5, Q}, {NOTE_E5, Q}, {NOTE_C5, Q}, {NOTE_B4, Q}, {NOTE_A4, H}, {NOTE_REST, Q},
    {NOTE_F5, DQ}, {NOTE_F5, E8}, {NOTE_E5, Q}, {NOTE_C5, Q}, {NOTE_D5, Q}, {NOTE_C5, H},
  };
  for (uint32_t i = 0; i < sizeof(melody) / sizeof(melody[0]); i++)
    Buzzer_Tone(melody[i][0], melody[i][1]);
}

void Play_Song3(void)
{
  /* Mary Had a Little Lamb — BTN_PAGE */
  static const uint32_t melody[][2] = {
    {NOTE_E4, Q}, {NOTE_D4, Q}, {NOTE_C4, Q}, {NOTE_D4, Q},
    {NOTE_E4, Q}, {NOTE_E4, Q}, {NOTE_E4, H},
    {NOTE_D4, Q}, {NOTE_D4, Q}, {NOTE_D4, H},
    {NOTE_E4, Q}, {NOTE_G4, Q}, {NOTE_G4, H},
    {NOTE_E4, Q}, {NOTE_D4, Q}, {NOTE_C4, Q}, {NOTE_D4, Q},
    {NOTE_E4, Q}, {NOTE_E4, Q}, {NOTE_E4, Q}, {NOTE_E4, Q},
    {NOTE_D4, Q}, {NOTE_D4, Q}, {NOTE_E4, Q}, {NOTE_D4, Q},
    {NOTE_C4, H},
  };
  for (uint32_t i = 0; i < sizeof(melody) / sizeof(melody[0]); i++)
    Buzzer_Tone(melody[i][0], melody[i][1]);
}
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
#ifdef USE_FULL_ASSERT
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
