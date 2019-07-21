/*
 * fpga_commander.h
 *
 *  Created on: May 25, 2019
 *      Author: khomin
 */

#ifndef FPGA_COMMANDER_H_
#define FPGA_COMMANDER_H_

#include "stm32f4xx_hal.h"
#include <stm32f4xx_hal_uart.h>

#define FPGA_TASK_STACK_SIZE	512
#define FPGA_TASK_PRIORYTY		osPriorityNormal

#define FPGA_ARRAY_RX_MAX_SIZE

void fpgaTask(void *argument);

void fpgaRxUartHandler(UART_HandleTypeDef *huart);

#endif /* FPGA_COMMANDER_H_ */
