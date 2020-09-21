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
#include "ya_api_thing_uer_define.h"
#include "ya_hardware_timer.h"

int32_t ya_thing_report_to_cloud(uint8_t *buf, uint16_t len)
{
	cJSON *cmd = NULL, *property = NULL;
	char *data = NULL;
	
	cmd = cJSON_CreateObject();
	if(!cmd)
		return C_ERROR;

	property = cJSON_Parse((char *)buf);
	if(!property)
	{
		if(cmd)
			cJSON_Delete(cmd);
		return C_ERROR;
	}

	cJSON_AddStringToObject(cmd, "method", "thing.service.device.property.report");
	cJSON_AddItemToObject(cmd, "property", property);

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

int32_t ya_thing_request_server_timer(void)
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
		ya_hal_os_memory_free(data);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return C_OK;
}

int32_t ya_cloud_to_thing_translate(uint8_t *buf, uint16_t len)
{
	cJSON *cmd = NULL, *property = NULL;
	char *data = NULL;

	cmd = cJSON_Parse((char *)buf);
	if(!cmd)
		goto err;

	property = cJSON_GetObjectItem(cmd, "property");
	if(!property)
		goto err;

	data = cJSON_PrintUnformatted(property); 

	if(data)
	{
		ya_thing_handle_downlink_data((uint8_t *)data, strlen(data));
		ya_hal_os_memory_free(data);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return C_OK;
	
	err:

	if(cmd)
		cJSON_Delete(cmd);

	return C_ERROR;
}


int32_t ya_handle_cloud_data(uint8_t *buf, uint16_t len)
{
	cJSON *cmd = NULL, *method = NULL, *property = NULL;
	char *data = NULL;

	cmd = cJSON_Parse((char *)buf);
	if(!cmd)
		goto err;

	method = cJSON_GetObjectItem(cmd, "method");
	if(!method)
		goto err;

	if(strcmp(method->valuestring, "thing.service.device.property.set") == 0)
	{
		ya_cloud_to_thing_translate(buf, len);
	}else if(strcmp(method->valuestring, "thing.service.device.severtime.set") == 0)
	{
		device_handle_severtime_set(buf, len);

	}else if(strcmp(method->valuestring, "thing.service.device.timer.set") == 0)
	{
		device_handle_cloud_timer(buf, len);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return C_OK;
	
	err:

	if(cmd)
		cJSON_Delete(cmd);

    return C_ERROR;
}


