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
#include "ya_hal_button.h"
#include "ya_example.h"

#include "ya_stripLights_app.h"
#include "ya_stripLights_example.h"

static int32_t scan_result[4];

uint8_t wait_to_restart = 0;
uint8_t ya_striplights_onoffline = 0;	
ya_router_status_t ya_router_status = MODULE_UNKNOWN;

extern int32_t ya_report_packet_formulate(char *value);

int32_t mcu_ota_process = 0;

uint32_t ya_mcu_ota_download_addr(void)
{
	return 0;
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

void ya_button_callback_handle(uint32_t button_push_ms)
{
	msg_t msg;
	ya_printf(C_LOG_INFO, "button time: %d\n", button_push_ms);
	memset(&msg, 0, sizeof(msg_t));

	if(button_push_ms >= (1000 * 5))
	{
		ya_stripLights_app_button_event_into_queue(CONFIG_TYPE);
	}else
	{
		ya_stripLights_app_button_event_into_queue(BUTTON_TYPE);
	}
}

void ya_thing_handle_downlink_data(uint8_t *buf, uint16_t len)
{
	ya_stripLights_app_cloud_event_into_queue(buf, len);
	return;
}

void ya_thing_handle_cloud_status(ya_cloud_status_t ya_cloud_status)
{
	if (ya_cloud_status == YA_CLOUD_ONLINE)
	{
		ya_striplights_onoffline = 1;
		ya_stripLights_cloud_attriReport();
		ya_clear_updata_stripLights();

	}else if (ya_cloud_status == YA_CLOUD_DEBIND)
	{
		ya_stripLights_app_sceneClearAll();
		ya_reset_module_to_factory();
		wait_to_restart = 1;
	}else if (ya_cloud_status == YA_CLOUD_OFFLINE)
	{
		ya_striplights_onoffline = 0;
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

	ya_router_status = ya_app_router_status;
}

void ya_handle_server_timer_callback(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, uint8_t week)
{
	ya_printf(C_LOG_INFO, "[timer]: y: %d, m: %d, d: %d, h: %d, m: %d, s: %d, w: %d\n", year, month, day, hour, minute, second, week);
}

void ya_striplight_init(void)
{
	ya_button_init();
	ya_set_button_callback(BUTTON1, ya_button_callback_handle, 5000);
}

ya_app_main_para_t test_striplight_main_para;

int32_t ya_check_license_exit()
{
	if (test_striplight_main_para.cloud_type == AKEETA_OVERSEA)
	{
		if (ya_check_us_license_exit()==0)
			return 0;		
	}else if (test_striplight_main_para.cloud_type == AKEETA_CN)
	{
		if (ya_check_cn_license_exit()==0)
			return 0;	
	}else if (test_striplight_main_para.cloud_type == UNKNOWN_CLOUD_TYPE)
	{
		if (ya_check_cn_license_exit()==0 || ya_check_us_license_exit()==0)
			return 0;
	}

	return -1;
}

static void ya_start_akeeta_sdk(void)
{
	memset(&test_striplight_main_para, 0, sizeof(ya_app_main_para_t));
	test_striplight_main_para.cloud_type = UNKNOWN_CLOUD_TYPE;
	test_striplight_main_para.sniffer_timeout = 180;
	test_striplight_main_para.ap_timeout = 180;
	test_striplight_main_para.ya_init_mode = SNIFFER_MODE;
	test_striplight_main_para.mcu_ota_enable = 0;
	test_striplight_main_para.enable_factory_uart = 0;
	test_striplight_main_para.enable_factory_router_scan = 1;
	test_striplight_main_para.enable_low_power = 0;
	test_striplight_main_para.enable_debind = 1;
	strcpy(test_striplight_main_para.cur_version, TEST_VERSION);
	strcpy(test_striplight_main_para.mcu_version, MCU_VERSION);
	test_striplight_main_para.server_timer_callback = &ya_handle_server_timer_callback;
	
	ya_start_app_main(test_striplight_main_para);
}

void ya_stripLights_app_task(void *arg)
{
	bool blink_flag = 0, set_init_flag = 0;
	msg_t ms_msg;
	int32_t ret = -1;
	uint32_t  time_blink = 0;
	ya_router_status_t striplight_router_status = MODULE_UNKNOWN;
	Ya_Timer ya_hardware_timer_blink;

	ya_striplight_init();
	ya_start_akeeta_sdk();

	while(1)
	{	
		if( striplight_router_status != ya_router_status)
		{
			striplight_router_status = ya_router_status;

			if (striplight_router_status != MODLUE_FACTORY_TEST)
			{
				if (striplight_router_status == MODULE_SNIFFER || striplight_router_status == MODULE_AP)
				{
					ya_stripLights_app_netConfigDisplay();
					blink_flag = 1;
					//set to idle when reboot again 
					ya_set_idle_next_reboot();

					if (striplight_router_status == MODULE_AP)
						time_blink = 2000;
					else
						time_blink = 500;
					
				}else if (set_init_flag == 0)
				{
					set_init_flag = 1;
					time_blink = 0;
					ya_striplights_app_init_mode();
				}	
			}else
			{
				//check license exit
				if (scan_result[1] > -60)
				{
					ret = ya_check_license_exit();
					if (ret == 0)
					{
						ya_printf(C_LOG_INFO, "license exit\n");
						time_blink = 0;
						ya_stripLights_app_factory_test();
					} else
					{
						time_blink = 500;
					}
				} else
				{
					time_blink = 2000;
				}
			}

			if(time_blink > 0)
			{
				ya_init_timer(&ya_hardware_timer_blink);
				ya_countdown_ms(&ya_hardware_timer_blink, time_blink);
			}

		}

		if(time_blink)
		{		
			if(ya_has_timer_expired(&ya_hardware_timer_blink) == C_OK)
			{
				ya_countdown_ms(&ya_hardware_timer_blink, time_blink);
				if(blink_flag)
				{
					ya_stripLights_switchoff();
					blink_flag = 0;
				} else
				{
					if (striplight_router_status != MODLUE_FACTORY_TEST)
						ya_stripLights_app_netConfigDisplay();
					else
						ya_stripLights_app_factory_test();

					blink_flag = 1;
				}
			}		
		}

		if (wait_to_restart)
		{
			ya_printf(C_LOG_INFO,"wait to restart\n");
			ya_wait_to_restart();
		}
		
		memset(&ms_msg, 0, sizeof(msg_t));
		ret = ya_hal_os_queue_recv(&ya_striplight_queue, &ms_msg, 10);

		if(ret == C_OK)
		{
			ya_printf(C_LOG_INFO,"get striplights packet: %d\n", ms_msg.type);

			if (ms_msg.type != CONFIG_TYPE)
			{
				if (striplight_router_status != MODULE_UNKNOWN && striplight_router_status != MODULE_SNIFFER && striplight_router_status != MODULE_AP)
					ya_stripLights_app_infraredHandle(ms_msg.type, ms_msg.subtype, ms_msg.addr, ms_msg.len);
			} else 
			{
				ya_stripLights_app_sceneClearAll();
				ya_set_toggle_mode(0);
				wait_to_restart = 1;
			}
			
			if (ms_msg.addr)
				ya_hal_os_memory_free(ms_msg.addr);
		}

		if (ya_striplights_onoffline)
		{
			if (ya_get_updata_stripLights())
			{
				ya_stripLights_cloud_attriReport();
				ya_clear_updata_stripLights();
			}
		}

	}	
}


void ya_stripLights_example(void)
{
	int ret = -1;

	ret = ya_hal_os_queue_create(&ya_striplight_queue, "ya_striplight_queue queue", sizeof(msg_t), 16);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create os queue error\n");
		return -1;
	}

	ret = ya_hal_os_thread_create(NULL, "ya_stripLights_app_task", ya_stripLights_app_task, 0, (2*1024), 3);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create ya_stripLights_app_task error\n");
	}
}

