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

#define COMMANDS_LIST_MAX_LEN				17
#define COMMANDS_TEXT_MAX_LEN				50

typedef enum {
	//-- gets
	e_get_keys,
	e_get_client_cert,
	e_get_client_private_device_cert,
	e_get_client_private_key,
	e_get_mqtt_url,
	e_get_device_name,
	e_get_topic_path,
	e_get_log_mode,
	e_get_firmware_fpga,
	//-- sets
	e_set_client_cert,
	e_set_client_private_device_cert,
	e_set_client_private_key,
	e_set_mqtt_url,
	e_set_device_name,
	e_set_topic_path,
	e_flush_full,
	e_set_log_mode,
	//-- other
	e_reboot,
	e_set_firmware_fpga
}eTypeCommands;

typedef struct {
	uint8_t command[COMMANDS_TEXT_MAX_LEN];
	eTypeCommands type;
}sCommandItem;

void commanderInit();
void commanderAppendData(uint8_t * data, uint16_t len);

void printToUsb(char * pdata, uint16_t len);
void putFpgaReocordToUsb(sFpgaData * pfpgaData);

#endif /* COMMANDER_COMMANDER_H_ */
