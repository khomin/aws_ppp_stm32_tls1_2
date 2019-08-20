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
	e_set_firmware_fpga,
	//-- other
	e_reboot
}eTypeCommands;

typedef struct {
	uint8_t command[COMMANDS_TEXT_MAX_LEN];
	eTypeCommands type;
}sCommandItem;

static const sCommandItem c_commands[COMMANDS_LIST_MAX_LEN] = {
		//-- gets
		{{"get -keys"}, e_get_keys},
		{{"get -key client cert"}, e_get_client_cert},
		{{"get -key client private device cert"}, e_get_client_private_device_cert},
		{{"get -key client private key"}, e_get_client_private_key},
		{{"get -mqtt url"}, e_get_mqtt_url},
		{{"get -device name"}, e_get_device_name},
		{{"get -topic path"}, e_get_topic_path},
		{{"get -log mode"}, e_get_log_mode},
		{{"get -firmware fpga"}, e_get_firmware_fpga},
		//-- sets
		{{"set -key client cert"}, e_set_client_cert},
		{{"set -key client private device cert"}, e_set_client_private_device_cert},
		{{"set -key client private key"}, e_set_client_private_key},
		{{"set -mqtt url"}, e_set_mqtt_url},
		{{"set -device name"}, e_set_device_name},
		{{"set -topic path"}, e_set_topic_path},
		{{"flush full"}, e_flush_full},
		{{"set -log mode"}, e_set_log_mode},
		{{"set -firmware fpga"}, e_set_firmware_fpga},
		{{"reboot"}, e_reboot}
};

void commanderInit();
void commanderAppendData(uint8_t * data, uint16_t len);

void printToUsbLite(char * pdata, uint16_t len);
void printToUsbDetail(char * pdata, uint16_t len);

void putFpgaReocordToUsb(sFpgaData * pfpgaData);

bool getLogUsbModeIsOff();
bool getLogUsbModeIsShort();
bool getLogUsbModeIsDetail();

#endif /* COMMANDER_COMMANDER_H_ */
