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
#include "../Src/sdram/sdram.h"

#define FPGA_BUFFER_RECORD_MAX_COUNT		16
#define FPGA_BUFFER_RECORD_MAX_SIZE			SDRAM_RECORD_MAX_LEN
#define FPGA_MAGIC_WORD						1973
#define FPGA_MIN_DATA_SIZE					10

typedef enum {
	efpgaStatusWait,
	efpgaStatusSent,
	efpgaStatusError
}eFpgaStatus;

typedef struct {
	sSdramAlloc * sdramData;
	eFpgaStatus statusProcessed;
	uint16_t magic_word;
}sFpgaData;

void init_fpga();
bool putFpgaRecord(sFpgaData  * pfpgaData);

#endif /* FPGA_BUF_FPGA_BUF_H_ */
