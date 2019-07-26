/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stdbool.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#ifdef USE_MBED_TLS
extern int mbedtls_hardware_poll( void *data, unsigned char *output, size_t len, size_t *olen );
#endif /* USE_MBED_TLS */

#define FLASH_PAGE_SIZE                    ((uint32_t)0x800)

/* Exported functions --------------------------------------------------------*/
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define FPGA_CS_LINE_Pin GPIO_PIN_4
#define FPGA_CS_LINE_GPIO_Port GPIOE
#define FPGA_REST_Pin GPIO_PIN_2
#define FPGA_REST_GPIO_Port GPIOI
#define USER_BUTTON_Pin GPIO_PIN_7
#define USER_BUTTON_GPIO_Port GPIOC
#define USER_BUTTON_EXTI_IRQn EXTI9_5_IRQn
#define GSM_STATUS_Pin GPIO_PIN_7
#define GSM_STATUS_GPIO_Port GPIOG
#define GSM_NETLIGHT_Pin GPIO_PIN_6
#define GSM_NETLIGHT_GPIO_Port GPIOG
#define GSM_RESET_Pin GPIO_PIN_12
#define GSM_RESET_GPIO_Port GPIOH
#define GSM_PWRKY_Pin GPIO_PIN_9
#define GSM_PWRKY_GPIO_Port GPIOH
#define OLED_RESET_Pin GPIO_PIN_11
#define OLED_RESET_GPIO_Port GPIOH
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
