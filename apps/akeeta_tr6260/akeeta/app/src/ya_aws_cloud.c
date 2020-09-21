/*
 * Copyright(c) 2018 - 2020 Yaguan Technology Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ya_common.h"
#include "cJSON.h"
#include "ya_config.h"
#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"


#if (CLOUD_SUPPORT == US_CN_CLOUD_SUPPORT || CLOUD_SUPPORT == US_CLOUD_SUPPORT)
#include "ya_aws_cloud.h"

char* aws_rootCABuff = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\n" \
"-----END CERTIFICATE-----\n";

//char *mcu_test_string = "{\"data\":{\"failRetryInternal\":\"10\",\"failRetryTimes\":\"2\",\"firmwareType\":\"mcu\",\"md5sum\":\"760b9b45dc989df1868ca25323b9a259\",\"size\":\"43040\",\"type\":\"wifi\",\"url\":\"http://device-open.oss-cn-shanghai.aliyuncs.com/index/top/mcu_1591234821111.bin\",\"version\":\"nxp.mcu.100.002\"},\"method\":\"thing.service.device.upgrade\"}";

typedef enum
{
	AWS_CLOUD_INIT	   				= 0x00,
	AWS_TO_CONNECT_CLOUD,
	AWS_CONNECTED_CLOUD,
	AWS_CONNECT_ERROR,
	AWS_RECONNECT_WAIT,
}ya_aws_cloud_state_t;

typedef enum
{
	YA_PUBLISH_RESPONSE	   				= 0x00,
	YA_HARDWARE_REPORT,
}ya_aws_cloud_queue_msg_type_t;

typedef struct ya_aws_cloud_event_callbackhandler{
	char *task_name;
	cloud_event_type_t event_type;
	cloud_event_callback	 ya_cloud_event_handler;

	struct ya_aws_cloud_event_callbackhandler *ya_aws_cloud_event_callbackhandler_next;
}ya_aws_cloud_event_callbackhandler_t;

ya_aws_cloud_event_callbackhandler_t *ya_aws_cloud_callback_list = NULL;


ya_hal_os_thread_t ya_aws_app_cloud = NULL;
ya_hal_os_queue_t ya_aws_cloud_msg_queue = NULL;

ya_aws_cloud_state_t ya_aws_cloud_state = AWS_CLOUD_INIT;
static uint8_t ya_cloud_connected = 0;
char *subscribe_topic = NULL;
AWS_IoT_Client aws_client;

uint8_t aws_cloud_domain = WIFI_US;

char *ya_get_CA_cert(void)
{
	return aws_rootCABuff;
}

int32_t ya_add_aws_cloud_event_listener(char *task_name, cloud_event_type_t event_type, cloud_event_callback ya_cloud_event_chandler)
{
	ya_aws_cloud_event_callbackhandler_t *list_node = NULL, *add_node = NULL;

	if(!task_name)
		return C_ERROR;

	list_node = ya_aws_cloud_callback_list;

	while(list_node)
	{
		if(list_node->task_name)
		{
			if(strcmp(list_node->task_name, task_name) == 0 && list_node->event_type == event_type)
				return C_OK;
		}

		if(!(list_node->ya_aws_cloud_event_callbackhandler_next))
			break;
		else
			list_node = list_node->ya_aws_cloud_event_callbackhandler_next;
	}

	add_node = (ya_aws_cloud_event_callbackhandler_t *)ya_hal_os_memory_alloc(sizeof(ya_aws_cloud_event_callbackhandler_t));
	if(!add_node)
		return C_ERROR;

	add_node->task_name = (char *)ya_hal_os_memory_alloc(strlen(task_name) + 1);
	if(!add_node->task_name)
	{
		if(add_node)
			ya_hal_os_memory_free(add_node);
		
		return C_ERROR;
	}

	strcpy(add_node->task_name,  task_name);
	add_node->event_type = event_type;
	add_node->ya_cloud_event_handler = ya_cloud_event_chandler;

	add_node->ya_aws_cloud_event_callbackhandler_next = NULL;

	if(list_node)
		list_node->ya_aws_cloud_event_callbackhandler_next = add_node;
	else
		ya_aws_cloud_callback_list = add_node;

	ya_printf(C_LOG_INFO,"ya_add_aws_cloud_event_listener: %s\n", task_name);
	return C_OK;
}

int32_t ya_remove_aws_cloud_event_listener(char *task_name, cloud_event_type_t event_type)
{
	ya_aws_cloud_event_callbackhandler_t *list_node = NULL, *pre_node = NULL;

	if(!task_name)
		return C_ERROR;

	list_node = ya_aws_cloud_callback_list;
	pre_node = list_node;
		
	while(list_node)
	{
		if(list_node->task_name)
		{
			if(strcmp(list_node->task_name, task_name) == 0 && list_node->event_type == event_type)
				break;
		}

		if(!(list_node->ya_aws_cloud_event_callbackhandler_next))
			return C_ERROR;
		else
		{
			pre_node = list_node;
			list_node = list_node->ya_aws_cloud_event_callbackhandler_next;
		}
	}

	if(list_node)
		return C_ERROR;

	if(list_node == ya_aws_cloud_callback_list && ya_aws_cloud_callback_list->ya_aws_cloud_event_callbackhandler_next == NULL)
		ya_aws_cloud_callback_list = NULL;
	else
	{
		if(list_node == ya_aws_cloud_callback_list)
			ya_aws_cloud_callback_list = list_node->ya_aws_cloud_event_callbackhandler_next;
		else
			pre_node->ya_aws_cloud_event_callbackhandler_next = list_node->ya_aws_cloud_event_callbackhandler_next;
	}
	
	if(list_node->task_name)
	{
		ya_hal_os_memory_free(list_node->task_name);
		list_node->task_name = NULL;
	}
	
	if(list_node)
		ya_hal_os_memory_free(list_node);

	ya_printf(C_LOG_INFO,"listener: %s, event: %d\n", task_name, event_type);

	return C_OK;
}

int32_t ya_trigger_aws_cloud_event_listener(uint8_t event_type, uint8_t *data, uint16_t data_len)
{ 
	ya_aws_cloud_event_callbackhandler_t *list_node = NULL;
	list_node = ya_aws_cloud_callback_list;
		
	while(list_node)
	{
		if(list_node->task_name)
		{
			if(list_node->event_type == event_type)
			{
				list_node->ya_cloud_event_handler(data, data_len);
			}
		}

		if(!(list_node->ya_aws_cloud_event_callbackhandler_next))
			return C_OK;
		else
		{
			list_node = list_node->ya_aws_cloud_event_callbackhandler_next;
		}
	}

	return C_OK;
}

void ya_aws_disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data)
{
	ya_aws_cloud_state = AWS_CONNECT_ERROR;	
}

static int32_t ya_send_cloud_status(uint8_t cloud_status)
{	
	uint8_t buf[1];
	buf[0] = cloud_status;
	return ya_trigger_aws_cloud_event_listener(CLOUD_ONOFF_EVENT, buf, 1);
}


int32_t ya_aws_subscribe_event_transfer(uint8_t *buf, uint16_t data_len)
{
	cJSON *cmd = NULL, *method = NULL;

	cmd = cJSON_Parse((char *)buf);
	if(!cmd)
		goto err;

	method = cJSON_GetObjectItem(cmd, "method");
	if(!method)
		goto err;

	if(strstr(method->valuestring, "set"))
	{
		ya_trigger_aws_cloud_event_listener(CLOUD_SET_EVENT, buf, data_len);
	}else if(strstr(method->valuestring, "upgrade"))
	{
		ya_trigger_aws_cloud_event_listener(CLOUD_UPGRADE_EVENT, buf, data_len);
	}else if(strstr(method->valuestring, "unregister"))
	{
		ya_send_cloud_status(YA_CLOUD_DEBIND);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return C_OK;

	err:

	if(cmd)
		cJSON_Delete(cmd);

	return C_ERROR;	
}

static void ya_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
									IoT_Publish_Message_Params *params, void *pData) 
{
	int32_t ret = -1;
	msg_t ms_msg;
	char *p = NULL;
	uint8_t *buff = NULL;
	char *topic_get = NULL;
	uint16_t msg_id_len = 0;
	
	char *payload = (char *)params->payload;

	topic_get = (char *)ya_hal_os_memory_alloc(topicNameLen + 1);
	memset(topic_get, 0, topicNameLen + 1);
	memcpy(topic_get, topicName, topicNameLen);

	payload[params->payloadLen] = '\0';

	ya_printf(C_LOG_INFO,"rece topic %s\n", topic_get);
	ya_printf(C_LOG_INFO,"rece buffer %s\n", payload);

	//get the messige id
	p = strstr(topic_get, "/request/");

	if(p)
	{
		p = p + strlen("/request/");
		msg_id_len = strlen(p);
		if(msg_id_len)
		{
			buff = (uint8_t *)ya_hal_os_memory_alloc(msg_id_len + 1);
			memset( buff, 0, (msg_id_len + 1));
			memcpy( buff, p, msg_id_len);

			memset(&ms_msg, 0, sizeof(msg_t));
			ms_msg.addr = buff;
			ms_msg.len = (msg_id_len + 1);
			ms_msg.type = YA_PUBLISH_RESPONSE;

			ret = ya_hal_os_queue_send(&ya_aws_cloud_msg_queue, &ms_msg, 10);
			if(ret != C_OK)
			{
				ya_printf(C_LOG_ERROR,"cloud queue error\n");

				if(ms_msg.addr)
					ya_hal_os_memory_free(ms_msg.addr);

				if(topic_get)
					ya_hal_os_memory_free(topic_get);

				return;
			}
		}

		ya_aws_subscribe_event_transfer(params->payload, params->payloadLen);
		
	}else
	{
		ya_printf(C_LOG_ERROR,"topic error\n");
	}	

	if(topic_get)
		ya_hal_os_memory_free(topic_get);

	return;
}

int32_t ya_get_subscribe_topic(void)
{
	const char *fmt = "/%s/rrpc/%s/request/+";
    int topic_len = 0;
	char *pClientId = NULL;

	pClientId = ya_get_client_id();
    topic_len = strlen(fmt) + strlen(pClientId) + strlen(ya_get_thing_type()) + 1;
    subscribe_topic = ya_hal_os_memory_alloc(topic_len);

   if (subscribe_topic == NULL) {
		ya_printf(C_LOG_ERROR, "memory not enough");
		return C_ERROR;
    }
	
    memset(subscribe_topic, 0, topic_len);
    snprintf(subscribe_topic, topic_len, fmt, ya_get_thing_type(), pClientId);

	return C_OK;
}

int ya_aws_subscribe(AWS_IoT_Client* client)
{
    int rc = 0;
	ya_printf(C_LOG_INFO, "subscribe topic is: %s\n", subscribe_topic);
	
	rc = aws_iot_mqtt_subscribe(client, subscribe_topic, strlen(subscribe_topic), QOS0, ya_subscribe_callback_handler, NULL);
	if(0 != rc) {
		ya_printf(C_LOG_ERROR, "Error subscribing : %d\n", rc);
	}

	return rc;
}

//finish the publish ack
int ya_aws_publish_ack(AWS_IoT_Client* client, char *msgid, char *buf)
{
	// /%s/rrpc/%s/response/{meesageId}
	int rc = 0;
	IoT_Publish_Message_Params paramsQOS0;
	char *pClientId = NULL;

	const char *fmt = "/%s/rrpc/%s/response/";
	char *topic = NULL;
	int topic_len = 0;

	pClientId = ya_get_client_id();
    topic_len = strlen(fmt) + strlen(pClientId) + strlen(ya_get_thing_type()) + strlen(msgid) + 1 ;
    topic = ya_hal_os_memory_alloc(topic_len);
    if (topic == NULL) 
	{
       ya_printf(C_LOG_ERROR, "memory not enough");
       goto err;
    }

    memset(topic, 0, topic_len);
    snprintf(topic, topic_len, fmt, ya_get_thing_type(), pClientId);
	memcpy(topic+strlen(topic), msgid, strlen(msgid));

	memset(&paramsQOS0, 0, sizeof(IoT_Publish_Message_Params));
	paramsQOS0.qos = QOS0;
	paramsQOS0.payload = (void *) buf;
	paramsQOS0.payloadLen = strlen(buf);
	paramsQOS0.isRetained = 0;

	ya_printf(C_LOG_INFO,"send ack topic %s\n", topic);
	ya_printf(C_LOG_INFO,"send buffer %s\n", buf);

	rc = aws_iot_mqtt_publish(client, topic, strlen(topic), &paramsQOS0);
	if (rc != 0) 
	{
		ya_printf(C_LOG_ERROR, "QOS0 publish ack not received.\n");
		 goto err;
	}

	ya_hal_os_memory_free(topic);
	return C_OK;
	
	err:
	ya_hal_os_memory_free(topic);
    return C_ERROR;
}

int32_t ya_report_msg_to_aws_cloud(uint8_t *data, uint16_t data_len)
{
	int32_t ret = -1;
	uint8_t *buf = NULL;
	msg_t ms_msg;
	
	buf = (uint8_t *)ya_hal_os_memory_alloc(data_len + 1);
	memset( buf, 0, (data_len + 1));
	memcpy( buf, data, data_len);
	
	memset(&ms_msg, 0, sizeof(msg_t));
	ms_msg.addr = buf;
	ms_msg.len = (data_len + 1);
	ms_msg.type = YA_HARDWARE_REPORT;
	
	ret = ya_hal_os_queue_send(&ya_aws_cloud_msg_queue, &ms_msg, 10);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR,"cloud queue error\n");
	
		if(ms_msg.addr)
			ya_hal_os_memory_free(ms_msg.addr);
	
		return -1;
	}
	
	return 0;
}


int ya_aws_report(AWS_IoT_Client* client, char *buf)
{
	// /productKey/deviceName/update
	int rc = 0;
	IoT_Publish_Message_Params paramsQOS0;
	char *pClientId = NULL;

	const char *fmt = "/%s/%s/update";
	char *topic = NULL;
	int topic_len = 0;

	pClientId = ya_get_client_id();
    topic_len = strlen(fmt) + strlen(pClientId) + strlen(ya_get_thing_type()) + 1 ;
    topic = ya_hal_os_memory_alloc(topic_len);
    if (topic == NULL) 
	{
       ya_printf(C_LOG_ERROR, "memory not enough");
       goto err;
    }

    memset(topic, 0, topic_len);
    snprintf(topic, topic_len, fmt, ya_get_thing_type(), pClientId);

	memset(&paramsQOS0, 0, sizeof(IoT_Publish_Message_Params));

	paramsQOS0.qos = QOS0;
	paramsQOS0.payload = (void *) buf;
	paramsQOS0.payloadLen = strlen(buf);
	paramsQOS0.isRetained = 0;

	ya_printf(C_LOG_INFO,"send topic %s\n", topic);
	ya_printf(C_LOG_INFO,"send buffer %s\n", buf);

	rc = aws_iot_mqtt_publish(client, topic, strlen(topic), &paramsQOS0);
	if (rc != 0) 
	{
		ya_printf(C_LOG_ERROR, "QOS0 publish not received.\n");
		 goto err;
	}

	if(topic)
		ya_hal_os_memory_free(topic);
	
	return C_OK;
	
	err:
	if(topic)
		ya_hal_os_memory_free(topic);

    return C_ERROR;

}

static int32_t ya_updata_device_version(void)
{
	cJSON *cmd = NULL, *property = NULL;
	char *data = NULL;

	char value[64+1] = {0};
	
	cmd = cJSON_CreateObject();
	if(!cmd)
		return C_ERROR;

	property = cJSON_CreateObject();
	if(!property)
	{
		if(cmd)
			cJSON_Delete(cmd);
		return C_ERROR;
	}

	cJSON_AddStringToObject(cmd, "method", "thing.service.device.bootInfo.report");
	cJSON_AddItemToObject(cmd, "property", property);

	cJSON_AddStringToObject(property, "startType", "2");

	memset(value, 0, 65);
	ya_get_ver_string(value);
	cJSON_AddStringToObject(property, "version", value);

	memset(value, 0, 65);
	ya_get_mcu_ver_string(value);
	if(value[0] != 0)
		cJSON_AddStringToObject(property, "mcuVersion", value);
	
	ya_get_mac_addr_string(value);
	cJSON_AddStringToObject(property, "mac", value);
	cJSON_AddStringToObject(property, "type", "wifi");

	data = cJSON_PrintUnformatted(cmd);	

	if(data)
	{
		ya_report_msg_to_aws_cloud((uint8_t *)data, strlen(data));
		ya_hal_os_memory_free(data);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return C_OK;
}

static int32_t ya_updata_debind_information(void)
{
	cJSON *cmd = NULL, *property = NULL;
	char *data = NULL;

	cmd = cJSON_CreateObject();
	if(!cmd)
		return C_ERROR;

	property = cJSON_CreateObject();
	if(!property)
	{
		if(cmd)
			cJSON_Delete(cmd);
		return C_ERROR;
	}

	cJSON_AddStringToObject(cmd, "method", "thing.service.device.allow.bind");
	cJSON_AddItemToObject(cmd, "property", property);

	cJSON_AddStringToObject(property, "bind", "1");
	cJSON_AddStringToObject(cmd, "productKey", ya_aws_get_thing_type());
	cJSON_AddStringToObject(cmd, "deviceName", ya_aws_get_client_id());

	data = cJSON_PrintUnformatted(cmd);	

	if(data)
	{
		ya_report_msg_to_aws_cloud((uint8_t *)data, strlen(data));
		ya_hal_os_memory_free(data);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return C_OK;
}

static int32_t ya_updata_random_information(void)
{
	int ret = -1;
	cJSON *cmd = NULL, *property = NULL;
	char *data = NULL;
	char random[33];

	memset(random, 0, 33);
	ret = ya_get_randomnum(random);

	if (ret == -1)
		return C_ERROR;

	ya_clear_randomnum();

	cmd = cJSON_CreateObject();
	if(!cmd)
		return C_ERROR;

	property = cJSON_CreateObject();
	if(!property)
	{
		if(cmd)
			cJSON_Delete(cmd);
		return C_ERROR;
	}

	cJSON_AddStringToObject(cmd, "method", "thing.service.device.bindToken.report");
	cJSON_AddItemToObject(cmd, "property", property);

	cJSON_AddStringToObject(property, "bindToken", random);
	cJSON_AddStringToObject(cmd, "productKey", ya_aws_get_thing_type());
	cJSON_AddStringToObject(cmd, "deviceName", ya_aws_get_client_id());

	data = cJSON_PrintUnformatted(cmd);	

	if(data)
	{
		ya_report_msg_to_aws_cloud((uint8_t *)data, strlen(data));
		ya_hal_os_memory_free(data);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return C_OK;
}


int ya_aws_gen_ack_publish_and_send(AWS_IoT_Client* client, char *msgid)
{
	int32_t ret = -1;
	cJSON *aws_ack = NULL;
	char *buf = NULL;

	aws_ack = cJSON_CreateObject();

	if(!aws_ack)
		goto err;

	cJSON_AddStringToObject(aws_ack, "message", "success");
	cJSON_AddNumberToObject(aws_ack, "code", 0);

	buf = cJSON_PrintUnformatted(aws_ack);	
	
	ret = ya_aws_publish_ack(client, msgid, buf);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR,"ya_aws_publish_ack error\n");
		goto err;
	}

	if(aws_ack)
		cJSON_Delete(aws_ack);

	if(buf)
		ya_hal_os_memory_free(buf);

	return C_OK;
	
	err:
	if(aws_ack)
		cJSON_Delete(aws_ack);

	if(buf)
		ya_hal_os_memory_free(buf);

	return C_ERROR;
}


int ya_aws_cloud_connect(AWS_IoT_Client* client) 
{
	IoT_Error_t rc = FAILURE;

	IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
	IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

	if(ya_get_CA_cert() == NULL || ya_get_client_cert() == NULL || ya_get_private_key() == NULL)
	{
		ya_printf(C_LOG_ERROR, "cloud para error\n");
		return C_ERROR;
	}
	
	mqttInitParams.enableAutoReconnect = false; // We enable this later below
	
	if (aws_cloud_domain == WIFI_US)
		mqttInitParams.pHostURL = AWS_IOT_MQTT_HOST_US;
	else
		mqttInitParams.pHostURL = AWS_IOT_MQTT_HOST_EU;

	mqttInitParams.port = AWS_IOT_MQTT_PORT;
	mqttInitParams.pRootCALocation = ya_get_CA_cert();
	mqttInitParams.pDeviceCertLocation = ya_get_client_cert();
	mqttInitParams.pDevicePrivateKeyLocation = ya_get_private_key();
	mqttInitParams.mqttCommandTimeout_ms = 20000;
	mqttInitParams.tlsHandshakeTimeout_ms = 20000;
	mqttInitParams.isSSLHostnameVerify = true;
	mqttInitParams.disconnectHandler = ya_aws_disconnectCallbackHandler;
	mqttInitParams.disconnectHandlerData = NULL;

	rc = aws_iot_mqtt_init(client, &mqttInitParams);
	if(SUCCESS != rc) {
		ya_printf(C_LOG_ERROR, "aws_iot_mqtt_init returned error : %d\n", rc);
		return rc;
	}

	connectParams.keepAliveIntervalInSec = 60;  
	connectParams.isCleanSession = true;
	connectParams.MQTTVersion = MQTT_3_1_1;
	connectParams.pClientID = ya_get_client_id();
	connectParams.clientIDLen = (uint16_t) strlen(connectParams.pClientID);
	connectParams.isWillMsgPresent = false;

	IOT_INFO("Connecting...");
	rc = aws_iot_mqtt_connect(client, &connectParams);
	if(SUCCESS != rc) {
		ya_printf(C_LOG_ERROR, "Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
		return rc;
	}
	/*
	 * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
	 *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
	 *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
	 */
	rc = aws_iot_mqtt_autoreconnect_set_status(client, false);
	if(SUCCESS != rc) {
		ya_printf(C_LOG_ERROR, "Unable to set Auto Reconnect to false - %d\n", rc);
		return rc;
	}

	IOT_INFO("Subscribing...");
	rc = ya_aws_subscribe(client);
	if(SUCCESS != rc) {
		IOT_ERROR("Error subscribing : %d ", rc);
		return rc;
	}

	ya_printf(C_LOG_INFO, "subscribe success\n");
	return rc;
}


int32_t ya_aws_get_cloud_status(void)
{
	if(ya_cloud_connected == 1)
		return 1;

	return 0;
}

static void ya_cloud_app(void *arg)
{
	uint16_t count = 0;
	int32_t ret = -1;
	msg_t ms_msg;
	Ya_Timer ya_aws_cloud_timer;

	ya_get_subscribe_topic();
	ya_cloud_connected = 0;
	memset(&aws_client, 0, sizeof(AWS_IoT_Client));
	
	while(1)
	{
		if(ya_get_app_status() == YA_APP_CONNECTED && ya_aws_cloud_state == AWS_CLOUD_INIT)
		{
			ya_aws_cloud_state = AWS_TO_CONNECT_CLOUD;
		}else if(ya_get_app_status() != YA_APP_CONNECTED && ya_aws_cloud_state != AWS_CLOUD_INIT)
		{
			ret = aws_iot_mqtt_disconnect(&aws_client);
			if(ret != 0)
				ya_printf(C_LOG_ERROR,"aws_iot_mqtt_disconnect error: %d\n", ret);

			ret = aws_iot_mqtt_free(&aws_client);

			ya_printf_remain_heap();
			ya_cloud_connected = 0;
			ya_aws_cloud_state = AWS_CLOUD_INIT;
		}


		switch(ya_aws_cloud_state)
		{
			case AWS_CLOUD_INIT:
			break;

			case AWS_TO_CONNECT_CLOUD:
				memset(&aws_client, 0, sizeof(AWS_IoT_Client));
				ret = ya_aws_cloud_connect(&aws_client);
				if(ret != C_OK)
				{
					ya_aws_cloud_state = AWS_CONNECT_ERROR;
				}
				else
				{
					ya_cloud_connected = 1;
					ya_aws_cloud_state = AWS_CONNECTED_CLOUD;
					ya_send_cloud_status(YA_CLOUD_ONLINE);

					//update the device version
					if(ya_get_debind_enable())
					{
						ya_debind_disable();
						ya_updata_debind_information();
					}

					ya_updata_random_information();
					
					ya_updata_device_version();
					ya_printf_remain_heap();					
					ya_printf(C_LOG_INFO, "connect aws cloud success\n");

					#if 0
					if(flag == 0)
						ya_trigger_aws_cloud_event_listener(CLOUD_UPGRADE_EVENT, mcu_test_string, strlen(mcu_test_string));
					flag = 1;
					#endif
				}
			break;

			case AWS_CONNECTED_CLOUD:
				
			break;

			case AWS_CONNECT_ERROR:
				if(ya_cloud_connected)
				{
					ya_cloud_connected = 0;
					ya_send_cloud_status(YA_CLOUD_OFFLINE);
				}
				ya_init_timer(&ya_aws_cloud_timer);
				ya_countdown_ms(&ya_aws_cloud_timer, 5000);

				ret = aws_iot_mqtt_disconnect(&aws_client);
				if(ret != 0)
					ya_printf(C_LOG_ERROR,"aws_iot_mqtt_disconnect error\n");
				
				ret = aws_iot_mqtt_free(&aws_client);

				ya_printf_remain_heap();
				ya_aws_cloud_state = AWS_RECONNECT_WAIT;
			break;
			
			case AWS_RECONNECT_WAIT:
				if(ya_has_timer_expired(&ya_aws_cloud_timer) == C_OK)
				{
					ya_printf(C_LOG_INFO, "wait reconnect ok\n");
					ya_aws_cloud_state = AWS_TO_CONNECT_CLOUD;
				}
			break;

			default:

			break;
		}

		if(ya_aws_cloud_state == AWS_CONNECTED_CLOUD)
		{
			ret = aws_iot_mqtt_yield(&aws_client, 100);
			if(ret != 0) 
			{
				ya_printf(C_LOG_ERROR, "yield error: %d\n", ret);
				ya_aws_cloud_state = AWS_CONNECT_ERROR;
			}else
			{
				if(count >=  100)
				{
					ya_printf(C_LOG_INFO, "aws yielding\n");
					count = 0;
				}else
					count++;
			}
		}
		
		memset(&ms_msg, 0, sizeof(msg_t));
		ret = ya_hal_os_queue_recv(&ya_aws_cloud_msg_queue, &ms_msg, 20);

		if(ret == C_OK)
		{
			ya_printf(C_LOG_INFO, "[C-T]: %02x\n",ms_msg.type);
			ya_printf_remain_heap();

			if(ya_cloud_connected)
			{
				switch(ms_msg.type)
				{
					case YA_PUBLISH_RESPONSE:
						ret = ya_aws_gen_ack_publish_and_send(&aws_client, (char *)ms_msg.addr);
						if(ret != C_OK)
						{
							ya_printf(C_LOG_ERROR,"ya_aws_gen_ack_publish_and_send error\n");
						}
					break;

					case YA_HARDWARE_REPORT:
						ret = ya_aws_report(&aws_client, (char *)ms_msg.addr);
						if(ret != C_OK)
						{
							ya_printf(C_LOG_ERROR,"ya_aws_report error\n");
						}
					break;

					default:

					break;
				}
			}
			if(ms_msg.addr)
				ya_hal_os_memory_free(ms_msg.addr);
		}
	}

}

int32_t ya_start_cloud_aws_app()
{
	int32_t ret = 0;

	ret = ya_hal_os_queue_create(&ya_aws_cloud_msg_queue, "ya aws hardware queue", sizeof(msg_t), MSG_QUEUE_LENGTH);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create os queue error\n");
		return C_ERROR;
	}

	ret = ya_hal_os_thread_create(&ya_aws_app_cloud, "ya_aws_app_main_thread", ya_cloud_app, 0, (8*1024), 5);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create ya_aws_app_main error\n");
		return C_ERROR;
	}
		
	return C_OK;
}


cloud_context_t* ya_aws_cloud_create(MODULE_DOMAIN cloud_type)
{	
	aws_cloud_domain = cloud_type;

	cloud_context_t* cloud_conn = NULL;
	cloud_conn = (cloud_context_t *)ya_hal_os_memory_alloc(sizeof(cloud_context_t));

	if(cloud_conn)
	{
		cloud_conn->add_listener = &ya_add_aws_cloud_event_listener;
		cloud_conn->remove_listener = &ya_remove_aws_cloud_event_listener;
		cloud_conn->send_msg = &ya_report_msg_to_aws_cloud;
		cloud_conn->start_cloud = &ya_start_cloud_aws_app;
		cloud_conn->cloud_get_thing_name = &ya_aws_get_thing_type;
		cloud_conn->cloud_get_client_id = &ya_aws_get_client_id;
		cloud_conn->cloud_get_status  = &ya_aws_get_cloud_status;
	}
	
	return cloud_conn;
}

#endif

