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
#include "ya_flash.h"
#include "ya_config.h"
#include "ya_hardware_timer_hal.h"

#define MAX_TIMER_DEVICE_DATA		(4096*2 - 256)	
#define MAX_ACTION_LEN				256
#define MAX_TIMER_NUM				20
#define MAX_TIMER_ID_LEN			16

typedef struct
{
	uint16_t year;
	uint8_t  month;
	uint8_t  day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t week;
}sys_time_t;

typedef struct device_common_timer{
	uint16_t year;
	uint8_t  month;
	uint8_t  day;
	uint8_t hour;
	uint8_t minute;
	
	uint8_t week;
	
	uint8_t enable;
	char	timer_id[MAX_TIMER_ID_LEN];

	uint8_t second;
	uint8_t triggered;
	uint8_t reserv[6];
	
	uint16_t action_len;
	char *action;
	
	struct device_common_timer *device_common_timer_next;
}device_common_timer_t;

device_common_timer_t *device_common_timer_header = NULL;
sys_time_t sys_time;

void sys_timer_printf(void)
{
	ya_printf(C_LOG_INFO, "y: %d, m: %d, d: %d, w: %d, h: %d, m: %d\n", sys_time.year, sys_time.month, sys_time.day, sys_time.week, sys_time.hour, sys_time.minute);
}

int32_t device_delete_all_timer()
{
	device_common_timer_t *cur_node = NULL;
	cur_node = device_common_timer_header;

	while(cur_node)
	{
		device_common_timer_header = cur_node->device_common_timer_next;

		if(cur_node->action)
			ya_hal_os_memory_free(cur_node->action);

		ya_hal_os_memory_free(cur_node);

		cur_node = device_common_timer_header;
	}
	return 0;
}

int32_t device_delete_list_timer(char *timer_id)
{
	device_common_timer_t *cur_node = NULL, *pre_node = NULL;
	cur_node = device_common_timer_header;
	pre_node = device_common_timer_header;

	while(cur_node)
	{
		if(memcmp(cur_node->timer_id, timer_id, MAX_TIMER_ID_LEN) == 0)
		{
			ya_printf(C_LOG_INFO, "find obj: %s\n", timer_id);
			break;
		}
		pre_node = cur_node;
		cur_node = cur_node->device_common_timer_next;
	}

	if(cur_node)
	{
		if(cur_node == device_common_timer_header && device_common_timer_header->device_common_timer_next == NULL)
		{
			ya_printf(C_LOG_INFO,"O-node\r\n");
			device_common_timer_header = NULL;
		}else
		{
			if(cur_node == device_common_timer_header)
			{
				ya_printf(C_LOG_INFO,"H-node\r\n");
				device_common_timer_header = cur_node->device_common_timer_next;
			}else
			{
				ya_printf(C_LOG_INFO,"M-node\r\n");
				pre_node->device_common_timer_next = cur_node->device_common_timer_next;
			}
		}
		
		if(cur_node->action)
		{
			ya_hal_os_memory_free(cur_node->action);
			cur_node->action = NULL;
		}
		
		ya_hal_os_memory_free(cur_node);
	}else
	{
		return -1;
	}

	return 0;
}


int32_t device_add_list_timer(device_common_timer_t *device_common_timer_node)
{
	uint8_t index_timer = 0;
	device_common_timer_t *cur_node = NULL, *add_node = NULL;
	cur_node = device_common_timer_header;

	while(cur_node)
	{
		index_timer++;
		if(memcmp(cur_node->timer_id, device_common_timer_node->timer_id, MAX_TIMER_ID_LEN) == 0)
		{
			ya_printf(C_LOG_INFO, "has been exit\n");
			return 0;
		}
		
		if(!(cur_node->device_common_timer_next))
			break;
		else
			cur_node = cur_node->device_common_timer_next;
	}

	if(index_timer > MAX_TIMER_NUM)
	{
		ya_printf(C_LOG_ERROR, "timer num is max\n");
		return -1;
	}

	ya_printf(C_LOG_INFO, "timer num: %d\n", index_timer);
	
	add_node = (device_common_timer_t *)ya_hal_os_memory_alloc(sizeof(device_common_timer_t));
	if(!add_node)
		return -1;

	memset(add_node, 0, sizeof(device_common_timer_t));

	add_node->action = (char *)ya_hal_os_memory_alloc(device_common_timer_node->action_len + 1);
	if(!add_node->action)
	{
		if(add_node)
			ya_hal_os_memory_free(add_node);
		
		return -1;
	}

	memset(add_node->action, 0, sizeof(device_common_timer_node->action_len + 1));
	strcpy(add_node->action,  device_common_timer_node->action);
	add_node->action_len = (device_common_timer_node->action_len + 1);

	add_node->year = device_common_timer_node->year;
	add_node->month = device_common_timer_node->month;
	add_node->day = device_common_timer_node->day;
	add_node->hour = device_common_timer_node->hour;
	add_node->minute = device_common_timer_node->minute;
	add_node->week = device_common_timer_node->week;
	add_node->second = device_common_timer_node->second;
	add_node->triggered = 0;

	add_node->enable = device_common_timer_node->enable;
	memcpy(add_node->timer_id, device_common_timer_node->timer_id, MAX_TIMER_ID_LEN);
	
	add_node->device_common_timer_next = NULL;
	
	if(cur_node == NULL)
	{
		device_common_timer_header = add_node;
	}else
	{
		cur_node->device_common_timer_next = add_node;
	}
	return 0;
}

int32_t device_save_timer_into_flash(void)
{
	int ret = -1;
	device_common_timer_t *cur_node = NULL;
	uint32_t pos = 0;

	uint8_t *buf = NULL; 
	buf = (uint8_t *)ya_hal_os_memory_alloc(MAX_TIMER_DEVICE_DATA);
	if(!buf) goto err;
	
	memset(buf, 0, MAX_TIMER_DEVICE_DATA);
	cur_node = device_common_timer_header;
	while(cur_node)
	{
		if(pos + 8 + MAX_TIMER_ID_LEN + 8 + 2 + cur_node->action_len> MAX_TIMER_DEVICE_DATA)
			goto err;

		buf[pos++] = (uint8_t)((cur_node->year & 0xFF00) >> 8);
		buf[pos++] = (uint8_t)(cur_node->year & 0x00FF);
		buf[pos++] = cur_node->month;
		buf[pos++] = cur_node->day;
		buf[pos++] = cur_node->hour;
		buf[pos++] = cur_node->minute;
		buf[pos++] = cur_node->week;

		buf[pos++] = cur_node->enable;
		memcpy(buf + pos, cur_node->timer_id, MAX_TIMER_ID_LEN);
		pos = pos + MAX_TIMER_ID_LEN;
		buf[pos] = cur_node->second;
		pos = pos + 8;

		if(cur_node->action_len < MAX_ACTION_LEN)
		{
			buf[pos++] = (uint8_t)((cur_node->action_len & 0xFF00) >> 8);
			buf[pos++] = (uint8_t)(cur_node->action_len & 0x00FF);

			memcpy(buf + pos, cur_node->action, cur_node->action_len);
			pos = pos + cur_node->action_len;
		}else
		{
			goto err;
		}

		cur_node = cur_node->device_common_timer_next;
	}

	if(pos > 0)
	{
		ret = ya_write_flash(YA_DEVICE_TIMER_ADDR, buf, pos, 2);
		if(ret != 0)  ya_printf(C_LOG_ERROR, "ya_write_flash error\n");
	}else
	{
		ya_erase_flash(YA_DEVICE_TIMER_ADDR, 1);
	}

	if(buf)
		ya_hal_os_memory_free(buf);
	
	return 0;

	err:
	if(buf)
		ya_hal_os_memory_free(buf);

	return -1;
}

int32_t device_read_timer_into_flash(void)
{
	int ret = -1;
	device_common_timer_t device_common_timer_node;

	uint8_t *buf = NULL; 
	uint32_t read_len = 0, pos = 0;
	
	buf = (uint8_t *)ya_hal_os_memory_alloc(MAX_TIMER_DEVICE_DATA);
	if(!buf)
		goto err;

	read_len = MAX_TIMER_DEVICE_DATA;
	memset(buf, 0, MAX_TIMER_DEVICE_DATA);
	
	ret = ya_read_flash_with_var_len(YA_DEVICE_TIMER_ADDR, buf, &read_len, 2);
	if(ret != 0 || read_len == 0)
		goto err;

	while(pos < read_len)
	{
		
		memset(&device_common_timer_node, 0, sizeof(device_common_timer_t));	

		if(pos + 16 + MAX_TIMER_ID_LEN + 2  > read_len)
			goto err;
		
		device_common_timer_node.year = ((uint16_t)buf[pos] << 8) + buf[pos+1];
		pos = pos + 2;
		
		device_common_timer_node.month = buf[pos++];
		device_common_timer_node.day = buf[pos++];
		device_common_timer_node.hour = buf[pos++];
		device_common_timer_node.minute = buf[pos++];
		device_common_timer_node.week = buf[pos++];
		
		device_common_timer_node.enable = buf[pos++];
		memcpy(device_common_timer_node.timer_id, buf + pos, MAX_TIMER_ID_LEN);
		pos = pos + MAX_TIMER_ID_LEN;
		device_common_timer_node.second = buf[pos];
		pos = pos + 8;

		device_common_timer_node.action_len = ((uint16_t)buf[pos] << 8) + buf[pos+1];
		pos = pos + 2;

		if(pos + device_common_timer_node.action_len > read_len)
			goto err;

		if(device_common_timer_node.action_len < MAX_ACTION_LEN)
		{
			device_common_timer_node.action = buf + pos;
			device_add_list_timer(&device_common_timer_node);
		}else
			goto err;

		pos = pos + device_common_timer_node.action_len;

		ya_printf(C_LOG_INFO, "timer id: %s\n", device_common_timer_node.timer_id);
		ya_printf(C_LOG_INFO, "y: %d, m: %d, d: %d, h: %d, m: %d, w: %d\n", \
				device_common_timer_node.year, device_common_timer_node.month, device_common_timer_node.day, \
				device_common_timer_node.hour, device_common_timer_node.minute, device_common_timer_node.week);

		ya_printf(C_LOG_INFO, "len: %d, action: %s\n", device_common_timer_node.action_len,  device_common_timer_node.action);

	}

	if(buf)
		ya_hal_os_memory_free(buf);

	return 0;

	err:
	if(buf)
		ya_hal_os_memory_free(buf);

	return -1;
}

int32_t device_handle_cloud_timer(uint8_t *data_buf, uint8_t data_len)
{
	int ret = -1;
	uint8_t save_enable = 0;
	char *cmd_buf = NULL;
	uint32_t value = 0;
	char date[16];
	char timer_id[MAX_TIMER_ID_LEN];
	cJSON *root = NULL, *array = NULL, *array_node = NULL;
	cJSON *property = NULL, *subproperty = NULL;
	uint16_t check_action_index = 0, index = 0, item_num = 0, week_index = 0;

	int p1 = 0, p2 = 0, p3 = 0;

	device_common_timer_t device_common_timer_node;
	root = cJSON_Parse((char *)data_buf);
	if(!root)
		goto err;

	for(check_action_index = 0; check_action_index < 2; check_action_index++)
	{
		if(check_action_index == 0)
			array = cJSON_GetObjectItem(root, "add");
		else
			array = cJSON_GetObjectItem(root, "update");

		if(array)
		{
			item_num = cJSON_GetArraySize(array);
			printf("num: %d\n", item_num);
			for(index = 0; index < item_num; index++)
			{
				memset(&device_common_timer_node, 0, sizeof(device_common_timer_t));
		
				array_node = cJSON_GetArrayItem(array,index);
				if(!array_node) goto err;
		
				property = cJSON_GetObjectItem(array_node, "id");
				if(!property) goto err;
				if(strlen(property->valuestring) >= MAX_TIMER_ID_LEN) goto err;
				memcpy(device_common_timer_node.timer_id, property->valuestring, strlen(property->valuestring));

				printf("timer id: %s\n", device_common_timer_node.timer_id);

				ret = device_delete_list_timer(device_common_timer_node.timer_id);
				if(ret != 0)  ya_printf(C_LOG_ERROR, "new timer\n");
				
				property = cJSON_GetObjectItem(array_node, "enable");
				if(!property) goto err;
		
				value = atoi(property->valuestring);
				if(value > 0) value = 1;
				device_common_timer_node.enable = value;
				printf("timer enable: %d\n", device_common_timer_node.enable);

				property = cJSON_GetObjectItem(array_node, "timer");
				if(!property) goto err;

				subproperty = cJSON_GetObjectItem(property, "week");
				if(!subproperty) goto err;

				for(week_index = 0; week_index < 7; week_index++)
				{
					memset(date, 0, 16);
					snprintf(date, 3, "%d", (week_index+1));
					if(strstr(subproperty->valuestring, date))
						device_common_timer_node.week = device_common_timer_node.week + (1<< week_index);
				}

				printf("timer week: %d\n", device_common_timer_node.week);

				subproperty = cJSON_GetObjectItem(property, "hour");
				if(!subproperty || strlen(subproperty->valuestring) == 0) goto err;
				device_common_timer_node.hour = atoi(subproperty->valuestring);

				printf("timer hour: %d\n", device_common_timer_node.hour);

				subproperty = cJSON_GetObjectItem(property, "minute");
				if(!subproperty || strlen(subproperty->valuestring) == 0) goto err;
				device_common_timer_node.minute = atoi(subproperty->valuestring);

				printf("timer minute: %d\n", device_common_timer_node.minute);

				subproperty = cJSON_GetObjectItem(property, "second");
				if(!subproperty || strlen(subproperty->valuestring) == 0) 
				{
					device_common_timer_node.second = 0;
				}else
				{
					device_common_timer_node.second = atoi(subproperty->valuestring);
				}
				
				printf("timer second: %d\n", device_common_timer_node.second);

				subproperty = cJSON_GetObjectItem(property, "date");
				if(!subproperty) goto err;
				if(strlen(subproperty->valuestring) >= 8)
				{
					
					ret = sscanf(subproperty->valuestring, "%d-%d-%d", &p1, &p2, &p3);
					if(ret != 3) goto err;

					device_common_timer_node.year = p1;
					device_common_timer_node.month = p2;
					device_common_timer_node.day = p3;
					
					printf("year: %d, month: %d, day: %d\n", device_common_timer_node.year, device_common_timer_node.month, device_common_timer_node.day);
				}	
		
				property = cJSON_GetObjectItem(array_node, "cmd");
				if(!property) goto err;
		
				cmd_buf = cJSON_PrintUnformatted(property);
				if(strlen(cmd_buf) >= MAX_ACTION_LEN)
				{
					ya_hal_os_memory_free(cmd_buf);
					goto err;
				}
		
				device_common_timer_node.action_len = strlen(cmd_buf);
				device_common_timer_node.action = cmd_buf;

				printf("timer cmd: %s\n", device_common_timer_node.action);
				ret = device_add_list_timer(&device_common_timer_node);
				if(ret != 0)
				{
					ya_printf(C_LOG_ERROR, "add/update timer error\n");
					goto err;
				}

				ya_hal_os_memory_free(cmd_buf);
				save_enable = 1;
			}
		}
	}

	array = cJSON_GetObjectItem(root, "delete");
	
	if(array)
	{
		item_num = cJSON_GetArraySize(array);
		for(index = 0; index < item_num; index++)
		{
			array_node = cJSON_GetArrayItem(array,index);
			if(!array_node) goto err;
			
			property = cJSON_GetObjectItem(array_node, "id");
			if(!property) goto err;
			if(strlen(property->valuestring) >= MAX_TIMER_ID_LEN) goto err;

			memset(timer_id, 0, MAX_TIMER_ID_LEN);
			strcpy(timer_id, property->valuestring);
			
			ret = device_delete_list_timer(timer_id);
			if(ret != 0)  ya_printf(C_LOG_ERROR, "delete timer error\n");
			
			save_enable = 1;
		}
	}

	property = cJSON_GetObjectItem(root, "deleteAll");
	if(property)
	{
		device_delete_all_timer();
		save_enable = 1;
	}

	if(save_enable)
	{
		ret = device_save_timer_into_flash();
		if(ret != 0)  ya_printf(C_LOG_ERROR, "update flash timer error\n");
	}
	
	if(root)
		cJSON_Delete(root);

	return 0;

	err:

	if(root)
		cJSON_Delete(root);

	return -1;
}

uint8_t  check_enable = 0;

extern void ya_put_timer_action_into_queue(uint8_t *data, uint16_t data_len);
int32_t device_check_cloud_timer(void)
{
	uint8_t flag = 0;
	device_common_timer_t *cur_node = NULL;
	cur_node = device_common_timer_header;

	if(check_enable == 0 || sys_time.year < 2020)
	{
		return 0;
	}

	check_enable = 0;
	
	while(cur_node)
	{
		flag = 0;
		if(cur_node->enable)
		{			
			if(cur_node->week)
			{
				if((cur_node->week >> (sys_time.week - 1)) & 0x01)
				{
					flag = 1;
				}
			}else if(cur_node->year)
			{
				if(cur_node->year == sys_time.year && cur_node->month == sys_time.month && cur_node->day == sys_time.day)
				{
					flag = 1;
				}
			}

			if (cur_node->triggered)
			{
				cur_node->triggered++;
				if(cur_node->triggered > 60)
					cur_node->triggered = 0;
			}
		}

		if(flag == 1)
		{
			if(cur_node->minute == sys_time.minute && cur_node->hour == sys_time.hour && cur_node->second == sys_time.second && cur_node->triggered == 0)
			{	
				ya_printf(C_LOG_INFO, "week: %d, year: %d, month: %d, day: %d\n", cur_node->week, cur_node->year, cur_node->month, cur_node->day);
				ya_printf(C_LOG_INFO, "action occur: %s\n", cur_node->timer_id);
				ya_put_timer_action_into_queue((uint8_t *)cur_node->action, cur_node->action_len);

				cur_node->triggered = 1;
			}
		}
		cur_node = cur_node->device_common_timer_next;
	}

	return 0;
}



bool is_leap_year(unsigned int year)
{
    return (!(year % 4) && (year % 100)) || !(year % 400);
}

static uint8_t days_in_month(uint8_t month, uint16_t year)
{
	if(month == 1 || month == 3 || month == 5 || month == 7 ||
		month == 8 || month == 10 || month == 12 )
		return 31;
	else if(month == 4 || month == 6 || month == 9 || month == 10)
		return 30;
	else if(month == 2)
	{
		return (28 + is_leap_year(year));
	}

	return 31;
}

void sys_update_cloud_time(uint8_t second_add)
{
	uint8_t min_add = 0;
	
	if(second_add >= 60)
		return;
		
	sys_time.second += second_add;
	if(sys_time.second > 59) 
	{								
		sys_time.second = sys_time.second - 60;   
		sys_time.minute++;
		min_add = 1;
		
		if(sys_time.minute > 59) 
		{						   
			sys_time.minute = 0;							  
			sys_time.hour++;
			
			if(sys_time.hour > 23) 
			{					   
				sys_time.hour = 0;														

				sys_time.day++;  
				sys_time.week++;

				if(sys_time.week > 7)					  
					sys_time.week = 1;						 
														
				if(sys_time.day > days_in_month(sys_time.month, sys_time.year)) 
				{
					sys_time.day = 1;  
					sys_time.month++;
					
					if(sys_time.month > 12) 
					{				
						sys_time.month = 1; 				   
						sys_time.year++;					
					}											   
				}													
			}														
		}															
	}

	if(min_add)
	{
		sys_timer_printf();
	}

	
	check_enable = 1;
}

void sys_abstime_running(void)
{
	sys_update_cloud_time(1);
}

int32_t device_handle_severtime_set(uint8_t *data_buf, uint8_t data_len)
{
	cJSON *root = NULL, *time = NULL;
	cJSON *property = NULL;
	sys_time_t sys_time_obj;
	uint32_t delta_time = 0;

	memset(&sys_time_obj, 0, sizeof(sys_time_t));

	root = cJSON_Parse((char *)data_buf);
	if(!root)  goto err;	

	time = cJSON_GetObjectItem(root, "time");
	if(!time) goto err;

	property = cJSON_GetObjectItem(time, "year");
	if(!property) goto err;
	sys_time_obj.year = atoi(property->valuestring);

	if(sys_time_obj.year < 2020)
		goto err;

	property = cJSON_GetObjectItem(time, "month");
	if(!property) goto err;
	sys_time_obj.month = atoi(property->valuestring);

	if(sys_time_obj.month > 12 || sys_time_obj.month == 0)
		goto err;

	property = cJSON_GetObjectItem(time, "day");
	if(!property) goto err;
	sys_time_obj.day = atoi(property->valuestring);
	
	if(sys_time_obj.day > 31 || sys_time_obj.day == 0)
		goto err;

	property = cJSON_GetObjectItem(time, "hour");
	if(!property) goto err;
	sys_time_obj.hour = atoi(property->valuestring);

	if(sys_time_obj.hour > 23)
		goto err;

	property = cJSON_GetObjectItem(time, "minute");
	if(!property) goto err;
	sys_time_obj.minute = atoi(property->valuestring);

	if(sys_time_obj.minute > 59)
		goto err;

	property = cJSON_GetObjectItem(time, "second");
	if(!property) goto err;
	sys_time_obj.second = atoi(property->valuestring);

	if(sys_time_obj.second > 59)
		goto err;

	property = cJSON_GetObjectItem(time, "week");
	if(!property) goto err;
	sys_time_obj.week = atoi(property->valuestring);

	if(sys_time_obj.week > 7 || sys_time_obj.week == 0)
		goto err;

	ya_callback_servertimer_set(sys_time_obj.year, sys_time_obj.month, sys_time_obj.day, sys_time_obj.hour, sys_time_obj.minute, sys_time_obj.second, sys_time_obj.week);

	ya_hal_hardware_timer_stop();
	memcpy(&sys_time, &sys_time_obj, sizeof(sys_time_t));

	ya_hal_hardware_timer_start();
	//sys_timer_printf();
	
	if(root)
		cJSON_Delete(root);

	return 0;

	err:

	if(root)
		cJSON_Delete(root);

	return -1;
}

void sys_abstime_init(void)
{
	sys_time.year = 2000;
	sys_time.month = 1;
	sys_time.day = 1;
	sys_time.hour = 0;
	sys_time.minute = 0;
	sys_time.second = 0;
	sys_time.week = 6;
}

void ya_hardware_timer_init(void)
{
	device_read_timer_into_flash();
}


void ya_clear_timer_flash(void)
{
	device_delete_all_timer();
	device_save_timer_into_flash();
}

