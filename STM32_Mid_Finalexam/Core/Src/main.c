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
#include <string.h>
#include <stdio.h>
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
UART_HandleTypeDef hlpuart1;

TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
#define RX_BUF_SIZE 64
uint8_t rx_byte;
uint8_t rx_buffer[RX_BUF_SIZE];
uint16_t rx_head = 0, rx_tail = 0;
char line_buffer[64];
uint8_t line_idx = 0;

// C4 狀態變數
uint8_t C4_set = 0;
uint8_t bakudan = 0;
uint8_t dfbool = 0;
uint8_t CUT = 0;
uint8_t Defuse = 0;
uint8_t DFS = 0;

uint32_t Bomb_tick = 0;
uint32_t Defuse_tick = 0;
uint32_t CUT_Time = 10000; // 預設無鉗子 10 秒 (10000ms)，有鉗子 5 秒
int flash = 10;            // 呼吸燈循環速度變數

static uint32_t breath_tick = 0;
static int breath_dir = 1;
static uint32_t ccr_val = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
void parse_command(char *cmd);
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
  MX_LPUART1_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  // 啟動 TIM3 PWM 輸出 (Channel 2, 3, 4)
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

    // 啟動 LPUART1 接收中斷 (Ring Buffer 接收起點)
    HAL_UART_Receive_IT(&hlpuart1, &rx_byte, 1);

    // 程式一開始發送歡迎訊息
    char *hello_msg = "From-STM32:Hello World\r\n";
    HAL_UART_Transmit(&hlpuart1, (uint8_t*)hello_msg, strlen(hello_msg), HAL_MAX_DELAY);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
	  uint32_t current_tick = HAL_GetTick();

	      // 1. 處理 UART Ring/Line Buffer 接收資料
	      while (rx_tail != rx_head) {
	          char c = rx_buffer[rx_tail];
	          rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
	          if (c == '\n' || c == '\r') {
	              if (line_idx > 0) {
	                  line_buffer[line_idx] = '\0';
	                  parse_command(line_buffer);
	                  line_idx = 0;
	              }
	          } else {
	              if (line_idx < sizeof(line_buffer) - 1) {
	                  line_buffer[line_idx++] = c;
	              }
	          }
	      }

	      // 2. C4 啟動中邏輯 (C4_set == 1)
	      if (C4_set == 1) {
	          uint32_t elapsed = current_tick - Bomb_tick;

	          if (elapsed > 20000) {
	              bakudan = 1;
	              C4_set = 0;
	              char *msg = "From-STM32:Booooooooooooom\r\n";
	              HAL_UART_Transmit(&hlpuart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
	          } else if (elapsed > 15000) {
	              flash = 1;
	          } else if (elapsed > 10000) {
	              flash = 3;
	          } else {
	              flash = 5;
	          }

	          // 呼吸燈邏輯 (Channel 3 紅燈與 Channel 4 蜂鳴器)
	          if (current_tick - breath_tick >= (uint32_t)flash) {
	              breath_tick = current_tick;
	              if (breath_dir) {
	                  ccr_val += 15;
	                  if (ccr_val >= 1000) breath_dir = 0;
	              } else {
	                  ccr_val -= 15;
	                  if (ccr_val == 0) breath_dir = 1;
	              }
	              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, ccr_val);
	              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, ccr_val);
	          }

	          // 拆彈按鈕檢測 (PA11)
	          if (Defuse == 1) {
	              if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11) == GPIO_PIN_RESET) { // 按鈕按下
	                  if (DFS == 0) {
	                      Defuse_tick = current_tick;
	                      DFS = 1;
	                  }
	                  uint32_t hold_time = current_tick - Defuse_tick;
	                  uint32_t pwm_val = 0;

	                  if (CUT == 0) {
	                      pwm_val = hold_time / 5; // 無鉗子
	                  } else {
	                      pwm_val = hold_time / 10; // 有鉗子
	                  }
	                  if (pwm_val > 1000) pwm_val = 1000;
	                  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwm_val);

	                  if (hold_time >= CUT_Time) {
	                      dfbool = 1;
	                      C4_set = 0;
	                      char *msg = "From-STM32:Defuse Successful\r\n";
	                      HAL_UART_Transmit(&hlpuart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
	                  }
	              } else {
	                  // 放開按鈕或未按
	                  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
	                  DFS = 0;
	              }
	          }
	      }
	      // 3. C4 停止計時狀態 (C4_set == 0)
	      else {
	          if (bakudan == 1) {
	              // 爆炸：紅燈與蜂鳴器常亮
	              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 999);
	              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 999);
	              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
	          } else if (dfbool == 1) {
	              // 拆除成功：綠燈恆亮
	              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 999);
	              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
	              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
	          } else {
	              // 初始待機狀態
	              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
	              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
	              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
	          }
	      }
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

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
  htim3.Init.Prescaler = 169;
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
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// UART 接收中斷回呼函數 (Ring Buffer 實作)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == LPUART1) {
        rx_buffer[rx_head] = rx_byte;
        rx_head = (rx_head + 1) % RX_BUF_SIZE;
        HAL_UART_Receive_IT(&hlpuart1, &rx_byte, 1);
    }
}

// 外部中斷回呼函數 (檢查 PA11)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_11) {
    	HAL_Delay(10);
        Defuse = 1;
    }
}

// 指令解析函數
void parse_command(char *cmd) {
    if (strcmp(cmd, "7355608") == 0) {
        if (C4_set == 0 && bakudan == 0 && dfbool == 0) {
            C4_set = 1;
            Bomb_tick = HAL_GetTick();
            bakudan = 0;
            dfbool = 0;
            char *msg = "From-STM32:Bomb SET\r\n";
            HAL_UART_Transmit(&hlpuart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
        }
    } else if (strcmp(cmd, "CUT") == 0) {
        CUT = 1;
        CUT_Time = 5000; // 有鉗子拆彈時間縮短為 5 秒
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET); // 點亮黃燈 (PC6)
    }
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
