/*
 * gsm.c
 *
 *  Created on: May 25, 2019
 *      Author: khomin
 */
#include "gsm.h"
#include "gsmLLR.h"
#include "gsmLLR2.h"
#include "gsmPPP.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdlib.h>
#include "debug_print.h"
#include "time.h"

sGsmState gsmState = {0};
uint8_t gsmCsqValue = 0;
char aIMEI[16] = "000000000000000";
char aIMSI[16] = "000000000000000";
char aSimCardNumber[16] = "000000000000000";
char aManufacturedId[30] = "-";
char aModelId[30] = "-";
char aVerionSoftware[30] = "-";
char aLocation[25] = "0";
char strOperatorName[20] = {0};
char gsmSmsBuff[64];
char gsmSmsNumBuff[20];
extern char strOperatorName[20];
xTaskHandle gsmInitTaskId;
extern sGsmUartParcer uartParcerStruct;
extern bool pppIsOpen;

sGsmSettings gsmSettings = {0, "internet", "gdata", "gdata"};

sConnectSettings connectSettings = {{"193.193.165.166", "111", 20332}};

void vGsmTask( void * pvParameters );

/* init GSM */
void gsmTaskInit(void) {
	xTaskCreate(vGsmTask, "gsmTsk", (configMINIMAL_STACK_SIZE*2), 0, tskIDLE_PRIORITY+1, &gsmInitTaskId);
}

/* task GSM module */
void vGsmTask( void * pvParameters ) {	
	// lowLevel init
	gsmLLR_Init();
	gsmLLR2_Init();
	gsmPPP_Init();

	while((gsmState.initLLR != true) && (gsmState.initLLR2 != true)) {};

	while(1) {
		if(gsmState.init == false) {
			if(gsmLLR_PowerUp() != eOk) {
				gsmLLR_ModuleLost();
				continue;
			}
			// если модуль перестал отвечать
			if(gsmState.notRespond == true) {
				DBGInfo("GSM: INIT Module lost");
				gsmLLR_ModuleLost();
				continue;
			}
			// готовность модуля
			if(gsmLLR_ATAT() != eOk) {
				gsmState.notRespond = true;
				continue;
			}
			// настройки ответа
			if(gsmLLR_FlowControl() != eOk) {
				gsmState.notRespond = true;
				continue;
			}
//			// читаем IMEI
//			if(gsmLLR_GetIMEI(aIMEI) != eOk) {
//				gsmState.notRespond = true;
//				continue;
//			}
//			DBGInfo("GSM: module IMEI=%s", aIMEI);
//			// читаем imsi
//			if(gsmLLR_GetIMSI(aIMSI) != eOk) {
//				gsmState.notRespond = true;
//				continue;
//			}
//			DBGInfo("GSM: module IMSI=%s", aIMSI);
//			// Версия Software
//			if(gsmLLR_GetModuleSoftWareVersion(aVerionSoftware) != eOk) {
//				gsmState.notRespond = true;
//				continue;
//			}
			// вывод сообщения о регистрации сети (URC)
			if(gsmLLR_AtCREG() != eOk) {
				gsmState.notRespond = true;
				continue;
			}
			DBGInfo("GSM: CREG OK");
			// attach gprs
//			vTaskDelay(DELAY_REPLY_INIT/portTICK_RATE_MS);
//			if(gsmLLR_AttachGPRS() != eOk) {
//				DBGInfo("GSM: get time ERROR, -RELOAD");
//				gsmState.notRespond = true;
//				continue;
//			}
			//start ppp
			DBGInfo("GSM: INIT PPPP");
			if(gsmLLR_StartPPP(&gsmSettings) == eOk) {
				DBGInfo("GSM: INIT PPPP - PPP RUN");
				xQueueReset(uartParcerStruct.uart.rxQueue);
				uartParcerStruct.ppp.pppModeEnable = true;
				uartParcerStruct.uart.receiveState = true;
				gsmState.init = true;
			} else {
				DBGInfo("GSM: INIT PPPP - PPP ERROR!!!");
				gsmState.notRespond = true;
				continue;
			}
		}

		vTaskDelay(3000/portTICK_RATE_MS);
	}
}
