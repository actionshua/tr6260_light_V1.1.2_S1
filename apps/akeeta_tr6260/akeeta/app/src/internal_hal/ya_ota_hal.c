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
#include "network_platform.h"
#include "mbedtls_md5.h"
#include "ya_ota_hal.h"
#include "ya_config.h"


extern unsigned int get_OTA_write_data_addr(void);

void ya_hal_ota_done(void)
{
	return otaHal_done();
}

uint32_t ya_get_file_header_writer_flash_enable(void)
{
	return 1;
}

uint32_t ya_get_ota_download_addr(void)
{
	return 0xB6000;
}

int32_t ya_verify_mcu_file_checksum(uint32_t addr, uint32_t file_size, uint8_t *file_md5, uint8_t *buf)
{
	//md5 checksum
	uint16_t index = 0;
	int32_t ret = -1;
	uint32_t read_len = 0, cur_len = 0;
	uint32_t total_section = 0; 
	uint8_t md5_clc[16];
	mbedtls_md5_context ctx;

	memset(md5_clc, 0, 16);
	total_section = file_size/HTTP_BUF_SIZE;
	if(file_size % HTTP_BUF_SIZE > 0)
		total_section++;

	ya_printf(C_LOG_INFO, "total_section is: %d\n", total_section);

	mbedtls_md5_init(&ctx);
	mbedtls_md5_starts(&ctx);

	for(index = 0; index < total_section; index++)
	{
		if(index == (total_section -1))
		{
			read_len = file_size - (total_section - 1) * HTTP_BUF_SIZE;
			printf("last section len: %d\n", read_len);
		}else
		{
			read_len = HTTP_BUF_SIZE;
		}

		ret = ya_hal_flash_read(addr+cur_len, buf, read_len);
		if(ret != 0)
			return -1;

		mbedtls_md5_update(&ctx, buf, read_len);
		
		cur_len = cur_len + read_len;
	}

	mbedtls_md5_finish(&ctx, md5_clc);
	mbedtls_md5_free(&ctx);

	printf("obj md5: \n");
	for(index = 0; index < 16; index++)
	{
		printf("%02x ", md5_clc[index]);
	}
	printf("\n");

	printf("cur md5: \n");
	for(index = 0; index < 16; index++)
	{
		printf("%02x ", file_md5[index]);
	}
	printf("\n");

	if(memcmp(md5_clc, file_md5, 16) !=0)
		return -1;

	return 0;

}


int32_t ya_verify_ota_checksum(uint32_t addr, uint32_t file_size, uint8_t *pre_file, uint8_t *file_md5, uint8_t *buf)
{
	//md5 checksum
	uint16_t index = 0;
	int32_t ret = -1;
	uint32_t read_len = 0, cur_len = 0;
	uint32_t total_section = 0; 
	uint8_t md5_clc[16];
	mbedtls_md5_context ctx;
	
	memset(md5_clc, 0, 16);

	addr=get_OTA_write_data_addr();
	total_section = file_size/HTTP_BUF_SIZE;
	if(file_size % HTTP_BUF_SIZE > 0)
		total_section++;

	ya_printf(C_LOG_INFO, "total_section is: %d\n", total_section);

	mbedtls_md5_init(&ctx);
	mbedtls_md5_starts(&ctx);

	for(index = 0; index < total_section; index++)
	{
		if(index == (total_section -1))
		{
			read_len = file_size - (total_section - 1) * HTTP_BUF_SIZE;
			printf("last section len: %d\n", read_len);
		}else
		{
			read_len = HTTP_BUF_SIZE;
		}

		ret = ya_hal_flash_read(addr+cur_len, buf, read_len);
		if(ret != 0)
			return -1;

		mbedtls_md5_update(&ctx, buf, read_len);
		cur_len = cur_len + read_len;
	}

	mbedtls_md5_finish(&ctx, md5_clc);
	mbedtls_md5_free(&ctx);

	printf("obj md5: \n");
	for(index = 0; index < 16; index++)
	{
		printf("%02x ", md5_clc[index]);
	}
	printf("\n");

	printf("cur md5: \n");
	for(index = 0; index < 16; index++)
	{
		printf("%02x ", file_md5[index]);
	}
	printf("\n");

	if(memcmp(md5_clc, file_md5, 16) !=0)
		return -1;
	return 0;

}


int32_t ya_http_prefile_header_init(void)
{
	return 0;
}

int32_t ya_http_prefile_header_handle(http_file_para_t *ya_http_file, int socket_id, uint8_t *buf)
{
	return 1;
}

int32_t ya_get_download_whole_file_finish(void)
{
	return 0;
}
int32_t ya_http_ota_sector_erase(uint32_t address, uint32_t size)
{
	return otaHal_init();
}

int32_t ya_http_ota_sector_write(uint32_t address ,uint32_t Length, uint8_t *data)
{
	return otaHal_write(data,Length);
}

