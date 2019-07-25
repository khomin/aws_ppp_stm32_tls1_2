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
#include "prepareJson.h"
#include "commander/commander.h"

//#define TEST_FPGA				1

extern UART_HandleTypeDef huart2;
static bool isActiveUart = false;

static sFpgaData fpgaData;
static uint8_t rxByte = 0;
xQueueHandle fpgaDataQueue;

static const char caption_out_of_memory [] = "FPGA: new data, put error [not enough memory]";

static void initiateNewFpgaData();

#ifdef TEST_FPGA
void testFillData();
void printfTestFillData();
void testSendParserQueue();
#endif

void fpgaTask(void *argument) {
	init_fpga();

	fpgaDataQueue = xQueueCreate(32, sizeof(sFpgaData));

	HAL_UART_Receive_IT(&huart2, &rxByte, 1);

#if	TEST_FPGA
//	xTaskCreate(testSendParserQueue, "testSendParserQueue", 512, 0, tskIDLE_PRIORITY, NULL);
#endif

//	testFillData();
//	printfTestFillData();

	/* Infinite loop */
	for(;;) {

		// detect new data uart
#if	TEST_FPGA
//		testFillData();
//		printfTestFillData();
#else
//		initiateNewFpgaData();
#endif
		if(isActiveUart) {
			vTaskDelay(100/portTICK_RATE_MS);
			isActiveUart = false;
			vTaskDelay(100/portTICK_RATE_MS);
			if(!isActiveUart) {
				if(fpgaData.sdramData != NULL) {
					if(fpgaData.sdramData->len > FPGA_MIN_DATA_SIZE) {
						DBGLog("FPGA: new data, size %lu", fpgaData.sdramData->len);
						//-- put to queue
						if(putFpgaRecord(&fpgaData)) {
							putFpgaReocordToUsb(&fpgaData);
						} else {
							printToUsb((char*)caption_out_of_memory, strlen((char*)caption_out_of_memory));
							DBGLog("FPGA: putFpgaRecord error");
						}
					}
					memset((void*)&fpgaData, 0, sizeof(fpgaData));
				}
			}
		}

		vTaskDelay(3000/portTICK_RATE_MS);
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
	uint32_t counter = 0;
	char insertChar = 0;
	for(uint32_t i=0; i<10000; i++) { //FPGA_BUFFER_RECORD_MAX_SIZE
		if(counter >= 1000) {
			counter = 0;
			insertChar++;
		}
		counter++;
		*(((char*)fpgaData.sdramData->data) + fpgaData.sdramData->len) = insertChar;
		fpgaData.sdramData->len++;
	}
}

static uint8_t printTestBuf[2150] = {0};
void printfTestFillData() {
	uint32_t offset = 0;
	for(uint8_t index=0; index<10; index++) {
		memset(printTestBuf, 0, sizeof(printTestBuf));
		convertBufRawToText((uint8_t*)fpgaData.sdramData->data + offset, 1000, printTestBuf, sizeof(printTestBuf));
		offset += 1000;
		DBGLog("printfTestFillData: %s", printTestBuf);
	}
}

#include "aws_iot_config.h"
#include "aws_iot_mqtt_client.h"

static uint8_t publishBuf[AWS_IOT_MQTT_TX_BUF_LEN] = {0};
IoT_Publish_Message_Params paramsQOS1 = {QOS1, 0, 0, 0, NULL,0};

void testSendParserQueue() {
	//-- send data
	sFpgaData * p = NULL;
	IoT_Error_t rc = AWS_SUCCESS;
	for(;;) {
		if(xQueuePeek(fpgaDataQueue, &p, 1000/portTICK_PERIOD_MS) == pdTRUE) {
			if(p != NULL) {
				if(p->sdramData->data != NULL) {
					uint8_t * pSendData = NULL;
					uint16_t sentCounter = 0;
					uint16_t offsetCounter = 0;
					pSendData = (uint8_t*)p->sdramData->data;
					static uint32_t counterPacket = 0;

					do {
						if((p->sdramData->len - sentCounter) > 3800) {
							offsetCounter = 3800;
						} else {
							offsetCounter = (p->sdramData->len - sentCounter);
						}

						memset(publishBuf, 0, sizeof(publishBuf));
						insertJsonStartField(publishBuf, counterPacket++);

						uint16_t len = strlen((char*)publishBuf);
						convertBufRawToText((uint8_t*)pSendData, offsetCounter, publishBuf + len, sizeof(publishBuf)- 10 - len);

						len = strlen((char*)publishBuf);
						insertJsonEndField(publishBuf + len);

						/* create desired message */
						paramsQOS1.payload = publishBuf;
						paramsQOS1.payloadLen = strlen((char*)publishBuf);

						printToUsb(paramsQOS1.payload, paramsQOS1.payloadLen);

						DBGLog("AWS: %s", (char*)paramsQOS1.payload);

						sentCounter += offsetCounter;
						pSendData += offsetCounter;

						if (rc != AWS_SUCCESS) {
							break;
						}
					} while((MQTT_REQUEST_TIMEOUT_ERROR == rc) || (sentCounter < p->sdramData->len));
					if(sentCounter == p->sdramData->len) {
						DBGLog("Published to topic: - final");
						xQueueReceive(fpgaDataQueue, &p, NULL);
						freeSdramBuff(p->sdramData);
						vPortFree(p);
					}
				} else {
					DBGErr("AWS: sdramData == null");
				}
			}
		}
		vTaskDelay(1000/portTICK_PERIOD_MS);
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
			*((uint8_t*)fpgaData.sdramData->data + fpgaData.sdramData->len) = rxByte;
			fpgaData.sdramData->len++;
		} else {
			DBGErr("FPGA: buffer overflow");
			fpgaData.sdramData->len = 0;
		}
	}
	HAL_UART_Receive_IT(huart, &rxByte, 1);
}
