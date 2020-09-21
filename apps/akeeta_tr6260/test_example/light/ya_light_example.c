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
#include "ya_app_main.h"
#include "ya_thing_data_api.h"
#include "ya_mode_api.h"
#include "ya_api_thing_uer_define.h"
#include "ya_light_io.h"
#include "ya_mcu_ota_user_define.h"
#include "ya_light_product_test.h"
#include "ya_light_example.h"
#include "ya_common_func.h"
#include "ya_flash.h"
#include "ya_mcu_ota_api.h"
#include "ya_hal_pwm.h"
#include "drv_gpio.h"
#include "soc_pin_mux.h"

ya_hal_os_queue_t ya_light_queue = NULL;
ya_save_light_data_t  ya_save_light_data;

typedef enum{
	W_CONTROL = 0,
	C_CONTROL,
	S_CONTROL,
}ya_light_mode;

typedef enum{
	L_READ = 0,
	L_WORK,
	L_NIGHT,
	L_IDLE,
}ya_light_scene_mode;

typedef enum{
	NON_CONTROL = 0,
	WHITE_CONTROL,
	COLOR_CONTROL,
	SCENE_CONTROL,
}ya_light_control_mode;

#define MAX_W_COLOR    6500 //K
#define MIN_W_COLOR	   2000 //K

#define DEFAULT_W_COLOR	3000  //K

#define DEFAULT_W_COLOR_SCALE		(((DEFAULT_W_COLOR - MIN_W_COLOR) * 100)/(MAX_W_COLOR - MIN_W_COLOR))

#define DEFAULT_W_COLOR_READ_SCALE   (((4500- MIN_W_COLOR) * 100)/(MAX_W_COLOR - MIN_W_COLOR))
#define DEFAULT_W_COLOR_WORK_SCALE   (((3000- MIN_W_COLOR) * 100)/(MAX_W_COLOR - MIN_W_COLOR))
#define DEFAULT_W_COLOR_NIGHT_SCALE   (((6000- MIN_W_COLOR) * 100)/(MAX_W_COLOR - MIN_W_COLOR))
#define DEFAULT_W_COLOR_IDLE_SCALE   (((3500- MIN_W_COLOR) * 100)/(MAX_W_COLOR - MIN_W_COLOR))

ya_app_main_para_t test_light_main_para;

ya_cloud_light_data_t ya_cloud_light_data_new;
ya_cloud_light_data_t ya_cloud_light_data_default;
int32_t mcu_ota_process = 0;
uint8_t wait_to_restart = 0;

static int32_t scan_result[4];

uint32_t ya_mcu_ota_download_addr(void)
{
	return 0x1A0000;
}

void ya_wait_to_restart()
{
	while(1)
	{
		ya_delay(2000);
	}
}


void ya_mcu_download_finish(uint32_t ota_size, char *new_version)
{
	ya_printf(C_LOG_INFO, "ya_mcu_download_finish: %d, %s\n", ota_size, new_version);
	return;
}

void ya_cloud_light_set_to_config(ya_cloud_light_data_t *ya_cloud_light_data_config)
{
	memcpy(ya_cloud_light_data_config, &ya_cloud_light_data_default, sizeof(ya_cloud_light_data_t));
}

void ya_cloud_light_set_to_idle(ya_cloud_light_data_t *ya_cloud_light_data_config)
{
	ya_cloud_light_data_config->mode = W_CONTROL;
	ya_cloud_light_data_config->onoff = 0;
	ya_cloud_light_data_config->w_l = 0;
	ya_cloud_light_data_config->w_color = 0;
	ya_cloud_light_data_config->c_h = 0;
	ya_cloud_light_data_config->c_s = 0;
	ya_cloud_light_data_config->c_b = 0;
	ya_cloud_light_data_config->scene = 0;
}

void ya_cloud_light_set_to_mode(ya_cloud_light_data_t *ya_cloud_light_data_config, ya_light_scene_mode mode)
{
	switch(mode)
	{
		case L_READ:
			ya_cloud_light_data_config->onoff = 1;
			ya_cloud_light_data_config->mode = W_CONTROL;
			ya_cloud_light_data_config->w_l = 75;
			ya_cloud_light_data_config->w_color = DEFAULT_W_COLOR_READ_SCALE;
			ya_cloud_light_data_config->c_h = 0;
			ya_cloud_light_data_config->c_s = 0;
			ya_cloud_light_data_config->c_b = 0;
			ya_cloud_light_data_config->scene = L_READ;
			break;

		case L_WORK:
			ya_cloud_light_data_config->onoff = 1;
			ya_cloud_light_data_config->mode = W_CONTROL;
			ya_cloud_light_data_config->w_l = 80;
			ya_cloud_light_data_config->w_color = DEFAULT_W_COLOR_WORK_SCALE;
			ya_cloud_light_data_config->c_h = 0;
			ya_cloud_light_data_config->c_s = 0;
			ya_cloud_light_data_config->c_b = 0;
			ya_cloud_light_data_config->scene = L_WORK;
			break;		

		case L_NIGHT:
			ya_cloud_light_data_config->onoff = 1;
			ya_cloud_light_data_config->mode = W_CONTROL;
			ya_cloud_light_data_config->w_l = 5;
			ya_cloud_light_data_config->w_color = DEFAULT_W_COLOR_NIGHT_SCALE;
			ya_cloud_light_data_config->c_h = 0;
			ya_cloud_light_data_config->c_s = 0;
			ya_cloud_light_data_config->c_b = 0;
			ya_cloud_light_data_config->scene = L_NIGHT;
			break;	

		case L_IDLE:
			ya_cloud_light_data_config->onoff = 1;
			ya_cloud_light_data_config->mode = W_CONTROL;
			ya_cloud_light_data_config->w_l = 50;
			ya_cloud_light_data_config->w_color = DEFAULT_W_COLOR_IDLE_SCALE;
			ya_cloud_light_data_config->c_h = 0;
			ya_cloud_light_data_config->c_s = 0;
			ya_cloud_light_data_config->c_b = 0;
			ya_cloud_light_data_config->scene = L_IDLE;
			break;

		default:
			break;
	}
}

int32_t ya_report_whole_key_value_formulate(void)
{
	int32_t ret = -1;
	cJSON *property = NULL;
	char *buf = NULL;
	
	property = cJSON_CreateObject();
	if(!property)
		return -1;
	
	cJSON_AddStringToObject(property, "Switch", ya_int_to_string(ya_cloud_light_data_new.onoff));

	if (ya_cloud_light_data_new.onoff == 1)
	{
		cJSON_AddStringToObject(property, "WorkMode",  ya_int_to_string(ya_cloud_light_data_new.mode));
		cJSON_AddStringToObject(property, "Luminance", ya_int_to_string(ya_cloud_light_data_new.w_l));
		cJSON_AddStringToObject(property, "ColorTemperature",  ya_int_to_string(ya_cloud_light_data_new.w_color));
		cJSON_AddStringToObject(property, "Hue",  ya_int_to_string(ya_cloud_light_data_new.c_h));
		cJSON_AddStringToObject(property, "Saturation",  ya_int_to_string(ya_cloud_light_data_new.c_s));
		cJSON_AddStringToObject(property, "Brightness",  ya_int_to_string(ya_cloud_light_data_new.c_b));
		cJSON_AddStringToObject(property, "LightScene",  ya_int_to_string(ya_cloud_light_data_new.scene));
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

void ya_set_light_para_to_queue(ya_cloud_light_data_t *ya_cloud_ligth_para)
{
	uint8_t *buf = NULL;
	msg_t ms_msg;

	buf = (ya_cloud_light_data_t *)ya_hal_os_memory_alloc(sizeof(ya_cloud_light_data_t));
	if (!buf) return;

	memset(buf, 0, sizeof(ya_light_data_t));
	memcpy(buf, ya_cloud_ligth_para, sizeof(ya_cloud_light_data_t));
	memset(&ms_msg, 0, sizeof(msg_t));
	ms_msg.addr = (uint8_t *)buf;
	ms_msg.len = sizeof(ya_light_data_t);

	if(ya_hal_os_queue_send(&ya_light_queue, &ms_msg, 10) != C_OK)
	{
		ya_printf(C_LOG_ERROR,"light queue error\n");
	
		if(ms_msg.addr)
			ya_hal_os_memory_free(ms_msg.addr);
	
		return;
	}
}

void ya_light_control(ya_cloud_light_data_t *ya_ligth_cloud)
{
	ya_light_data_t ya_ligth_para;

	ya_ligth_para.c_h = ya_ligth_cloud->c_h;
	ya_ligth_para.c_b = ya_ligth_cloud->c_b;
	ya_ligth_para.c_s = ya_ligth_cloud->c_s;
	ya_ligth_para.w_color = ya_ligth_cloud->w_color;
	ya_ligth_para.w_l = ya_ligth_cloud->w_l;

	if(ya_ligth_cloud->onoff == 1)
	{
		if(ya_ligth_cloud->mode == W_CONTROL || ya_ligth_cloud->mode == S_CONTROL)
		{
			ya_ligth_para.c_b = 0;
			ya_ligth_para.c_h = 0;
			ya_ligth_para.c_s = 0;

			if(ya_ligth_cloud->w_l < 10)
				ya_ligth_para.w_l = 10;
		}else
		{
			ya_ligth_para.w_l = 0;
			ya_ligth_para.w_color = 0;
		}
	}else
	{
		ya_ligth_para.c_b = 0;
		ya_ligth_para.c_h = 0;
		ya_ligth_para.c_s = 0;
		ya_ligth_para.w_l = 0;
		ya_ligth_para.w_color = 0;
	}

	ya_write_light_para(ya_ligth_para);
}


void ya_thing_handle_downlink_data(uint8_t *buf, uint16_t len)
{
	uint8_t enable_send = 0, get_switch_off = 0;
	ya_light_control_mode control_type = NON_CONTROL;
	cJSON *property = NULL, *json_key = NULL;
	uint32_t value = 0;

	property = cJSON_Parse((char *)buf);
	if(!property)
		goto err;

	json_key = cJSON_GetObjectItem(property, "Switch");
	if(json_key)
	{
		if(json_key->type !=cJSON_String)
			goto err;

		value = atoi(json_key->valuestring);
		if(value != 0)	
		{
			ya_cloud_light_data_new.onoff = 1;
		}
		else
		{
			get_switch_off = 1;
			ya_cloud_light_data_new.onoff = 0;
		}
		enable_send = 1;
	}

	if(!get_switch_off)
	{
		json_key = cJSON_GetObjectItem(property, "WorkMode");
		if(json_key)
		{
			if(json_key->type !=cJSON_String)
				goto err;
		
			value = atoi(json_key->valuestring);
			ya_cloud_light_data_new.mode = value;		
			enable_send = 1;
			control_type = ya_cloud_light_data_new.mode + 1;
		}

		if(control_type == WHITE_CONTROL || control_type == NON_CONTROL)
		{
			json_key = cJSON_GetObjectItem(property, "Luminance");
			if(json_key)
			{
				if(json_key->type !=cJSON_String)
					goto err;

				value = atoi(json_key->valuestring);

				ya_cloud_light_data_new.w_l = value;
				ya_cloud_light_data_new.mode = W_CONTROL;
				ya_cloud_light_data_new.onoff = 1;

				enable_send = 1;
				control_type = WHITE_CONTROL;
			}

			json_key = cJSON_GetObjectItem(property, "ColorTemperature");
			if(json_key)
			{
				if(json_key->type !=cJSON_String)
					goto err;
			
				value = atoi(json_key->valuestring);
				ya_cloud_light_data_new.w_color = value;
				ya_cloud_light_data_new.mode = W_CONTROL;
				ya_cloud_light_data_new.onoff = 1;

				enable_send = 1;
				control_type = WHITE_CONTROL;
			}

		}

		if(control_type == COLOR_CONTROL || control_type == NON_CONTROL)
		{
			json_key = cJSON_GetObjectItem(property, "Hue");
			if(json_key)
			{
				if(json_key->type !=cJSON_String)
					goto err;

				value = atoi(json_key->valuestring);
				ya_cloud_light_data_new.c_h = value;
				ya_cloud_light_data_new.onoff = 1;
				ya_cloud_light_data_new.mode = C_CONTROL;

				enable_send = 1;
				control_type = COLOR_CONTROL;
			}

			json_key = cJSON_GetObjectItem(property, "Saturation");
			if(json_key)
			{
				if(json_key->type !=cJSON_String)
					goto err;

				value = atoi(json_key->valuestring);
				ya_cloud_light_data_new.c_s = value;
				ya_cloud_light_data_new.onoff = 1;
				ya_cloud_light_data_new.mode = C_CONTROL;

				enable_send = 1;
				control_type = COLOR_CONTROL;
			}

			json_key = cJSON_GetObjectItem(property, "Brightness");
			if(json_key)
			{
				if(json_key->type !=cJSON_String)
					goto err;

				value = atoi(json_key->valuestring);
				ya_cloud_light_data_new.c_b = value;
				ya_cloud_light_data_new.onoff = 1;
				ya_cloud_light_data_new.mode = C_CONTROL;

				enable_send = 1;
				control_type = COLOR_CONTROL;
			}
		}

		if(control_type == SCENE_CONTROL || control_type == NON_CONTROL)
		{

			json_key = cJSON_GetObjectItem(property, "LightScene");
			if(json_key)
			{
				if(json_key->type !=cJSON_String)
					goto err;

				value = atoi(json_key->valuestring);
				ya_cloud_light_data_new.scene = value;
				ya_cloud_light_data_new.mode = S_CONTROL;
				ya_cloud_light_data_new.onoff = 1;

				enable_send = 1;
				control_type = SCENE_CONTROL;
			}
		}
	}

	if(enable_send)
	{
		ya_set_light_para_to_queue(&ya_cloud_light_data_new);		
		ya_report_whole_key_value_formulate();
	}

	if(property)
		cJSON_Delete(property);

	return;
	
	err:

	if(property)
		cJSON_Delete(property);

	return;
}

uint8_t ya_has_been_connect_cloud = 0;
static ya_router_status_t cur_status = MODULE_UNKNOWN;

void ya_thing_handle_cloud_status(ya_cloud_status_t ya_cloud_status)
{
	if (ya_cloud_status == YA_CLOUD_ONLINE)
	{
		ya_report_whole_key_value_formulate();
		ya_has_been_connect_cloud = 1;
	}else if (ya_cloud_status == YA_CLOUD_DEBIND)
	{
		ya_save_light_data.config_default = 0;
		ya_write_flash(YA_LIGHT_FLASH_PARA_ADDR, (uint8_t *)&ya_save_light_data, sizeof(ya_save_light_data_t),1);

		wait_to_restart = 1;
		ya_reset_module_to_factory();
	}
}

void ya_thing_handle_router_status_callback(ya_router_status_t ya_app_router_status, void *data)
{
	int32_t *p = (int32_t *)data;
	if (ya_app_router_status == MODLUE_FACTORY_TEST)
	{
		scan_result[0] = p[0];
		scan_result[1] = p[1];
		scan_result[2] = p[2];
		scan_result[3] = p[3];		
		ya_printf(C_LOG_INFO, "[scan]: %d, %d, %d, %d\n", scan_result[0], scan_result[1], scan_result[2], scan_result[3]);
	}

	cur_status = ya_app_router_status;
}

void ya_light_read_power_on_flag(void)
{
	int32_t ret = -1;

	memset(&ya_save_light_data, 0, sizeof(ya_save_light_data_t));
	ret = ya_read_flash_with_fix_len(YA_LIGHT_FLASH_PARA_ADDR, (uint8_t *)&ya_save_light_data, sizeof(ya_save_light_data_t), 1);

	if (ret != 0)
	{
		memset(&ya_save_light_data, 0, sizeof(ya_save_light_data_t));
	}

	ya_printf(C_LOG_INFO, "light data1: %d, %d\n", ya_save_light_data.num, ya_save_light_data.config_default);
	ya_printf(C_LOG_INFO, "light data2: %d, %d, %d, %d, %d, %d\n", ya_save_light_data.ya_cloud_light.mode, ya_save_light_data.ya_cloud_light.c_h,  ya_save_light_data.ya_cloud_light.c_s,ya_save_light_data.ya_cloud_light.c_b, ya_save_light_data.ya_cloud_light.w_l, ya_save_light_data.ya_cloud_light.w_color);

	ya_save_light_data.num++;

	if (ya_save_light_data.num == 4)
	{
		ya_set_toggle_mode(1);
	}
	
	if (ya_save_light_data.num >= 4)
	{
		ya_save_light_data.config_default = 0;
	}
	
	if (ya_save_light_data.config_default)
	{
		memcpy(&ya_cloud_light_data_default, &(ya_save_light_data.ya_cloud_light), sizeof(ya_cloud_light_data_t));
	}else
	{
		ya_cloud_light_data_default.mode = W_CONTROL;
		ya_cloud_light_data_default.onoff = 1;
		ya_cloud_light_data_default.w_l = 50;
		ya_cloud_light_data_default.w_color = 50;
		ya_cloud_light_data_default.c_h = 0;
		ya_cloud_light_data_default.c_s = 100;
		ya_cloud_light_data_default.c_b = 100;
		ya_cloud_light_data_default.scene = 0;
	}

	ya_cloud_light_set_to_config(&ya_cloud_light_data_new);
	ya_set_light_para_to_queue(&ya_cloud_light_data_new);
	
	ya_write_flash(YA_LIGHT_FLASH_PARA_ADDR, (uint8_t *)&ya_save_light_data, sizeof(ya_save_light_data_t),1);
}

void ya_clear_power_on_flag_callback(TimerHandle_t xTimer)
{
	ya_save_light_data.num = 0;
	ya_write_flash(YA_LIGHT_FLASH_PARA_ADDR, (uint8_t *)&ya_save_light_data, sizeof(ya_save_light_data_t),1);
}

void ya_init_light_io(void)
{
	TimerHandle_t xTimer;
	memset(&ya_cloud_light_data_new, 0, sizeof(ya_cloud_light_data_t));
	ya_light_read_power_on_flag();
	
	xTimer = xTimerCreate("power_on_clear_flag", ya_hal_os_msec_to_ticks(5000), pdFALSE, (void * )0, ya_clear_power_on_flag_callback);
	if( xTimer != NULL )
	{
		xTimerStart(xTimer, 0);
	}
	
	ya_light_control_pwm_para_init();
	ya_hal_pwm_init();
}


void ya_handle_server_timer_callback(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, uint8_t week)
{
	ya_printf(C_LOG_INFO, "[timer]: y: %d, m: %d, d: %d, h: %d, m: %d, s: %d, w: %d\n", year, month, day, hour, minute, second, week);
}

static void ya_start_akeeta_sdk(void)
{
	memset(&test_light_main_para, 0, sizeof(ya_app_main_para_t));

#if (LIGHT_CLOUD == 0)
		test_light_main_para.cloud_type = AKEETA_CN;
#elif (LIGHT_CLOUD == 1)
		test_light_main_para.cloud_type = AKEETA_OVERSEA;
#else
		test_light_main_para.cloud_type = UNKNOWN_CLOUD_TYPE;
#endif

	test_light_main_para.sniffer_timeout = 300;
	test_light_main_para.ap_timeout = 300;
	test_light_main_para.ya_init_mode = SNIFFER_MODE;
	test_light_main_para.mcu_ota_enable = 0;
	test_light_main_para.enable_factory_uart = 0;
	test_light_main_para.enable_factory_router_scan = 1;
	test_light_main_para.enable_low_power = 0;
	test_light_main_para.enable_debind = 1;
	strcpy(test_light_main_para.cur_version, TEST_WIFI_LIGHT_VERSION);
	strcpy(test_light_main_para.mcu_version, MCU_LIGHT_VERSION);
	test_light_main_para.server_timer_callback = &ya_handle_server_timer_callback;
	
	ya_start_app_main(&test_light_main_para);

}

void ya_light_task(void *arg)
{
	ya_cloud_light_data_t ya_cloud_light_data_scene;
	uint8_t control_flag = 0;
	uint32_t  time_blink = 0;
	int ret = -1;
	msg_t msg_light;
	ya_router_status_t light_router_status = MODULE_UNKNOWN;
	Ya_Timer ya_hardware_timer_blink;	
	ya_cloud_light_data_t ya_light;

	ya_init_light_io();
	ya_start_akeeta_sdk();
	
	while(1)
	{
		if(light_router_status != cur_status)
		{
			light_router_status = cur_status;

			if (light_router_status != MODLUE_FACTORY_TEST)
			{
				if (light_router_status == MODULE_SNIFFER)
				{
					time_blink = 500;
				}else if(light_router_status == MODULE_AP)
				{
					time_blink = 2000;
				}else if(light_router_status == MODULE_CONNECTING || light_router_status == MODULE_IDLE ||light_router_status == MODLUE_CONNECT)
				{
					time_blink = 0;
					if(ya_has_been_connect_cloud == 0)
					{
						ya_cloud_light_set_to_config(&ya_cloud_light_data_new);
						ya_set_light_para_to_queue(&ya_cloud_light_data_new);
					}
				}
				
				if (time_blink != 0)
				{
					ya_init_timer(&ya_hardware_timer_blink);
					ya_countdown_ms(&ya_hardware_timer_blink, time_blink);
				
					ya_cloud_light_set_to_config(&ya_cloud_light_data_new);
					ya_set_light_para_to_queue(&ya_cloud_light_data_new);
				}
			}else
			{
					if (scan_result[0] == 1)
					{
						//go to test1
						ya_printf(C_LOG_INFO, "light test 1\n");	

						#if (LIGHT_CLOUD == 0)
						ya_light_factory_test(scan_result[1], 0, AKEETA_CN);
						#elif (LIGHT_CLOUD == 1)
						ya_light_factory_test(scan_result[1], 0, AKEETA_OVERSEA);
						#else
						ya_light_factory_test(scan_result[1], 0, UNKNOWN_CLOUD_TYPE);
						#endif
					} else if(scan_result[2] == 1)
					{
						ya_printf(C_LOG_INFO, "light test 2\n");	
						#if (LIGHT_CLOUD == 0)
							ya_light_factory_test(scan_result[3], 1, AKEETA_CN);
						#elif (LIGHT_CLOUD == 1)
							ya_light_factory_test(scan_result[3], 1, AKEETA_OVERSEA);
						#else
							ya_light_factory_test(scan_result[3], 1, UNKNOWN_CLOUD_TYPE);
						#endif
					}
			}
		}

		if(time_blink)
		{		
			if(ya_has_timer_expired(&ya_hardware_timer_blink) == C_OK)
			{
				ya_countdown_ms(&ya_hardware_timer_blink, time_blink);
				if(ya_cloud_light_data_new.onoff)
					ya_cloud_light_set_to_idle(&ya_cloud_light_data_new);
				else
					ya_cloud_light_set_to_config(&ya_cloud_light_data_new);
				
				ya_set_light_para_to_queue(&ya_cloud_light_data_new);
			}	
		}

		if (wait_to_restart)
		{
			ya_printf(C_LOG_INFO,"wait to restart\n");
			ya_wait_to_restart();
		}

		control_flag = 0;

		do{
			memset(&msg_light, 0, sizeof(msg_t));
		
			ret = ya_hal_os_queue_recv(&ya_light_queue, &msg_light, 0);
			if (ret == C_OK)
			{	
				memcpy(&ya_light, msg_light.addr, sizeof(ya_cloud_light_data_t));
				control_flag = 1;
			}

			if (msg_light.addr)
				ya_hal_os_memory_free(msg_light.addr);
			
		}while(ret == C_OK);

		if (control_flag)
		{
			if (ya_has_been_connect_cloud)
			{
				if (ya_light.onoff != 0)
				{
					ya_save_light_data.config_default = 1;
					memcpy(&(ya_save_light_data.ya_cloud_light), &ya_light, sizeof(ya_cloud_light_data_t));					
					ya_write_flash(YA_LIGHT_FLASH_PARA_ADDR, (uint8_t *)&ya_save_light_data, sizeof(ya_save_light_data_t),1);
				}
			}

			if (ya_light.mode != S_CONTROL || ya_light.onoff == 0)
				ya_light_control(&ya_light);
			else
			{
				ya_cloud_light_set_to_mode(&ya_cloud_light_data_scene, ya_light.scene);
				ya_light_control(&ya_cloud_light_data_scene);
			}			
		}
		ya_delay(50);

	}	
}



void ya_light_example(void)
{
	int ret = -1;

	ret = ya_hal_os_queue_create(&ya_light_queue, "ya light queue", sizeof(msg_t), MSG_QUEUE_LENGTH);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create os queue error\n");
	}

	ret = ya_hal_os_thread_create(NULL, "ya_light_task", ya_light_task, 0, (4*1024), 3);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create ya_light_task error\n");
	}
}

