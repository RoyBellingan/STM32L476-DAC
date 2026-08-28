/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SYNC_Pin GPIO_PIN_0
#define SYNC_GPIO_Port GPIOB
#define SYNC_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

/* Strap Arduino A0 (PA0) to GND, or hold the Nucleo USER button (PC13). */
#define AUTOSTART_Pin GPIO_PIN_0
#define AUTOSTART_GPIO_Port GPIOA
#define USER_BTN_Pin GPIO_PIN_13
#define USER_BTN_GPIO_Port GPIOC

/* PA4 = DAC1 OUT1 (single-ended into BatV1). PA5 = Nucleo LD2. */
#define RUN_LED_Pin GPIO_PIN_5
#define RUN_LED_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

extern DAC_HandleTypeDef hdac1;
extern TIM_HandleTypeDef htim6;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_dac1;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
