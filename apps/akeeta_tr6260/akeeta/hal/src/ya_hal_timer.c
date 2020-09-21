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

#include "ya_hal_timer.h"

int32_t ya_has_timer_expired(Ya_Timer *Ya_Timer)
{
	uint32_t cur_timer =  (uint32_t)(xTaskGetTickCount() * portTICK_RATE_MS);
	uint32_t init_timer = (uint32_t)(Ya_Timer->init_tick * portTICK_RATE_MS);
	uint32_t delta_timer = 0;

	if(cur_timer >= init_timer)
		delta_timer = cur_timer - init_timer;
	else
		delta_timer = 0xffffffff - init_timer + cur_timer;

	if (Ya_Timer->type == YA_TIMER_TYPE_MS)
	{
		if (delta_timer > Ya_Timer->time_out)
			return 0;
	}else
	{
		if (delta_timer/1000 > Ya_Timer->time_out)
			return 0;
	}
		
	return -1;
}

void ya_countdown_ms(Ya_Timer *Ya_Timer, uint32_t timeout)
{
    if (Ya_Timer == NULL || timeout <= 0) {
        return;
    }
    Ya_Timer->init_tick = xTaskGetTickCount();
    Ya_Timer->time_out = timeout;
    Ya_Timer->type = YA_TIMER_TYPE_MS;
}

void ya_countdown_sec(Ya_Timer *Ya_Timer, uint32_t timeout)
{
    if (Ya_Timer == NULL || timeout <= 0) {
        return;
    }
    Ya_Timer->init_tick = xTaskGetTickCount();
    Ya_Timer->time_out = timeout;
    Ya_Timer->type = YA_TIMER_TYPE_S;
}

uint32_t ya_left_ms(Ya_Timer *Ya_Timer)
{
	uint32_t result_ms = 0;
	uint32_t cur_tick = (uint32_t) xTaskGetTickCount();
	uint32_t delta_tick = 0;

	if(cur_tick >= Ya_Timer->init_tick)
		delta_tick = cur_tick - Ya_Timer->init_tick;
	else
		delta_tick = 0xffffffff - Ya_Timer->init_tick + cur_tick;

    if (Ya_Timer->type == YA_TIMER_TYPE_MS) {
        result_ms = Ya_Timer->time_out - (delta_tick * portTICK_RATE_MS);
    } else {
       result_ms = (Ya_Timer->time_out * 1000) - (delta_tick * portTICK_RATE_MS);
    }

    return result_ms;

}


int32_t ya_timer_compare(uint32_t time_start_ms, uint32_t time_end_ms, uint32_t timeout_ms)
{
	uint32_t delta_timer = 0;
	if (time_end_ms < time_start_ms)
	{
		delta_timer = 0xffffffff - time_start_ms + time_end_ms;
	}else
	{
		delta_timer = time_end_ms - time_start_ms;
	}

	if (delta_timer > timeout_ms)
		return 0;

	return -1;
}

void ya_init_timer(Ya_Timer *Ya_Timer)
{
    Ya_Timer->time_out = 0;
    Ya_Timer->init_tick = 0;
    Ya_Timer->type = YA_TIMER_TYPE_MS;
}

//weishan add
void ya_delay(uint32_t milliseconds)
{
	vTaskDelay(ya_hal_os_msec_to_ticks(milliseconds));
}




