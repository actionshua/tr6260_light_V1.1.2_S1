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
#include "ya_hal_flash.h"
#include "drv_spiflash.h"
#include "ya_common.h"
#define ya_SPIFLASH_SECTOR_SIZE	0x1000 	// 4KB

static ya_hal_os_mutex_t Semaphore_flash_lock = NULL;

int32_t ya_hal_flash_read(uint32_t flash_addr, uint8_t *data, uint32_t data_len)
{	
	int ret;
	ret=hal_spiflash_read(flash_addr,data,data_len);
	return ret;
}


int32_t ya_hal_flash_erase(uint32_t flash_addr, uint32_t size)
{
	int ret;
	uint32_t i = 0, sector_count = 0;

	sector_count = size / ya_SPIFLASH_SECTOR_SIZE;
	if(size % ya_SPIFLASH_SECTOR_SIZE > 0)
		sector_count++;

	for(i=0; i < sector_count; i++)
	{
		ret = hal_spiflash_erase(flash_addr + i*ya_SPIFLASH_SECTOR_SIZE,ya_SPIFLASH_SECTOR_SIZE);
		if(ret != 0)
			return -1;
	}

	return 0;
}


int32_t ya_hal_flash_write(uint32_t flash_addr, uint8_t *data, uint32_t data_len)
{
	int ret;
	ret=ya_hal_flash_erase(flash_addr, data_len);
	if(ret != 0)
		return -1;
	ret=hal_spifiash_write(flash_addr,data,data_len);
	if(ret != 0)
		return -1;
}


