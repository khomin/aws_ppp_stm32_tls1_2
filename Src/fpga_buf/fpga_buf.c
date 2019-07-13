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
#include <semphr.h>
#include <string.h>
#include <stdlib.h>
#include "debug_print.h"

static xSemaphoreHandle xLogFpga;

extern xQueueHandle fpgaDataQueue;

void init_fpga() {
	xLogFpga = xSemaphoreCreateMutex();
}

bool putFpgaRecord(uint8_t* pdata, int len) {
	bool res = false;
	sFpgaDataStruct *p = NULL;
	if(xSemaphoreTake(xLogFpga, (TickType_t)100) == true) {
		if((len * 2 + strlen("\r\n")) >= sizeof(p->data)) {
			DBGErr("putFpgaRecord - buffer overflow");
		} else {
			p = malloc(sizeof(sFpgaDataStruct));
			if(p != NULL) {
				uint16_t count_str = 0;
				for(uint16_t count=0; count < len; count ++) {
					sprintf((p->data+count_str), "%02x", *(pdata+count));
					count_str += 2;
				}
				sprintf(p->data[count_str], "%s", "\r\n");

				if(xQueueSend(fpgaDataQueue, &p, 1000/portTICK_PERIOD_MS) == pdTRUE) {
					DBGInfo("putFpgaRecord: record add -OK");
					res = true;
				} else {
					DBGErr("putFpgaRecord -send data to queue -ERROR (queue full");
					free(p);
				}
			}
		}
		xSemaphoreGive(xLogFpga);
	} else {
		DBGErr("putFpgaRecord - buffer malloc error");
	}
	return res;
}

#endif /* FPGA_BUF_FPGA_BUF_C_ */
