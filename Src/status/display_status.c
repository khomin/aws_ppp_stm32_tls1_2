/*
 * display_status.c
 *
 *  Created on: Jul 7, 2019
 *      Author: khomin
 */
#include "status/display_status.h"
#include "ssd1306_oled/ssd1306.h"

extern FontDef Font_16x26;

const char caption_display_init_system[] = "Init: system\r\n";
const char caption_display_init_gsm[] = "Init: GSM\r\n";
const char caption_display_init_ppp[] = "Init: PPP\r\n";
const char caption_display_connecting_ppp[] = "Connecting: PPP\r\n";
const char caption_display_connecting_mqtt[] = "Connecting: mqtt\r\n%s\r\n";
const char caption_display_mqtt_ready_publish[] = "Mqtt: ready\r\npublish\r\n";
const char caption_display_fpga_rx_data[] = "FPGA:\r\nrxData %lu\r\n";
const char caption_display_fpga_rx_complete[] = "FPGA:\r\nrx complete\r\n";
const char caption_display_mqtt_send_data[] = "Mqtt:\r\nsent: %lu\r\nTotal len: %lu\r\n";
const char caption_display_mqtt_send_total[] = "Mqtt:\r\nsent total %lu -Ok\r\n";
const char caption_display_mqtt_send_error[] = "Mqtt:\r\nsend != SUCCESS\r\nDisconnect!\r\n";
const char caption_display_mqtt_certs_error[] = "Mqtt:\r\ncerts -incorrect\r\n";

char caption_temp_buff[128] = {0};

#define DISPLAY_MAX_ROWS				5
#define DISPLAY_MAX_CHARTS_IN_LINE		30

static char printBuf[DISPLAY_MAX_ROWS][DISPLAY_MAX_CHARTS_IN_LINE];

void setDisplayStatus(char* pCaption) {
	uint8_t line = 0;
	char *pLastChar = NULL;
	char *pCurrnetChar = NULL;

	memset(printBuf, 0, sizeof(printBuf));
	ssd1306_Clear();
	ssd1306_Fill();

	pCurrnetChar = pCaption;
	while(*pCurrnetChar != 0) {
		pLastChar = strchr((char*)pCurrnetChar, '\r');
		if(pLastChar != NULL) {
			memcpy((char*)printBuf[line++], (char*)pCurrnetChar, (pLastChar - pCurrnetChar));
			if((pLastChar+2) !=0) {
				pLastChar += 2;
			}
			pCurrnetChar = pLastChar;
		} else {
			pCurrnetChar ++;
		}
	}

	pCurrnetChar++;
	line = 0;
	for(uint8_t i=0; i<DISPLAY_MAX_ROWS; i++) {
		ssd1306_SetCursor(0, line);
		ssd1306_WriteString(printBuf[i], Font_7x10);
		line += 10;
	}
	ssd1306_UpdateScreen();
}
