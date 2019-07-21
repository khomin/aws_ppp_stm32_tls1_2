/*
 * fpga_commander.c
 *
 *  Created on: May 25, 2019
 *      Author: khomin
 */
#include "fpga_buf/fpga_commander.h"
#include "../sdram/sdram.h"
#include "fpga_buf.h"
#include <stm32f4xx_it.h>
#include "FreeRTOS.h"
#include "task.h"
#include "debug_print.h"
#include "string.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "main.h"

#define TEST_FPGA				1

extern UART_HandleTypeDef huart2;
static bool isActiveUart = false;

static sFpgaData fpgaData;
static uint8_t rxByte = 0;
xQueueHandle fpgaDataQueue;

static void insertJsonStartField(sFpgaData * pdata);
static void insertJsonEndField(sFpgaData * pdata);
static void initiateNewFpgaData();
#ifdef TEST_FPGA
void testFillData();
#endif

void fpgaTask(void *argument) {
	init_fpga();

	fpgaDataQueue = xQueueCreate(16, sizeof(sFpgaData));

	HAL_UART_Receive_IT(&huart2, &rxByte, 1);

	/* Infinite loop */
	for(;;) {

		// detect new data uart
#if	TEST_FPGA
		testFillData();

#else
		initiateNewFpgaData();
#endif
		if(isActiveUart) {
			vTaskDelay(100/portTICK_RATE_MS);
			isActiveUart = false;
			vTaskDelay(100/portTICK_RATE_MS);
			if(!isActiveUart) {
				if(fpgaData.sdramData != NULL) {
					if(fpgaData.sdramData->len > FPGA_MIN_DATA_SIZE) {
						DBGLog("FPGA: new data, size %lu", fpgaData.sdramData->len);
						//-- insert json header
						insertJsonEndField(&fpgaData);
						//-- put to queue
						if(putFpgaRecord(fpgaData)) {
							putFpgaReocordToUsb(fpgaData);
						} else {
							DBGLog("FPGA: putFpgaRecord error");
						}
					}
					memset((void*)&fpgaData, 0, sizeof(fpgaData));
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

void initiateNewFpgaData() {
	HAL_GPIO_WritePin(FPGA_CS_LINE_GPIO_Port, FPGA_CS_LINE_Pin, GPIO_PIN_SET);
	vTaskDelay(100/portTICK_RATE_MS);
	HAL_GPIO_WritePin(FPGA_CS_LINE_GPIO_Port, FPGA_CS_LINE_Pin, GPIO_PIN_RESET);
	vTaskDelay(100/portTICK_RATE_MS);
}

#ifdef TEST_FPGA
void testFillData() {
	isActiveUart = true;
	if(fpgaData.sdramData == NULL) {
		fpgaData.sdramData = createNewSdramBuff();
		if(fpgaData.sdramData == NULL) {
			DBGErr("FPGA: buf == null");
			return;
		}
	}
//	FPGA_BUFFER_RECORD_MAX_SIZE
	for(uint32_t i=0; i<15; i++) {
		if(fpgaData.sdramData->len == 0) { //-- add start json
			insertJsonStartField(&fpgaData);
		}
		sprintf(((char*)fpgaData.sdramData->data) + fpgaData.sdramData->len, "%02x", rxByte);
		fpgaData.sdramData->len += 2;
	}
}
#endif

void fpgaRxUartHandler(UART_HandleTypeDef *huart) {
	isActiveUart = true;
	if(huart->Instance->SR & USART_SR_IDLE) {
		DBGLog("FPGA: buffer ready");
	} else {
		if(fpgaData.sdramData == NULL) {
			fpgaData.sdramData = createNewSdramBuff();
			if(fpgaData.sdramData == NULL) {
				DBGErr("FPGA: buf == null");
				return;
			}
		}
		if(fpgaData.sdramData->len < FPGA_BUFFER_RECORD_MAX_SIZE) {
			if(fpgaData.sdramData->len == 0) { //-- add start json
				insertJsonStartField(&fpgaData);
			}
			sprintf((char*)&fpgaData.sdramData->data[fpgaData.sdramData->len], "%02x", rxByte);
			fpgaData.sdramData->len += 2;
		} else {
			DBGErr("FPGA: buffer overflow");
			fpgaData.sdramData->len = 0;
		}
	}
	HAL_UART_Receive_IT(huart, &rxByte, 1);
}

void insertJsonStartField(sFpgaData * pdata) {
	static long id = 0;
	sprintf((char*)pdata->sdramData->data, "{\"id\":%ld,\"state\":{\"desired\":{\"message\":\"", id++);
	pdata->sdramData->len = strlen((char*)pdata->sdramData->data);
}

void insertJsonEndField(sFpgaData * pdata) {
	*((uint8_t*)pdata->sdramData->data + pdata->sdramData->len++) = '"';
	*((uint8_t*)pdata->sdramData->data + pdata->sdramData->len++) = '}';
	*((uint8_t*)pdata->sdramData->data + pdata->sdramData->len++) = '}';
	*((uint8_t*)pdata->sdramData->data + pdata->sdramData->len++) = '}';
}
