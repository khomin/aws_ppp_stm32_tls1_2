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
#include "semphr.h"
#include "main.h"

//#define TEST_FPGA				1

extern UART_HandleTypeDef huart2;
static bool isActiveUart = false;

static sFpgaData fpgaData = {0};
static uint8_t rxByte = 0;

xQueueHandle fpgaDataQueue;

static void insertJsonStartField(sFpgaData * pdata);
static void insertJsonEndField(sFpgaData * pdata);
static void getNewFpgaData();

void fpgaTask(void *argument) {
	init_fpga();

	fpgaDataQueue = xQueueCreate(1, sizeof(sFpgaData*));

	HAL_UART_Receive_IT(&huart2, &rxByte, 1);

	/* Infinite loop */
	for(;;) {

		getNewFpgaData();

		// detect new data uart
#ifndef TEST_FPGA
		if(isActiveUart) {
			vTaskDelay(100/portTICK_RATE_MS);
			isActiveUart = false;
			vTaskDelay(100/portTICK_RATE_MS);
			if(!isActiveUart) {
				if(fpgaData.count > FPGA_MIN_DATA_SIZE) {
					DBGLog("FPGA: new data, size %d", fpgaData.count);

					insertJsonEndField(&fpgaData);

					while(fpgaData.statusProcessed != efpgaStatusSent) {
						fpgaData.statusProcessed = efpgaStatusWait;
						fpgaData.magic_word = FPGA_MAGIC_WORD;
						//-- put to queue
						if(putFpgaRecord(&fpgaData)) {
							//-- print to usb
							putFpgaReocordToUsb(&fpgaData);
							//-- while mqtt will be send it
							while(fpgaData.statusProcessed == efpgaStatusWait) {}
							if(fpgaData.statusProcessed == efpgaStatusError) {
								DBGLog("FPGA: send error");
							}
							vTaskDelay(500/portTICK_RATE_MS);
						} else {
							DBGLog("FPGA: putFpgaRecord error");
						}
					}
					memset(fpgaData.data, 0, fpgaData.count);
					fpgaData.count = 0;
				}
			}
		}
#else

		insertJsonStartField(&fpgaData);

		for(; fpgaData.count<sizeof(fpgaData.data)-50; fpgaData.count++) {
			fpgaData.data[fpgaData.count] = 'x';
		}
		insertJsonEndField(&fpgaData);

		while(fpgaData.statusProcessed != efpgaStatusSent) {
			fpgaData.statusProcessed = efpgaStatusWait;
			fpgaData.magic_word = FPGA_MAGIC_WORD;
			if(putFpgaRecord(&fpgaData)) {
				while(fpgaData.statusProcessed == efpgaStatusWait) {}
				if(fpgaData.statusProcessed == efpgaStatusError) {
					DBGLog("FPGA: send error");
				}
				vTaskDelay(500/portTICK_RATE_MS);
			} else {
				DBGLog("FPGA: putFpgaRecord error");
			}
		}
		memset(fpgaData.data, 0, sizeof(fpgaData.count));
#endif

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

void getNewFpgaData() {
	HAL_GPIO_WritePin(FPGA_CS_LINE_GPIO_Port, FPGA_CS_LINE_Pin, GPIO_PIN_SET);
	vTaskDelay(100/portTICK_RATE_MS);
	HAL_GPIO_WritePin(FPGA_CS_LINE_GPIO_Port, FPGA_CS_LINE_Pin, GPIO_PIN_RESET);
	vTaskDelay(100/portTICK_RATE_MS);
}

void fpgaRxUartHandler(UART_HandleTypeDef *huart) {
	isActiveUart = true;
	if(huart->Instance->SR & USART_SR_IDLE) {
		DBGLog("FPGA: buffer ready");
	} else {
		if(fpgaData.count < FPGA_BUFFER_RECORD_MAX_SIZE) {
			if(fpgaData.count == 0) { //-- add start json
				insertJsonStartField(&fpgaData);
			}
			sprintf((char*)&fpgaData.data[fpgaData.count], "%02x", rxByte);
			fpgaData.count += 2;
		} else {
			DBGErr("FPGA: buffer overflow");
			fpgaData.count = 0;
		}
	}
	HAL_UART_Receive_IT(huart, &rxByte, 1);
}

void insertJsonStartField(sFpgaData * pdata) {
	static long id = 0;
	sprintf(pdata->data, "{\"id\":%ld,\"state\":{\"desired\":{\"message\":\"", id++);
	pdata->count = strlen(pdata->data);
}

void insertJsonEndField(sFpgaData * pdata) {
	pdata->data[pdata->count++] = '"';
	pdata->data[pdata->count++] = '}';
	pdata->data[pdata->count++] = '}';
	pdata->data[pdata->count++] = '}';
}
