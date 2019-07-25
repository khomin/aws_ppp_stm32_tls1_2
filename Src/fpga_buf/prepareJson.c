/*
 * prepareJson.c
 *
 *  Created on: Jul 22, 2019
 *      Author: khomin
 */
#include "stm32f4xx_hal.h"
#include "prepareJson.h"

void convertBufRawToText(const uint8_t *srcBuf, uint16_t srcLen, uint8_t *outBuf, uint16_t outMaxLen) {
	uint8_t * pOutIt = outBuf;
	uint16_t outLen = 0;
	for(uint16_t index=0; index<srcLen; index++) {
		if(outLen > outMaxLen - 100) {
			DBGErr("convertBufRawToText: len out of scope");
			return;
		}
		sprintf((char*)pOutIt+outLen, "%02x", *(srcBuf + index));
		outLen += 2;
	}
}

void insertJsonStartField(uint8_t * pdata, uint32_t optionsArg) {
	static long id = 0;
#ifdef USE_BILLS
	//sprintf((char*)pdata, "{\"id\":%ld,\"packet\":%ld,\"data\":\"", id++, optionsArg);
#else
	sprintf((char*)pdata, "{\"id\":%ld,\"state\":{\"desired\":{\"message\":\"", id++);
#endif
}

void insertJsonEndField(uint8_t * pdata) {
	uint8_t i=0;
#ifdef USE_BILLS
//	pdata[i++] = '"';
//	pdata[i++] = '}';
#else
	pdata[i++] = '"';
	pdata[i++] = '}';
	pdata[i++] = '}';
	pdata[i++] = '}';
#endif
}
