/*
 * commander.h
 *
 *  Created on: Jul 6, 2019
 *      Author: khomin
 */

#ifndef COMMANDER_COMMANDER_H_
#define COMMANDER_COMMANDER_H_

#include <stdint.h>
#include "fpga_buf.h"

void commanderInit();
void commanderAppendData(uint8_t * data, uint16_t len);

void printToUsb(char * pdata, uint16_t len);
void putFpgaReocordToUsb(sFpgaData * pfpgaData);

#endif /* COMMANDER_COMMANDER_H_ */
