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
#include "cloud.h"

#define COMMANDER_MAX_BUFF_SIZE				2128

static void commanderTask(void * arg);
xSemaphoreHandle usbLock;

static uint8_t buffRx[COMMANDER_MAX_BUFF_SIZE];
static uint16_t buffRxLen = 0;
static uint8_t buffTx[COMMANDER_MAX_BUFF_SIZE];
static uint16_t buxTxLen = 0;
static bool usbIsActive = false;
static uint8_t* handleCommandData(uint8_t * pdata, uint16_t len);

#define COMMANDS_LIST_MAX_LEN				15
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
	//-- sets
	e_set_client_cert,
	e_set_client_private_device_cert,
	e_set_client_private_key,
	e_set_mqtt_url,
	e_set_device_name,
	e_set_topic_path,
	e_flush_full,
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
		//-- sets
		{{"set -key client cert"}, e_set_client_cert},
		{{"set -key client private device cert"}, e_set_client_private_device_cert},
		{{"set -key client private key"}, e_set_client_private_key},
		{{"set -mqtt url"}, e_set_mqtt_url},
		{{"set -device name"}, e_set_device_name},
		{{"set -topic path"}, e_set_topic_path},
		{{"flush full"}, e_flush_full},
		{{"reboot"}, e_reboot}
};

static const uint8_t command_not_found_caption[] = "command not found\r\n";
static const uint8_t command_options_executed_caption[] = "executed\r\n";
static const uint8_t command_options_execut_error_caption[] = "execut error or bad argument\r\n";

static uint16_t findLenOffset(uint8_t * pdata, uint8_t firstChar);
static bool prepareDataCertificate(uint8_t * commandHeader, uint8_t *p, uint8_t * temp_buf, uint16_t maxLen);
static bool prepareDataOneArgument(uint8_t * commandHeader, uint8_t *p, uint8_t * temp_buf, uint16_t maxLen);

void commanderInit() {
	usbLock = xSemaphoreCreateMutex();
	xTaskCreate(commanderTask, "commanderTask", 1024, 0, tskIDLE_PRIORITY, NULL);
}

void printToUsb(uint8_t * pdata, uint16_t len) {
	CDC_Transmit_FS(pdata, len);
}

void commanderTask(void * arg) {
	for(;;) {
		if(usbIsActive) {
			usbIsActive = false;
			vTaskDelay(100/portTICK_PERIOD_MS);
			if(!usbIsActive) {
				uint8_t* command_res = handleCommandData(buffRx, buffRxLen);
				memset(buffRx, 0, sizeof(buffRx));
				buffRxLen = 0;
				if(command_res != NULL) {
					DBGLog("command: result [%s]", command_res);

					xSemaphoreTake(usbLock, 500/portTICK_PERIOD_MS);
					CDC_Transmit_FS(command_res, strlen((char*)command_res));
					xSemaphoreGive(usbLock);

					vTaskDelay(100/portTICK_PERIOD_MS);
				} else {
					DBGLog("command: result is null");
				}
			}
		}

		vTaskDelay(100/portTICK_PERIOD_MS);
	}
}

static uint8_t* handleCommandData(uint8_t * pdata, uint16_t len) {
	uint8_t * tempbuf = NULL;
	memset(buffTx, 0, sizeof(buffTx));
	char * p = NULL;

	for(uint8_t index=0; index<COMMANDS_LIST_MAX_LEN; index++) {
		if(strstr((char*)pdata, (char*)c_commands[index].command) != NULL) {

			switch(c_commands[index].type) {
			//--
			//-- gets
			//--
			case e_get_keys : {
				tempbuf = buffTx;
				sprintf((char*)tempbuf,
						"Root CA:\r\n%s\nClient Cert:\r\n%s\nClient Key:\r\n%s\r\n",
						getKeyCLIENT_CERTIFICATE_PEM_IsExist() == true ? "OK" : "not found",
								getKeyCLIENT_PRIVATE_DEVICE_CERT_PEM_IsExist() == true ? "OK" : "not found",
										getKeyCLIENT_PRIVATE_KEY_PEM_IsExist() == true ? "OK" : "not found"
				);
			} break;

			case e_get_client_cert : {
				tempbuf = buffTx;
				strcpy((char*)tempbuf, "client cert:\r\n");
				uint16_t len = strlen((char*)tempbuf);
				p = (char*)getKeyCLIENT_CERTIFICATE_PEM();
				while(*p != 0) {
					*(tempbuf + len) = *p;
					p++;
					len++;
				}
				*(tempbuf+len+1) = '\r';
				*(tempbuf+len+2) = '\n';
			} break;

			case e_get_client_private_device_cert : {
				tempbuf = buffTx;
				strcpy((char*)tempbuf, "client private device cert:\r\n");
				uint16_t len = strlen((char*)tempbuf);
				p = (char*)getKeyCLIENT_PRIVATE_DEVICE_CERT();
				while(*p != 0) {
					*(tempbuf + len) = *p;
					p++;
					len++;
				}
				*(tempbuf+len+1) = '\r';
				*(tempbuf+len+2) = '\n';
			} break;

			case e_get_client_private_key : {
				tempbuf = buffTx;
				strcpy((char*)tempbuf, "client private key:\r\n");
				uint16_t len = strlen((char*)tempbuf);
				p = (char*)getKeyCLIENT_PRIVATE_KEY_PEM();
				while(*p != 0) {
					*(tempbuf + len) = *p;
					p++;
					len++;
				}
				strcpy((char*)&tempbuf[strlen(tempbuf)], "\r\n\r\n");
			} break;

			case e_get_mqtt_url: {
				tempbuf = buffTx;
				sprintf((char*)tempbuf, "url aws:\r\n%s\r\n", getMqttDestEndpoint());
			}
			break;

			case e_get_device_name: {
				tempbuf = buffTx;
				sprintf((char*)tempbuf, "url aws:\r\n%s\r\n", getDeviceName());
			} break;

			case e_get_topic_path: {
				tempbuf = buffTx;
				sprintf((char*)tempbuf, "device name:\r\n%s\r\n", getTopicPath());
			} break;

			//--
			//-- sets
			//--
			case e_set_client_cert: {
				tempbuf = buffTx;
				p = strstr((char*)pdata, (char*)c_commands[index].command);
				bool execute_res = false;
				if(prepareDataCertificate((uint8_t*)c_commands[index].command, (uint8_t*)p, tempbuf, getRootCaCertMaxSize())) {
					execute_res = setKeyCLIENT_CERTIFICATE_PEM((uint8_t*)tempbuf, strlen((char*)tempbuf));
				}
				sprintf((char*)tempbuf, "%s:\r\n%s\r\n",
						c_commands[index].command,
						execute_res ? command_options_executed_caption : command_options_execut_error_caption);
			} break;

			case e_set_client_private_device_cert: {
				tempbuf = buffTx;
				p = strstr((char*)pdata, (char*)c_commands[index].command);
				bool execute_res = false;
				if(prepareDataCertificate((uint8_t*)c_commands[index].command, (uint8_t*)p, tempbuf, getPrivateDeviceCertMaxSize())) {
					execute_res = setKeyCLIENT_PRIVATE_DEVICE_CERT((uint8_t*)tempbuf, strlen((char*)tempbuf));
				}
				sprintf((char*)tempbuf, "%s:\r\n%s\r\n",
						c_commands[index].command,
						execute_res ? command_options_executed_caption: command_options_execut_error_caption
				);
			} break;

			case e_set_client_private_key: {
				tempbuf = buffTx;
				p = strstr((char*)pdata, (char*)c_commands[index].command);
				bool execute_res = false;
				if(prepareDataCertificate((uint8_t*)c_commands[index].command, (uint8_t*)p, tempbuf, getPrivateKeyMaxSize())) {
					execute_res = setKeyCLIENT_PRIVATE_KEY_PEM((uint8_t*)tempbuf, strlen((char*)tempbuf));
				}
				sprintf((char*)tempbuf, "%s:\r\n%s\r\n",
						c_commands[index].command,
						execute_res ? command_options_executed_caption: command_options_execut_error_caption
				);
			} break;

			case e_set_mqtt_url: {
				tempbuf = buffTx;
				bool execute_res = false;
				p = strstr((char*)pdata, (char*)c_commands[index].command);
				if(prepareDataOneArgument((uint8_t*)c_commands[index].command, (uint8_t*)p, tempbuf, getMqttEndpointMaxSize())) {
					execute_res = setMqttDestEndpoint(tempbuf, strlen((char*)tempbuf));
				}
				sprintf((char*)tempbuf, "%s:\r\n%s\r\n",
						c_commands[index].command,
						execute_res ? command_options_executed_caption: command_options_execut_error_caption
				);
			} break;

			case e_set_device_name: {
				tempbuf = buffTx;
				bool execute_res = false;
				p = strstr((char*)pdata, (char*)c_commands[index].command);
				if(prepareDataOneArgument((uint8_t*)c_commands[index].command, (uint8_t*)p, tempbuf, getDeviceNameMaxSize())) {
					execute_res = setDeviceName(tempbuf, strlen((char*)tempbuf));
				}
				sprintf((char*)tempbuf, "%s:\r\n%s\r\n",
						c_commands[index].command,
						execute_res ? command_options_executed_caption: command_options_execut_error_caption
				);
			} break;

			case e_set_topic_path: {
				tempbuf = buffTx;
				bool execute_res = false;
				p = strstr((char*)pdata, (char*)c_commands[index].command);
				if(prepareDataOneArgument((uint8_t*)c_commands[index].command, (uint8_t*)p, tempbuf, getTopicPathMaxSize())) {
					execute_res = setTopicPath(tempbuf, strlen((char*)tempbuf));
				}
				sprintf((char*)tempbuf, "%s:\r\n%s\r\n",
						c_commands[index].command,
						execute_res ? command_options_executed_caption: command_options_execut_error_caption
				);
			} break;

			case e_flush_full: {
				tempbuf = buffTx;
				sprintf((char*)tempbuf, "%s:\r\n%s\r\n",
						c_commands[index].command,
						(flushSettingsFullSector())
						? command_options_executed_caption: command_options_execut_error_caption);
			} break;

			case e_reboot:
				tempbuf = buffTx;
				NVIC_SystemReset();
				break;
			}
		}
	}

	if(tempbuf == NULL) {
		tempbuf = (uint8_t*)command_not_found_caption;
	}

	return tempbuf;
}

bool prepareDataCertificate(uint8_t * commandHeader, uint8_t *p, uint8_t * temp_buf, uint16_t maxLen) {
	bool res = false;
	uint16_t len = 0;
	if((p != NULL) && (maxLen != 0)) {
		p += strlen((char*)commandHeader);
		p = strstr((char*)p, "\r");
		if(p != NULL) {
			//-- unix or windows
			if(*(p+1) != '\n') {
				len++;
			} else {
				len+=2;
			}
			p += len; //-- offset \r\n
			uint8_t * p_handBuf = temp_buf;
			while(*p != 0) {
				if(len >= maxLen) {
					return false;
				}
				*p_handBuf++ = *p;
				//-- unix - windows
				if((*p == '\r') && (*(p+1) != '\n')) {
					*p_handBuf++ = '\n';
				}
				p++; len++;
			}
			res = true;
		}
	}
	return res;
}

bool prepareDataOneArgument(uint8_t * commandHeader, uint8_t *p, uint8_t * temp_buf, uint16_t maxLen) {
	bool res = false;
	uint16_t len = 0;
	if((p != NULL) && (maxLen != 0)) {
		p += strlen((char*)commandHeader);
		while(len < maxLen) {
			if(*p == 0) {
				return false;
			}
			if(*p == ' ') {
				break;
			}
			p++;
			len++;
		}

		if(*p == ' ') {
			p++; len++;
			//-- unix or windows
			uint8_t * p_handBuf = temp_buf;
			while((*p != 0) && ((*p != '\r') || (*p != '\n'))) {
				if(len >= maxLen) {
					return false;
				}
				*p_handBuf++ = *p;
				p++; len++;
			}
			res = true;
		}
	}
	return res;
}


void commanderAppendData(uint8_t * data, uint16_t len) {
	xSemaphoreTakeFromISR(usbLock, 0);
	usbIsActive = true;
	if((buffRxLen + len) < sizeof(buffRx)) {
		memcpy(&buffRx[buffRxLen], data, len);
		buffRxLen += len;
	} else {
		DBGErr("commander: append override buf!");
	}

	xSemaphoreGive(usbLock);
}

uint16_t findLenOffset(uint8_t * pdata, uint8_t firstChar) {
	for(uint16_t index=0; pdata[index] != NULL; index++) {
		if(pdata[index] == firstChar) {
			return index;
		}
	}
	return NULL;
}
