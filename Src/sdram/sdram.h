/*
 * sdram.h
 *
 *  Created on: Jul 17, 2019
 *      Author: khomin
 */

#ifndef SDRAM_SDRAM_H_
#define SDRAM_SDRAM_H_

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_sdram.h"
#include "debug_print.h"

#define SDRAM_ALLOC_PULL_MAX_LEN				32
#define SDRAM_RECORD_MAX_LEN					133120
#define SDRAM_ADDR_COMMAND_BUF					0x430800
#define SDRAM_COMMAND_BUF_MAX_SIZE				1000000

typedef struct {
	uint32_t* data;
	uint32_t len;
	uint32_t maxLen;
}sSdramAlloc;

void sdramInit();
sSdramAlloc* createNewSdramBuff();
bool freeSdramBuff(sSdramAlloc *pbuff);

uint8_t* getCommandBuf();
uint32_t getCommandBufLen();

#endif /* SDRAM_SDRAM_H_ */
