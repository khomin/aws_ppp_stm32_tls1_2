#ifndef GSMLLR_H
#define GSMLLR_H

#include "stdint.h"
#include "stdbool.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define MAX_STR_SIZE_GSM 						512
#define DELAY_REPLY_INIT						3000
#define DELAY_SEND_AT							500
#define DELAY_REPLY_AT							600
#define	DELAY_AFTER_RESET						2000
#define GSM_PACKET_MAX_SIZE						1512
#define GSM_POLLING_DELAY 						500
#define GSM_CONNECTION_ERROR_DELAY 				1000
#define GSM_DICONNECT_DELAY 					5000
#define GSM_PHONE_LEN							16
#define GSM_CALL_DELAY							5000
#define GSM_WHILE_RECEIVE_COUNT					5
#define QUEUE_LENGTH 							1024
#define AT_BUFF_LENGTH 							256
#define GSM_MALLOC_COMMAND_SIZE					128
#define AT_MARKERS_SIZE							4
#define MAX_SRV_ADDR_LEN						64
#define MAX_SRV_PASS_LEN						16
#define MAX_GPRS_APN_LEN						64
#define MAX_GPRS_USER_LEN						16
#define MAX_GPRS_PASS_LEN						16

typedef enum {
	POWER_STATE_DOWN,
	POWER_STATE_NORMAL,
}eGsmPowerState;

/* типы AT команд */
typedef enum{
	atCommonType,
	atSmsType,
	atGprsType,
	atCallStartPhoneType,
} eAtType;

/* типы результата базовой AT команды */
/* типы результата смс AT команды */
typedef enum {
	atSmsSend,
	atSmsSendError,
	atSmsReceiv,
	atSmsReceivError,
	atSmsError
}eAtSmsStatus;

typedef enum {
	eError,				//Ошибка выполнения команды без указания причины
	eOk,					//Команда выполнена успешно
	eNoCarrier,		//Ошибка выполнения команды из за отсутствия связи
	eCmeERROR, 		//Ошибка выполнения команды с указанием причины
	eCmsERROR, 		//Ошибка выполнения команды с указанием причины
	eConnect,			//Установлено CSD соединение
	eConnectError,// Ошибка соединения
	eNoDialtone,	//
	eBusy,				//Линия занята
	eWaiting,
	eGsmNotResponding,
	eInvitedSendPacket,
	eReceivePacket,
	eRingIncoming,
	eIncomingSms,
	eNothing,
	ePPP,
	eSmsReady
}eRetComm;

typedef struct{
	bool  atReplyfound;
	eRetComm atReplyType;
	struct { 
		uint8_t *pData;
		uint16_t rxCount;
	}ppp;
}T_MemBuff;

typedef struct{
	bool  atReplyfound;
	eRetComm atReplyType;
}sAtReplyParsingResult;

typedef enum{
	CALL_ERROR,
	CALL_NO_DIALTONE,
	CALL_BUSY,
	CALL_NO_CARRIER,
	CALL_NO_ANSWER,
	CALL_CONNECT
}eCallState;

typedef struct {
	eRetComm typeCommand;
	char *command;
}sAtMarkers[AT_MARKERS_SIZE];

typedef enum{
	_GSM_DATA_IDLE,
	_GSM_DATA_REQUSTED,
	_GSM_DATA_RX
}eGsmDataState;

// отправка-чтение смс
typedef struct{
	eRetComm status;
	eAtSmsStatus smsStatus;
	uint8_t telNumber[12];
	uint32_t smsData;
	uint8_t smsText[256];
}sAtSmsParam;

typedef struct __attribute__((packed)) {
	bool initLLR;
	bool initLLR2;
	bool init;
	eGsmPowerState powerState;
	bool notRespond;
	char incommingNumber[GSM_PHONE_LEN];
	char gsmIp[16];
}sGsmState;

typedef struct __attribute__((packed)) {
	uint16_t simPin;//пин-код сим-карты //TODO поменять на массив из 4ех символов
	char gprsApn[MAX_GPRS_APN_LEN];//точка доступа
	char gprsUser[MAX_GPRS_USER_LEN];//логин
	char gprsPass[MAX_GPRS_PASS_LEN];//пароль
}sGsmSettings;

typedef struct __attribute__((packed)) {
	char srvAddr[MAX_SRV_ADDR_LEN];
	char srvPass[MAX_SRV_PASS_LEN];
	uint16_t srvPort;
}sConnectSettings;

typedef struct {
	eRetComm state;
	uint8_t *pData;
}sResultCommand;

/**************************************************/
/* 							СИМ КАРТА					 								*/
/**************************************************/
typedef enum
{
	sim_ready,	//
	sim_pin,		//MT is not pending for any password
	sim_puk,		//MT is waiting SIM PIN to be given //MT is waiting for SIM PUK to be
	ph_sim_pin, // ME is waiting for phone to SIM card (antitheft)
	ph_sim_puk,	// ME is waiting for SIM PUK (antitheft)
	sim_pin2, 	//PIN2, e.g. for editing the FDN book possible only 
	sim_puk2,		//Poss ible only    if    preceding    Command    was  acknowledged with error +CME ERROR
	error,
}ePinCode;

typedef enum {
	MTS,
	MEGAFON,
	BEELINE,
	TELE2
}eOperatorType;

typedef struct {
	struct {
		eOperatorType type;
		char *name;
		char *apn;
		char *user;
		char *password;
	}code[5];
	uint8_t len;
}sOperatorList;

typedef struct {
	struct {
		eCallState state;
		char *caption;
	}code[6];
	uint8_t len;
}sCallState;

typedef struct {
	ePinCode simCode;
	char * caption;
}sParam;

typedef struct {
	sParam simCode[8];
	uint8_t size;
}sPinCodeData;

//Режимы отправки/чтения СМС
typedef enum {
	sms_PDU,
	sms_TEXT
}eSmsMode;

typedef enum {
	SOCKET,
	TRANSPARENT,
	FTP
} T_SrvType;	

//Типы состояния подключения к сервису 
typedef struct
{		
	uint8_t serviceNum;
	char *address;
} T_InternetServiceSettings;

typedef struct {
	bool simCardIsInseted;
}sSimCardState;

typedef struct __attribute__((packed)) {
	uint16_t simPin;//пин-код сим-карты //TODO поменять на массив из 4ех символов
	char gprsApn[MAX_GPRS_APN_LEN];//точка доступа
	char gprsUser[MAX_GPRS_USER_LEN];//логин
	char gprsPass[MAX_GPRS_PASS_LEN];//пароль
}sGsmPropertyApn;

/**************************************************/
/*				EXTERNAL FUNCTIONS				  */
/**************************************************/
eRetComm gsmLLR_Init(void);
eRetComm gsmLLR_PowerUp(void);
eRetComm gsmLLR_PowerDown(void);
eRetComm gsmLLR_Reset(void);
void gsmLLR_ModuleLost(void);
eRetComm gsmLLR_WarningOff(void);
eRetComm gsmLLR_Reset(void);
eRetComm gsmLLR_ATAT(void);
eRetComm gsmLLR_GetSimCardNum(char *pNumber);
eRetComm gsmLLR_FlowControl(void);
eRetComm gsmLLR_StartPPP(sGsmSettings *pProperty);
eRetComm gsmLLR_CallNumber(uint8_t *phoneNumber);
eRetComm gsmLLR_CallInUp(void);
eRetComm gsmLLR_CallInDown(void);
eRetComm gsmLLR_SendSMS(uint8_t *pData, uint16_t size, uint8_t *strTelNumber);
eRetComm gsmLLR_SmsModeSelect(eSmsMode mode);
eRetComm gsmLLR_SmsClearAll(void);
eRetComm gsmLLR_GetTime(void);
eRetComm gsmLLR_AttachGPRS(void);
eRetComm gsmLLR_EnabledOldGprsProtocol(void);
eRetComm gsmLLR_SmsGetLastMessage(char *pMessage, char *pNumber);
uint8_t gsmLLR_GetCSQ(void);
eRetComm gsmLLR_UpdateCSQ(uint8_t *value);
eRetComm gsmLLR_GetIMEI(char *strImei);
eRetComm gsmLLR_GetIMSI(char *strImsi);
uint8_t* gsmLLR_GetImeiPoint(void);
uint8_t* gsmLLR_GetImsiPoint(void);
eRetComm gsmLLR_GetModuleSoftWareVersion(char *strSoftwareVersion);
eRetComm gsmLLR_AtCREG(void);
eRetComm gsmLLR_GetNameOperator(char *strNameOperator);
eRetComm gsmLLR_ConnectService(uint8_t numConnect);
eRetComm gsmLLR_DisconnectService(uint8_t numConnect);
eRetComm gsmLLR_ConnectServiceStatus(uint8_t numConnect);
//uint16_t gsmLLR_TcpGetRxCount(uint8_t serviceNum);
//eRetComm gsmLLR_TcpSend(uint8_t serviceNum, uint8_t *pData, uint16_t size);
//int	gsmLLR_TcpReadData(uint8_t **ppData, uint16_t buffSize);
//xSemaphoreHandle* gsmLLR_GetRxSemphorePoint(uint8_t num);
//
//eRetComm getParameter(uint8_t paramNum, char *pStartChar, char *strInput, char *strOutput);

#endif /* __AT_PARAM_STRUCT_H */
