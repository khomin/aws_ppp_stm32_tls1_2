/*
 * settings.c
 *
 *  Created on: Jul 6, 2019
 *      Author: khomin
 */
#include "settings/settings.h"
#include "stm32f4xx.h"
#include <string.h>
#include <stdio.h>
#include "iot_flash_config.h"
#include "debug_print.h"

uint8_t __attribute__((section (".rodata"))) CLIENT_CERTIFICATE_PEM_addr [USER_CONF_TLS_ROOT_CA_CERT];
uint8_t __attribute__((section (".rodata"))) keyCLIENT_PRIVATE_DEVICE_CERT_PEM_addr [USER_CONF_TLS_OBJECT_MAX_SIZE];
uint8_t __attribute__((section (".rodata"))) keyCLIENT_PRIVATE_KEY_PEM_addr [USER_CONF_TLS_OBJECT_MAX_SIZE];

void settingsInit() {
	DBGLog("settings: init: status certificates - %d %d %d",
			keyCLIENT_CERTIFICATE_PEM_IsExist(),
			keyCLIENT_PRIVATE_KEY_PEM_IsExist(),
			keyCLIENT_PRIVATE_DEVICE_CERT_PEM_IsExist()
	);
}

bool keyCLIENT_CERTIFICATE_PEM_IsExist() {
	bool res = false;
	for(uint32_t i=0; i<sizeof(CLIENT_CERTIFICATE_PEM_addr); i++) {
		if(CLIENT_CERTIFICATE_PEM_addr[i] != 0) {
			res = true;
		}
	}
	return res;
}

bool keyCLIENT_PRIVATE_KEY_PEM_IsExist() {
	bool res = false;
	for(uint32_t i=0; i<sizeof(keyCLIENT_PRIVATE_KEY_PEM_addr); i++) {
		if(keyCLIENT_PRIVATE_KEY_PEM_addr[i] != 0) {
			res = true;
		}
	}
	return res;
}

bool keyCLIENT_PRIVATE_DEVICE_CERT_PEM_IsExist() {
	bool res = false;
	for(uint32_t i=0; i<sizeof(keyCLIENT_PRIVATE_DEVICE_CERT_PEM_addr); i++) {
		if(keyCLIENT_PRIVATE_DEVICE_CERT_PEM_addr[i] != 0) {
			res = true;
		}
	}
	return res;
}

/*
 * PEM-encoded client certificate
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
bool keyCLIENT_CERTIFICATE_PEM_set(uint8_t *pdata, uint16_t len) {
	HAL_FLASH_Unlock();
	for(uint16_t i=0; i<len; i++) {
		if(HAL_OK != HAL_FLASH_Program(TYPEPROGRAM_BYTE, (&CLIENT_CERTIFICATE_PEM_addr + i), pdata[i])) {
			HAL_FLASH_Lock();
			return false;
		}
	}
	HAL_FLASH_Lock();
}

/*
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
bool keyCLIENT_PRIVATE_KEY_PEM_set(uint8_t *pdata, uint16_t len) {

}

/*
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
bool keyCLIENT_PRIVATE_DEVICE_CERT_set(uint8_t *pdata, uint16_t len) {

}

bool keyCLIENT_CERTIFICATE_PEM_read(uint8_t *pdata, uint16_t maxLen) {

}

bool keyCLIENT_PRIVATE_KEY_PEM_read(uint8_t *pdata, uint16_t maxLen) {

}

bool keyCLIENT_PRIVATE_DEVICE_CERT_read(uint8_t *pdata, uint16_t maxLen) {

}
