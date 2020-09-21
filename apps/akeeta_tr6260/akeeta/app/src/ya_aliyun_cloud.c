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

#if (CLOUD_SUPPORT == US_CN_CLOUD_SUPPORT || CLOUD_SUPPORT == CN_CLOUD_SUPPORT)

#include "infra_defs.h"
#include "mqtt_api.h"

uint8_t send_connect_flag = 0;

typedef enum
{
	ALY_UNCONNECTED_CLOUD	   				= 0x00,
	ALY_CONNECTED_CLOUD,
}ya_aly_cloud_state_t;

typedef enum
{
	YA_HARDWARE_REPORT	   				= 0x00,
}ya_cloud_queue_msg_type_t;

typedef struct ya_cloud_event_callbackhandler{
	char *task_name;
	cloud_event_type_t event_type;
	cloud_event_callback	 ya_cloud_event_handler;

	struct ya_cloud_event_callbackhandler *ya_cloud_event_callbackhandler_next;
}ya_cloud_event_callbackhandler_t;

ya_cloud_event_callbackhandler_t *ya_cloud_callback_list = NULL;

static char product_key[IOTX_PRODUCT_KEY_LEN + 1]={0};
static char device_name[IOTX_DEVICE_NAME_LEN + 1]={0};

ya_hal_os_thread_t ya_app_cloud = NULL;
ya_hal_os_queue_t ya_cloud_msg_queue = NULL;

uint8_t ya_cloud_connected = ALY_UNCONNECTED_CLOUD;

int32_t ya_add_aly_cloud_event_listener(char *task_name, cloud_event_type_t event_type, cloud_event_callback ya_cloud_event_chandler)
{
	ya_cloud_event_callbackhandler_t *list_node = NULL, *add_node = NULL;

	if(!task_name)
		return C_ERROR;

	list_node = ya_cloud_callback_list;

	while(list_node)
	{
		if(list_node->task_name)
		{
			if(strcmp(list_node->task_name, task_name) == 0 && list_node->event_type == event_type)
				return C_OK;
		}

		if(!(list_node->ya_cloud_event_callbackhandler_next))
			break;
		else
			list_node = list_node->ya_cloud_event_callbackhandler_next;
	}

	add_node = (ya_cloud_event_callbackhandler_t *)HAL_Malloc(sizeof(ya_cloud_event_callbackhandler_t));
	if(!add_node)
		return C_ERROR;

	add_node->task_name = (char *)HAL_Malloc(strlen(task_name) + 1);
	if(!add_node->task_name)
	{
		if(add_node)
			ya_hal_os_memory_free(add_node);
		
		return C_ERROR;
	}

	strcpy(add_node->task_name,  task_name);
	add_node->event_type = event_type;
	add_node->ya_cloud_event_handler = ya_cloud_event_chandler;

	add_node->ya_cloud_event_callbackhandler_next = NULL;

	if(list_node)
		list_node->ya_cloud_event_callbackhandler_next = add_node;
	else
		ya_cloud_callback_list = add_node;

	ya_printf(C_LOG_INFO,"ya_add_cloud_event_listener: %s\n", task_name);
	return C_OK;
}

int32_t ya_remove_aly_cloud_event_listener(char *task_name, cloud_event_type_t event_type)
{
	ya_cloud_event_callbackhandler_t *list_node = NULL, *pre_node = NULL;

	if(!task_name)
		return C_ERROR;

	list_node = ya_cloud_callback_list;
	pre_node = list_node;
		
	while(list_node)
	{
		if(list_node->task_name)
		{
			if(strcmp(list_node->task_name, task_name) == 0 && list_node->event_type == event_type)
				break;
		}

		if(!(list_node->ya_cloud_event_callbackhandler_next))
			return C_ERROR;
		else
		{
			pre_node = list_node;
			list_node = list_node->ya_cloud_event_callbackhandler_next;
		}
	}

	if(list_node)
		return C_ERROR;

	if(list_node == ya_cloud_callback_list && ya_cloud_callback_list->ya_cloud_event_callbackhandler_next == NULL)
		ya_cloud_callback_list = NULL;
	else
	{
		if(list_node == ya_cloud_callback_list)
			ya_cloud_callback_list = list_node->ya_cloud_event_callbackhandler_next;
		else
			pre_node->ya_cloud_event_callbackhandler_next = list_node->ya_cloud_event_callbackhandler_next;
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

int32_t ya_trigger_aly_cloud_event_listener(uint8_t event_type, uint8_t *data, uint16_t data_len)
{ 
	ya_cloud_event_callbackhandler_t *list_node = NULL;
	list_node = ya_cloud_callback_list;
		
	while(list_node)
	{
		if(list_node->task_name)
		{
			if(list_node->event_type == event_type)
				list_node->ya_cloud_event_handler(data, data_len);
		}

		if(!(list_node->ya_cloud_event_callbackhandler_next))
			return C_OK;
		else
		{
			list_node = list_node->ya_cloud_event_callbackhandler_next;
		}
	}

	return C_OK;
}

int32_t ya_send_cloud_status(uint8_t cloud_status)
{	
	uint8_t buf[1];
	
	buf[0] = cloud_status;
	return ya_trigger_aly_cloud_event_listener(CLOUD_ONOFF_EVENT, buf, 1);
}

int32_t ya_aly_subscribe_event_transfer(char const *buf, uint16_t data_len)
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
		ya_trigger_aly_cloud_event_listener(CLOUD_SET_EVENT, (uint8_t *)buf, data_len);
	}else if(strstr(method->valuestring, "upgrade"))
	{
		ya_trigger_aly_cloud_event_listener(CLOUD_UPGRADE_EVENT, (uint8_t *)buf, data_len);
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


#define MSG_ID_LEN_MAX      (64)
#define PAYLOAD_LEN_MAX 1024*4
#define TOPIC_LEN_MAX 256
#define TOPIC_RRPC_RESPONSE_FMT                   "/sys/%s/%s/rrpc/response/%s"

static int ya_aly_rrpc_response(void *pclient,const char* message_id,const char* response)
{
	int             res = 0;
	char *topic;
	
	int topic_len = strlen(TOPIC_RRPC_RESPONSE_FMT) + strlen(product_key) + strlen(device_name) + strlen(message_id) + 1;

	topic = (char *)HAL_Malloc(topic_len);
	if(topic == NULL){
		ya_printf(C_LOG_ERROR," memory alloc fail ...\n");
		return -1;
	}

    HAL_Snprintf(topic,topic_len,TOPIC_RRPC_RESPONSE_FMT,product_key,device_name,message_id);

	ya_printf(C_LOG_INFO,"rrpc response topic:[%s]",topic);
	ya_printf(C_LOG_INFO,"rrpc response payload:[%s]",response);
	
    res = IOT_MQTT_Publish_Simple(pclient, topic, IOTX_MQTT_QOS0, (void *)response, strlen(response));
	HAL_Free(topic);
    if (res < 0) {
        ya_printf(C_LOG_INFO,"publish failed, res = %d", res);
        return -1;
    }
    return 0;
}

void ya_subscribe_callback_handler(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    iotx_mqtt_topic_info_t     *topic_info = (iotx_mqtt_topic_info_pt) msg->msg;
	int rc = 0;
	//ya_printf(C_LOG_INFO,"msg->event_type:%u\n",msg->event_type);
    switch (msg->event_type) {
        case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
            /* print topic name and topic message */
            ya_printf(C_LOG_INFO,"cloud packet: %.*s\n", topic_info->payload_len, topic_info->payload);
			rc = ya_aly_subscribe_event_transfer(topic_info->payload,topic_info->payload_len);
            break;
        default:
            break;
    }

	if(strstr(topic_info->ptopic,"/rrpc/request/")){
#define TOPIC_RRPC_REQ       "/sys///rrpc/request/"
		int topic_len = 0;
		char resp[128]={0};
		char msg_id[MSG_ID_LEN_MAX + 1]={0};
		int msg_id_len;
		
		topic_len = strlen(TOPIC_RRPC_REQ) + strlen(product_key) + strlen(device_name);
		msg_id_len = topic_info->topic_len - topic_len;
		//assert(msg_id_len < MSG_ID_LEN_MAX);
		
		HAL_Snprintf(msg_id,msg_id_len+1, "%s",topic_info->ptopic + topic_len);
		HAL_Snprintf(resp,sizeof(resp),"{\"code\":%d,\"message\":\"null\"}",rc);
		ya_aly_rrpc_response(pclient,msg_id,resp);
	}
}

int32_t ya_report_msg_to_aly_cloud(uint8_t *data, uint16_t data_len)
{
	int32_t ret = -1;
	uint8_t *buf = NULL;
	msg_t ms_msg;

	if(ya_cloud_connected == ALY_UNCONNECTED_CLOUD){
		ya_printf(C_LOG_ERROR,"send message error: Not connected to cloud\r\n[%s]\n",(char *)data);
		return -1;
	}
	buf = (uint8_t *)HAL_Malloc(data_len + 1);
	memset( buf, 0, (data_len + 1));
	memcpy( buf, data, data_len);
	
	memset(&ms_msg, 0, sizeof(msg_t));
	ms_msg.addr = buf;
	ms_msg.len = (data_len + 1);
	ms_msg.type = YA_HARDWARE_REPORT;
	
	ret = ya_hal_os_queue_send(&ya_cloud_msg_queue, &ms_msg, 10);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR,"cloud queue error\n");
	
		if(ms_msg.addr)
			ya_hal_os_memory_free(ms_msg.addr);
	
		return -1;
	}
	
	return 0;
}

int32_t ya_ali_get_cloud_status(void)
{
	if(ya_cloud_connected == ALY_CONNECTED_CLOUD)
		return 1;

	return 0;
}

char *ya_aly_get_client_id(void)
{
	if(strlen(device_name)==0){
		HAL_GetDeviceName(device_name);
	}

	if (strlen(device_name)==0)
		return NULL;
	
	return device_name;
}

char *ya_aly_get_thing_name(void)
{	
	if (strlen(product_key)==0){
		HAL_GetProductKey(product_key);
	}

	if (strlen(product_key)==0)
		return NULL;
	
	return product_key;
}


#define TOPIC_UPDATE_FMT "/%s/%s/update"
//#define REPORT_UPDATE_FMT "{ \"method\":\"thing.service.device.property.report\",\"property\":{\"%s\":\"%s\"}}"
int ya_aly_report(void* pclient, char *buf)
{
    int             res = 0;
	char *topic;
	int topic_len = 0;

	topic_len = strlen(TOPIC_UPDATE_FMT) + strlen(product_key) + strlen(device_name) + 1;
	topic = (char *)HAL_Malloc(topic_len);
	if(topic == NULL){
		ya_printf(C_LOG_ERROR," memory alloc fail ...\n");
		return -1;
	}

    HAL_Snprintf(topic,TOPIC_LEN_MAX,TOPIC_UPDATE_FMT,product_key,device_name);

	//HAL_Printf("topic:[%s]\n",topic);
	//HAL_Printf("payload:[%s]\n",buf);
	
    res = IOT_MQTT_Publish_Simple(pclient, topic, IOTX_MQTT_QOS0, buf, strlen(buf));
	HAL_Free(topic);
    if (res < 0) {
        ya_printf(C_LOG_ERROR,"publish failed, res = %d", res);		
        return -1;
    };
    return 0;
}

int32_t ya_updata_device_version(void)
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
		ya_report_msg_to_aly_cloud((uint8_t *)data, strlen(data));
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
	cJSON_AddStringToObject(cmd, "productKey", ya_aly_get_thing_name());
	cJSON_AddStringToObject(cmd, "deviceName", ya_aly_get_client_id());

	data = cJSON_PrintUnformatted(cmd);	

	if(data)
	{
		ya_report_msg_to_aly_cloud((uint8_t *)data, strlen(data));
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
	cJSON_AddStringToObject(cmd, "productKey", ya_aly_get_thing_name());
	cJSON_AddStringToObject(cmd, "deviceName", ya_aly_get_client_id());

	data = cJSON_PrintUnformatted(cmd);	

	if(data)
	{
		ya_report_msg_to_aly_cloud((uint8_t *)data, strlen(data));
		ya_hal_os_memory_free(data);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return C_OK;
}


void ya_aly_cloud_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    ya_printf(C_LOG_INFO,"msg->event_type : %d\n", msg->event_type);

	switch(msg->event_type){
		case IOTX_MQTT_EVENT_RECONNECT: 
		case IOTX_MQTT_EVENT_SUBCRIBE_SUCCESS:
			if (ya_cloud_connected != ALY_CONNECTED_CLOUD)
			{
				ya_cloud_connected = ALY_CONNECTED_CLOUD;
				ya_send_cloud_status(YA_CLOUD_ONLINE);

				send_connect_flag = 1;
			}
			ya_printf(C_LOG_INFO, "Successfully connected to cloud\r\n");
			break;
		case  IOTX_MQTT_EVENT_DISCONNECT:
			if (ya_cloud_connected != ALY_UNCONNECTED_CLOUD)
			{
				ya_cloud_connected = ALY_UNCONNECTED_CLOUD;
				ya_send_cloud_status(YA_CLOUD_OFFLINE);
			}
	}

}

static int ya_aly_topic_subscribe(void *handle,const char *fmt){
    int res = 0;
	char *topic;
	int topic_len = 0;

	topic_len = strlen(fmt) + strlen(product_key) + strlen(device_name) + 1;
	topic = (char *)HAL_Malloc(topic_len);
	if(topic == NULL){
		ya_printf(C_LOG_ERROR," memory alloc fail ...\n");
		return -1;
	}
    memset(topic, 0, topic_len);
    HAL_Snprintf(topic, topic_len, fmt, product_key, device_name);
	
	ya_printf(C_LOG_INFO,"subscribe topic [%s]\r\n",topic);
	
    res = IOT_MQTT_Subscribe(handle, topic, IOTX_MQTT_QOS0, ya_subscribe_callback_handler, NULL);
    if (res < 0) {
        ya_printf(C_LOG_ERROR,"subscribe failed\n");
		HAL_Free(topic);
        return -1;
    }
	HAL_Free(topic);
    return 0;
}

static int ya_aly_subscribe(void *handle)
{
	ya_aly_topic_subscribe(handle,"/ext/error/%s/%s");
	ya_aly_topic_subscribe(handle,"/sys/%s/%s/rrpc/request/+");
	ya_aly_topic_subscribe(handle,"/%s/%s/dispatch/instruction/execution");
    return 0;
}

static void *ya_aly_cloud_start(void)
{
	int ret = -1;
	void *pclient = NULL;
	iotx_mqtt_param_t    mqtt_params;
	
	memset(&mqtt_params, 0, sizeof(mqtt_params));
	mqtt_params.handle_event.h_fp = ya_aly_cloud_event_handle;

    pclient = IOT_MQTT_Construct(&mqtt_params);
    if (NULL == pclient) 
	{	
        ya_printf(C_LOG_ERROR,"MQTT construct failed\r\n");
        return NULL;
    }
	
	ret = ya_aly_subscribe(pclient);
    if (ret < 0)
	{
		ya_printf(C_LOG_ERROR,"subscribe failed, ret = %d", ret);
        IOT_MQTT_Destroy(&pclient);
        return NULL;
    }

	return pclient; 
}
static void ya_cloud_loop(void *pclient){
	uint16_t count = 0;
	int32_t ret = -1;
	msg_t ms_msg;

	if(!pclient)
		return;

	while(1)
	{
		if(send_connect_flag)
		{
			//update the device version
			if(ya_get_debind_enable())
			{
				ya_debind_disable();
				ya_updata_debind_information();
			}

			ya_updata_random_information();
			ya_updata_device_version();
			send_connect_flag = 0;
		}
	
		IOT_MQTT_Yield(pclient, 20);
		
		memset(&ms_msg, 0, sizeof(msg_t));
		ret = ya_hal_os_queue_recv(&ya_cloud_msg_queue, &ms_msg, 50);
	
		if(ret == C_OK)
		{
			ya_printf(C_LOG_INFO, "[C-T]: %02x (%d)%s\n",ms_msg.type,ms_msg.len,(char *)ms_msg.addr);
	
			if(ya_cloud_connected)
			{
				switch(ms_msg.type)
				{
					case YA_HARDWARE_REPORT:
						ret = ya_aly_report(pclient, (char *)ms_msg.addr);
						if(ret != C_OK)
						{
							ya_printf(C_LOG_ERROR,"ya_aly_report error\n");
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

static void ya_cloud_app(void *arg)
{

	void *pclient = NULL;

	while(1){

		if (ya_get_app_status() !=	YA_APP_CONNECTED)
		{
			HAL_SleepMs(200);
			continue;
		}
		
		pclient = ya_aly_cloud_start();
		//ya_printf_remain_heap();
		ya_cloud_loop(pclient);
		HAL_SleepMs(1000);
	}

}

int32_t ya_start_cloud_aly_app()
{
	int32_t ret = 0;
	
	HAL_GetDeviceName(device_name);
	HAL_GetProductKey(product_key);

	ret = ya_hal_os_queue_create(&ya_cloud_msg_queue, "ya aly hardware queue", sizeof(msg_t), MSG_QUEUE_LENGTH);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create os queue error\n");
		return C_ERROR;
	}

	ret = ya_hal_os_thread_create(&ya_app_cloud, "ya_aly_app_main_thread", ya_cloud_app, 0, (6*1024), 5);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create ya_aly_app_main error\n");
		return C_ERROR;
	}
		
	return C_OK;
}


cloud_context_t* aliyun_cloud_create(void)
{
	cloud_context_t* cloud_conn = NULL;
	cloud_conn = (cloud_context_t *)ya_hal_os_memory_alloc(sizeof(cloud_context_t));

	if(cloud_conn)
	{
		cloud_conn->add_listener = &ya_add_aly_cloud_event_listener;
		cloud_conn->remove_listener = &ya_remove_aly_cloud_event_listener;
		cloud_conn->send_msg = &ya_report_msg_to_aly_cloud;
		cloud_conn->start_cloud = &ya_start_cloud_aly_app;
		cloud_conn->cloud_get_thing_name = &ya_aly_get_thing_name;
		cloud_conn->cloud_get_client_id = &ya_aly_get_client_id;
		cloud_conn->cloud_get_status  = &ya_ali_get_cloud_status;
	}
	return cloud_conn;
}

#endif

