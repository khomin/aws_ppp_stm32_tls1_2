/*
 * commander.h
 *
 *  Created on: Jul 6, 2019
 *      Author: khomin
 */

#ifndef COMMANDER_COMMANDER_H_
#define COMMANDER_COMMANDER_H_

#include <stdint.h>

void commanderInit();
void commanderAppendData(uint8_t * data, uint16_t len);

void printToUsb(uint8_t * pdata, uint16_t len);

#endif /* COMMANDER_COMMANDER_H_ */
