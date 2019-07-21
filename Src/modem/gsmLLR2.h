#ifndef GSMLLR2_H
#define GSMLLR2_H

#include "stdint.h"
#include "stdbool.h"
#include "stm32f4xx_hal.h"
#include "gsmLLR.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "queue.h"

typedef struct {
	size_t poolItemSize;
	size_t poolSize;
	size_t queueItemSize;
	size_t queueSize;
	xQueueHandle queue;

	struct {
		UART_HandleTypeDef* huart;
		xSemaphoreHandle txSemaphore;
		xQueueHandle rxQueue;
		uint8_t rxCh;
		uint8_t *pRxBuffer;
		uint8_t receiveCount;
		bool receiveState;
	}uart;
	struct {
		bool pppModeEnable;
		bool tildOne;
		bool tildTwo;
	}ppp;
}sGsmUartParcer;

bool gsmLLR2_Init(void);
void gsmUartInit(sGsmUartParcer *pParcerStruct);
void gsmUartDeinit(sGsmUartParcer *pParcerStruct);
sAtReplyParsingResult uartParceReceiveData(sGsmUartParcer *uartParcer);
//
void gsm_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void gsm_UART_TxCpltCallback(UART_HandleTypeDef *huart);

#endif /* __AT_PARAM_STRUCT_H */
