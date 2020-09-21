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

#ifndef _YA_OTA_HAL_H_
#define _YA_OTA_HAL_H_

#include "ya_config.h" 

#define HTTP_BUF_SIZE   				4096

/*这参数保留ota文件前面若干字节，用于文件的校验*/
#if (YA_CHIP == YA_8710_CX)
#define HTTP_FILE_HEADER_LEN			32
#elif (YA_CHIP == YA_8710_BX)
#define HTTP_FILE_HEADER_LEN			8
#else
#define HTTP_FILE_HEADER_LEN			8
#endif

typedef struct
{
	uint32_t header_len;
	uint32_t toatal_rece_len;
	uint32_t file_len;
	
	int32_t body_read_len;
	uint32_t body_cur_pos;

	uint8_t	 body_flie_header_cur_len;
	uint8_t	 body_file_header[HTTP_FILE_HEADER_LEN];

	uint8_t  body_file_header_enbable;

}http_file_para_t;

/*当校验文件成功后，执行这个函数进行重启 */
void ya_hal_ota_done(void);

/*是否需要将ota文件中开头长度为HTTP_FILE_HEADER_LEN 在下载过程中就存于flash中，有些芯片校验通过后这一段最后才存储。
 *如果要立即储存，则返回1，如果校验通过后才存储，则返回0.
*/	
uint32_t ya_get_file_header_writer_flash_enable(void);

/*返回文件ota的下载地址
*/	
uint32_t ya_get_ota_download_addr(void);

/*下载完ota文件后，校验文件是否合法
* 0：成功，非0：失败
*/
int32_t ya_verify_ota_checksum(uint32_t addr, uint32_t file_size, uint8_t *pre_file, uint8_t *file_md5, uint8_t *buf);

/*针对8710bx芯片，只需要返回0即可
*/
int32_t ya_http_prefile_header_init(void);

/*针对8710bx芯片，只需要返回1即可
*/
int32_t ya_http_prefile_header_handle(http_file_para_t *ya_http_file, int socket_id, uint8_t *buf);

int32_t ya_get_download_whole_file_finish(void);
/*擦除ota区域的数据
* 0：成功，非0：失败
*/
int32_t ya_http_ota_sector_erase(uint32_t address, uint32_t size);

/*往ota区域写数据
* 0：成功，非0：失败
*/
int32_t ya_http_ota_sector_write(uint32_t address ,uint32_t Length, uint8_t *data);

/*下载完mcu文件后，对mcu文件进行校验
* 0：成功，非0：失败
*/
int32_t ya_verify_mcu_file_checksum(uint32_t addr, uint32_t file_size, uint8_t *file_md5, uint8_t *buf);

#endif

