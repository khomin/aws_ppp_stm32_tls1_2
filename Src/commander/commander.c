/*
 * commander.c
 *
 *  Created on: Jul 6, 2019
 *      Author: khomin
 */

#include "commander/commander.h"
#include "FreeRTOS.h"
#include "usb_device.h"
#include "debug_print.h"
#include "gsm.h"
#include "task.h"
#include "stats.h"
#include "gsmPPP.h"
#include "semphr.h"
#include <stdio.h>

#define COMMANDER_MAX_BUFF_SIZE				2048

static void commanderTask(void * arg);
static xSemaphoreHandle lock;

static uint8_t buffRx[COMMANDER_MAX_BUFF_SIZE];
static uint16_t buffRxLen = 0;
//static uint8_t buffTx[COMMANDER_MAX_BUFF_SIZE];
//static uint16_t buxTxLen = 0;
static bool usbIsActive = false;

void commanderInit() {
	lock = xSemaphoreCreateMutex();
	xTaskCreate(commanderTask, "commanderTask", 1024, 0, tskIDLE_PRIORITY, NULL);
}

void commanderTask(void * arg) {

	for(;;) {
		vTaskDelay(100/portTICK_PERIOD_MS);

		//-- if usb packet was finished
		if(!usbIsActive) {
			//CDC_Transmit_FS
		} else {
			usbIsActive = false;
		}
	}
}

void commanderAppendInByte(uint8_t byte) {
	xSemaphoreTakeFromISR(lock, 0);

	usbIsActive = true;

	if(buffRxLen+1 < sizeof(buffRx)) {
		buffRx[buffRxLen++] = byte;
	} else {
		DBGErr("commander: append override buf!");
	}

	xSemaphoreGive(lock);
}
