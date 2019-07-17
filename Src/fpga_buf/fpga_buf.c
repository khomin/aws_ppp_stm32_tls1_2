/*
 * fpga_buf.h
 *
 *  Created on: May 25, 2019
 *      Author: khomin
 */

#ifndef FPGA_BUF_FPGA_BUF_C_
#define FPGA_BUF_FPGA_BUF_C_

#include "fpga_buf/fpga_buf.h"
#include <FreeRTOS.h>
#include "task.h"
#include <semphr.h>
#include <string.h>
#include <stdlib.h>
#include "debug_print.h"
#include "usbd_cdc_if.h"

static xSemaphoreHandle xLogFpga;
extern xSemaphoreHandle usbLock;
extern xQueueHandle fpgaDataQueue;

void init_fpga() {
	xLogFpga = xSemaphoreCreateMutex();
}

bool putFpgaRecord(sFpgaData * pfpgaData) {
	bool res = false;

	if(xSemaphoreTake(xLogFpga, (TickType_t)500) == true) {
		if(xQueueSend(fpgaDataQueue, &pfpgaData, 1000/portTICK_PERIOD_MS) == pdTRUE) {
			DBGInfo("putFpgaRecord: record add -OK");
			res = true;
		} else {
			DBGErr("putFpgaRecord -send data to queue -ERROR (queue full");
		}
		xSemaphoreGive(xLogFpga);
	} else {
		DBGErr("putFpgaRecord - buffer malloc error");
	}
	return res;
}

void putFpgaReocordToUsb(sFpgaData * pfpgaData) {
	xSemaphoreTake(usbLock, 500/portTICK_PERIOD_MS);
	CDC_Transmit_FS((uint8_t*)"\r\nFPGA:\r\n", strlen("\r\nFPGA:\r\n"));
	vTaskDelay(1000/portTICK_PERIOD_MS);
	CDC_Transmit_FS(pfpgaData->data, pfpgaData->count);
	xSemaphoreGive(usbLock);
}

#endif /* FPGA_BUF_FPGA_BUF_C_ */
