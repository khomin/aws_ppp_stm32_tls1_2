/*
 * display_status.c
 *
 *  Created on: Jul 7, 2019
 *      Author: khomin
 */
#include "status/display_status.h"
#include "ssd1306_oled/ssd1306.h"

extern FontDef Font_16x26;

static const char statusMessages [8][25]  = {
		{"Init..."},
		{"Init GSM..."},
		{"Init PPP..."},
		{"Wait until connect..."},
		{"Connected"},
		{"Connected and send"},
		{"Connect was lost"},
		{"Connecing - cert empty"}
};

void setDisplayStatus(eStatusDisplayTypes status) {
	ssd1306_DisplayOn();
	ssd1306_Clear();
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString((char*) statusMessages[status], Font_16x26);
}
