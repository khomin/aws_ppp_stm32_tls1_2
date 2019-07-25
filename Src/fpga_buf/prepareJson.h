/*
 * prepareJson.h
 *
 *  Created on: Jul 22, 2019
 *      Author: khomin
 */

#ifndef FPGA_BUF_PREPAREJSON_H_
#define FPGA_BUF_PREPAREJSON_H_

#include "debug_print.h"
#include "string.h"
#include "fpga_buf.h"

void insertJsonStartField(uint8_t * pdata, uint32_t optionsArg);
void insertJsonEndField(uint8_t * pdata);

void convertBufRawToText(const uint8_t *srcBuf, uint16_t srcLen, uint8_t *outBuf, uint16_t outMaxLen);

#endif /* FPGA_BUF_PREPAREJSON_H_ */
