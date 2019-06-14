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
sConnectSettings connectSettings = {{"31.10.4.146", "111", 45454}};

void vGsmTask( void * pvParameters );

/* init GSM */
void gsmTaskInit(void) {
	xTaskCreate(vGsmTask, "gsmTsk", 1024, 0, tskIDLE_PRIORITY+1, &gsmInitTaskId);
}

/* task GSM module */
void vGsmTask( void * pvParameters ) {	
	gsmLLR_Init();
	gsmLLR2_Init();
	gsmPPP_Init();

	while((gsmState.initLLR != true) && (gsmState.initLLR2 != true)) {};

	while(1) {
		//
		// if gsm is ready
		// then the task does nothing in the pause
		// otherwise begin initialization
		//
		if(gsmState.init == false) {
			//
			// if one of the stages returns an error
			// then it all starts with the first step
			//
			if(gsmLLR_PowerUp() != eOk) {
				gsmLLR_ModuleLost();
				continue;
			}
			// if the module stops responding
			if(gsmState.notRespond == true) {
				DBGInfo("GSM: init -module lost");
				gsmLLR_ModuleLost();
				continue;
			}
			// check module is ready
			if(gsmLLR_ATAT() != eOk) {
				gsmState.notRespond = true;
				continue;
			}
			// disable power warnings
			if(gsmLLR_WarningOff() != eOk) {
				gsmState.notRespond = true;
				continue;
			}
			// set type reply
			if(gsmLLR_FlowControl() != eOk) {
				gsmState.notRespond = true;
				continue;
			}
			// get IMEI
			if(gsmLLR_GetIMEI(aIMEI) != eOk) {
				gsmState.notRespond = true;
				continue;
			}
			DBGInfo("GSM: module IMEI=%s", aIMEI);
			// get IMSI
			if(gsmLLR_GetIMSI(aIMSI) != eOk) {
				gsmState.notRespond = true;
				continue;
			}
			DBGInfo("GSM: module IMSI=%s", aIMSI);
			// get software version
			if(gsmLLR_GetModuleSoftWareVersion(aVerionSoftware) != eOk) {
				gsmState.notRespond = true;
				continue;
			}
			// set the type of registration message
			if(gsmLLR_AtCREG() != eOk) {
				gsmState.notRespond = true;
				continue;
			}
			DBGInfo("GSM: creg -OK");

			// get csq
			if(gsmLLR_UpdateCSQ(&gsmCsqValue) != eOk) {
				DBGInfo("GSM: get CSQ ERROR, -RELOAD");
				gsmState.notRespond = true;
				continue;
			} else {
				DBGInfo("GSM: csq value %d", gsmCsqValue);

				// get phone number
				if(gsmLLR_GetSimCardNum(aSimCardNumber) != eOk) {
					gsmState.notRespond = true;
					continue;
				}

				// start ppp
				DBGInfo("GSM: init PPP...");
				if(gsmLLR_StartPPP(&gsmSettings) == eOk) {
					DBGInfo("GSM: init PPP -ready");
					xQueueReset(uartParcerStruct.uart.rxQueue);
					uartParcerStruct.ppp.pppModeEnable = true;
					uartParcerStruct.uart.receiveState = true;
					gsmState.init = true;
				} else {
					DBGInfo("GSM: init PPP -ERROR");
					gsmState.notRespond = true;
					continue;
				}
			}
		}

		vTaskDelay(3000/portTICK_RATE_MS);
	}
}
