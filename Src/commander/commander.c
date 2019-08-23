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
#include "../firmwareUpdate/mt25q.h"

#define COMMANDER_MAX_BUFF_SIZE				2128

static void commanderTask(void * arg);
xSemaphoreHandle usbLock;

static uint8_t* buffRx;
static uint16_t buffRxLen = 0;
static uint8_t buffTx[COMMANDER_MAX_BUFF_SIZE];
//static uint16_t buxTxLen = 0;
static bool usbIsActive = false;
static uint8_t* handleCommandData(uint8_t * pdata, uint16_t len);
static uint8_t logModePrintUsb = false;

static const uint8_t command_not_found_caption[] = "command not found\r\n";
static const uint8_t command_options_executed_caption[] = "executed\r\n";
static const uint8_t command_options_execut_error_caption[] = "execut error or bad argument\r\n";

static uint8_t usbPrintBuf[1024 + 10] = {0};

static bool prepareDataCertificate(uint8_t * commandHeader, uint8_t *p, uint8_t * temp_buf, uint16_t maxLen);
static bool prepareDataOneArgument(uint8_t * commandHeader, uint8_t *p, uint8_t * temp_buf, uint16_t maxLen);
static bool prepareDataFirmwareFpga(uint8_t *data, uint16_t len);

void commanderInit() {
	usbLock = xSemaphoreCreateMutex();
	xTaskCreate(commanderTask, "commanderTask", 1024, 0, tskIDLE_PRIORITY, NULL);
}

void commanderTask(void * arg) {
	buffRx = getCommandBuf();

	mt25Q_init();

	for(;;) {
		if(usbIsActive) {
			usbIsActive = false;
			vTaskDelay(100/portTICK_PERIOD_MS);
			if(!usbIsActive) {
				uint8_t* command_res = handleCommandData(buffRx, buffRxLen);
				if(command_res != NULL) {
					DBGLog("command: result [%s]", command_res);
					memset(buffRx, 0, getCommandBufLen());
					buffRxLen = 0;
					xSemaphoreTake(usbLock, 500/portTICK_PERIOD_MS);
					CDC_Transmit_FS(command_res, strlen((char*)command_res));
					xSemaphoreGive(usbLock);

					vTaskDelay(100/portTICK_PERIOD_MS);
				} else {
					DBGLog("command: result is null");
//					if(buffRxLen > 100) {
//						DBGLog("command: is firmware");
//						prepareDataFirmwareFpga(buffRx, buffRxLen);
//					}
					memset(buffRx, 0, getCommandBufLen());
					buffRxLen = 0;
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

			case e_get_log_mode: {
				tempbuf = buffTx;
				sprintf((char*)tempbuf, "log mode:\r\n%d\r\n", logModePrintUsb);
			} break;

			case e_get_firmware_fpga: {	// TODO: get firmware?
				tempbuf = buffTx;
				sprintf((char*)tempbuf, "firmware fpga:\r\n%s\r\n", "unspecified");
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

			case e_set_log_mode: {
				tempbuf = buffTx;
				bool execute_res = false;
				p = strstr((char*)pdata, (char*)c_commands[index].command);
				if(prepareDataOneArgument((uint8_t*)c_commands[index].command, (uint8_t*)p, tempbuf, sizeof(int))) {
					logModePrintUsb = atoi((char*)tempbuf);
					execute_res = true;
				}
				sprintf((char*)tempbuf, "%s:\r\n%s\r\n",
						c_commands[index].command,
						execute_res ? command_options_executed_caption: command_options_execut_error_caption
				);
			} break;

			case e_reboot: {
				tempbuf = buffTx;
				NVIC_SystemReset();
				break;
			}

			case e_set_firmware_fpga: {
				tempbuf = buffTx;
				p = strstr((char*)pdata, (char*)c_commands[index].command);
				bool execute_res = false;
				sprintf((char*)tempbuf, "%s:\r\n%s\r\n",
						c_commands[index].command,
						execute_res ? command_options_executed_caption: command_options_execut_error_caption
				);
			} break;
			}

			if(tempbuf == NULL) {
				tempbuf = (uint8_t*)command_not_found_caption;
			}
		}
	}

	return tempbuf;
}

bool getLogUsbModeIsOff() {
	return logModePrintUsb == 0;
}

bool getLogUsbModeIsShort() {
	return logModePrintUsb == 1;
}

bool getLogUsbModeIsDetail() {
	return logModePrintUsb == 2;
}

void printToUsbLite(char * pdata, uint16_t len) {
	CDC_Transmit_FS((uint8_t*)pdata, len);
	vTaskDelay(500/portTICK_PERIOD_MS);
}

void printToUsbDetail(char * pdata, uint16_t len) {
	memset(usbPrintBuf, 0, sizeof(usbPrintBuf));
	sprintf((char*)usbPrintBuf, "\r\nPRINT FPGA: data [len:%lu]\r\n", len);
	CDC_Transmit_FS((uint8_t*)usbPrintBuf, strlen(usbPrintBuf));
	vTaskDelay(500/portTICK_PERIOD_MS);

	uint32_t sentCounter = 0;
	uint32_t offsetCounter = 0;
	do {
		if((len - sentCounter) > 1024) {
			offsetCounter = 1024;
		} else {
			offsetCounter = (len - sentCounter);
		}

		CDC_Transmit_FS((uint8_t*)pdata + sentCounter, offsetCounter);
		vTaskDelay(500/portTICK_PERIOD_MS);

		sentCounter += offsetCounter;
	} while(sentCounter < len);

	memset(usbPrintBuf, 0, sizeof(usbPrintBuf));
	sprintf((char*)usbPrintBuf, "\r\nPRINT FPGA: end\r\n");
	CDC_Transmit_FS(usbPrintBuf, strlen(usbPrintBuf));
	vTaskDelay(500/portTICK_PERIOD_MS);
}

void putFpgaReocordToUsb(sFpgaData * pfpgaData) {
	if(!getLogUsbModeIsOff()) {
		if(getLogUsbModeIsShort()) {
			sprintf((char*)usbPrintBuf, "\r\nFPGA: new data [len:%lu]\r\n", pfpgaData->sdramData->len);
			printToUsbLite(usbPrintBuf, strlen(usbPrintBuf));
		}
		if(getLogUsbModeIsDetail()) {
			printToUsbDetail(pfpgaData->sdramData->data, pfpgaData->sdramData->len);
		}
	}
	xSemaphoreGive(usbLock);
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

bool prepareDataFirmwareFpga(uint8_t *data, uint16_t len) {
	bool res = false;
	if((data != NULL) && (len != 0)) {
//		FRESULT fResult;
//		UINT writeResultBytes = 0;
//		/* reset fpga down */
//		HAL_GPIO_WritePin(FPGA_REST_GPIO_Port, FPGA_REST_Pin, GPIO_PIN_RESET);
//
//		/* write to flash */
//		fResult = f_open(&fileDescriptor, "./fpga.bin", FA_WRITE | FA_CREATE_ALWAYS);
//		if(fResult == FR_OK) {
//			DBGLog("Firmware FPGA: f_open OK");
//			fResult = f_write(&fileDescriptor, data, len, &writeResultBytes);
//			if(len == writeResultBytes) {
//				DBGLog("Firmware FPGA: write success");
//			} else {
//				DBGLog("Firmware FPGA: write ERROR\r\nNeed write %i, written %i", len, writeResultBytes);
//			}
//		} else {
//			DBGErr("Firmware FPGA: f_open ERROR");
//		}
//		/* reset mcu up */
//		HAL_GPIO_WritePin(FPGA_REST_GPIO_Port, FPGA_REST_Pin, GPIO_PIN_SET);
//		res = true;
	}
	return res;
}

//bool handlerWriteFile(sFrameOutDataToPc *dataToPc, sFrameInputDataOfPc *dataOfPc)
//{
//	FRESULT fResult;
//	UINT resultBytes = 0;
//	// если первый пакет - стираем или создаем файл
//	if(dataOfPc->variant.file.numPacket == NULL) {
//		fResult = f_open(&usbFileDescriptor, dataOfPc->variant.file.path, FA_WRITE | FA_CREATE_ALWAYS);
//		DBGInfo("handlerWriteFile==0: path=%s", dataOfPc->variant.file.path);
//	}
//	fResult = f_write(&usbFileDescriptor, dataOfPc->variant.file.dataBuff, dataOfPc->variant.file.sizePacket, &resultBytes);
//	DBGInfo("handlerWriteFile: pack_count=%d size=%d", \
//			dataOfPc->variant.file.sizePacket,  \
//			dataOfPc->variant.file.sizeRecorded);
//
//	DBGInfo("handlerWriteFile: [%s] %d\n", dataOfPc->variant.file.dataBuff, dataOfPc->variant.file.sizePacket);
//
//	// запускаем таймер таймаута
//	xTimerStart(usbTimeoutWriteFile, 0);
//	// если все ок и дошли до конца файла
//	if((fResult == FR_OK) && (resultBytes == dataOfPc->variant.file.sizePacket)) {
//		if(dataOfPc->variant.file.sizeRecorded == dataOfPc->variant.file.fileFullSize) { // если записали весь файл тогда закрываем и сбрасываем таймер
//			DBGInfo("handlerWriteFile: pack_count=end %d", dataOfPc->variant.file.sizePacket);
//			xTimerStop(usbTimeoutWriteFile, 0);
//			// и закрываем файл
//			f_close(&usbFileDescriptor);
//		}
//		dataToPc->variant.file.result = true;
//		return true;
//	} else {
//		dataToPc->variant.file.result = false;
//		return false;
//	}
//}


void commanderAppendData(uint8_t * data, uint16_t len) {
	xSemaphoreTakeFromISR(usbLock, 0);
	usbIsActive = true;
	if((buffRxLen + len) < getCommandBufLen()) {
		memcpy(getCommandBufLen + buffRxLen, data, len);
		buffRxLen += len;
	} else {
		DBGErr("commander: append override buf!");
	}

	xSemaphoreGive(usbLock);
}
