/*
 * uart_custom.c
 *
 *  Created on: Jun 2, 2019
 *      Author: khomin
 */
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "debug_print.h"
#include "fpga_commander.h"
#include "gsmLLR2.h"

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if(huart->Instance == huart2.Instance) {
		fpgaRxUartHandler(huart);
	} else {
		gsm_UART_RxCpltCallback(huart);
	}
}
