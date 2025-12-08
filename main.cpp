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
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

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
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  int security_open=1, alert_state=0, alert_period=0;
  int  tilt_rec,  tilt_cooldown = 0, tilt_press=0;
  int laser_rec, laser_cooldown = 0;
  int timer=HAL_GetTick();
  int led_yellow = 1, led_red = 1, led_period=1000, led_cooldown=0, led_state=0;
  uint8_t rx_buffer[500]="", tx_buffer[3];
  uint8_t rx;
  uint8_t rx_char;
  uint8_t idx = 0;

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 1); // open laser

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

//	   time
	  int time_now = HAL_GetTick();

	  tilt_cooldown -= time_now-timer;
	  if (tilt_cooldown<0) tilt_cooldown=0;
	  tilt_press -= time_now-timer;
	  if (tilt_press<0) tilt_press=0;
	  laser_cooldown -= time_now-timer;
	  if (laser_cooldown<0) laser_cooldown=0;
	  led_cooldown -= time_now-timer;
	  if (led_cooldown<0) led_cooldown=0;
	  alert_period -= time_now-timer;
	  if (alert_period<0) alert_period=0;

	  timer=time_now;

	  // blink LED
	  if (led_cooldown<=0){
		  led_cooldown=led_period;
		  led_state^=1;
		  if (led_state){
			  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, led_yellow);
			  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, led_red);
		  }
		  else{
			  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, 0);
			  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, 0);
		  }
	  }

	  // UART receive

	  if (HAL_UART_Receive(&huart1, &rx_char, 1, 10) == HAL_OK)
	      {
	          // If Enter key pressed
	          if (rx_char == '\r' || rx_char == '\n')
	          {
	              tx_buffer[idx] = '\0';  // null-terminate the string

	              // Echo back to UART
	              sprintf((char*)rx_buffer, "input: %s!\r\n", tx_buffer);
	              HAL_UART_Transmit(&huart2, rx_buffer, strlen((char*)rx_buffer), HAL_MAX_DELAY);

	              // Your command logic
	              if (tx_buffer[0] == '0')
	              {
	                  led_red = 0;
	                  led_yellow = 1;
	                  led_period = 1500;
	                  security_open = 0;
	              }
	              else if (tx_buffer[0] == '1')
	              {
	                  security_open = 1;
	              }

	              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, security_open);

	              // Reset buffer
	              idx = 0;
	          }
	          else
	          {
	              // Store character if not Enter
	              if (idx < 63)
	              {
	                  tx_buffer[idx++] = rx_char;
	              }
	          }
	      }
//	  if (HAL_UART_Receive(&huart2, tx_buffer, sizeof(tx_buffer), 5)==HAL_OK){
//		  sprintf(rx_buffer, "input: %s!\n\r", tx_buffer);
//		  HAL_UART_Transmit(&huart2, rx_buffer, sizeof(rx_buffer), HAL_MAX_DELAY);
//		  if (tx_buffer[0]=='0'){
//			  led_red=0;
//			  led_yellow=1;
//			  led_period=1500;
//			  security_open=0;
//		  }
//		  if (tx_buffer[0]=='1'){
//			  security_open=1;
//		  }
//		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, security_open);
//		  tx_buffer[0]=='!';
//	  }

//
//	  // security state
//
	  if (!security_open){
		  continue;
	  }
//
	  if (alert_state && alert_period<=0){
		  led_red=1;
		  led_yellow=1;
		  led_period=1000;
		  alert_state=0;
	  }
//
	  // sensors

	  tilt_rec = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5);
	  laser_rec = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);

	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, tilt_rec);

	  if (tilt_cooldown<=0){
		  if (tilt_press<=0 && !tilt_rec){
			  sprintf(rx_buffer, "tilt detected %d!\n", tilt_rec);
			  HAL_UART_Transmit(&huart1, rx_buffer, sizeof(rx_buffer), HAL_MAX_DELAY);
			  char tx_buffer[100];

			  snprintf(tx_buffer, sizeof(tx_buffer), "%s\r", rx_buffer);   // Add \r
			  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), HAL_MAX_DELAY);

			  alert_state=1;
			  alert_period=1000;
			  led_red=1;
			  led_yellow=0;
			  led_period=100;
			  tilt_cooldown=1000;
		  }
		  else if (tilt_rec){
			  tilt_press=1500;
		  }
	  }

	  if (laser_cooldown<=0 && laser_rec){
		  sprintf(rx_buffer, "laser detected %d!\n", laser_rec);
		  HAL_UART_Transmit(&huart1, rx_buffer, sizeof(rx_buffer), HAL_MAX_DELAY);
		  char tx_buffer[100];

		  snprintf(tx_buffer, sizeof(tx_buffer), "%s\r", rx_buffer);   // Add \r
		  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), HAL_MAX_DELAY);

		  alert_state=1;
		  alert_period=1000;
		  led_red=1;
		  led_yellow=0;
		  led_period=100;
		  laser_cooldown=1000;
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|LD2_Pin|GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 LD2_Pin PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|LD2_Pin|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
