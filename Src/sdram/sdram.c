/*
 * sdram.c
 *
 *  Created on: Jul 17, 2019
 *      Author: khomin
 */
#include "sdram.h"
#include "string.h"

extern SDRAM_HandleTypeDef hsdram1;
FMC_SDRAM_CommandTypeDef command;

static sSdramAlloc sdramAllocPull[SDRAM_ALLOC_PULL_MAX_LEN];

static void BSP_SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command);
static void fillBuffer(uint32_t *pBuffer, uint32_t uwBufferLenght, uint32_t uwOffset);

void sdramInit() {
	FMC_SDRAM_TimingTypeDef  SDRAM_Timing;
	/* Status variables */
	__IO uint32_t uwWriteReadStatus = 0;

	/* Counter index */
	uint32_t uwIndex = 0;

	uint32_t testBufTx[SDRAM_TEST_BUF_MAX_SIZE];
	uint32_t testBufRx[SDRAM_TEST_BUF_MAX_SIZE];

	/* SDRAM device configuration */
	hsdram1.Instance = FMC_SDRAM_DEVICE;

	SDRAM_Timing.LoadToActiveDelay    = 2;
	SDRAM_Timing.ExitSelfRefreshDelay = 6;
	SDRAM_Timing.SelfRefreshTime      = 4;
	SDRAM_Timing.RowCycleDelay        = 6;
	SDRAM_Timing.WriteRecoveryTime    = 2;
	SDRAM_Timing.RPDelay              = 2;
	SDRAM_Timing.RCDDelay             = 2;

	hsdram1.Init.SDBank             = FMC_SDRAM_BANK1;
	hsdram1.Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_8;
	hsdram1.Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12;
	hsdram1.Init.MemoryDataWidth    = SDRAM_MEMORY_WIDTH;
	hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
	hsdram1.Init.CASLatency         = FMC_SDRAM_CAS_LATENCY_3;
	hsdram1.Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
	hsdram1.Init.SDClockPeriod      = SDCLOCK_PERIOD;
	hsdram1.Init.ReadBurst          = FMC_SDRAM_RBURST_ENABLE;
	hsdram1.Init.ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_0;

	/* Initialize the SDRAM controller */
	if(HAL_SDRAM_Init(&hsdram1, &SDRAM_Timing) != HAL_OK) {
		DBGErr("SDRAM: init error");
		while(1){}
	}

	/* Program the SDRAM external device */
	BSP_SDRAM_Initialization_Sequence(&hsdram1, &command);

	/* Fill the buffer to write */
	fillBuffer(testBufTx, SDRAM_TEST_BUF_MAX_SIZE, 0);

	/* Write data to the SDRAM memory */
	for (uwIndex = 0; uwIndex < SDRAM_TEST_BUF_MAX_SIZE; uwIndex++) {
		*(__IO uint32_t*) (SDRAM_BANK_ADDR + WRITE_READ_ADDR + 4*uwIndex) = testBufTx[uwIndex];
	}

	/* Read back data from the SDRAM memory */
	for (uwIndex = 0; uwIndex < SDRAM_TEST_BUF_MAX_SIZE; uwIndex++) {
		testBufRx[uwIndex] = *(__IO uint32_t*) (SDRAM_BANK_ADDR + WRITE_READ_ADDR + 4*uwIndex);
	}

	/* Checking data integrity */
	for (uwIndex = 0; (uwIndex < SDRAM_TEST_BUF_MAX_SIZE) && (uwWriteReadStatus == 0); uwIndex++) {
		if (testBufRx[uwIndex] != testBufTx[uwIndex]) {
			uwWriteReadStatus++;
		}
	}

	/* Write data to the SDRAM memory */
	for (uwIndex = 0; uwIndex < SDRAM_RECORD_MAX_LEN; uwIndex++) {
		*(__IO uint32_t*) (SDRAM_BANK_ADDR + WRITE_READ_ADDR + 4*uwIndex) = 0;
	}


	if (uwWriteReadStatus != PASSED) {
		DBGErr("SDRAM: test error");
		while(1){}
	} else {
		DBGLog("SDRAM: init ok");
	}

	memset((void*)&sdramAllocPull, 0, sizeof(sdramAllocPull));
}

sSdramAlloc* createNewSdramBuff() {
	DBGLog("SDRAM: create new buf");
	for(uint32_t index=0; index<SDRAM_ALLOC_PULL_MAX_LEN; index++) {
		if((sdramAllocPull[index].data == 0) && (sdramAllocPull[index].len == 0)) {
			uint32_t offset = 0;
			for(uint16_t i=0; i<index; i++) {
				offset += SDRAM_RECORD_MAX_LEN;
			}
			sdramAllocPull[index].data = (__IO uint32_t*) (SDRAM_BANK_ADDR + WRITE_READ_ADDR + offset);
			sdramAllocPull[index].maxLen = SDRAM_RECORD_MAX_LEN - WRITE_READ_ADDR;
			DBGLog("SDRAM: create new buf: %xlu len %xlu", sdramAllocPull[index].data, sdramAllocPull[index].len);
			return &sdramAllocPull[index];
		}
	}
	DBGLog("SDRAM: create new buf -error");
	return NULL;
}

bool freeSdramBuff(sSdramAlloc *pbuff) {
	bool res = false;
	for(uint32_t index=0; index<SDRAM_ALLOC_PULL_MAX_LEN; index++) {
		if(sdramAllocPull[index].data == pbuff->data) {
			memset(sdramAllocPull[index].data, 0, sdramAllocPull[index].maxLen);
			memset(&sdramAllocPull[index], 0, sizeof(sSdramAlloc));
			res = true;
			break;
		}
	}
	return res;
}

/**
 * @brief  Perform the SDRAM exernal memory inialization sequence
 * @param  hsdram1: SDRAM handle
 * @param  Command: Pointer to SDRAM command structure
 * @retval None
 */
static void BSP_SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command)
{
  __IO uint32_t tmpmrd =0;
  /* Step 3:  Configure a clock configuration enable command */
  Command->CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
  Command->CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
  Command->AutoRefreshNumber = 1;
  Command->ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);

  /* Step 4: Insert 100 us minimum delay */
  /* Inserted delay is equal to 1 ms due to systick time base unit (ms) */
  HAL_Delay(1);

  /* Step 5: Configure a PALL (precharge all) command */
  Command->CommandMode = FMC_SDRAM_CMD_PALL;
  Command->CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
  Command->AutoRefreshNumber = 1;
  Command->ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);

  /* Step 6 : Configure a Auto-Refresh command */
  Command->CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
  Command->CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
  Command->AutoRefreshNumber = 8;
  Command->ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);

  /* Step 7: Program the external memory mode register */
  tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1          |
                     SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
                     SDRAM_MODEREG_CAS_LATENCY_3           |
                     SDRAM_MODEREG_OPERATING_MODE_STANDARD |
                     SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

  Command->CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
  Command->CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
  Command->AutoRefreshNumber = 1;
  Command->ModeRegisterDefinition = tmpmrd;

  /* Send the command */
  HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);

  /* Step 8: Set the refresh rate counter */
  /* (15.62 us x Freq) - 20 */
  /* Set the device refresh counter */
  hsdram->Instance->SDRTR |= ((uint32_t)((1386)<< 1));
}

/**
 * @brief  Fills buffer with user predefined data.
 * @param  pBuffer: pointer on the buffer to fill
 * @param  uwBufferLenght: size of the buffer to fill
 * @param  uwOffset: first value to fill on the buffer
 * @retval None
 */
static void fillBuffer(uint32_t *pBuffer, uint32_t uwBufferLenght, uint32_t uwOffset) {
	uint32_t tmpIndex = 0;

	/* Put in global buffer different values */
	for (tmpIndex = 0; tmpIndex < uwBufferLenght; tmpIndex++ )
	{
		pBuffer[tmpIndex] = tmpIndex + uwOffset;
	}
}
