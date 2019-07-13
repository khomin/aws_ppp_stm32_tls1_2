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

#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11 	((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */
#define FLASH_USER_START_ADDR	((uint32_t)0x080E0000)
#define FLASH_USER_END_ADDR		((uint32_t)0x080FFFFF)
#define DATA_32 				((uint32_t)0xFFFFFFFF)

#define USER_CONF_C2C_SOAPC_MAX_LENGTH  16
#define USER_CONF_C2C_USERID_MAX_LENGTH 16
#define USER_CONF_C2C_PSW_MAX_LENGTH    16

#define USER_CONF_WIFI_SSID_MAX_LENGTH  32
#define USER_CONF_WIFI_PSK_MAX_LENGTH   64

#define USER_CONF_DEVICE_NAME_LENGTH    300   /**< Must be large enough to hold a complete connection string */
#define USER_CONF_SERVER_NAME_LENGTH    128
#define USER_CONF_TLS_OBJECT_MAX_SIZE   2048
#define USER_CONF_TLS_ROOT_CA_CERT		(USER_CONF_TLS_OBJECT_MAX_SIZE * 3)
#define USER_CONF_MAGIC                 0x0123456789ABCDEFuLL

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define PEM_READ_LINE_SIZE    120
#define PEM_READ_BUFFER_SIZE  8192  /**< Max size which can be got from the terminal in a single getInputString(). */

typedef struct {
  uint64_t magic;                                     /**< The USER_CONF_MAGIC magic word signals that the structure was once written to FLASH. */
  char * device_name;
  char * server_name;
} iot_config_t;

/** Static user configuration data which must survive reboot and firmware update.
 * Do not change the field order, due to firewall constraint the tls_device_key size must be placed at a 64 bit boundary.
 * Its size sould also be multiple of 64 bits.
 *
 * Depending on the available board peripherals, the c2c_config and wifi_config fields may not be used.
 */
typedef struct {
  uint8_t * tls_root_ca_cert; /* Allow room for 3 root CA certificates */
  uint8_t * tls_device_cert;
  /* Above structure member must be aligned on 256 byte boundary
     to match firewall constraint , same for the size. */
  uint8_t * tls_device_key;
#ifdef USE_C2C
  c2c_config_t c2c_config;
#endif
#ifdef USE_WIFI
  wifi_config_t wifi_config;
#endif
  iot_config_t iot_config;
  uint64_t ca_tls_magic;        /**< The USER_CONF_MAGIC magic word signals that the TLS root CA certificates strings
                                    (tls_root_ca_cert) are present in Flash. */
  uint64_t device_tls_magic;    /**< The USER_CONF_MAGIC magic word signals that the TLS device certificate and key
                                    (tls_device_cert and tls_device_key) are present in Flash. */
} user_config_t;

void settingsInit();

bool getKeyCLIENT_CERTIFICATE_PEM_IsExist();
bool getKeyCLIENT_PRIVATE_KEY_PEM_IsExist();
bool getKeyCLIENT_PRIVATE_DEVICE_CERT_PEM_IsExist();
bool getMqttDestEndpoint_isExist();
bool getTopicPath_isExist();
bool getDeviceName_isExist();
bool flushSettingsFullSector();
bool setKeyCLIENT_CERTIFICATE_PEM(uint8_t *pdata, uint16_t len);
bool setKeyCLIENT_PRIVATE_KEY_PEM(uint8_t *pdata, uint16_t len);
bool setKeyCLIENT_PRIVATE_DEVICE_CERT(uint8_t *pdata, uint16_t len);
bool setMqttDestEndpoint(uint8_t *pdata, uint16_t len);
bool setTopicPath(uint8_t *pdata, uint16_t len);
bool setDeviceName(uint8_t *pdata, uint16_t len);

const char * getKeyCLIENT_CERTIFICATE_PEM();
const char * getKeyCLIENT_PRIVATE_KEY_PEM();
const char * getKeyCLIENT_PRIVATE_DEVICE_CERT();
const char * getMqttDestEndpoint();
const char * getTopicPath();
const char* getDeviceName();

#endif /* SETTINGS_SETTINGS_H_ */
