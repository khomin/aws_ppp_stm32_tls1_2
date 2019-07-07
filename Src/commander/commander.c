/*
 * commander.c
 *
 *  Created on: Jul 6, 2019
 *      Author: khomin
 */

#include "commander/commander.h"
#include "FreeRTOS.h"
#include "usb_device.h"
#include "debug_print.h"
#include "gsm.h"
#include "task.h"
#include "stats.h"
#include "gsmPPP.h"
#include "semphr.h"
#include <stdio.h>
#include "usbd_cdc_if.h"
#include "settings/settings.h"

#define COMMANDER_MAX_BUFF_SIZE				2048

static void commanderTask(void * arg);
static xSemaphoreHandle lock;

static uint8_t buffRx[COMMANDER_MAX_BUFF_SIZE];
static uint16_t buffRxLen = 0;
static uint8_t buffTx[COMMANDER_MAX_BUFF_SIZE];
static uint16_t buxTxLen = 0;
static bool usbIsActive = false;
static uint8_t* handleCommandData(uint8_t * pdata, uint16_t len);

#define COMMANDS_LIST_MAX_LEN				14
#define COMMANDS_TEXT_MAX_LEN				35

typedef enum {
	//-- gets
	e_get_keys,
	e_get_client_cert,
	e_get_client_private_device_cert,
	e_get_client_private_key,
	e_get_mqtt_url,
	e_get_device_name,
	e_get_thing_name,
	//-- sets
	e_set_client_cert,
	e_set_client_private_device_cert,
	e_set_client_private_key,
	e_set_mqtt_url,
	e_set_device_name,
	e_set_thing_name,
	e_flush_full
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
		{{"get -thing name"}, e_get_thing_name},
		//-- sets
		{{"set -key client cert"}, e_set_client_cert},
		{{"set -key client private device cert"}, e_set_client_private_device_cert},
		{{"set -key client private key"}, e_set_client_private_key},
		{{"set -mqtt url"}, e_set_mqtt_url},
		{{"set -device name"}, e_set_device_name},
		{{"set -thing name"}, e_set_thing_name},
		{{"flush full"}, e_flush_full}
};

static const uint8_t command_not_found_caption[] = "command not found\r\n";
static const uint8_t command_options_executed_caption[] = "executed\r\n";
static const uint8_t command_options_execut_error_caption[] = "execut error\r\n";
static const uint8_t command_options_error_caption[] = "error options\r\n";

static uint16_t findLenOffset(uint8_t * pdata, uint8_t firstChar);

void commanderInit() {
	lock = xSemaphoreCreateMutex();
	xTaskCreate(commanderTask, "commanderTask", 1024, 0, tskIDLE_PRIORITY, NULL);
}

void commanderTask(void * arg) {
	for(;;) {
		//-- if usb packet was finished
		if(usbIsActive) {
			usbIsActive = false;
			vTaskDelay(100/portTICK_PERIOD_MS);
			if(!usbIsActive) {
				uint8_t* command_res = handleCommandData(buffRx, buffRxLen);
				if(command_res != NULL) {
					DBGLog("command: result [%s]", command_res);
					CDC_Transmit_FS(command_res, strlen((char*)command_res));
				} else {
					DBGLog("command: result is null");
				}
			}
		}
		vTaskDelay(100/portTICK_PERIOD_MS);
	}
}

static uint8_t* handleCommandData(uint8_t * pdata, uint16_t len) {
	uint8_t * res = buffTx;
	memset(buffTx, 0, sizeof(buffTx));

	for(uint8_t index=0; index<COMMANDS_LIST_MAX_LEN; index++) {
		if(strstr((char*)pdata, (char*)c_commands[index].command) != NULL) {
			switch(c_commands[index].type) {

			//--
			//-- gets
			//--
			case e_get_keys : {
				res = buffTx;
				sprintf((char*)res,
						"Root CA:\r\n%s\nClient Cert:\r\n%s\nClient Key:\r\n%s\r\n",
						getKeyCLIENT_CERTIFICATE_PEM_IsExist() == true ? "OK" : "not found",
						getKeyCLIENT_PRIVATE_DEVICE_CERT_PEM_IsExist() == true ? "OK" : "not found",
						getKeyCLIENT_PRIVATE_KEY_PEM_IsExist() == true ? "OK" : "not found"
				);
			}
			break;

			case e_get_client_cert : {
				sprintf((char*)res, "client cert:\r\n%s\r\n", getKeyCLIENT_CERTIFICATE_PEM());
			}
			break;

			case e_get_client_private_device_cert : {
				sprintf((char*)res, "client private device cert:\r\n%s\r\n", getKeyCLIENT_PRIVATE_DEVICE_CERT());
			}
			break;

			case e_get_client_private_key : {
				sprintf((char*)res, "client private key:\r\n%s\r\n", getKeyCLIENT_PRIVATE_KEY_PEM());
			}
			break;

			case e_get_mqtt_url : {
				sprintf((char*)res, "url aws:\r\n%s\r\n", getMqttDestEndpoint());
			}
			break;

			case e_get_device_name : {
				sprintf((char*)res, "device name:\r\n%s\r\n", getDeviceName());
			}
			break;

			case e_get_thing_name : {
				sprintf((char*)res, "thing name:\r\n%s\r\n", getThingName());
			}
			break;

			//--
			//-- sets
			//--
			case e_set_client_cert: {
				char * p = strstr((char*)pdata, "-----BEGIN");
				bool execute_res = false;
				if(p != NULL) {
					setKeyCLIENT_CERTIFICATE_PEM((uint8_t*)p, findLenOffset(res, *p));
				}
				sprintf((char*)res, "%s:\r\n%s\r\n",
						c_commands[index].command,
						(p == NULL) ? (command_options_error_caption)
								: (execute_res ? command_options_executed_caption: command_options_execut_error_caption));
			} break;
			case e_set_client_private_device_cert: {} break;
			case e_set_client_private_key: {} break;
			case e_set_mqtt_url: {
				bool execute_res = false;
				uint8_t url[32] = {0};
				if(sscanf(pdata, "set -mqtt url %s", url)) {
					setMqttDestEndpoint(url, strlen(url));
					execute_res = true;
				}
				sprintf((char*)res, "%s:\r\n%s\r\n",
						c_commands[index].command,
						execute_res ? command_options_executed_caption: command_options_execut_error_caption
				);
			} break;
			case e_set_device_name: {} break;
			case e_set_thing_name: {} break;

			case e_flush_full: {
				bool execute_res = false;
				sprintf((char*)res, "%s:\r\n%s\r\n",
						c_commands[index].command,
						(flushSettingsFullSector()) ? command_options_executed_caption: command_options_execut_error_caption);
			} break;
			}
		}
	}

	if(res == NULL) {
		res = (uint8_t*)command_not_found_caption;
	}

	return res;
}

void commanderAppendData(uint8_t * data, uint16_t len) {
	xSemaphoreTakeFromISR(lock, 0);
	usbIsActive = true;
	if((sizeof(buffRx)+1) > len) {
		buffRxLen = 0;
		memcpy(buffRx, data, len);
	} else {
		DBGErr("commander: append override buf!");
	}

	xSemaphoreGive(lock);
}

uint16_t findLenOffset(uint8_t * pdata, uint8_t firstChar) {
	for(uint16_t index=0; pdata[index] != NULL; index++) {
		if(pdata[index] == firstChar) {
			return index;
		}
	}
	return NULL;
}
