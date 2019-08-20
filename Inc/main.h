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
#define FLASH_EXTERN_CS_Pin GPIO_PIN_4
#define FLASH_EXTERN_CS_GPIO_Port GPIOE
#define FLASH_EXTERN_SCK_Pin GPIO_PIN_2
#define FLASH_EXTERN_SCK_GPIO_Port GPIOE
#define SD_CARD_CLK_Pin GPIO_PIN_12
#define SD_CARD_CLK_GPIO_Port GPIOC
#define FLASH_EXTERN_MISO_Pin GPIO_PIN_5
#define FLASH_EXTERN_MISO_GPIO_Port GPIOE
#define FLASH_EXTERN_MOSI_Pin GPIO_PIN_6
#define FLASH_EXTERN_MOSI_GPIO_Port GPIOE
#define SD_CARD_D3_Pin GPIO_PIN_11
#define SD_CARD_D3_GPIO_Port GPIOC
#define SD_CARD_D2_Pin GPIO_PIN_10
#define SD_CARD_D2_GPIO_Port GPIOC
#define FPGA_REST_Pin GPIO_PIN_2
#define FPGA_REST_GPIO_Port GPIOI
#define SD_CARD_CMD_Pin GPIO_PIN_2
#define SD_CARD_CMD_GPIO_Port GPIOD
#define FPGA_CS_Pin GPIO_PIN_15
#define FPGA_CS_GPIO_Port GPIOC
#define SD_CARD_D1_Pin GPIO_PIN_9
#define SD_CARD_D1_GPIO_Port GPIOC
#define SD_CARD_DO_Pin GPIO_PIN_8
#define SD_CARD_DO_GPIO_Port GPIOC
#define USER_BUTTON_Pin GPIO_PIN_7
#define USER_BUTTON_GPIO_Port GPIOC
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
