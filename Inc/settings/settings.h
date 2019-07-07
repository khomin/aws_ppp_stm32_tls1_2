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

bool getKeyCLIENT_CERTIFICATE_PEM_IsExist();
bool getKeyCLIENT_PRIVATE_KEY_PEM_IsExist();
bool getKeyCLIENT_PRIVATE_DEVICE_CERT_PEM_IsExist();
bool getMqttDestEndpoint_isExist();
bool getDeviceName_isExist();
bool getThingName_isExist();

bool flushSettingsFullSector();
bool setKeyCLIENT_CERTIFICATE_PEM(uint8_t *pdata, uint16_t len);
bool setKeyCLIENT_PRIVATE_KEY_PEM(uint8_t *pdata, uint16_t len);
bool setKeyCLIENT_PRIVATE_DEVICE_CERT(uint8_t *pdata, uint16_t len);
bool setMqttDestEndpoint(uint8_t *pdata, uint16_t len);
bool setDeviceName(uint8_t *pdata, uint16_t len);
bool setThingName(uint8_t *pdata, uint16_t len);

const uint8_t * getKeyCLIENT_CERTIFICATE_PEM();
const uint8_t * getKeyCLIENT_PRIVATE_KEY_PEM();
const uint8_t * getKeyCLIENT_PRIVATE_DEVICE_CERT();
const uint8_t * getMqttDestEndpoint();
const uint8_t * getDeviceName();
const uint8_t * getThingName();

#endif /* SETTINGS_SETTINGS_H_ */
