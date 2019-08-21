/*
 * mt25q.c
 *
 *  Created on: Aug 21, 2019
 *      Author: khomin
 */
#include "mt25q.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_spi.h"
#include "debug_print.h"
#include "main.h"

extern SPI_HandleTypeDef hspi4;

void mt25Q_init() {
	uint16_t pageAddr = 0x123;
	const char wmsg[] = "This is a test message";
	char rmsg[sizeof(wmsg)] = {0};
	HAL_StatusTypeDef res1, res2;

	// read the device id
	{
		uint8_t devid_cmd[1] = { 0x9F };
		uint8_t devid_res[5];

		/* select switcher flash */
//		HAL_GPIO_WritePin(FPGA_CS_GPIO_Port, FPGA_CS_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(FPGA_CS_GPIO_Port, FPGA_CS_Pin, GPIO_PIN_RESET);

		HAL_GPIO_WritePin(FLASH_EXTERN_CS_GPIO_Port, FLASH_EXTERN_CS_Pin, GPIO_PIN_RESET);
		res1 = HAL_SPI_Transmit(&hspi4, devid_cmd, sizeof(devid_cmd), HAL_MAX_DELAY);
		res2 = HAL_SPI_Receive(&hspi4, devid_res, sizeof(devid_res), HAL_MAX_DELAY);
		HAL_GPIO_WritePin(FLASH_EXTERN_CS_GPIO_Port, FLASH_EXTERN_CS_Pin, GPIO_PIN_SET);

		if((res1 != HAL_OK) || (res2 != HAL_OK)) {
			char msg[256] = {0};
			snprintf(msg, sizeof(msg), "Error during getting the device id, res1 = %d, res2 = %d\r\n", res1, res2);
			DBGLog((uint8_t*)msg);
			return;
		}

		{
			char msg[256] = {0};
			snprintf(msg, sizeof(msg),
					"Manufacturer ID: 0x%02X\r\n"
					"Device ID (byte 1): 0x%02X\r\n"
					"Device ID (byte 2): 0x%02X\r\n"
					"Extended device information (EDI) string length: 0x%02X\r\n"
					"EDI byte 1: 0x%02X\r\n"
					"--------\r\n",
					devid_res[0], devid_res[1], devid_res[2], devid_res[3], devid_res[4]);
			DBGLog((uint8_t*)msg);
		}
	}

	// write test
	/* if(0) */ {
		uint8_t wcmd[128] = {0};
		for(uint8_t i=0; i<sizeof(wcmd); i++) {
			wcmd[i] = i+1;
		}
		// opcode
		wcmd[0] = 0xB1; //0x82; // 0x82 for buffer 1, 0x85 for buffer 2
		// for 512 bytes/page chip address is transfered in form:
		// 000AAAAA AAAAAAAa aaaaaaaa
		// wcmd[1] = (pageAddr >> 7) & 0x1F;
		// wcmd[2] = (pageAddr << 1) & 0xFE;
		// wcmd[3] = 0x00;

		// 00PPPPPP PPPPPPBB BBBBBBBB
		wcmd[1] = (pageAddr >> 6) & 0x3F;
		wcmd[2] = (pageAddr << 2) & 0xFC;
		wcmd[3] = 0x00;

		HAL_GPIO_WritePin(FLASH_EXTERN_CS_GPIO_Port, FLASH_EXTERN_CS_Pin, GPIO_PIN_RESET);
		res1 = HAL_SPI_Transmit(&hspi4, wcmd, sizeof(wcmd), HAL_MAX_DELAY);
		res2 = HAL_SPI_Transmit(&hspi4, (uint8_t*)wmsg, sizeof(wmsg), HAL_MAX_DELAY);
		HAL_GPIO_WritePin(FLASH_EXTERN_CS_GPIO_Port, FLASH_EXTERN_CS_Pin, GPIO_PIN_SET);

		if((res1 != HAL_OK) || (res2 != HAL_OK)) {
			char msg[256];
			snprintf(msg, sizeof(msg), "Error during writing the data, res1 = %d, res2 = %d\r\n", res1, res2);
			DBGLog((uint8_t*)msg);
			return;
		}
	}

//	// wait until device is ready (using HAL_Delay is error-prone!)
//	{
//		uint32_t delta = HAL_GetTick();
//		uint32_t cnt = 0;
//
//		uint8_t status_cmd[1] = { 0xD7 };
//		uint8_t status_res[2];
//		HAL_GPIO_WritePin(FLASH_EXTERN_CS_GPIO_Port, FLASH_EXTERN_CS_Pin, GPIO_PIN_RESET);
//		HAL_SPI_Transmit(&hspi4, status_cmd, sizeof(status_cmd), HAL_MAX_DELAY);
//		do {
//			cnt++;
//			res1 = HAL_SPI_Receive(&hspi4, status_res, sizeof(status_res), HAL_MAX_DELAY);
//			if(res1 != HAL_OK)
//				break;
//		} while (! (status_res[0] & 0x80)); // check RDY flag
//
//		HAL_GPIO_WritePin(FLASH_EXTERN_CS_GPIO_Port, FLASH_EXTERN_CS_Pin, GPIO_PIN_SET);
//
//		delta = HAL_GetTick() - delta;
//		uint8_t protect = (status_res[0] >> 1) & 0x01;
//		uint8_t page_size = (status_res[0]) & 0x01;
//		uint8_t epe = (status_res[1] >> 5) & 0x01;
//		uint8_t sle = (status_res[1] >> 3) & 0x01;
//		char msg[256];
//		snprintf(msg, sizeof(msg),
//				"Await loop took %ld ms, %ld iterations\r\n"
//				"Sector protection status: %s\r\n"
//				"Page size: %d bytes\r\n"
//				"Erase/program error: %s\r\n"
//				"Sector lockdown command: %s\r\n"
//				"--------\r\n",
//				delta, cnt,
//				protect ? "enabled" : "disabled",
//						page_size ? 512 : 528,
//								epe ? "ERROR!" : "no error",
//										sle ? "enabled" : "disabled");
//		DBGLog((uint8_t*)msg);
//	}

	// read test
	{
		uint8_t rcmd[128] = {0};
		// opcode
		rcmd[0] = 0xB5; //0x0B; // Note: 0x1B is supported by Adesto chips, but not Atmel chips, so use 0x0B

		// for 512 bytes/page chip address is transfered in form:
		// rcmd[1] = (pageAddr >> 7) & 0x1F;
		// rcmd[2] = (pageAddr << 1) & 0xFE;
		// rcmd[3] = 0x00;

		// 00PPPPPP PPPPPPBB BBBBBBBB
		rcmd[1] = (pageAddr >> 6) & 0x3F;
		rcmd[2] = (pageAddr << 2) & 0xFC;
		rcmd[3] = 0x00;

		// one dummy byte
		rcmd[4] = 0x00;

		HAL_GPIO_WritePin(FLASH_EXTERN_CS_GPIO_Port, FLASH_EXTERN_CS_Pin, GPIO_PIN_RESET);
		res1 = HAL_SPI_Transmit(&hspi4, rcmd, sizeof(rcmd), HAL_MAX_DELAY);
		res2 = HAL_SPI_Receive(&hspi4, (uint8_t*)rmsg, sizeof(rmsg), HAL_MAX_DELAY);
		HAL_GPIO_WritePin(FLASH_EXTERN_CS_GPIO_Port, FLASH_EXTERN_CS_Pin, GPIO_PIN_SET);

		if((res1 != HAL_OK) || (res2 != HAL_OK)) {
			char msg[256];
			snprintf(msg, sizeof(msg), "Error during reading the data, res1 = %d, res2 = %d\r\n", res1, res2);
			DBGLog((uint8_t*)msg);
			return;
		}
	}

	if(memcmp(rmsg, wmsg, sizeof(rmsg)) == 0) {
		const char result[] = "Test passed!\r\n";
		DBGLog((uint8_t*)result);
	} else {
		char msg[256];
		snprintf(msg, sizeof(msg), "Test failed: wmsg = '%s', rmsg = '%s'\r\n", wmsg, rmsg);
		DBGLog((uint8_t*)msg);
	}
}
