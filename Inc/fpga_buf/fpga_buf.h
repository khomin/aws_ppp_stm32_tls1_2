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
#define FPGA_MAGIC_WORD						1973

typedef struct {
	uint8_t data[FPGA_BUFFER_RECORD_MAX_SIZE];
	uint16_t len;
	// TODO: not used
	uint32_t timestamp;
	uint16_t magic_word;
}sFpgaDataStruct;

void init_fpga();
bool putFpgaRecord(uint8_t* pData, int len);

#endif /* FPGA_BUF_FPGA_BUF_H_ */
