/*
 * settings.h
 *
 *  Created on: Jul 6, 2019
 *      Author: khomin
 */

#ifndef SETTINGS_SETTINGS_H_
#define SETTINGS_SETTINGS_H_

#include <stdbool.h>
#include <stdint.h>

void settingsInit();

bool keyCLIENT_CERTIFICATE_PEM_IsExist();
bool keyCLIENT_PRIVATE_KEY_PEM_IsExist();
bool keyCLIENT_PRIVATE_DEVICE_CERT_PEM_IsExist();

bool keyCLIENT_CERTIFICATE_PEM_set(uint8_t *pdata, uint16_t len);
bool keyCLIENT_PRIVATE_KEY_PEM_set(uint8_t *pdata, uint16_t len);
bool keyCLIENT_PRIVATE_DEVICE_CERT_set(uint8_t *pdata, uint16_t len);

bool keyCLIENT_CERTIFICATE_PEM_read(uint8_t *pdata, uint16_t maxLen);
bool keyCLIENT_PRIVATE_KEY_PEM_read(uint8_t *pdata, uint16_t maxLen);
bool keyCLIENT_PRIVATE_DEVICE_CERT_read(uint8_t *pdata, uint16_t maxLen);

#endif /* SETTINGS_SETTINGS_H_ */
