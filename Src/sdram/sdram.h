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
#define SDRAM_RECORD_MAX_LEN					128000

/* Exported types ------------------------------------------------------------*/
typedef enum {PASSED = 0, FAILED = !PASSED} TestStatus_t;
/* Exported constants --------------------------------------------------------*/
#define SDRAM_TEST_BUF_MAX_SIZE					4

#define WRITE_READ_ADDR     					((uint32_t)0x0800)

#define SDRAM_BANK_ADDR                 		((uint32_t)0xC0000000)

/* #define SDRAM_MEMORY_WIDTH            		FMC_SDRAM_MEM_BUS_WIDTH_8  */
#define SDRAM_MEMORY_WIDTH            			FMC_SDRAM_MEM_BUS_WIDTH_16
/* #define SDRAM_MEMORY_WIDTH               	FMC_SDRAM_MEM_BUS_WIDTH_32 */

#define SDCLOCK_PERIOD                   		FMC_SDRAM_CLOCK_PERIOD_2
/* #define SDCLOCK_PERIOD                		FMC_SDRAM_CLOCK_PERIOD_3 */

#define SDRAM_TIMEOUT     						((uint32_t)0xFFFF)

#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

typedef struct {
	uint32_t* data;
	uint32_t len;
	uint32_t maxLen;
}sSdramAlloc;

void sdramInit();
sSdramAlloc* createNewSdramBuff();
bool freeSdramBuff(sSdramAlloc *pbuff);

#endif /* SDRAM_SDRAM_H_ */
