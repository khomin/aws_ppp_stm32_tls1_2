/**
 ******************************************************************************
 * @file    subscribe_publish_sensor_values.c
 * @author  MCD Application Team
 * @brief   Control of the measurement sampling and MQTT reporting loop.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
 * All rights reserved.</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of other
 *    contributors to this software may be used to endorse or promote products
 *    derived from this software without specific written permission.
 * 4. This software, including modifications and/or derivative works of this
 *    software, must execute solely and exclusively on microcontroller or
 *    microprocessor devices manufactured by or for STMicroelectronics.
 * 5. Redistribution and use of this software other than as permitted under
 *    this license is void and will automatically terminate your rights under
 *    this license.
 *
 * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
 * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
 * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "msg.h"
#include "aws_iot_error.h"
#include "aws_clientcredential_keys.h"
#include "aws_clientcredential.h"
#include "fpga_buf.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "fpga_buf/fpga_buf.h"
#include "fpga_buf/fpga_commander.h"
#include "FreeRTOS.h"
#include "task.h"
#include "settings/settings.h"
#include "../Src/fpga_buf/prepareJson.h"
#include "../Inc/status/display_status.h"
#include "commander/commander.h"

extern xQueueHandle fpgaDataQueue;

static void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data);

/* Private defines ------------------------------------------------------------*/
#define MQTT_CONNECT_MAX_ATTEMPT_COUNT 1
#define TIMER_COUNT_FOR_SENSOR_PUBLISH 10
static char cPTopicName[MAX_SHADOW_TOPIC_LENGTH_BYTES] = "";

/**
 * @brief main entry function to AWS IoT code
 *
 * @param no parameter
 * @return AWS_SUCCESS: 0
          FAILURE: -1
 */
int subscribe_publish_sensor_values(void) {
	bool loop_is_normal = false;
	const char *pServerAddress = NULL;
	const char *pCaCert = NULL;
	const char *pClientCert = NULL;
	const char *pClientPrivateKey = NULL;
	const char *pTopicName = NULL;
	const char *pDeviceName = NULL;
	int connectCounter;
	IoT_Error_t rc = FAILURE;
	AWS_IoT_Client client;

	memset(&client, 0, sizeof(AWS_IoT_Client));
	IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
	IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

	pTopicName = (char*)getTopicPath();
	if (strlen(pTopicName) >= MAX_SIZE_OF_THING_NAME) {
		msg_error("The length of the device name stored in the iot user configuration is larger than the AWS client MAX_SIZE_OF_THING_NAME.\n");
		return -1;
	}

	snprintf(cPTopicName, sizeof(cPTopicName), "%s", pTopicName);

	msg_info("AWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

	pServerAddress = (char*)getMqttDestEndpoint();
	pCaCert = (char*)getKeyCLIENT_CERTIFICATE_PEM();
	pClientCert = (char*)getKeyCLIENT_PRIVATE_DEVICE_CERT();
	pClientPrivateKey = (char*)getKeyCLIENT_PRIVATE_KEY_PEM();

	mqttInitParams.enableAutoReconnect = false; /* We enable this later below */
	mqttInitParams.pHostURL = (char *) pServerAddress;
	mqttInitParams.port = AWS_IOT_MQTT_PORT;
	mqttInitParams.pRootCALocation = (char *) pCaCert;
	mqttInitParams.pDeviceCertLocation = (char *) pClientCert;
	mqttInitParams.pDevicePrivateKeyLocation = (char *) pClientPrivateKey;
	mqttInitParams.mqttCommandTimeout_ms = 20000;
	mqttInitParams.tlsHandshakeTimeout_ms = 5000;
	mqttInitParams.isSSLHostnameVerify = true;
	mqttInitParams.disconnectHandler = disconnectCallbackHandler;
	mqttInitParams.disconnectHandlerData = NULL;

	rc = aws_iot_mqtt_init(&client, &mqttInitParams);

	if(AWS_SUCCESS != rc)
	{
		msg_error("aws_iot_mqtt_init returned error : %d\n", rc);
		return -1;
	}

	pDeviceName = getDeviceName();
	connectParams.keepAliveIntervalInSec = 30;
	connectParams.isCleanSession = true;
	connectParams.MQTTVersion = MQTT_3_1_1;
	connectParams.pClientID = (char *) pDeviceName;
	connectParams.clientIDLen = (uint16_t) strlen(pDeviceName);
	connectParams.isWillMsgPresent = false;

	connectCounter = 0;
	do {
		connectCounter++;
		printf("MQTT connection in progress:   Attempt %d/%d ...\n",
				connectCounter,
				MQTT_CONNECT_MAX_ATTEMPT_COUNT);
		rc = aws_iot_mqtt_connect(&client, &connectParams);
	} while((rc != AWS_SUCCESS) && (connectCounter < MQTT_CONNECT_MAX_ATTEMPT_COUNT));

	if(AWS_SUCCESS != rc)
	{
		msg_error("Error(%d) connecting to %s:%d\n", rc, mqttInitParams.pHostURL, mqttInitParams.port);
		return -1;
	}
	else
	{
		printf("Connected to %s:%d\n", mqttInitParams.pHostURL, mqttInitParams.port);
	}

	/*
	 * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
	 *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
	 *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
	 */
	rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);

	if(AWS_SUCCESS != rc)
	{
		msg_error("Unable to set Auto Reconnect to true - %d\n", rc);

		if (aws_iot_mqtt_is_client_connected(&client))
		{
			aws_iot_mqtt_disconnect(&client);
		}

		return -1;
	}

	IoT_Publish_Message_Params paramsQOS1 = {QOS1, 0, 0, 0, NULL,0};

	printf("Press the User button (Blue) to publish the LED desired value on the %s topic\n", cPTopicName);

	//-- flag that everything is normal and the cycle continues
	//-- if disconnect, then it is reset.
	loop_is_normal = true;

	while((NETWORK_ATTEMPTING_RECONNECT == rc
			|| NETWORK_RECONNECTED == rc
			|| AWS_SUCCESS == rc)
			&& loop_is_normal)
	{
		/* Max time the yield function will wait for read messages */
		rc = aws_iot_mqtt_yield(&client, 100);

		if(NETWORK_ATTEMPTING_RECONNECT == rc)
		{
			/* Delay to let the client reconnect */
			HAL_Delay(1000);
			msg_info("Attempting to reconnect\n");
			/* If the client is attempting to reconnect we will skip the rest of the loop */
			continue;
		}
		if(NETWORK_RECONNECTED == rc)
		{
			msg_info("Reconnected.\n");
		}

		setDisplayStatus((char*)caption_display_mqtt_ready_publish);

		//-- send data
		sFpgaData * p = NULL;
		if(xQueuePeek(fpgaDataQueue, &p, 1500) == pdTRUE) {
			if(p != NULL) {
				if(p->sdramData->data != NULL) {
					uint32_t sentCounter = 0;
					uint32_t offsetCounter = 0;
					do {
						if((p->sdramData->len - sentCounter) > 10240) {
							offsetCounter = 10240;
						} else {
							offsetCounter = (p->sdramData->len - sentCounter);
						}

						paramsQOS1.payload = (char*)p->sdramData->data + sentCounter;

						paramsQOS1.payloadLen = offsetCounter;

						rc = aws_iot_mqtt_publish(&client, cPTopicName, strlen(cPTopicName), &paramsQOS1);

						sentCounter += offsetCounter;

						DBGLog("MTTT: sent: %lu, total size: %lu", sentCounter, p->sdramData->len);

						sprintf(caption_temp_buff, caption_display_mqtt_send_data, sentCounter, p->sdramData->len);

						printToUsb(caption_temp_buff, strlen(caption_temp_buff));
						setDisplayStatus(caption_temp_buff);

						if (rc == AWS_SUCCESS) {
							DBGLog("Published");
						} else {
							break;
						}
					} while((MQTT_REQUEST_TIMEOUT_ERROR == rc &&(loop_is_normal))
							|| (sentCounter < p->sdramData->len));
					if(sentCounter == p->sdramData->len) {
						DBGLog("Published to topic: - final");
						xQueueReceive(fpgaDataQueue, &p, NULL);
						freeSdramBuff(p->sdramData);
						vPortFree(p);
					}
				} else {
					DBGErr("AWS: sdramData == null");
				}
			}
		}

	} /* End of while */

	/* Wait for all the messages to be received */
	aws_iot_mqtt_yield(&client, 10);

	rc = aws_iot_mqtt_disconnect(&client);

	return rc;
}

/**
 * @brief MQTT disconnect callback hander
 *
 * @param pClient: pointer to the AWS client structure
 * @param data:
 * @return no return
 */
static void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data) {
	msg_warning("MQTT Disconnect\n");
	IoT_Error_t rc = FAILURE;

	if(NULL == data) {
		return;
	}

	AWS_IoT_Client *client = (AWS_IoT_Client *)data;

	if(aws_iot_is_autoreconnect_enabled(client)) {
		msg_info("Auto Reconnect is enabled, Reconnecting attempt will start now\n");
	} else {
		msg_warning("Auto Reconnect not enabled. Starting manual reconnect...\n");
		rc = aws_iot_mqtt_attempt_reconnect(client);

		if(NETWORK_RECONNECTED == rc) {
			msg_warning("Manual Reconnect Successful\n");
		} else {
			msg_warning("Manual Reconnect Failed - %d\n", rc);
		}
	}
}