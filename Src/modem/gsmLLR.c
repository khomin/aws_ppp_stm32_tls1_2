/*
 * gsmLLR.c
 *
 *  Created on: May 25, 2019
 *      Author: khomin
 */
#include "gsmLLR.h"
#include "gsmLLR2.h"
#include "gsmPPP.h"
#include "string.h"
#include "main.h"
#include "cmsis_os.h"
#include "cmsis_os.h"
#include "debug_print.h"
#include <stdlib.h>

extern sConnectSettings connectSettings;
extern sGsmSettings gsmSettings;

#define CTRL_Z	0x1A
extern UART_HandleTypeDef huart3;
extern sGsmState gsmState;
extern sGsmUartParcer uartParcerStruct;
extern UART_HandleTypeDef huart3;
//Буферы данных
static uint8_t gsmCsqValue = 0;
extern char aIMEI[16];
extern char aIMSI[16];
extern char aSimCardNumber[16];
extern char aManufacturedId[30];
extern char aModelId[30];
extern char aVerionSoftware[30];
extern char aLocation[25];
extern char strOperatorName[20];

static const char *comATAT[] = {"AT"};
static const char *comGetIMEI[] = {"AT+GSN"};
static const char *comGetIMSI[] = {"AT+CIMI"};
static const char *comModemVersionSoftware[] = {"AT+CGMR"};
//static const char *comFlowControl[] = {"AT+IFC=2,2"};
static const char *comCSQ[] = {"AT+CSQ"};
static const char *comCREG[] = {"AT+CREG=1"};
static const char *comCGATT[] = {"AT+CGATT=1"};
// формат SMS
static const char *comSmsFormat[] = {"AT+CMGF=1"};
static const char *comNumberSimCard[] = {"AT+CNUM"};
static const char *comOldGprsProtocol[] = {"AT+CR99=0/1"};

// Удаление всех SMS
static const char *comDeleteSMS[] = {"AT+CMGDA=\"DEL ALL\""};
// получить время
static const char *comGetTime[] = {"AT+CCLK?"};
// Отправка SMS
static const char *comSmsSend[] = {"AT+CMGS"};
static const char *comATZ[] = {"ATZ"};
// Чтение SMS непрочинанные
static const char *comSmsReadUnRead[] = {"AT+CMGL=\"ALL\""};
static const char *comWarningOff[]	= {"AT+CBATRANG=\"1\",\"4700\",\"3300\",\"4600\",\"3400\""};
static const char *comPPP_0[] = {"AT+CGDCONT=1,\"IP\","};
static const char *comPPP_1[] = {"ATD*99#"};
static xSemaphoreHandle xGsmLockMutex;
xSemaphoreHandle sioWriteSemaphore;

static bool gsmLLR_PeriperalInit(void);
static bool gsmLLR_SemaphoreInit(void);
static bool gsmLLR_GetMutex();
static bool gsmLLR_GiveMutex();
static eRetComm runAtCommand(const char* pCommand, sResultCommand *resultCommand);

/**************************************************/
/* 	Настройка переферии, создание мьютексов 			*/
/**************************************************/
eRetComm gsmLLR_Init(void) {
	gsmLLR_PeriperalInit();
	gsmState.initLLR = true;	
	return eOk;
}

bool gsmLLR_PeriperalInit(void) {
	uartParcerStruct.uart.huart = &huart3;
	// размер пула
	uartParcerStruct.poolItemSize = MAX_STR_SIZE_GSM;
	uartParcerStruct.poolSize = 1;
	// размер очереди (для передачи буфа с указателм на пул)
	uartParcerStruct.queueItemSize = sizeof(T_MemBuff);	
	uartParcerStruct.queueSize = 10;

	uartParcerStruct.uart.pRxBuffer	= NULL;
	uartParcerStruct.uart.receiveCount = 0;
	uartParcerStruct.uart.rxQueue = xQueueCreate(QUEUE_LENGTH, 1);
	gsmUartInit(&uartParcerStruct);
	gsmLLR_SemaphoreInit();

	HAL_GPIO_WritePin(GSM_RESET_GPIO_Port, GSM_RESET_Pin, RESET);

	return true;
}

bool gsmLLR_SemaphoreInit(void) {
	xGsmLockMutex = xSemaphoreCreateMutex();
	return true;
}

bool gsmLLR_GetMutex(void) {
	if(xSemaphoreTake(xGsmLockMutex, portMAX_DELAY) == pdPASS) {
		return true;
	} else {
		return false;
	}
}

bool gsmLLR_GiveMutex(void) {
	xSemaphoreGive(xGsmLockMutex);
	return true;
}

/**************************************************/
/* 				power and reset					  */
/**************************************************/
eRetComm gsmLLR_PowerUp(void) {
	uint8_t attemp = 0;
	DBGInfo("GSM: power up begin");
	// если уже высокий
	if(HAL_GPIO_ReadPin(GSM_STATUS_GPIO_Port, GSM_STATUS_Pin) == GPIO_PIN_RESET) {
		DBGInfo("GSM: module already up");
		return true;
	} else {
		for(attemp = 0; attemp <3; attemp++) {
			DBGInfo("GSM: power up, attempt %d", attemp);
			// если статус высокий, не сбрасывать
			if(HAL_GPIO_ReadPin(GSM_STATUS_GPIO_Port, GSM_STATUS_Pin) == GPIO_PIN_RESET) {
				if(gsmLLR_ATAT() == eOk) {
					gsmState.init = false;
					gsmState.powerState = POWER_STATE_NORMAL;
					DBGInfo("GSM: power up done ok");
					return true;
				} else {
					DBGInfo("GSM: not reply");
				}
			} else {
				HAL_GPIO_WritePin(GSM_PWRKY_GPIO_Port, GSM_PWRKY_Pin, SET);
				vTaskDelay(6000/portTICK_RATE_MS);
				DBGInfo("GSM: pwkey done");
			}
		}
	}
	return false;
}

eRetComm gsmLLR_PowerDown(void) {
	return eOk;
}

// модуль не отвечает, попытка перезагрузить
void gsmLLR_ModuleLost(void) {
	gsmState.init = false;
	DBGInfo("GSM: module  -reset");
	gsmLLR_Reset();
	gsmState.notRespond = false;
}

xSemaphoreHandle * gsmLLR_GetRxSemphorePoint(uint8_t num) {
	return gsmPPP_GetRxSemaphorePoint(num);
}

eRetComm gsmLLR_Reset(void) {
	HAL_GPIO_WritePin(GSM_RESET_GPIO_Port, GSM_RESET_Pin, SET);
	vTaskDelay(1000/portTICK_RATE_MS);
	HAL_GPIO_WritePin(GSM_RESET_GPIO_Port, GSM_RESET_Pin, RESET);
	vTaskDelay(5000/portTICK_RATE_MS);
	return eOk;
}

eRetComm gsmLLR_WarningOff(void) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comWarningOff, &resultCommand);
		ret = runAtCommand(*comATZ, &resultCommand);
		gsmLLR_GiveMutex();
	}
	return ret;
}

eRetComm gsmLLR_ATAT(void) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comATAT, &resultCommand);
		gsmLLR_GiveMutex();
	}
	return ret;
}

eRetComm gsmLLR_StartPPP(sGsmSettings *pProperty) {
	sResultCommand resultCommand;
	uint8_t *pData = NULL;
	if(gsmLLR_GetMutex() == true) {
		pData = pvPortMalloc(GSM_MALLOC_COMMAND_SIZE);
		if(pData != NULL) {
			memset(pData, 0, GSM_MALLOC_COMMAND_SIZE);

//			sprintf((char*)pData, "%s", "AT+COPS=?"); // empty
//			runAtCommand((char*)pData, &resultCommand);

			sprintf((char*)pData, "%s", "AT+CFUN=1");
			runAtCommand((char*)pData, &resultCommand);

//			sprintf((char*)pData, "%s", "AT+COPS=?");
//			runAtCommand((char*)pData, &resultCommand);
//
//			sprintf((char*)pData, "%s", "AT+CMNB=1");		// AT+CMNB=1...OK......
//			runAtCommand((char*)pData, &resultCommand);
//
//			sprintf((char*)pData, "%s", "AT+CBANDCFG=\"CAT-M\",8");
//			runAtCommand((char*)pData, &resultCommand);		// .AT+CBANDCFG="CAT-M",8...OK
//
//			sprintf((char*)pData, "%s", "AT+CBANDCFG=?");
//			runAtCommand((char*)pData, &resultCommand);		// .AT+CBANDCFG=?...+CBANDC FG: (CAT-M,NB-IOT),(3,5,8,20,28)....OK
//
//			sprintf((char*)pData, "%s", "AT+CNMP=38");
//			runAtCommand((char*)pData, &resultCommand);
//
//			sprintf((char*)pData, "%s", "AT+CSTT?");		// .AT+CNMP=38...OK........
//			runAtCommand((char*)pData, &resultCommand);
//
//			sprintf((char*)pData, "%s", "AT+CPSI?");
//			runAtCommand((char*)pData, &resultCommand);
//
//			sprintf((char*)pData, "%s", "AT+COPS=?");		 // .AT+COPS=?..
//			runAtCommand((char*)pData, &resultCommand);

			vTaskDelay(2000/portTICK_RATE_MS);

			sprintf((char*)pData, "%s\"%s\"", "AT+CSTT=", "internet");	// AT+CSTT="internet"...OK
			runAtCommand((char*)pData, &resultCommand);

			sprintf((char*)pData, "%s", "AT+CGDCONT=1,\"IP\",\"internet\"");	/// .
			runAtCommand((char*)pData, &resultCommand);

//			sprintf((char*)pData, "%s", "AT+CUSD=1,\"*99#\"");
			sprintf((char*)pData, "%s", "ATD*99#");
			runAtCommand(*comPPP_1, &resultCommand);

			uartParcerStruct.ppp.pppModeEnable = true;
			uartParcerStruct.uart.receiveState = true;

			memset(pData, 0, GSM_MALLOC_COMMAND_SIZE);
			vPortFree(pData);
		}
		gsmLLR_GiveMutex();
	}
	return eOk;
}

//sResultCommand resultCommand;
//char **comPPP_Mass[3] = {comPPP_2, comPPP_3, comPPP_4};
//uint8_t *pData = NULL;
//if(gsmLLR_GetMutex() == true) {
//	pData = pvPortMalloc(GSM_MALLOC_COMMAND_SIZE);
//	if(pData != NULL) {
//		memset(pData, 0, GSM_MALLOC_COMMAND_SIZE);
//		sprintf((char*)pData, "%s%s", comPPP_0[0], "\"internet\"");
//		runAtCommand((char*)pData, &resultCommand);
//
//		uint8_t stepIndex = 0;
//		while(stepIndex != (3)) {
//			uint16_t len = strlen((char*)*comPPP_Mass[stepIndex]);
//			sprintf((char*)pData, "%s\0", (char*)*comPPP_Mass[stepIndex]);
//			runAtCommand((char*)pData, &resultCommand);
//			stepIndex++;
//		}
//		memset(pData, 0, GSM_MALLOC_COMMAND_SIZE);
//		vPortFree(pData);
//	}
//	gsmLLR_GiveMutex();
//}
//return eOk;

eRetComm gsmLLR_FlowControl(void) {
	eRetComm ret;
//	sResultCommand resultCommand;
//	if(gsmLLR_GetMutex() == true) {
//		ret = runAtCommand(*comFlowControl, &resultCommand);
//		gsmLLR_GiveMutex();
//	}
//	return ret;
	return eOk;
}

/* значение уровня сигнала 0-31 или 99 если ошибка */
uint8_t gsmLLR_GetCSQ(void) {
	gsmCsqValue = 27;
	return gsmCsqValue;
}

eRetComm gsmLLR_UpdateCSQ(uint8_t *value) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comCSQ, &resultCommand);
		gsmLLR_GiveMutex();
	}
	return ret;
}

eRetComm gsmLLR_GetIMEI(char *strImei) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comGetIMEI, &resultCommand);
		if(ret == eOk) {
			sscanf((char*)uartParcerStruct.uart.pRxBuffer, "\nAT+GSN\r\r\n%s\r\n", strImei);
		}		
		gsmLLR_GiveMutex();
	}
	return ret;
}

eRetComm gsmLLR_GetIMSI(char *strImsi) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comGetIMSI, &resultCommand);
		if(ret == eOk) {
			sscanf((char*)uartParcerStruct.uart.pRxBuffer, "\nAT+CIMI\r\r\n%s\r\n", strImsi);
		}		
		gsmLLR_GiveMutex();
	}
	return ret;
}

eRetComm gsmLLR_GetModuleSoftWareVersion(char *strSoftwareVersion) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comModemVersionSoftware, &resultCommand);
		if(ret == eOk) {
			sscanf((char*)uartParcerStruct.uart.pRxBuffer, "\nAT+CGMR\r\r\nRevision:%s\r\n", strSoftwareVersion);
		}		
		gsmLLR_GiveMutex();
	}
	return ret;
}

eRetComm gsmLLR_AtCREG(void) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comCREG, &resultCommand);
		gsmLLR_GiveMutex();
	}
	return ret;
}

uint8_t* gsmLLR_GetImeiPoint(void) {
	return (uint8_t*)&aIMEI[0];
}

uint8_t* gsmLLR_GetImsiPoint(void) {
	return (uint8_t*)&aIMSI[0];
}

/* Включение оповещение о SMS с помощью URC кодов  */
eRetComm gsmLLR_SmsUrcEnable(void) {
	return eOk;
}

/* Чтение номера симкарты */
eRetComm gsmLLR_GetSimCardNum(char *pNumber) {
	eRetComm ret;
	char* p_start = NULL;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comNumberSimCard, &resultCommand);
		if(ret == eOk) {
			p_start = strstr((char*)uartParcerStruct.uart.pRxBuffer, "\"+");
			if(p_start != NULL) {
				sscanf(p_start, "\"+%[^\"],\r\n", pNumber);
			}
		}
		gsmLLR_GiveMutex();
	}
	return ret;
}

/* Выбор режима SMS: _PDU или _TEXT_SMS */
eRetComm gsmLLR_SmsModeSelect(eSmsMode mode) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comSmsFormat, &resultCommand);
		gsmLLR_GiveMutex();
	}
	return ret;
}

/* Включение gprs для устаревшего оборудования оператора */
eRetComm gsmLLR_EnabledOldGprsProtocol(void) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comOldGprsProtocol, &resultCommand);
		gsmLLR_GiveMutex();
	}
	return ret;
}

eRetComm gsmLLR_SmsClearAll(void) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comDeleteSMS, &resultCommand);
		gsmLLR_GiveMutex();
	}
	return ret;
}

eRetComm gsmLLR_GetTime(void) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand("AT+CLTS=1;&W", &resultCommand);
		gsmLLR_GiveMutex();
	}
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comGetTime, &resultCommand);
		gsmLLR_GiveMutex();
	}
	return ret;
}

eRetComm gsmLLR_AttachGPRS(void) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comCGATT, &resultCommand);
		gsmLLR_GiveMutex();
	}
	return ret;
}

eRetComm gsmLLR_SendSMS(uint8_t *pData, uint16_t size, uint8_t *strTelNumber) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		uint8_t *pMessage = NULL;
		pMessage = pvPortMalloc(GSM_MALLOC_COMMAND_SIZE);
		if(pMessage != NULL) {
			sprintf((char*)pMessage, "%s%s%s%s%s", *comSmsSend, "=",(char*)"\"",(char*)strTelNumber, (char*)"\"");
			// отправляем должно прийти приглашение
			runAtCommand((char*)pMessage, &resultCommand);
			// отправляем само сообщение
			if(memchr((char*)uartParcerStruct.uart.pRxBuffer, '>', AT_BUFF_LENGTH) != NULL) {
				memset(pMessage, 0, GSM_MALLOC_COMMAND_SIZE);
				sprintf((char*)pMessage, "%s%c", (char*)pData, CTRL_Z);
				ret = runAtCommand((char*)pMessage, &resultCommand);
			}
			memset(pMessage, 0, GSM_MALLOC_COMMAND_SIZE);
			vPortFree(pMessage);
		}
		gsmLLR_GiveMutex();
	}
	return ret;
}

/* Чтение номера последнего непрочитанного SMS сообщения
	eNothing - ничего, eOk - есть, eError - ошибка чтения */
eRetComm gsmLLR_SmsGetLastMessage(char *pMessage, char *pNumber) {
	eRetComm ret;
	sResultCommand resultCommand;
	if(gsmLLR_GetMutex() == true) {
		ret = runAtCommand(*comSmsReadUnRead, &resultCommand);
		vTaskDelay(500/portTICK_PERIOD_MS);
		// читаем индекс
		uint8_t *pBuf = NULL;
		pBuf = pvPortMalloc(GSM_MALLOC_COMMAND_SIZE);
		if(pBuf != NULL) {
			if(ret == eOk) {
				memset(pBuf, 0, GSM_MALLOC_COMMAND_SIZE);
				getParameter(0, ":", (char*)uartParcerStruct.uart.pRxBuffer, (char*)pBuf);
				getParameter(0, ":", (char*)uartParcerStruct.uart.pRxBuffer, (char*)pBuf);
				memset(pBuf, 0, GSM_MALLOC_COMMAND_SIZE);
				getParameter(1, ":", (char*)uartParcerStruct.uart.pRxBuffer, (char*)pBuf);
				// если статус подходящий, забираем
				memset(pBuf, 0, GSM_MALLOC_COMMAND_SIZE);
				// берем номер
				getParameter(2, ":", (char*)uartParcerStruct.uart.pRxBuffer, pNumber);
				// берем текст сообщения
				sscanf((char*)uartParcerStruct.uart.pRxBuffer, "%s", pMessage);
			}
			memset(pBuf, 0, GSM_MALLOC_COMMAND_SIZE);
			vPortFree(pBuf);
		}
		gsmLLR_GiveMutex();
	}
	return ret;
}

/**********************************************************************/
/**											TCP 																				 **/
/**********************************************************************/
eRetComm gsmLLR_ConnectService(uint8_t numConnect) {
	eRetComm ret = eError;
	if(gsmLLR_GetMutex() == true) {
		if(gsmPPP_Connect(
				numConnect,
				connectSettings[0].srvAddr,
				connectSettings[0].srvPort) == true) {
			ret = eOk;
		}
		gsmLLR_GiveMutex();
	}
	return ret;
}

eRetComm gsmLLR_DisconnectService(uint8_t numConnect) {
	eRetComm ret = eError;
	if(gsmLLR_GetMutex() == true) { // отключаем соединение
		if(gsmPPP_Disconnect(numConnect) == true) {
			ret = eOk;
		}
		gsmLLR_GiveMutex();
	}
	return ret;
}

eRetComm gsmLLR_ConnectServiceStatus(uint8_t numConnect) {
	eRetComm ret = eError;
	if(gsmLLR_GetMutex() == true) {
		if(gsmPPP_ConnectStatus(numConnect) == true) {
			ret = eOk;
		}
		gsmLLR_GiveMutex();
	}
	return ret;
}

/**
 * @brief: Чтение размера принятых данных.
 * @param uint8_t serviceNum: - номер интернет-сервиса
 * @return Ошибка/Размер считанных данных
 */
uint16_t gsmLLR_TcpGetRxCount(uint8_t serviceNum) {
	return gsmPPP_GetRxLenData(serviceNum);
}

eRetComm gsmLLR_TcpSend(uint8_t serviceNum, uint8_t *pData, uint16_t sendSize) {
	eRetComm ret = eError;
	uint16_t packetSize = 0;
	uint16_t packetPos = 0;

	if(gsmLLR_GetMutex() == true) {
		// передаем
		packetPos = 0;
		while(sendSize != 0) {
			// если размер данных больше, чем может сразу отправить модем
			if(sendSize > GSM_PACKET_MAX_SIZE) { // разбиваем
				packetSize = GSM_PACKET_MAX_SIZE;
			} else {
				packetSize = sendSize;
			}
			// передача данных
			if(gsmPPP_SendData(serviceNum, &pData[packetPos], packetSize) == eOk) {
				DBGInfo("GSMSEND: SEND OK, packSize %d", packetSize);
				ret = eOk;
			} else {
				DBGInfo("GSMSEND: ERROR SEND");
				ret = eError;
				break;
			}
			sendSize -= packetSize;
			packetPos += packetSize;
		}
		vTaskDelay(500/portTICK_PERIOD_MS);
		gsmLLR_GiveMutex();
	}
	return ret;
}

/**
 * @brief: Чтение Tcp сообщения.
 * @param uint8_t serviceNum: - номер интернет-сервиса
 * @param uint8_t *rxBuffer: - указатель куда будет считаны данные
 * @param uint8_t size - размер буфера данных
 * @return Ошибка/Размер считанных данных
 */
int gsmLLR_TcpReadData(uint8_t serviceNum, uint8_t **ppData, uint16_t buffSize) {
	uint16_t retLen = 0;
	if(serviceNum > SERVERS_COUNT) {
		retLen = 0;
	} else { // копируем в буфер протокола
		retLen = gsmPPP_ReadRxData(serviceNum, ppData);
	}
	return retLen;
}

eRetComm gsmLLR_CallNumber(uint8_t *phoneNumber) {
	return eOk;
}

eRetComm gsmLLR_CallInUp(void) {
	return eOk;
}

// входяший звонок
void gsmLLR_CallIncomingHandler(uint8_t *number) {
	strcpy((char*)&gsmState.incommingNumber, (char*)number); // скопировать номер
}

eRetComm gsmLLR_CallInDown(void) {
	return eOk;
}

/**************************************************/
/* 			СЛУЖЕБНЫЕ ФУНКЦИИ					  */
/**************************************************/
eRetComm runAtCommand(const char* pCommand, sResultCommand *resultCommand) {
	eRetComm result;
	sAtReplyParsingResult replayResult;
	HAL_UART_StateTypeDef halState;
	vTaskDelay(DELAY_SEND_AT/portTICK_RATE_MS);
	uint8_t *pData = NULL;
	pData = pvPortMalloc(128);
	if(pData != NULL) {
		memset(pData, 0, GSM_MALLOC_COMMAND_SIZE);
		sprintf((char*)pData, "%s%s", pCommand, "\r\n");
		uint16_t len = strlen((char*)pData);

		uartParcerStruct.uart.receiveCount = 0;
		uartParcerStruct.uart.pRxBuffer = pvPortMalloc(AT_BUFF_LENGTH);
		memset(uartParcerStruct.uart.pRxBuffer, 0, AT_BUFF_LENGTH);
		uartParcerStruct.uart.receiveState = true;

		//TODO если не RX то запустить
		halState = HAL_UART_GetState(uartParcerStruct.uart.huart);
		if ( (halState != HAL_UART_STATE_BUSY_RX) && (halState !=  HAL_UART_STATE_BUSY_TX_RX) ) {
			if(HAL_UART_Receive_IT(uartParcerStruct.uart.huart, &(uartParcerStruct.uart.rxCh), 1) != HAL_OK) {
				DBGInfo("HAL_UART_Receive_IT - != HAL OK");
			}
		}

		if(HAL_OK == HAL_UART_Transmit_IT(uartParcerStruct.uart.huart, (uint8_t*)pData, len)) {
			uint8_t ch = 0;
			while(xQueueReceive(uartParcerStruct.uart.rxQueue, &ch, 1000/portTICK_RATE_MS) == pdTRUE) {
				//DBGInfo("Queue recive %c ", ch);
				*(uartParcerStruct.uart.pRxBuffer + uartParcerStruct.uart.receiveCount) = ch;
				uartParcerStruct.uart.receiveCount++;

				replayResult = uartParceReceiveData(&uartParcerStruct);
				if((replayResult.atReplyfound == true) && ((replayResult.atReplyType == eOk) || (replayResult.atReplyType == ePPP))) {
					resultCommand->state = eOk;
					result = eOk;
					//Выход из цикла
					break;
				}
			}
			DBGInfo("GSM_PARSER: reply %s", uartParcerStruct.uart.pRxBuffer);
			// flush last result
			uartParcerStruct.uart.receiveState = false;
		} else {
			DBGInfo("GSM_PARSER: WRITE ERROR TIMEOUT");
		}
		// flush - uartParcerStruct.uart.pRxBuffer
		vPortFree(uartParcerStruct.uart.pRxBuffer);
		memset(pData, 0, GSM_MALLOC_COMMAND_SIZE);
		vPortFree(pData);
	}
	return result;
}

eRetComm getParameter(uint8_t paramNum, char *pStartChar, char *strInput, char *strOutput) {
	int len = 0, i = 0, i2 = 0;
	char *p = NULL;
	char pBuff[128] = {0};
	uint8_t paramCounter = 0;
	if((strInput == NULL) || (strOutput == NULL)) {
		return eError;
	}
	// находим начало
	if(pStartChar == NULL) {
		// если символ начала не определен, начинаем с первого символа
		p = strInput;
	} else {
		p = strchr(strInput, *pStartChar);	
	}
	// не нашли
	if(p == NULL) {
		return eError;
	}

	len = strlen(strInput);
	memset(pBuff, 0, sizeof(pBuff));
	// вырезаем пробелы, переносы и пр.
	len = strlen(p);
	for(i=0; i<len; i++) {
		if((p[i] != '\r') && (p[i] != '\n') && (p[i] != '"') && (p[i] != ':') && (p[i] != 0)){
			if(i2 < len) {	
				pBuff[i2] = p[i];
				i2++;
			}
		}
	}
	len = strlen(pBuff);

	// поиск аргументов, кавычек, двоеточия, равно
	// проверка количества параметров
	// счетчик найденых параметров
	paramCounter = 0;
	// пока не конец
	i = 0;
	p = &pBuff[0];
	uint8_t lenOut = 0;

	while((i<len) && (*p!='\0')) {
		// если не пробел, значит данные
		if(((*p) != ' ') || (((*p) == ' ') && (i!='\0'))) {
			// если запятая, конец параметра
			if(((*p) == ',') || (*p == 0)) {
				paramCounter ++;
				if(paramCounter > paramNum) {
					return eOk;
				}
			} else {
				// иначе копируем
				if(paramCounter == paramNum) {
					// обрезаем кавычки
					if((*p) != '\"') {
						strOutput[lenOut++] = *p;
					}
				}
			}
		}
		i++;
		p++;
	}
	return eError;
}
