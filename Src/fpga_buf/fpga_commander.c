/*
 * fpga_commander.c
 *
 *  Created on: May 25, 2019
 *      Author: khomin
 */
#include "fpga_buf/fpga_commander.h"
#include "fpga_buf.h"
#include <stm32f4xx_it.h>
#include "FreeRTOS.h"
#include "task.h"
#include "debug_print.h"
#include "string.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "main.h"

extern UART_HandleTypeDef huart2;
static bool isActiveUart = false;

xQueueHandle fpgaDataQueue;

static uint8_t fpga_temp_buff[FPGA_BUFFER_RECORD_MAX_SIZE] = {0};
static uint16_t fpga_temp_buff_count = 0;

void fpgaTask(void *argument) {
	/* USER CODE BEGIN 5 */
	init_fpga();

	fpgaDataQueue = xQueueCreate(4, sizeof(sFpgaDataStruct*));

	HAL_UART_Receive_IT(&huart2, fpga_temp_buff, 1);

	/* Infinite loop */
	for(;;) {
		// detect new data uart
		if(isActiveUart) {
			vTaskDelay(500/portTICK_RATE_MS);
			isActiveUart = false;
			vTaskDelay(500/portTICK_RATE_MS);
			if(!isActiveUart) {
				if(fpga_temp_buff_count != 0) {
					DBGLog("FPGA: new data, size %d", fpga_temp_buff_count);
					putFpgaRecord(fpga_temp_buff, fpga_temp_buff_count);

					fpga_temp_buff_count = 0;
					memset(fpga_temp_buff, 0, sizeof(fpga_temp_buff));
				}
			}
		}

		vTaskDelay(500/portTICK_RATE_MS);
	}
	/* USER CODE END 5 */
}

//--- detect user button
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if(!HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin)) {
		HAL_GPIO_WritePin(FPGA_CS_LINE_GPIO_Port, FPGA_CS_LINE_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(FPGA_CS_LINE_GPIO_Port, FPGA_CS_LINE_Pin, GPIO_PIN_RESET);
	}
}

void fpgaRxUartHandler(UART_HandleTypeDef *huart) {
	isActiveUart = true;
	if(huart->Instance->SR & USART_SR_IDLE) {
		DBGLog("FPGA: buffer ready");
	} else {
		if(fpga_temp_buff_count < FPGA_BUFFER_RECORD_MAX_SIZE) {
			fpga_temp_buff_count += 1;
		} else {
			DBGErr("FPGA: buffer overflow");
			fpga_temp_buff_count = 0;
		}
	}
	HAL_UART_Receive_IT(huart, fpga_temp_buff + fpga_temp_buff_count, 1);
}
