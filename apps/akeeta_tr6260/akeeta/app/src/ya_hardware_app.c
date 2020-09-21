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
#include "ya_config.h"
#include "ya_hardware_timer.h"
#include "ya_hal_uart.h"

typedef enum
{
	YA_HARDWARE_HANDLE_CLOUD_DATA	   				= 0x00,
	YA_HARDWARE_HANDLE_TIMER_ACTION					= 0x01,
	YA_HARDWARE_HANDLE_UART_DATA					= 0x02,
	YA_HARDWARE_CLEAR_TIMER_FLASH 					= 0x03,
}ya_aws_hardware_queue_msg_type_t;

ya_hal_os_thread_t ya_app_hardware = NULL;
ya_hal_os_queue_t ya_hardware_msg_queue = NULL;

void ya_hardware_handle_cloud_onff_event(uint8_t *data, uint16_t data_len)
{
	if(data_len > 0)
	{
		if (data[0] == 2)
		{
			ya_printf(C_LOG_INFO,"ya hardware debind\n");
			ya_thing_handle_cloud_status(YA_CLOUD_DEBIND);
		}else if (data[0] == 1)
		{
			ya_printf(C_LOG_INFO,"ya hardware online\n");
			ya_thing_handle_cloud_status(YA_CLOUD_ONLINE);
			ya_thing_request_server_timer();
			//request the timer
		}else if (data[0] == 0)
		{
			ya_printf(C_LOG_INFO,"ya hardware offline\n");
			ya_thing_handle_cloud_status(YA_CLOUD_OFFLINE);
		}
	}
}

void ya_put_cloud_data_into_queue(uint8_t *data, uint16_t data_len)
{
	int ret = -1;
	msg_t ms_msg;
	uint8_t *buff = NULL;

	buff = (uint8_t *)ya_hal_os_memory_alloc(data_len + 1);
	if(!buff) return;
	
	memset( buff, 0, (data_len + 1));
	memcpy( buff, data, data_len);
	
	memset(&ms_msg, 0, sizeof(msg_t));
	ms_msg.addr = buff;
	ms_msg.len = data_len + 1;
	ms_msg.type = YA_HARDWARE_HANDLE_CLOUD_DATA;
	
	ret = ya_hal_os_queue_send(&ya_hardware_msg_queue, &ms_msg, 10);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR,"hardware queue error\n");
	
		if(ms_msg.addr)
			ya_hal_os_memory_free(ms_msg.addr);
	}
}

void ya_put_timer_action_into_queue(uint8_t *data, uint16_t data_len)
{
	int ret = -1;
	msg_t ms_msg;
	uint8_t *buff = NULL;

	buff = (uint8_t *)ya_hal_os_memory_alloc(data_len + 1);
	if(!buff) return;
	
	memset( buff, 0, (data_len + 1));
	memcpy( buff, data, data_len);
	
	memset(&ms_msg, 0, sizeof(msg_t));
	ms_msg.addr = buff;
	ms_msg.len = data_len + 1;
	ms_msg.type = YA_HARDWARE_HANDLE_TIMER_ACTION;

	printf("len: %d, timer action: %s\n", data_len, buff);
	
	ret = ya_hal_os_queue_send(&ya_hardware_msg_queue, &ms_msg, 10);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR,"hardware queue error\n");
	
		if(ms_msg.addr)
			ya_hal_os_memory_free(ms_msg.addr);
	}
	
}

void ya_put_data_uart_into_queue(uint8_t *data, uint16_t data_len)
{
	int ret = -1;
	msg_t ms_msg;
	uint8_t *buff = NULL;

	buff = (uint8_t *)ya_hal_os_memory_alloc(data_len + 1);
	if(!buff) return;
	
	memset( buff, 0, (data_len + 1));
	memcpy( buff, data, data_len);
	
	memset(&ms_msg, 0, sizeof(msg_t));
	ms_msg.addr = buff;
	ms_msg.len = data_len + 1;
	ms_msg.type = YA_HARDWARE_HANDLE_UART_DATA;
	
	ret = ya_hal_os_queue_send(&ya_hardware_msg_queue, &ms_msg, 10);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR,"hardware queue error\n");
	
		if(ms_msg.addr)
			ya_hal_os_memory_free(ms_msg.addr);
	}
	
}


void ya_clear_timer_flash_into_hardware_queue(void)
{
	int ret = -1;
	msg_t ms_msg;
	memset(&ms_msg, 0, sizeof(msg_t));
	ms_msg.type = YA_HARDWARE_CLEAR_TIMER_FLASH;
	ret = ya_hal_os_queue_send(&ya_hardware_msg_queue, &ms_msg, 10);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR,"hardware queue error\n");
	}	
}
void ya_hardware_app(void *arg)
{
	int32_t ret = -1;
	msg_t ms_msg;

	ya_hardware_timer_init();
	sys_abstime_init();
	sys_start_abstime();
	
	Ya_Timer ya_aws_hardware_timer_app;
	ya_init_timer(&ya_aws_hardware_timer_app);
	ya_countdown_ms(&ya_aws_hardware_timer_app, SEVER_TIME_UPDATE_PERIOD);

	while(1)
	{
		sys_run_abstime_1s();
		device_check_cloud_timer();
		
		if(ya_has_timer_expired(&ya_aws_hardware_timer_app) == 0)
		{
			ya_countdown_ms(&ya_aws_hardware_timer_app, SEVER_TIME_UPDATE_PERIOD);
			ya_thing_request_server_timer();
		}
		
		memset(&ms_msg, 0, sizeof(msg_t));
		ret = ya_hal_os_queue_recv(&ya_hardware_msg_queue, &ms_msg, 20);

		if(ret == C_OK)
		{
			ya_printf(C_LOG_INFO, "[H-S]: %02x\n", ms_msg.type);
			switch(ms_msg.type)
			{
				case YA_HARDWARE_HANDLE_CLOUD_DATA:
				{
					ret = ya_handle_cloud_data(ms_msg.addr, ms_msg.len);
					if(ret != C_OK)
						ya_printf(C_LOG_ERROR, "YA_HARDWARE_HANDLE_CLOUD_DATA error\n");
				}
				break;

				case YA_HARDWARE_HANDLE_TIMER_ACTION:
				{
					ya_thing_handle_downlink_data(ms_msg.addr, ms_msg.len);
				}
				break;

				case YA_HARDWARE_HANDLE_UART_DATA:
				{
					ya_hal_uart_write(ms_msg.addr, ms_msg.len);
				}
				break;
				case YA_HARDWARE_CLEAR_TIMER_FLASH:
				{
					ya_clear_timer_flash();
				}
				break;
				
				default:
					break;
			}
			
			if(ms_msg.addr)
				ya_hal_os_memory_free(ms_msg.addr);
		}
	}
}


void ya_add_hareware_listen_event_from_cloud()
{
	cloud_add_event_listener("ya_app_hardware thread", CLOUD_ONOFF_EVENT, ya_hardware_handle_cloud_onff_event);
	cloud_add_event_listener("ya_app_hardware thread", CLOUD_SET_EVENT, ya_put_cloud_data_into_queue);
}

int ya_start_hardware_app(void)
{
	int32_t ret = 0;

	ret = ya_hal_os_queue_create(&ya_hardware_msg_queue, "ya aws hardware queue", sizeof(msg_t), MSG_QUEUE_LENGTH);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create os queue error\n");
		return -1;
	}
	
	ret = ya_hal_os_thread_create(&ya_app_hardware, "ya_app_hardware thread", ya_hardware_app, 0, (1024 + 512), 5);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create ya_app_hardware error\n");
		return -1;
	}

	return 0;
}

