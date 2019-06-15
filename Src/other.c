/*
 * other.c
 *
 *  Created on: Jun 15, 2019
 *      Author: khomin
 */

#include "stm32f4xx_hal.h"
#include "lwip/arch.h"
#include "FreeRTOS.h"
#include "task.h"

u32_t sys_now(void) {
	return HAL_GetTick();
}

u32_t sys_jiffies(void) {
	return xTaskGetTickCount();
}
