/*
 * gsmLLR2.c
 *
 *  Created on: May 25, 2019
 *      Author: khomin
 */
#include "gsmLLR2.h"
#include "gsmLLR.h"
#include "gsm.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "queue.h"
#include "task.h"
#include "string.h"
#include <stdlib.h>
#include "debug_print.h"

extern sGsmState gsmState;
extern xSemaphoreHandle sioWriteSemaphore;
extern xSemaphoreHandle xGsmSemaphoreReplayReceived;
extern osStatus osPoolCFree (osPoolId pool_id, void *block);
sGsmUartParcer uartParcerStruct;
xTaskHandle gsmParcerTaskHandle;
xQueueHandle uartRxQueue = NULL;

const sAtMarkers atReplyMarkers = {
		{eError, "\nERROR\r"},
		{eOk, "\nOK\r"},
		{ePPP, "\nCONNECT\r"},
		{eSmsReady, "SMS Ready"},
};

// запуск задачи парсера ответов GSM
bool gsmLLR2_Init(void) {
	gsmState.initLLR2 = true;
	return gsmState.initLLR2;
}

void gsmUartInit(sGsmUartParcer *pParcerStruct) {
	//Создание пула
	//	poolDef.item_sz = pParcerStruct->poolItemSize;
	//	poolDef.pool_sz = pParcerStruct->poolSize;
	pParcerStruct->uart.txSemaphore = xSemaphoreCreateBinary();
	//Очередь с указателями на буферы данных
	pParcerStruct->queue = xQueueCreate(pParcerStruct->queueSize, pParcerStruct->queueItemSize);
	HAL_UART_Init(pParcerStruct->uart.huart);
	// на прием
	if(HAL_UART_Receive_IT(uartParcerStruct.uart.huart, &(uartParcerStruct.uart.rxCh), 1) != HAL_OK) {
		DBGInfo("HAL_UART_Receive_IT - != HAL OK");
	}
}

void gsmUartDeinit(sGsmUartParcer *pParcerStruct) {
	HAL_UART_DeInit(pParcerStruct->uart.huart);
	//Удаление очереди
	vQueueDelete(pParcerStruct->queue);
}

void gsm_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if(uartParcerStruct.ppp.pppModeEnable) {
		xSemaphoreGiveFromISR(sioWriteSemaphore, NULL);
	} else {
		xSemaphoreGiveFromISR(uartParcerStruct.uart.txSemaphore, NULL);
	}
}

void gsm_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	// принимаем, проверка парсинга, если ничего не нашли принимаем дальше
	if (uartParcerStruct.uart.receiveState == true) {
		if(xQueueSendFromISR(uartParcerStruct.uart.rxQueue,&(uartParcerStruct.uart.rxCh), NULL) != pdTRUE) {
			DBGInfo("UART: HAL_UART_RxCpltCallback Queue - FULL!!!");
			//			SystemReset(_Reset_Error_UartLLR2);
		}
	}
	if(HAL_UART_Receive_IT(huart, &(uartParcerStruct.uart.rxCh), 1) != HAL_OK) {
		DBGInfo("HAL ERROR: (HAL_UART_Receive_IT != HAL OK)");
		//		SystemReset(_Reset_Error_UartLLR2);
	}
}

sAtReplyParsingResult uartParceReceiveData(sGsmUartParcer *uartParcer) {
	sAtReplyParsingResult replayResult = {false, eError};

	uint8_t *pRxData = (uartParcer->uart.pRxBuffer);
	// разбираем принятое
	//Условие вызова CallBack для вывода полученной строки, пустые строки отбрасываем	
	if(uartParcer->ppp.pppModeEnable != true) {
		// если режим AT, должно быть больше 2-х символов
		if(uartParcerStruct.uart.receiveCount > 2) { // конец ответа команды
			if(strstr((char*)pRxData, "\r\n") != NULL) {
				// ищем слово в ответе
				for(uint8_t i=0; i<AT_MARKERS_SIZE; i++) {
					char *pStartCommand = NULL;
					pStartCommand = strstr((char*)pRxData, (char*)atReplyMarkers[i].command);
					if(pStartCommand != NULL) { //Если нашли конец ответа на АТ команду
						DBGInfo("UART Parcer: Find AT %s", atReplyMarkers[i].command);
						// разбираем								
						replayResult.atReplyfound = true;
						replayResult.atReplyType = atReplyMarkers[i].typeCommand;
						break;									
					}
				}
			}
		}
	} else{
		DBGInfo("UART: UartParceReceiveData -  pppModeEnable = TRUE ERROR!!!!!");
	}
	return replayResult;
}
