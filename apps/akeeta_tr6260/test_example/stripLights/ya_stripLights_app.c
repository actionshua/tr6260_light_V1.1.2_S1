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
#include "ya_stripLights_app.h"
#include "ya_config.h"
#include "ya_common_func.h"


#define SCENE_MAX_LEN	48

typedef struct
{
	bool 	 switchstate;		
	uint8_t  work_mode;					
	uint8_t  white_temp;			
	uint8_t  white_bright;			
	uint8_t  color[3];
	uint8_t  scene[SCENE_MAX_LEN + 1];
	uint8_t  music;
	uint8_t  mic_sensitivity;
	uint8_t  mic_rate;
} st_stripLigths_thingModel; 


uint8_t ya_update_stripLights_data_to_cloud_enable = 0;
ya_hal_os_queue_t ya_striplight_queue = NULL;

st_stripLigths_thingModel ya_stripLights_thing_mode;

int32_t ya_stripLights_app_sceneClearAll(void)
{
	int32_t ret = 0;
	//clear app config data
	return ret;
}
bool ya_get_updata_stripLights(void)
{
	return ya_update_stripLights_data_to_cloud_enable;
}	


void ya_clear_updata_stripLights(void)
{
	ya_update_stripLights_data_to_cloud_enable = 0;
}

void ya_stripLights_app_button_event_into_queue(uint8_t msg_event)
{
	msg_t msg;
	memset(&msg, 0, sizeof(msg_t)); 		
	msg.type = msg_event;
	ya_hal_os_queue_send(&ya_striplight_queue, &msg, 0);
}

void ya_stripLights_app_cloud_event_into_queue(uint8_t *buf, uint16_t len)
{
	int32_t ret = -1;
	msg_t msg;
	memset(&msg, 0, sizeof(msg_t)); 		
	msg.type = CLOUD_TYPE;

	msg.addr = (uint8_t *)ya_hal_os_memory_alloc(len);
	memcpy(msg.addr, buf, len);
	msg.len = len;

	ret = ya_hal_os_queue_send(&ya_striplight_queue, &msg, 0);
	if (ret != C_OK)
	{
		if (msg.addr)
			ya_hal_os_memory_free(msg.addr);
	}
}

void ya_stripLights_cloud_attriReport(void)
{
	char temp_string[100];
	uint32_t hsb_value = 0;
	int32_t ret = -1;
	cJSON *property = NULL;
	char *buf = NULL;
	
	property = cJSON_CreateObject();
	if(!property)
		return -1;

	cJSON_AddStringToObject(property, "switch", ya_int_to_string(ya_stripLights_thing_mode.switchstate));

	if (ya_stripLights_thing_mode.switchstate)
	{
		cJSON_AddStringToObject(property, "work_mode", ya_int_to_string(ya_stripLights_thing_mode.work_mode));
		switch (ya_stripLights_thing_mode.work_mode)
		{
			case WORKMODE_WHITE:
				cJSON_AddStringToObject(property, "white_temp", ya_int_to_string(ya_stripLights_thing_mode.white_temp));
				cJSON_AddStringToObject(property, "white_bright", ya_int_to_string(ya_stripLights_thing_mode.white_bright));
				break;

			case WORKMODE_COLOR:
				memset(temp_string, 0, 100);
				HexArrayToString(&ya_stripLights_thing_mode.color[0], 3, temp_string);
				cJSON_AddStringToObject(property, "color", temp_string);
				break;			

			case WORKMODE_SCENE:
				memset(temp_string, 0, 100);
				HexArrayToString(ya_stripLights_thing_mode.scene+1, ya_stripLights_thing_mode.scene[0], temp_string);
				cJSON_AddStringToObject(property, "scene", temp_string);
				break;

			case WORKMODE_MUSIC:
				cJSON_AddStringToObject(property, "music", ya_int_to_string(ya_stripLights_thing_mode.music));
				cJSON_AddStringToObject(property, "mic_sensitivity", ya_int_to_string(ya_stripLights_thing_mode.mic_sensitivity));
				cJSON_AddStringToObject(property, "mic_rate", ya_int_to_string(ya_stripLights_thing_mode.mic_rate));
				break;

			default:
				break;
		}

	}

	buf = cJSON_PrintUnformatted(property);	
	if(buf)
	{
		ret = ya_thing_report_to_cloud((uint8_t *)buf, strlen(buf));
		if(ret != 0)
			ya_printf(C_LOG_ERROR, "ya_thing_to_cloud_translate error\n");

		ya_hal_os_memory_free(buf);
	}

	if(property)
		cJSON_Delete(property);

	return 0;

}

void ya_stripLights_app_cloudScenehandle(uint8_t *buf, uint16_t len)
{
	uint8_t index = 0, scene_para_get = 0;
	uint8_t scene_data[SCENE_MAX_LEN + 1];
	
	uint32_t value = 0;
	uint8_t save_flag = 1, tmp_value = 0;

	cJSON *root = NULL, *json_key = NULL;

	memset(scene_data, 0, SCENE_MAX_LEN + 1);

	root = cJSON_Parse(buf);
	if (!root) return;

	json_key = cJSON_GetObjectItem(root, "switch");
	if (json_key && json_key->type == cJSON_String)
	{
		value = atoi(json_key->valuestring);
		if (value > 0)
			ya_stripLights_thing_mode.switchstate = 1;
		else
			ya_stripLights_thing_mode.switchstate = 0;
	} else
	{
		json_key = cJSON_GetObjectItem(root, "work_mode");
		if (json_key && json_key->type == cJSON_String)
		{
			ya_stripLights_thing_mode.switchstate = 1;
			value = atoi(json_key->valuestring);
			if (value < WORKMODE_MAX)
				ya_stripLights_thing_mode.work_mode = atoi(json_key->valuestring);
		} 

		json_key = cJSON_GetObjectItem(root, "white_temp");
		if (json_key && json_key->type == cJSON_String)
		{
			value = atoi(json_key->valuestring);
			if (value <= 100)
			{
				ya_stripLights_thing_mode.white_temp = atoi(json_key->valuestring);
				ya_stripLights_thing_mode.work_mode = WORKMODE_WHITE;
				ya_stripLights_thing_mode.switchstate = 1;
			}
		} 		

		json_key = cJSON_GetObjectItem(root, "white_bright");
		if (json_key && json_key->type == cJSON_String)
		{
			value = atoi(json_key->valuestring);
			if (value <= 100)
			{
				ya_stripLights_thing_mode.white_bright = atoi(json_key->valuestring);
				ya_stripLights_thing_mode.work_mode = WORKMODE_WHITE;
				ya_stripLights_thing_mode.switchstate = 1;
			}
		} 	

		json_key = cJSON_GetObjectItem(root, "color");
		if (json_key && json_key->type == cJSON_String && strlen(json_key->valuestring) == 6)
		{
			ya_stripLights_thing_mode.switchstate = 1;
			StringToHexArray(&ya_stripLights_thing_mode.color[0], 3, json_key->valuestring);
			ya_stripLights_thing_mode.work_mode = WORKMODE_COLOR;
		} 

		json_key = cJSON_GetObjectItem(root, "scene");
		if (json_key && json_key->type == cJSON_String && strlen(json_key->valuestring) <= (SCENE_MAX_LEN*2) && strlen(json_key->valuestring) >= 4)
		{
			scene_para_get = 1;
			scene_data[0] = StringToHexArray(scene_data + 1, SCENE_MAX_LEN, json_key->valuestring);
			ya_stripLights_thing_mode.work_mode = WORKMODE_SCENE;
			ya_stripLights_thing_mode.switchstate = 1;
		}

		json_key = cJSON_GetObjectItem(root, "music");
		if (json_key && json_key->type == cJSON_String)
		{
			value = atoi(json_key->valuestring);
			if (value < MUSICMODE_MAX)
			{
				ya_stripLights_thing_mode.switchstate = 1;
				ya_stripLights_thing_mode.music = atoi(json_key->valuestring);
				ya_stripLights_thing_mode.work_mode = WORKMODE_MUSIC;
			}
		}

		json_key = cJSON_GetObjectItem(root, "mic_sensitivity");
		if (json_key && json_key->type == cJSON_String)
		{
			value = atoi(json_key->valuestring);
			if (value <= 100)
			{
				ya_stripLights_thing_mode.switchstate = 1;
				ya_stripLights_thing_mode.mic_sensitivity = atoi(json_key->valuestring);
				ya_stripLights_thing_mode.work_mode = WORKMODE_MUSIC;
			}
		}

		json_key = cJSON_GetObjectItem(root, "mic_rate");
		if (json_key && json_key->type == cJSON_String)
		{
			value = atoi(json_key->valuestring);
			if (value <= 100)
			{
				ya_stripLights_thing_mode.switchstate = 1;
				ya_stripLights_thing_mode.mic_rate = atoi(json_key->valuestring);
				ya_stripLights_thing_mode.work_mode = WORKMODE_MUSIC;
			}
		}
	}
	
	ya_printf(C_LOG_INFO,"s: %d, w: %d, l_w: %d, l_b: %d, l_h: %d, l_s: %d, l_c: %d, l_m: %d, l_m_s: %d, l_m_r %d\r\n",
	ya_stripLights_thing_mode.switchstate,
	ya_stripLights_thing_mode.work_mode,
	ya_stripLights_thing_mode.white_temp,
	ya_stripLights_thing_mode.white_bright,
	ya_stripLights_thing_mode.color[0],
	ya_stripLights_thing_mode.color[1],
	ya_stripLights_thing_mode.color[2], 
	ya_stripLights_thing_mode.music,
	ya_stripLights_thing_mode.mic_sensitivity,
	ya_stripLights_thing_mode.mic_rate);

	#if 0
	if (scene_para_get == 0 && ya_stripLights_thing_mode.work_mode == WORKMODE_SCENE)
	{
		memcpy(scene_data, ya_stripLights_thing_mode.scene, (SCENE_MAX_LEN + 1));
	}
	#else
	if (scene_para_get == 1 && ya_stripLights_thing_mode.work_mode == WORKMODE_SCENE)
	{
		memcpy(ya_stripLights_thing_mode.scene, scene_data,(SCENE_MAX_LEN + 1));
	}
	#endif

	for (index = 1; index < (scene_data[0] + 1); index++)
		ya_printf(C_LOG_INFO, " %02x", scene_data[index]);

	ya_printf(C_LOG_INFO, "\r\n");

	if (root) cJSON_Delete(root);
	
	//report cloud data 
	ya_update_stripLights_data_to_cloud_enable = 1;
}

void  ya_stripLights_app_infraredHandle(uint8_t type, uint8_t subtype, uint8_t *buf, uint16_t len)
{	
	switch(type)
	{
		case IR_TYPE:
		break;

		case BUTTON_TYPE:
		break;

		case CLOUD_TYPE:
			ya_stripLights_app_cloudScenehandle(buf, len);
		break;

		default:

		break;
	}
}

void ya_striplights_app_init_mode()
{
	//init app config data
}

void ya_stripLights_app_netConfigDisplay(void)
{
	//open netConfig Indicator light
}

void ya_stripLights_app_factory_test(void)
{
	//open factory Indicator light
}

void ya_stripLights_switchoff(void)
{
	//close Indicator light
}