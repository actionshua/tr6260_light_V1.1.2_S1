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
#include "ya_aws_cloud.h"
#include "ya_aliyun_cloud.h"

cloud_context_t* cloud_conn = NULL;

int32_t cloud_add_event_listener(char *listener_name, cloud_event_type_t event_type, cloud_event_callback ya_cloud_event_handler)
{
	int32_t ret = -1;

	if (cloud_conn)
	{
		ret = cloud_conn->add_listener(listener_name, event_type, ya_cloud_event_handler);
	}

	return ret;
}

int32_t cloud_remove_event_listener(char *listener_name, cloud_event_type_t event_type)
{
	int32_t ret = -1;

	if (cloud_conn)
	{
		ret = cloud_conn->remove_listener(listener_name, event_type);
	}

	return ret;
}

int32_t cloud_report_msg(uint8_t *data, uint16_t data_len)
{
	int32_t ret = -1;
	if (cloud_conn)
	{
		ret = cloud_conn->send_msg(data, data_len);
	}
	
	return ret;
}

int32_t cloud_request_server_timer(void)
{
	cJSON *cmd = NULL;
	char *data = NULL;
	
	cmd = cJSON_CreateObject();
	if(!cmd)
		return C_ERROR;

	cJSON_AddStringToObject(cmd, "method", "thing.service.device.severtime.require");
	cJSON_AddStringToObject(cmd, "property", "time");

	data = cJSON_PrintUnformatted(cmd);	

	if(data)
	{
		cloud_report_msg((uint8_t *)data, strlen(data));
		free(data);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return C_OK;
}

int32_t ya_cloud_unregister(void)
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

	cJSON_AddStringToObject(cmd, "method", "thing.service.single.device.unregister");
	cJSON_AddItemToObject(cmd, "property", property);

	cJSON_AddStringToObject(property, "unregister", "1");
	cJSON_AddStringToObject(cmd, "productKey", ya_get_thing_type());
	cJSON_AddStringToObject(cmd, "deviceName", ya_get_client_id());

	data = cJSON_PrintUnformatted(cmd);	

	if(data)
	{
		cloud_report_msg((uint8_t *)data, strlen(data));
		ya_hal_os_memory_free(data);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return C_OK;
}



char *ya_get_client_id(void)
{
	if (cloud_conn)
	{
		return cloud_conn->cloud_get_client_id();
	}

	return NULL;
}

char *ya_get_thing_type(void)
{
	if (cloud_conn)
	{
		return cloud_conn->cloud_get_thing_name();
	}

	return NULL;
}

int32_t ya_get_cloud_status(void)
{
	if (cloud_conn)
	{
		return cloud_conn->cloud_get_status();
	}

	return 0;
}


int32_t cloud_create(MODULE_DOMAIN cloud_type)
{
	switch(cloud_type)
	{
		#if (CLOUD_SUPPORT == US_CN_CLOUD_SUPPORT)
		
		case WIFI_CN:
			cloud_conn = aliyun_cloud_create();
			break;

		case WIFI_US:
		case WIFI_EU:
			cloud_conn = ya_aws_cloud_create(cloud_type);
			break;

		#elif (CLOUD_SUPPORT == US_CLOUD_SUPPORT)

		case WIFI_US:
		case WIFI_EU:
			cloud_conn = ya_aws_cloud_create();
			break;
		#else

		case AKEETA_CN:
			cloud_conn = aliyun_cloud_create();
			break;	
		#endif
		
		default:
			break;
	}
	if (cloud_conn)
		return 0;
	else 
		return -1;
}

int32_t cloud_start(CLOUD_T cloud)
{
	int ret = -1;
	
	if (cloud_conn)
		ret = cloud_conn->start_cloud();

	return ret;
}



