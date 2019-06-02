/*
 * fpga_buf.h
 *
 *  Created on: May 25, 2019
 *      Author: khomin
 */

#ifndef FPGA_BUF_FPGA_BUF_H_
#define FPGA_BUF_FPGA_BUF_H_

#include <stdbool.h>
#include <stdint.h>

#define FPGA_BUFFER_RECORD_MAX_COUNT		16
#define FPGA_BUFFER_RECORD_MAX_SIZE			512

typedef struct {
	uint8_t p_buffer[FPGA_BUFFER_RECORD_MAX_SIZE][FPGA_BUFFER_RECORD_MAX_COUNT];
	int records_count;
}sFpgaData;

typedef struct {
	// TODO: not used
	uint32_t timestamp;
	uint8_t data[FPGA_BUFFER_RECORD_MAX_SIZE];
	int data_len;
	// TODO: not used
	uint16_t crc;
}sRecord;

void init_fpga();
bool putFpgaRecord(uint8_t* pData, int len);
int getFpgaLastRecord(uint8_t* pData, int maxLen);
void flushFpgaAll();

#endif /* FPGA_BUF_FPGA_BUF_H_ */
