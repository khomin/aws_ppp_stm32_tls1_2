/*
 * firmwareUpdate.c
 *
 *  Created on: Jul 14, 2019
 *      Author: khomin
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "rfu.h"
#ifdef USE_FIREWALL
#include "firewall_wrapper.h"
#endif
#include "settings/settings.h"
#include "msg.h"
#include "cloud.h"
#include "aws_clientcredential_keys.h"
#include "aws_clientcredential.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define PEM_READ_LINE_SIZE    120
#define PEM_READ_BUFFER_SIZE  8192  /**< Max size which can be got from the terminal in a single getInputString(). */

/* Private function prototypes -----------------------------------------------*/
int CaptureAndFlashPem(char *pem_name, char const *flash_addr, bool restricted_area);
/* Functions Definition ------------------------------------------------------*/

#ifdef RFU
/**
 * @brief  Firmware version management dialog.
 *         Allows:
 *             - Selecting a different FW version for the next boot, if already programmed in the other FLASH bank.
 *             - Download a FW file from an HTTP URL and program it to the other FLASH bank.
 */
#define DEFAULT_FW_URL      "http://192.168.3.113/Project-gentit.sfu"
#define MAX_FW_URL_LENGTH   100

int updateFirmwareVersion() {
	printf("\n*** Firmware version management ***\n");
	printf("\nPress the BLUE user button within the next 5 seconds\nto change the firmware version\n");

//	if (Button_WaitForPush(5000))
//	{
//		char fw_url[MAX_FW_URL_LENGTH];
//		strncpy(fw_url, DEFAULT_FW_URL, sizeof(fw_url));
//
//		printf("\nEnter the URL of the new firmware file:(By default: %s) :", fw_url);
//		getInputString(fw_url, sizeof(fw_url));
//		msg_info("read: --->\n%s\n<---\n", fw_url);
//
//		printf("Downloading and programming the new firmware into the alternate FLASH bank.\n");
//
//		int ret = rfu_update(fw_url);
//		switch (ret)
//		{
//		case RFU_OK:
//			printf("\nProgramming done. Now you can reset the board.\n\n");
//			break;
//		case RFU_ERR_HTTP:
//			printf("\nError: Programming failed. Reason: HTTP error - check your network connection, "
//					"and that the HTTP server supports HTTP/1.1 and the progressive download.\n\n");
//			break;
//		case RFU_ERR_FF:
//			printf("\nError: Programming failed. Reason: Invalid firmware fileformat - check that the IAR simple-code format is used.\n\n");
//			break;
//		case RFU_ERR_FLASH:
//			printf("\nError: Programming failed. Reason: FLASH memory erase/write - check that the firmware file matches the SoC FLASH memory mapping"
//					"and write protection settings. Double check that there is no illegal write to the FLASH address range.\n\n");
//			break;
//		default:
//			printf("\nError: Programming failed. Unknown reason.\n\n");
//		}
//	}

	return 0;
}
#endif /* RFU support */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
