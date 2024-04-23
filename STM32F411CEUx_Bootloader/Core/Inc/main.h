/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define OTA_DATA_MAX_SIZE ( 1024 )  //Maximum data Size
#define OTA_DATA_OVERHEAD (    9 )  //data overhead
#define OTA_PACKET_MAX_SIZE ( OTA_DATA_MAX_SIZE + OTA_DATA_OVERHEAD )
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

extern unsigned char data_in[OTA_PACKET_MAX_SIZE];
extern unsigned int DataPos;
extern unsigned int TotalCharsReceived;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BTN_OTA_Pin GPIO_PIN_0
#define BTN_OTA_GPIO_Port GPIOA
#define APPMODE_LED_Pin GPIO_PIN_1
#define APPMODE_LED_GPIO_Port GPIOA
#define BOOTMODE_LED_Pin GPIO_PIN_2
#define BOOTMODE_LED_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
