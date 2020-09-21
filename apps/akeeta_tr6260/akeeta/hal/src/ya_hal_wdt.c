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

#include "FreeRTOS.h"
#include "ya_hal_wdt.h"
#include "wdt.h"
#include "soc_top_reg.h"


#ifndef bool
#define bool char
#endif
#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif

#define WTD_CTRL_RSTTIME_7		7
#define WTD_CTRL_INTTIME_8		8
#define WTD_CTRL_INTEN_DIS		0
#define WTD_CTRL_INTEN_EN		1
#define WTD_CTRL_RSTEN_DIS		0
#define WTD_CTRL_RSTEN_EN		1

#define WTD_CTRL_INTTIME_9		9
#define WTD_CTRL_INTTIME_10		10  	// 64 sec


static bool isinitWdt = FALSE;

void watchdog_irq_handler(uint32_t iBark)
{
	vTaskDelay(10);
	wdt_reset_chip(WDT_RESET_CHIP_TYPE_RESYSTEM);
}

void InitWdt(void)
{
	if(isinitWdt == FALSE)
	{
		CLK_ENABLE(CLK_WCT);
		irq_isr_register(IRQ_VECTOR_WDT, watchdog_irq_handler);
		isinitWdt = TRUE;
	}
	return;
}


int32_t ya_hal_wdt_set_timeout(uint32_t msecs)
{
	InitWdt();
	wdt_config(WDT_INTR_PERIOD_10, WDT_RST_TIME_7, WTD_CTRL_INTEN_EN, WTD_CTRL_RSTEN_DIS);
	return 0;
}


int32_t ya_hal_wdt_start(void)
{
	wdt_start(1);	
	ef_set_long("WtdFlag", 1);
	return 0;
}

int32_t ya_hal_wdt_feed(void)
{
	wdt_restart();
	return 0;
}

int32_t ya_hal_wdt_stop(void)
{
	wdt_stop();
	return 0;
}


