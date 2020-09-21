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
#include "ya_light_io.h"
#include "hls_rgb.h"
#include "ya_hal_pwm.h"

void ya_white_color_convert(uint8_t *cool, uint8_t *warm, uint8_t w_l, uint8_t w_c)
{
	uint8_t c = 0, w = 0; 

	c = w_c;
	w = 100 - w_c;

	c = c*w_l*2.55/100;
	w = w*w_l*2.55/100; 

	(*cool) = c;
	(*warm) = w;
}

void ya_pwm_led_write(unsigned char r,unsigned char g,unsigned char b,unsigned char c,unsigned char w)
{
	ya_hal_pwm_write(PWM_R,(uint32_t)(r * 999/255));
	ya_hal_pwm_write(PWM_G,(uint32_t)(g * 999/255));
	ya_hal_pwm_write(PWM_B,(uint32_t)(b * 999/255));
	
	ya_hal_pwm_write(PWM_COOL,(uint32_t)(c * 999/255));
	ya_hal_pwm_write(PWM_WARM,(uint32_t)(w * 999/255));
}

void ya_pwm_led_write_index(uint8_t index, uint8_t value)
{
	//ya_printf(C_LOG_INFO,"\nindex===%d,value===%d\n",index,value);

	uint32_t n = value * 999/255;
	switch(index)
	{
		case 0:
			ya_hal_pwm_write(PWM_R,n);
			break;

		case 1:
			ya_hal_pwm_write(PWM_G,n);
			break;
		
		case 2:
			ya_hal_pwm_write(PWM_B,n);
			break;

		case 3:
			ya_hal_pwm_write(PWM_COOL,n);
			break;

		case 4:
			ya_hal_pwm_write(PWM_WARM,n);
			break;

		default:
			break;
	}

}


typedef struct{

	uint8_t cur_value;
	uint8_t end_value;
	uint8_t flag;
}ya_control_pwm_t;

ya_control_pwm_t ya_control_pwm[PWM_NUM];


void ya_pwm_conttol_printf()
{
	ya_printf(C_LOG_INFO, "cur: %d, %d, %d, %d, %d\n", ya_control_pwm[0].cur_value, ya_control_pwm[1].cur_value, ya_control_pwm[2].cur_value, ya_control_pwm[3].cur_value, ya_control_pwm[4].cur_value);
	ya_printf(C_LOG_INFO, "end: %d, %d, %d, %d, %d\n", ya_control_pwm[0].end_value, ya_control_pwm[1].end_value, ya_control_pwm[2].end_value, ya_control_pwm[3].end_value, ya_control_pwm[4].end_value);
}

void ya_pwm_control_with_step(uint8_t start_index, uint8_t index_num)
{
	uint8_t index = 0, total_flag = 0;

	for (index = start_index; index < start_index + index_num; index++)
	{
		if (ya_control_pwm[index].cur_value == ya_control_pwm[index].end_value)
			ya_control_pwm[index].flag = 0;
		else if (ya_control_pwm[index].cur_value < ya_control_pwm[index].end_value)
			ya_control_pwm[index].flag = 1;
		else
			ya_control_pwm[index].flag = 2;
	}

	while(1)
	{
		total_flag = 0;
		for (index = start_index; index < start_index + index_num; index++)
		{
			if (ya_control_pwm[index].cur_value == ya_control_pwm[index].end_value)
			{
				total_flag++;
			}
		}

		if (total_flag == index_num)
			break;

		for (index = start_index; index < start_index + index_num; index++)
		{
			if (ya_control_pwm[index].cur_value == ya_control_pwm[index].end_value)
				continue;
		
			if (ya_control_pwm[index].flag == 1)
			{
				ya_control_pwm[index].cur_value++;
			} else if (ya_control_pwm[index].flag == 2)
			{
				ya_control_pwm[index].cur_value--;
			}			
			
			ya_pwm_led_write_index(index, ya_control_pwm[index].cur_value);
		}
		ya_delay(5);
	}
}

uint8_t mode_cur = 0;
void ya_write_light_para(ya_light_data_t ya_ligth_para)
{
	ya_printf(C_LOG_INFO, "para3: %d, %d, %d, %d, %d\n", ya_ligth_para.w_l, ya_ligth_para.w_color,  ya_ligth_para.c_h,ya_ligth_para.c_s,ya_ligth_para.c_b);
	uint8_t mode = 0;

	if ( ya_ligth_para.c_h >= 360)
		 ya_ligth_para.c_h = 0;

	if (ya_ligth_para.c_b || ya_ligth_para.c_h || ya_ligth_para.c_s)
	{
		mode = 1;
		ya_control_pwm[3].end_value = 0;
		ya_control_pwm[4].end_value = 0;
		HSVtoRGB(&(ya_control_pwm[0].end_value), &(ya_control_pwm[1].end_value), &(ya_control_pwm[2].end_value), ya_ligth_para.c_h, ya_ligth_para.c_s, ya_ligth_para.c_b);
	}else 
	{
		mode = 0;
		ya_control_pwm[0].end_value = 0;
		ya_control_pwm[1].end_value = 0;
		ya_control_pwm[2].end_value = 0;
		ya_white_color_convert(&(ya_control_pwm[3].end_value), &(ya_control_pwm[4].end_value),  ya_ligth_para.w_l, ya_ligth_para.w_color);
	}

	//ya_pwm_conttol_printf();

	if (mode_cur != mode)
	{
		mode_cur = mode;
		if (mode == 1)
		{
			ya_pwm_control_with_step(3, 2);
			ya_pwm_control_with_step(0, 3);
		}else
		{
			ya_pwm_control_with_step(0, 3);
			ya_pwm_control_with_step(3, 2);
		}

	}else
	{
		if (mode_cur == 0)
		{
			ya_pwm_control_with_step(3, 2);
		}else
		{
			ya_pwm_control_with_step(0, 3);
		}
	}
}


void ya_light_control_pwm_para_init(void)
{
	uint8_t index = 0;
	for (index = 0; index < PWM_NUM; index++ )
	{
		memset(&(ya_control_pwm[index]), 0, sizeof(ya_control_pwm_t));
	}
}


