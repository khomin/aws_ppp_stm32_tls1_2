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
#include "debug_print.h"

static sFpgaData fpgaData;
static xSemaphoreHandle xLogFpga;

void init_fpga() {
	fpgaData.records_count = 0;
	xLogFpga = xSemaphoreCreateMutex();
}

bool putFpgaRecord(uint8_t* pdata, int len) {
	bool res = false;
	if(xSemaphoreTake(xLogFpga, (TickType_t)100) == true) {
		if(fpgaData.records_count == 0) {
			memcpy(fpgaData.p_buffer[fpgaData.records_count], pdata, len);
			fpgaData.records_count ++;
			DBGInfo("putFpgaRecord: record add -OK");
			res = true;
		} else {
			if(fpgaData.records_count < FPGA_BUFFER_RECORD_MAX_COUNT) {
				memcpy(fpgaData.p_buffer[fpgaData.records_count], pdata, len);
				fpgaData.records_count ++;
				DBGInfo("putFpgaRecord: record add -OK");
				res = true;
			} else {
				DBGErr("putFpgaRecord: overflow");
				fpgaData.records_count = 0;
			}
		}
		xSemaphoreGive(xLogFpga);
	}

	return res;
}

int getFpgaLastRecord(uint8_t* pdata, int max_len) {
	uint8_t * pstart_rec_data = NULL;
	int res_rec_len = 0;

	xSemaphoreTake(xLogFpga, portMAX_DELAY);
	if(fpgaData.records_count == 0) {
		pstart_rec_data = fpgaData.p_buffer[0];
	} else {
		int cout = fpgaData.records_count * FPGA_BUFFER_RECORD_MAX_SIZE;
		if(cout < sizeof(fpgaData.p_buffer)) {
			pstart_rec_data = fpgaData.p_buffer[fpgaData.records_count * FPGA_BUFFER_RECORD_MAX_SIZE];
		}
	}

	if(pstart_rec_data != NULL) {
		memcpy(pdata, pstart_rec_data, max_len);
		res_rec_len = ((sRecord*)(pstart_rec_data))->data_len;
	}

	xSemaphoreGive(xLogFpga);

	return res_rec_len;
}

void flushFpgaAll() {
	xSemaphoreTake(xLogFpga, portMAX_DELAY);
	fpgaData.records_count = 0;
	memset(fpgaData.p_buffer[0], 0, FPGA_BUFFER_RECORD_MAX_SIZE);
	xSemaphoreGive(xLogFpga);
}

#endif /* FPGA_BUF_FPGA_BUF_C_ */
