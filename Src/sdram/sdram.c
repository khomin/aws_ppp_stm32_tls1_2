/*
 * sdram.c
 *
 *  Created on: Jul 17, 2019
 *      Author: khomin
 */
#include "sdram.h"
#include "sdram_ll.h"
#include "string.h"

static sSdramAlloc mSdramAllocPull[SDRAM_ALLOC_PULL_MAX_LEN];
static uint32_t *mCommandBuf = NULL;

void sdramInit() {
	if(sdram_ll_init(SDRAM_RECORD_MAX_LEN)) {
		mCommandBuf = (uint32_t*)(SDRAM_BANK_ADDR + WRITE_READ_ADDR + SDRAM_ADDR_COMMAND_BUF);
		DBGLog("SDRAM: init ok");
	} else {
		DBGErr("SDRAM: test error");
		while(1){}
	}

	memset((void*)&mSdramAllocPull, 0, sizeof(mSdramAllocPull));
}

sSdramAlloc* createNewSdramBuff() {
	DBGLog("SDRAM: create new buf");
	for(uint32_t index=0; index<SDRAM_ALLOC_PULL_MAX_LEN; index++) {
		if((mSdramAllocPull[index].data == 0) && (mSdramAllocPull[index].len == 0)) {
			uint32_t offset = 0;
			for(uint16_t i=0; i<index; i++) {
				offset += SDRAM_RECORD_MAX_LEN;
			}
			mSdramAllocPull[index].data = ( uint32_t*) (SDRAM_BANK_ADDR + WRITE_READ_ADDR + offset);
			mSdramAllocPull[index].maxLen = SDRAM_RECORD_MAX_LEN - WRITE_READ_ADDR;
			DBGLog("SDRAM: create new buf: %xlu len %xlu", mSdramAllocPull[index].data, mSdramAllocPull[index].len);
			return &mSdramAllocPull[index];
		}
	}
	DBGLog("SDRAM: create new buf -error");
	return NULL;
}

bool freeSdramBuff(sSdramAlloc *pbuff) {
	bool res = false;
	for(uint32_t index=0; index<SDRAM_ALLOC_PULL_MAX_LEN; index++) {
		if(mSdramAllocPull[index].data == pbuff->data) {
			memset(mSdramAllocPull[index].data, 0, mSdramAllocPull[index].maxLen);
			memset(&mSdramAllocPull[index], 0, sizeof(sSdramAlloc));
			res = true;
			break;
		}
	}
	return res;
}

uint8_t* getCommandBuf() {
	return (uint8_t*)mCommandBuf;
}

uint32_t getCommandBufLen() {
	return SDRAM_COMMAND_BUF_MAX_SIZE;
}
