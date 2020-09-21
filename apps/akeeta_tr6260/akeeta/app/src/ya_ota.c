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

#include "lwip/netdb.h"
#include "cJSON.h"
#include "ya_common.h"
#include "ya_config.h"
#include "ya_flash.h"
#include "ya_ota_hal.h"
#include "ya_mcu_ota_user_define.h"
#include "mbedtls_md5.h"

#define YA_MAX_MCU_VERSION_LEN			64

typedef enum
{
	YA_WIFI	   				= 0x00,
	YA_MCU					= 0x01,
}ya_ota_type_t;

typedef enum
{
	YA_WIFI_OTA_REQUIRE	   				= 0x00,
	YA_MCU_OTA_REQUIRE				= 0x01,
}ya_ota_queue_msg_type_t;

typedef enum
{
	YA_HTTP_LINK_FAILED            = -4,                    
	YA_CHECK_FILE_FAILED		   = -3,
	YA_DOWNLOAD_FILE_FAILED        = -2,
	YA_HTTP_OTA_FAILED			   = -1,

	YA_START_DOWN				   = 10,
	YA_ERASE_FLASH_OK			   = 30,
	YA_DOWNLOAD_OK				   = 80,
	YA_CHECKSUM_OK				   = 90, 
	YA_HTTP_SUCCESS				   = 100,
}ya_app_ota_result_t;

typedef enum
{
	YA_NORMAL            		   = 0,                    
	YA_NORMAL_WITH_OTA_SUCCESS	   = 1,
	YA_NORMAL_WITH_OTA_FAILE       = 2,
}ya_app_ota_sys_restart_t;


typedef enum
{
	YA_HTTP_START             = 0x00,                    
	YA_PARSE_HTTP_URL,	   				

	YA_HTTP_CONNECT,
	YA_HTTP_SEND_GET,
	
	YA_HTTP_HEADER_HANDLE,
	YA_PRE_FILE_HANDLE,

	YA_EARSE_FLASH,
	YA_HTTP_BODY_HANDLE,
	
	YA_HTTP_CHECKSUM,
	YA_ERROR_HANDLE,

	YA_HTTP_RETRY_WAIT,

	YA_WAIT_TO_REBOOT,

	YA_HTTP_IDLE,

	YA_NOTIFY_MCU,
}ya_http_ota_status_t;

typedef struct 
{
	char *ya_http_url;
	char *ya_http_host;
	char *ya_http_resource;
	uint16_t ya_http_port;

	int32_t retry_num;
	int32_t retry_interval;
	
	uint8_t md5[16];
	char ota_ver[YA_MAX_MCU_VERSION_LEN + 1];
	uint32_t ota_size;
}ya_http_ota_url_para_t;

#define HTTP_HEADER_BAK_LEN				32

typedef struct {
	uint32_t	status_code;
	uint32_t	header_len;     

	uint32_t	body_len;
	uint32_t	body_read_len;

	uint8_t		header_bak[HTTP_HEADER_BAK_LEN];
	uint32_t	parse_status;
} ya_http_response_result_t;

//==================wifi ota para=============================
ya_http_response_result_t ya_http_reponse_result;
ya_http_ota_url_para_t ya_http_ota_url_para;
http_file_para_t ya_http_file;

uint8_t ota_result_upload_enable = 0;

//===================mcu ota para==============================
ya_http_response_result_t ya_http_mcu_reponse_result;
ya_http_ota_url_para_t ya_http_mcu_ota_url_para;
http_file_para_t ya_http_mcu_file;

uint8_t mcu_ota_result_upload_enable = 0;
//============================================================

ya_hal_os_thread_t ya_ota_app = NULL;
ya_hal_os_queue_t ya_ota_queue = NULL;

char new_ota_version[VERSION_LEN + 1];

int ya_parser_url(ya_http_ota_url_para_t *ya_http_connect_para)
{
	char *url = NULL;
	url = ya_http_connect_para->ya_http_url;
	if(url)
	{
		char *http = NULL, *pos = NULL;

		http = strstr(url, "http://");
		if(http) // remove http
			url += strlen("http://");

		pos = strstr(url, ":"); // get port
		if(pos){
			memcpy(ya_http_connect_para->ya_http_host, url, (pos-url));
			pos += 1;
			ya_http_connect_para->ya_http_port = atoi(pos);
		}else{
			pos = strstr(url, "/");
			if(pos){
				memcpy(ya_http_connect_para->ya_http_host, url, (pos-url));
				url = pos;
			}
			ya_http_connect_para->ya_http_port = 80;
		}
		printf("server: %s\n\r", ya_http_connect_para->ya_http_host);
		printf("port: %d\n\r", ya_http_connect_para->ya_http_port);
		
		pos = strstr(url, "/");
		if(pos){
			memcpy(ya_http_connect_para->ya_http_resource, pos + 1, strlen(pos + 1));
		}
		printf("resource: %s\n\r", ya_http_connect_para->ya_http_resource);
		
		return 0;
	}
	return -1;
}

int ya_parse_http_header_response(uint8_t *response, uint32_t response_len, ya_http_response_result_t *result) 
{
    uint32_t i, p, q, m;
    uint32_t header_end = 0;

    //Get status code
	if(0 == result->parse_status){//didn't get the http response
		uint8_t status[4] = {0};
		i = p = q = m = 0;
		for (; i < response_len; ++i) {
			if (' ' == response[i]) {
				++m;
				if (1 == m) {//after HTTP/1.1
					p = i;
				} 
				else if (2 == m) {//after status code
					q = i;
					break;
				}
			}
		}
		if (!p || !q || q-p != 4) {//Didn't get the status code
			return -1;
		}
		memcpy(status, response+p+1, 3);//get the status code
		result->status_code = atoi((char const *)status);
		if(result->status_code == 200)
			result->parse_status = 1;
		else
		{
			printf("\n\r[%s] The http response status code is %d", __FUNCTION__, result->status_code);
			return -1;
		}
	}

	//if didn't receive the full http header
	if(3 == result->parse_status){//didn't get the http response
		p = q = 0;
		for (i = 0; i < response_len; ++i) {
			if (response[i] == '\r' && response[i+1] == '\n' &&
				response[i+2] == '\r' && response[i+3] == '\n') {//the end of header
				header_end = i+4;
				result->parse_status = 4;
				result->header_len = header_end;
				result->body_read_len = response_len - result->header_len;  //weishan
				break;
			}
		}
		if (3 == result->parse_status) {//Still didn't receive the full header	
			memset(result->header_bak, 0, strlen((char const*)result->header_bak));
			memcpy(result->header_bak, response + response_len - HTTP_HEADER_BAK_LEN, HTTP_HEADER_BAK_LEN);
		}
	}

    //Get Content-Length
	if(1 == result->parse_status){//didn't get the content length
		const uint8_t *content_length_buf1 = (uint8_t *)"CONTENT-LENGTH";
		const uint8_t *content_length_buf2 = (uint8_t *)"Content-Length";
		const uint32_t content_length_buf_len = strlen((char const*)content_length_buf1);
		p = q = 0;
		
		for (i = 0; i < response_len; ++i) {
			if (response[i] == '\r' && response[i+1] == '\n') {
				q = i;//the end of the line
				if (!memcmp(response+p, content_length_buf1, content_length_buf_len) ||
						!memcmp(response+p, content_length_buf2, content_length_buf_len)) {//get the content length
					int j1 = p+content_length_buf_len, j2 = q-1;
					while ( j1 < q && (*(response+j1) == ':' || *(response+j1) == ' ') ) ++j1;
					while ( j2 > j1 && *(response+j2) == ' ') --j2;
					uint8_t len_buf[12] = {0};
					memcpy(len_buf, response+j1, j2-j1+1);
					result->body_len = atoi((char const *)len_buf);
					result->parse_status = 2;
				}
				p = i+2;
			}
			if (response[i] == '\r' && response[i+1] == '\n' &&
					response[i+2] == '\r' && response[i+3] == '\n') {//Get the end of header
				header_end = i+4;//p is the start of the body
				if(result->parse_status == 2){//get the full header and the content length
					result->parse_status = 4;
					result->header_len = header_end;
					result->body_read_len = response_len - result->header_len;  //weishan
				}
				else {//there are no content length in header	
					printf("\n\r[%s] No Content-Length in header", __FUNCTION__);
					return -1;
				}
				break;
			}	
		}
		
		if (1 == result->parse_status) {//didn't get the content length and the full header
			memset(result->header_bak, 0, HTTP_HEADER_BAK_LEN);
			memcpy(result->header_bak, response + response_len - HTTP_HEADER_BAK_LEN, HTTP_HEADER_BAK_LEN);
		}
		else if (2 == result->parse_status) {//didn't get the full header but get the content length
			result->parse_status = 3;
			memset(result->header_bak, 0, HTTP_HEADER_BAK_LEN);
			memcpy(result->header_bak, response + response_len - HTTP_HEADER_BAK_LEN, HTTP_HEADER_BAK_LEN);
		}
	}

	return result->parse_status;
}

int ya_http_header_handle(ya_http_response_result_t *ya_http_header, int socket_id, uint8_t *buf)
{
	int32_t read_bytes = 0;
	uint16_t idx = 0, backup_len = 0;
	
	if(ya_http_header->parse_status <= 3)
	{
		if(ya_http_header->parse_status == 0)
			backup_len = 0;
		else if((ya_http_header->parse_status == 1) || (ya_http_header->parse_status == 3))
			backup_len = HTTP_HEADER_BAK_LEN;

		read_bytes = read(socket_id, buf + backup_len, (HTTP_BUF_SIZE - backup_len));
		if(read_bytes <= 0)
			return -1;

		idx = read_bytes + backup_len;

		if(backup_len)
			memcpy(buf, ya_http_header->header_bak, backup_len);

		if(ya_parse_http_header_response(buf, idx, ya_http_header) == -1)
			return -1;
	}

	if(ya_http_header->parse_status > 3)
		return 1;

	return 0;
}


int ya_http_body_handle(http_file_para_t *ya_http_file, int socket_id, uint8_t *buf, uint32_t ota_addr)
{
	uint8_t *pos = NULL;
	uint32_t len_header_remain = 0;
	ya_printf(C_LOG_INFO, "b-r-l: %d, b-c-p: %d\n", ya_http_file->body_read_len, ya_http_file->body_cur_pos);
	if(ya_http_file->body_read_len)
	{
		if(ya_http_file->body_file_header_enbable == 0 && ya_http_file->body_flie_header_cur_len < HTTP_FILE_HEADER_LEN)
		{		
			if(ya_http_file->body_read_len > (HTTP_FILE_HEADER_LEN - ya_http_file->body_flie_header_cur_len))
				len_header_remain = HTTP_FILE_HEADER_LEN - ya_http_file->body_flie_header_cur_len;
			else
				len_header_remain = ya_http_file->body_read_len;

			//get the hash value
			pos = buf + ya_http_file->header_len;			
			memcpy(ya_http_file->body_file_header + ya_http_file->body_flie_header_cur_len, pos, len_header_remain);

			ya_http_file->body_cur_pos += len_header_remain;
			pos += len_header_remain;
			
			ya_http_file->body_read_len =ya_http_file->body_read_len - len_header_remain;
			ya_http_file->body_flie_header_cur_len += len_header_remain;

			ya_http_file->header_len = 0;
			printf("h-r-l: %d\n ", ya_http_file->body_flie_header_cur_len);
		}else 
		{
			pos = buf + ya_http_file->header_len;
			ya_http_file->header_len = 0;
		}

		if(ya_http_file->body_read_len)
		{
			//write flash now
			if(ya_http_ota_sector_write(ota_addr + ya_http_file->body_cur_pos, ya_http_file->body_read_len, pos) < 0)
				return -1;

			ya_http_file->body_cur_pos += ya_http_file->body_read_len;
		}
	}

	if(ya_http_file->body_cur_pos >= ya_http_file->file_len)
		return 1;

	ya_http_file->body_read_len = read(socket_id, buf, HTTP_BUF_SIZE);
	if(ya_http_file->body_read_len < 0)
	{
		printf("read ota file error !!!\n");
		return -1;
	}
	return 0;
}

void ya_free_http_url_resource(ya_http_ota_url_para_t *ya_http_file)
{
	if(ya_http_file->ya_http_host)
	{
		ya_hal_os_memory_free(ya_http_file->ya_http_host);
		ya_http_file->ya_http_host = NULL;
	}
	
	if(ya_http_file->ya_http_resource)
	{
		ya_hal_os_memory_free(ya_http_file->ya_http_resource);
		ya_http_file->ya_http_resource = NULL;
	}
	
	if(ya_http_file->ya_http_url)
	{
		ya_hal_os_memory_free(ya_http_file->ya_http_url);
		ya_http_file->ya_http_url = NULL;
	}
}

int32_t ya_http_connect_server(char *host, int port)
{
	ya_printf(C_LOG_INFO,"[para] host: %s, port: %d\n", host, port);
	
	int32_t server_socket = -1, ret = -1;
	uint32_t des_ip = 0;

    ret = ya_hal_get_ip_by_name(host, &des_ip);
    if(ret == -1){ 
		return -1;
    }
	ya_printf(C_LOG_INFO,"ip is: %08x\n", ya_hal_get_ip_by_name);

	ret = ya_tcp_client_connect_block(&server_socket, des_ip, port, (40*1000));
	if(ret == 0)
	{
		ya_printf(C_LOG_INFO,"connnect cloud success\n");
		return server_socket;
	}

	return -1;
}

void ya_ota_handle_command(uint8_t *data, uint16_t data_len)
{
	int ret = -1;
	msg_t ms_msg;
	uint8_t *buff = NULL;

	ya_ota_type_t ya_ota_type = YA_WIFI;

	cJSON *root = NULL, *cmd = NULL, *subcmd = NULL;

	
	root = cJSON_Parse((char *)data);
	if(!root)
		goto err;
	
	cmd = cJSON_GetObjectItem(root, "data");
	if(!cmd)		
		goto err;

	subcmd = cJSON_GetObjectItem(cmd, "firmwareType");
	if(subcmd)		
	{
		if (strcmp(subcmd->valuestring, "mcu") == 0)
			ya_ota_type = YA_MCU;
	}


	buff = (uint8_t *)ya_hal_os_memory_alloc(data_len + 1);
	memset( buff, 0, (data_len + 1));
	memcpy( buff, data, data_len);
	
	memset(&ms_msg, 0, sizeof(msg_t));
	ms_msg.addr = buff;
	ms_msg.len = data_len + 1;
	
	if (ya_ota_type == YA_WIFI)
		ms_msg.type = YA_WIFI_OTA_REQUIRE;
	else
		ms_msg.type = YA_MCU_OTA_REQUIRE;
	
	ret = ya_hal_os_queue_send(&ya_ota_queue, &ms_msg, 10);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR,"hardware queue error\n");
	
		if(ms_msg.addr)
			ya_hal_os_memory_free(ms_msg.addr);
	}
	
	if(root)
		cJSON_Delete(root);

	return;

	err:

	if(root)
		cJSON_Delete(root);

	return;
}

int char_to_int(char *p, uint8_t *value)
{
	uint8_t index = 0, bytes = 0;

	for(index = 0; index<2; index++)
	{
		if(p[index]>= '0' && p[index] <= '9')
			bytes = p[index] - 48;
		else if(p[index]>= 'A' && p[index] <= 'F')
			bytes = p[index] - 55;
		else if(p[index]>= 'a' && p[index] <= 'f')
			bytes = p[index] - 87;
		else
			return -1;

		if(index == 0)
			*value = bytes << 4;
		else 
			*value += bytes;
	}

	return 0;
}

int32_t ya_report_ota_result(int32_t code)
{
	cJSON *cmd = NULL, *data = NULL;
	char *buf = NULL;
	char cur_version[YA_MAX_MCU_VERSION_LEN + 1];
	char report_value[16];
	
	memset(report_value, 0, 16);
	snprintf(report_value, 16, "%d", code);
	
	memset(cur_version, 0, YA_MAX_MCU_VERSION_LEN + 1);	
	ya_get_ver_string(cur_version);

	cmd = cJSON_CreateObject();
	if(!cmd)
	{	
		return -1;
	}
	
	data = cJSON_CreateObject();
	if(!data)
	{
		if(cmd)
			cJSON_Delete(cmd);
		return -1;
	}

	cJSON_AddStringToObject(cmd, "method", "thing.service.device.upgrade.report");
	cJSON_AddItemToObject(cmd, "data", data);

	cJSON_AddStringToObject(data, "otaStep", report_value);
	cJSON_AddStringToObject(data, "firmwareType", "module");
	cJSON_AddStringToObject(data, "currentVersion", cur_version);
	cJSON_AddStringToObject(data, "newVersion", new_ota_version);

	buf = cJSON_PrintUnformatted(cmd);	

	if(buf)
	{
		cloud_report_msg((uint8_t *)buf, strlen(buf));
		ya_hal_os_memory_free(buf);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return 0;
}

int32_t ya_report_mcu_ota_result(int32_t code)
{
	cJSON *cmd = NULL, *data = NULL;
	char *buf = NULL;
	char cur_version[YA_MAX_MCU_VERSION_LEN + 1];
	char report_value[16];
	
	memset(report_value, 0, 16);
	snprintf(report_value, 16, "%d", code);
	
	memset(cur_version, 0, YA_MAX_MCU_VERSION_LEN + 1);	
	ya_get_mcu_ver_string(cur_version);

	cmd = cJSON_CreateObject();
	if(!cmd)
	{	
		return -1;
	}
	
	data = cJSON_CreateObject();
	if(!data)
	{
		if(cmd)
			cJSON_Delete(cmd);
		return -1;
	}

	cJSON_AddStringToObject(cmd, "method", "thing.service.device.upgrade.report");
	cJSON_AddItemToObject(cmd, "data", data);

	cJSON_AddStringToObject(data, "otaStep", report_value);
	cJSON_AddStringToObject(data, "firmwareType", "mcu");
	cJSON_AddStringToObject(data, "currentVersion", cur_version);
	cJSON_AddStringToObject(data, "newVersion", ya_http_mcu_ota_url_para.ota_ver);

	if(code == 100)
		ya_set_mcu_ver_string(ya_http_mcu_ota_url_para.ota_ver);

	buf = cJSON_PrintUnformatted(cmd);	

	if(buf)
	{
		cloud_report_msg((uint8_t *)buf, strlen(buf));
		ya_hal_os_memory_free(buf);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return 0;
}


void ya_ota_handle_onoffline(uint8_t *data, uint16_t data_len)
{
	if(data_len > 0)
	{
		if(data[0] == YA_CLOUD_ONLINE)
		{
			//update ota result
			if(ota_result_upload_enable == YA_NORMAL_WITH_OTA_SUCCESS)
			{
				ya_report_ota_result(YA_HTTP_SUCCESS);
				ota_result_upload_enable = 0;
			}else if(ota_result_upload_enable == YA_NORMAL_WITH_OTA_FAILE)
			{
				ya_report_ota_result(YA_HTTP_OTA_FAILED);
				ota_result_upload_enable = 0;
			}
		}
	}
}

int32_t ya_write_ota_enable_into_flash(ya_http_ota_url_para_t *ya_http_para)
{
	uint8_t buf[64];

	memset(buf, 0, 64);
	buf[0] = 1;
	ya_printf(C_LOG_INFO, "write ver: %s\n", ya_http_para->ota_ver);
	memcpy(buf+1, ya_http_para->ota_ver, strlen(ya_http_para->ota_ver));

	return ya_write_flash(YA_OTA_DATA_ADDR, buf, 64, 1);
}

int32_t ya_read_ota_result_from_flash(void)
{
	int32_t ret = -1;
	uint8_t buf[64];

	memset(buf, 0, 64);
	ret = ya_read_flash_with_fix_len(YA_OTA_DATA_ADDR, buf, 64, 1);
	if(ret != 0)
		return -1;

	if(buf[0] > 0)
	{
		//compare version
		memset(new_ota_version, 0, VERSION_LEN + 1);
		ya_get_ver_string(new_ota_version);
		
		if(memcmp(buf+1, new_ota_version, VERSION_LEN) == 0)
			ota_result_upload_enable = YA_NORMAL_WITH_OTA_SUCCESS;
		else
			ota_result_upload_enable = YA_NORMAL_WITH_OTA_FAILE;

		memset(buf, 0, 64);
		ya_write_flash(YA_OTA_DATA_ADDR, buf, 64, 1);
	}

	return 0;
}


int32_t ya_ota_require_handle(ya_ota_type_t ota_type, ya_http_ota_url_para_t *ya_http_url_para, uint8_t *buf, uint16_t buf_len)
{
	int32_t eror_code = YA_HTTP_OTA_FAILED;
	int32_t cur_ver = 0, new_ver = 0;
	int32_t ret = -1;
	uint16_t index = 0;
	
	cJSON *root = NULL, *cmd = NULL, *property = NULL;
	char *ver_pre = NULL, *cur_pre = NULL;
	
	char cur_version[YA_MAX_MCU_VERSION_LEN + 1];
	memset(cur_version, 0, YA_MAX_MCU_VERSION_LEN + 1);

	if (ota_type == YA_WIFI)
	{
		ya_get_ver_string(cur_version);
	}else
	{
		ya_get_mcu_ver_string(cur_version);
	}
	
	root = cJSON_Parse((char *)buf);
	if(!root)
		goto err;
	
	cmd = cJSON_GetObjectItem(root, "data");
	if(!cmd)		
		goto err;

	property = cJSON_GetObjectItem(cmd, "size");
	if(!property)		
		goto err;

	ya_http_url_para->ota_size = atoi(property->valuestring);

	printf("size: %d\n", ya_http_url_para->ota_size);

	property = cJSON_GetObjectItem(cmd, "version");
	if(!property)
		goto err;

	if (ota_type == YA_WIFI)
	{
		if(strlen(property->valuestring) > VERSION_LEN || strlen(property->valuestring) < 4)
			goto err;

		strcpy(ya_http_url_para->ota_ver, property->valuestring);

		if(memcmp(ya_http_url_para->ota_ver, cur_version, strlen(cur_version) - 3) != 0)
		{
			ya_printf(C_LOG_ERROR, "ota prefix not equal\n");
			goto err;
		}
		
		ver_pre = ya_http_url_para->ota_ver + strlen(cur_version) - 3;
		cur_pre = cur_version + strlen(cur_version) - 3;

		for(index = 0; index < 3; index++)
		{
			if((!(ver_pre[index] >= '0' && ver_pre[index] <= '9')) || 
				(!(cur_pre[index] >= '0' && cur_pre[index] <= '9')))
			{
				ya_printf(C_LOG_ERROR, "the last three bytes is not number\n");
				goto err;
			}
		}

		new_ver = atoi(ver_pre);
		cur_ver = atoi(cur_pre);
		if(new_ver < cur_ver)
			goto err;
		else if(new_ver == cur_ver)
		{
			if(cmd)
				cJSON_Delete(cmd);

			eror_code = YA_HTTP_SUCCESS;
			return eror_code;
		}
	}else 
	{
		if(strlen(property->valuestring) > YA_MAX_MCU_VERSION_LEN)
			goto err;

		strcpy(ya_http_url_para->ota_ver, property->valuestring);
	}


	property = cJSON_GetObjectItem(cmd, "url");
	if(!property)
		goto err;

	ya_http_url_para->ya_http_url = (char *)ya_hal_os_memory_alloc(strlen(property->valuestring) + 1);

	if(!(ya_http_url_para->ya_http_url))
		goto err;

	memset(ya_http_url_para->ya_http_url, 0, (strlen(property->valuestring) + 1));
	strcpy(ya_http_url_para->ya_http_url, property->valuestring);	
	
	//host alloc
	ya_http_url_para->ya_http_host = (char *)ya_hal_os_memory_alloc(strlen(property->valuestring) + 1);
	if(!(ya_http_url_para->ya_http_host))  goto err;
	memset(ya_http_url_para->ya_http_host, 0, (strlen(property->valuestring) + 1));

	//resource alloc
	ya_http_url_para->ya_http_resource = (char *)ya_hal_os_memory_alloc(strlen(property->valuestring) + 1);
	if(!(ya_http_url_para->ya_http_resource))  goto err;
	memset(ya_http_url_para->ya_http_resource, 0, (strlen(property->valuestring) + 1));

	ret = ya_parser_url(ya_http_url_para);
	if(ret)
		goto err;

	property = cJSON_GetObjectItem(cmd, "md5sum");
	if(!property)
		goto err;

	if(strlen(property->valuestring) < 32)
		goto err;

	for(index = 0; index < 16; index++)
	{
		ret = char_to_int(property->valuestring + index * 2, &(ya_http_url_para->md5[index]));
		if(ret != 0) goto err;
	}

	property = cJSON_GetObjectItem(cmd, "failRetryTimes");
	if(!property)
		goto err;

	ya_http_url_para->retry_num = atoi(property->valuestring);

	property = cJSON_GetObjectItem(cmd, "failRetryInternal");
	if(!property)
		goto err;

	ya_http_url_para->retry_interval = atoi(property->valuestring);

	if(ya_http_url_para->retry_interval <= 0)
		ya_http_url_para->retry_interval = 5*1000;
	else
		ya_http_url_para->retry_interval = ya_http_url_para->retry_interval*60*1000;

	if(root)
		cJSON_Delete(root);

	return C_OK;
	
	err:
	ya_printf(C_LOG_ERROR, "ya_ota_require_handle error\n");
	
	if(root)
		cJSON_Delete(root);

	return C_ERROR;	
}

typedef struct{
	ya_http_ota_status_t ya_http_state;
	uint32_t ota_file_addr;
	uint8_t *ota_buffer;
	int32_t ota_server_socket;
	int32_t eror_code;
	Ya_Timer ya_ota_retry_timer;
	uint16_t curdelta;
	uint16_t delta;
	uint32_t count;
}ya_module_ota_para_t;

ya_module_ota_para_t ya_wifi_ota_para;
ya_module_ota_para_t ya_mcu_ota_para;

static uint8_t reset_update_ota_buffer_md5 = 0;
static int8_t update_ota_buffer_md5(uint8_t *buf , int32_t len)
{
	if(!buf)
		return -1;
	static uint8_t md5_init_flag = 0;
	static mbedtls_md5_context ctx;
	static uint32_t total_len = 0;		
	if(reset_update_ota_buffer_md5)
	{
		reset_update_ota_buffer_md5 = 0;
		mbedtls_md5_free(&ctx);
		md5_init_flag = 0;
		total_len = 0;
	}
	total_len += len;
	if(md5_init_flag == 0)
	{
		md5_init_flag = 1;
		mbedtls_md5_init(&ctx);
		mbedtls_md5_starts(&ctx);		
	}
	if(len > 0)
	{
		mbedtls_md5_update(&ctx, buf, len);
	}		
	if(total_len >= ya_http_reponse_result.body_len)
	{
		uint8_t md5_clc[16] = {0};
		mbedtls_md5_finish(&ctx, md5_clc);
		mbedtls_md5_free(&ctx);
		uint16_t index = 0;
		ya_printf(C_LOG_INFO,"obj md5: \n");
		ya_printf(C_LOG_INFO,"total_len===%d\n",total_len);
		for(index = 0; index < 16; index++)
		{
			ya_printf(C_LOG_INFO,"%02x ", md5_clc[index]);
		}
		ya_printf(C_LOG_INFO,"\n");
		ya_printf(C_LOG_INFO,"cur md5: \n");
		for(index = 0; index < 16; index++)
		{
			ya_printf(C_LOG_INFO,"%02x ", ya_http_ota_url_para.md5[index]);
		}
		ya_printf(C_LOG_INFO,"\n");
		if(memcmp(ya_http_ota_url_para.md5,md5_clc,16) == 0)
		{
			ya_printf(C_LOG_INFO,"\r\ncheck OTA md5 success\r\n");
			return 1;
		}			
		else
		{
			ya_printf(C_LOG_INFO,"\r\ncheck OTA md5 fail\r\n");
			return -1;
		}			
	}
	return 0;
}
void ya_wifi_ota_download(void)
{
	int32_t ret = -1;
	uint8_t *ya_request = NULL;
	static int8_t check_md5_result = 0;
	switch(ya_wifi_ota_para.ya_http_state)
	{
		case YA_HTTP_START:
			if( ya_http_ota_url_para.ya_http_host == NULL || ya_http_ota_url_para.ya_http_resource == NULL)
			{
				ya_printf(C_LOG_ERROR, "resource or host null error");
				ya_wifi_ota_para.ya_http_state = YA_HTTP_IDLE;
			}else
			{
				if(ya_wifi_ota_para.ota_buffer == NULL)
				{
					ya_wifi_ota_para.ota_buffer = (uint8_t *)ya_hal_os_memory_alloc(HTTP_BUF_SIZE);
					if(ya_wifi_ota_para.ota_buffer == NULL)
					{
						ya_printf(C_LOG_ERROR, "data_buf null error");
						ya_wifi_ota_para.ya_http_state = YA_HTTP_IDLE;
						break;
					}
				}

				if(ya_wifi_ota_para.ota_server_socket >= 0)
				{
					close(ya_wifi_ota_para.ota_server_socket);
					ya_wifi_ota_para.ota_server_socket = -1;
				}
				ya_wifi_ota_para.ya_http_state = YA_HTTP_CONNECT;
			}
		break;
		
		case YA_HTTP_CONNECT:
			ya_printf(C_LOG_INFO, "http ota connect\n");
			reset_update_ota_buffer_md5 = 1;
			memset(&ya_http_reponse_result, 0, sizeof(ya_http_response_result_t));
			memset(new_ota_version, 0, VERSION_LEN + 1);
			strcpy(new_ota_version, ya_http_ota_url_para.ota_ver);

			ya_wifi_ota_para.eror_code = YA_DOWNLOAD_FILE_FAILED;
			ya_wifi_ota_para.ota_server_socket = ya_http_connect_server(ya_http_ota_url_para.ya_http_host, ya_http_ota_url_para.ya_http_port);
			if(ya_wifi_ota_para.ota_server_socket == -1)
			{
				ya_wifi_ota_para.ya_http_state = YA_ERROR_HANDLE;
			}
			else
			{
				ya_wifi_ota_para.ya_http_state = YA_HTTP_SEND_GET;
				ya_report_ota_result(YA_START_DOWN);
			}
		break;

		case YA_HTTP_SEND_GET:
			ya_request = (unsigned char *)ya_hal_os_memory_alloc(strlen("GET /") + strlen(ya_http_ota_url_para.ya_http_resource) + strlen(" HTTP/1.1\r\nHost: ") 
				+ strlen(ya_http_ota_url_para.ya_http_host) + strlen("\r\n\r\n") + 1);

			sprintf((char*)ya_request, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", ya_http_ota_url_para.ya_http_resource, ya_http_ota_url_para.ya_http_host);

			ret = write(ya_wifi_ota_para.ota_server_socket, ya_request, strlen((char const*)ya_request));
			if(ret < 0)
			{
				ya_wifi_ota_para.ya_http_state = YA_ERROR_HANDLE;
			}else
			{
				ya_printf(C_LOG_INFO, "http ota get:\n");
				ya_printf(C_LOG_INFO, "%s\n", ya_request);
				
				memset(ya_wifi_ota_para.ota_buffer, 0, HTTP_BUF_SIZE);
				ya_wifi_ota_para.ya_http_state = YA_HTTP_HEADER_HANDLE;
			}

			if(ya_request)
				ya_hal_os_memory_free(ya_request);
		break;

		case YA_HTTP_HEADER_HANDLE:
			ret = ya_http_header_handle(&ya_http_reponse_result, ya_wifi_ota_para.ota_server_socket, ya_wifi_ota_para.ota_buffer);
			if(ret < 0)
			{
				ya_wifi_ota_para.ya_http_state = YA_ERROR_HANDLE;
			}else if(ret > 0)	
			{
				if(ya_http_reponse_result.body_len != ya_http_ota_url_para.ota_size)
				{
					ya_printf(C_LOG_ERROR, "ota size error: %d, %d\n", ya_http_reponse_result.body_len, ya_http_ota_url_para.ota_size);
					ya_wifi_ota_para.ya_http_state = YA_ERROR_HANDLE;
				}else
				{
					ya_wifi_ota_para.ya_http_state = YA_PRE_FILE_HANDLE;
					ya_http_prefile_header_init();
					memset(&ya_http_file, 0, sizeof(http_file_para_t));

					ya_http_file.body_file_header_enbable = ya_get_file_header_writer_flash_enable();
					ya_http_file.body_read_len = ya_http_reponse_result.body_read_len;
					ya_http_file.file_len = ya_http_reponse_result.body_len;
					ya_http_file.header_len = ya_http_reponse_result.header_len;
					check_md5_result = update_ota_buffer_md5(ya_wifi_ota_para.ota_buffer+ya_http_file.header_len,ya_http_file.body_read_len);
				}
			}
		break;

		case YA_PRE_FILE_HANDLE:				
			ret = ya_http_prefile_header_handle(&ya_http_file, ya_wifi_ota_para.ota_server_socket, ya_wifi_ota_para.ota_buffer);
			if(ret < 0)
			{
				ya_wifi_ota_para.ya_http_state = YA_ERROR_HANDLE;
			}else if(ret > 0)	
			{
				ya_wifi_ota_para.ya_http_state = YA_EARSE_FLASH;
			}else if(ret == 0)
			{
				check_md5_result = update_ota_buffer_md5(ya_wifi_ota_para.ota_buffer,ya_http_file.body_read_len);
			}
		break;

		case YA_EARSE_FLASH:
			ya_printf(C_LOG_INFO, "ota_file_addr: %08x, ota len: %d\n", ya_wifi_ota_para.ota_file_addr, ya_http_file.file_len);

			ret = ya_http_ota_sector_erase(ya_wifi_ota_para.ota_file_addr, ya_http_file.file_len);
			if(ret != 0)
				ya_wifi_ota_para.ya_http_state = YA_ERROR_HANDLE;
			else
			{
				ya_printf(C_LOG_INFO, "erase flash success\n");
				ya_wifi_ota_para.ya_http_state = YA_HTTP_BODY_HANDLE;
				ya_wifi_ota_para.curdelta = 0;
				ya_wifi_ota_para.delta = 0;
				ya_report_ota_result(YA_ERASE_FLASH_OK);
			}
		break;

		case YA_HTTP_BODY_HANDLE:
			ret = ya_http_body_handle(&ya_http_file, ya_wifi_ota_para.ota_server_socket, ya_wifi_ota_para.ota_buffer, ya_wifi_ota_para.ota_file_addr);
			if(ret < 0)
				ya_wifi_ota_para.ya_http_state = YA_ERROR_HANDLE;
			else if(ret > 0)
			{
				ya_printf(C_LOG_INFO, "download the file success: %d\n", ya_http_file.body_cur_pos);
				ya_report_ota_result(YA_DOWNLOAD_OK);
				if(ya_get_download_whole_file_finish() == 0)
				{
					if(check_md5_result == 1)
					{
				ya_wifi_ota_para.ya_http_state = YA_HTTP_CHECKSUM;
					}
					else
					{
						ya_printf(C_LOG_INFO, "\n check OTA file MD5 error\n");
						ya_wifi_ota_para.ya_http_state = YA_ERROR_HANDLE;
					}					
				}
				else
				{
					for(;;)
					{
						ya_http_file.body_read_len = read(ya_wifi_ota_para.ota_server_socket, ya_wifi_ota_para.ota_buffer, HTTP_BUF_SIZE);
						if(ya_http_file.body_read_len < 0)
						{
							ya_printf(C_LOG_INFO, "read ota file error !!!\n");
							ya_wifi_ota_para.ya_http_state = YA_ERROR_HANDLE;
							break;
						}	
						check_md5_result = update_ota_buffer_md5(ya_wifi_ota_para.ota_buffer,ya_http_file.body_read_len);
						if(check_md5_result == 1)
						{
							ya_wifi_ota_para.ya_http_state = YA_HTTP_CHECKSUM;
							break;
						}
						else if(check_md5_result == -1)
						{
							ya_printf(C_LOG_INFO, "\ncheck OTA file MD5 error\n");
							ya_wifi_ota_para.ya_http_state = YA_ERROR_HANDLE;
							break;
						}	
					}
				}
			}
			else 
			{
				if(ya_http_file.file_len >	ya_http_file.body_cur_pos)
				{
					ya_wifi_ota_para.delta =  (ya_http_file.body_cur_pos * 100)/ya_http_file.file_len;
					if(ya_wifi_ota_para.delta - ya_wifi_ota_para.curdelta > 10)
					{
						ya_wifi_ota_para.curdelta = ya_wifi_ota_para.delta;
						if((YA_ERASE_FLASH_OK + ya_wifi_ota_para.delta/2) < YA_DOWNLOAD_OK)
							ya_report_ota_result(YA_ERASE_FLASH_OK + ya_wifi_ota_para.delta/2);
					}	
				}

				check_md5_result = update_ota_buffer_md5(ya_wifi_ota_para.ota_buffer,ya_http_file.body_read_len);
			}
		break;

		case YA_HTTP_CHECKSUM:
			ret = ya_verify_ota_checksum(ya_wifi_ota_para.ota_file_addr,ya_http_file.file_len,ya_http_file.body_file_header, ya_http_ota_url_para.md5, ya_wifi_ota_para.ota_buffer);
			if(ret != 0)
			{
				ya_wifi_ota_para.eror_code = YA_CHECK_FILE_FAILED;
				ya_wifi_ota_para.ya_http_state = YA_ERROR_HANDLE;
			}else
			{
				ya_report_ota_result(YA_CHECKSUM_OK);
				ya_wifi_ota_para.ya_http_state = YA_WAIT_TO_REBOOT;
			}
		break;

		case YA_ERROR_HANDLE:
			if(ya_wifi_ota_para.ota_server_socket >= 0)
			{
				close(ya_wifi_ota_para.ota_server_socket);
				ya_wifi_ota_para.ota_server_socket = -1;
			}

			if(ya_wifi_ota_para.ota_buffer)
			{
				ya_hal_os_memory_free(ya_wifi_ota_para.ota_buffer);
				ya_wifi_ota_para.ota_buffer = NULL;
			}
			reset_update_ota_buffer_md5 = 1;
			if(ya_wifi_ota_para.count < ya_http_ota_url_para.retry_num)
			{
				ya_printf(C_LOG_INFO, "try again: %d, wait interval: %d\n", ya_wifi_ota_para.count, ya_http_ota_url_para.retry_interval);
				ya_wifi_ota_para.count++;
				ya_wifi_ota_para.ya_http_state = YA_HTTP_RETRY_WAIT;
				ya_init_timer(&ya_wifi_ota_para.ya_ota_retry_timer);
				ya_countdown_ms(&ya_wifi_ota_para.ya_ota_retry_timer, ya_http_ota_url_para.retry_interval);

			}else
			{
				ya_wifi_ota_para.ya_http_state = YA_HTTP_IDLE;
				//release the url host and resource
				ya_free_http_url_resource(&ya_http_ota_url_para);
				
				if(ya_wifi_ota_para.eror_code < 0)
					ya_report_ota_result(ya_wifi_ota_para.eror_code);
			}
		break;

		case YA_HTTP_RETRY_WAIT:
			if(ya_has_timer_expired(&ya_wifi_ota_para.ya_ota_retry_timer) == C_OK)
				ya_wifi_ota_para.ya_http_state = YA_HTTP_START;
		break;

		case YA_HTTP_IDLE:

		break;

		case YA_WAIT_TO_REBOOT:
			ya_delay(2000);
			ya_hal_ota_done();
		break;

		default:

		break;
	}

}

int32_t ya_wifi_ota_init(void)
{
	ota_result_upload_enable = 0;

	if(ya_wifi_ota_para.ota_server_socket >= 0)
	{
		close(ya_wifi_ota_para.ota_server_socket);
		ya_wifi_ota_para.ota_server_socket = -1;
	}
	ya_free_http_url_resource(&ya_http_ota_url_para);
	memset(&ya_http_ota_url_para, 0, sizeof(ya_http_ota_url_para_t));

	ya_wifi_ota_para.count = 0;

	return 0;
}

int32_t ya_mcu_ota_init(void)
{
	mcu_ota_result_upload_enable = 0;

	if(ya_mcu_ota_para.ota_server_socket >= 0)
	{
		close(ya_mcu_ota_para.ota_server_socket);
		ya_mcu_ota_para.ota_server_socket = -1;
	}
	ya_free_http_url_resource(&ya_http_mcu_ota_url_para);
	memset(&ya_http_mcu_ota_url_para, 0, sizeof(ya_http_ota_url_para_t));

	ya_mcu_ota_para.count = 0;

	return 0;
}

void ya_mcu_ota_download(void)
{
	int32_t ret = -1;
	uint8_t *ya_request = NULL;
	
	switch(ya_mcu_ota_para.ya_http_state)
	{
		case YA_HTTP_START:
			if( ya_http_mcu_ota_url_para.ya_http_host == NULL || ya_http_mcu_ota_url_para.ya_http_resource == NULL)
			{
				ya_printf(C_LOG_ERROR, "resource or host null error");

				ya_mcu_ota_para.ya_http_state = YA_HTTP_IDLE;
			}else
			{
				if(ya_mcu_ota_para.ota_buffer == NULL)
				{
					ya_mcu_ota_para.ota_buffer = (uint8_t *)ya_hal_os_memory_alloc(HTTP_BUF_SIZE);
					if(ya_mcu_ota_para.ota_buffer == NULL)
					{
						ya_printf(C_LOG_ERROR, "data_buf null error");

						ya_mcu_ota_para.ya_http_state = YA_HTTP_IDLE;
						break;
					}
				}

				if(ya_mcu_ota_para.ota_server_socket >= 0)
				{
					close(ya_mcu_ota_para.ota_server_socket);
					ya_mcu_ota_para.ota_server_socket = -1;
				}
				ya_mcu_ota_para.ya_http_state = YA_HTTP_CONNECT;
			}
		break;
		
		case YA_HTTP_CONNECT:
			ya_printf(C_LOG_INFO, "http ota connect\n");
			memset(&ya_http_mcu_reponse_result, 0, sizeof(ya_http_response_result_t));

			ya_mcu_ota_para.eror_code = YA_DOWNLOAD_FILE_FAILED;
			ya_mcu_ota_para.ota_server_socket = ya_http_connect_server(ya_http_mcu_ota_url_para.ya_http_host, ya_http_mcu_ota_url_para.ya_http_port);
			if(ya_mcu_ota_para.ota_server_socket == -1)
			{
				ya_mcu_ota_para.ya_http_state = YA_ERROR_HANDLE;
			}
			else
			{
				ya_mcu_ota_para.ya_http_state = YA_HTTP_SEND_GET;
			}
		break;

		case YA_HTTP_SEND_GET:
			ya_request = (unsigned char *)ya_hal_os_memory_alloc(strlen("GET /") + strlen(ya_http_mcu_ota_url_para.ya_http_resource) + strlen(" HTTP/1.1\r\nHost: ") 
				+ strlen(ya_http_mcu_ota_url_para.ya_http_host) + strlen("\r\n\r\n") + 1);

			sprintf((char*)ya_request, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", ya_http_mcu_ota_url_para.ya_http_resource, ya_http_mcu_ota_url_para.ya_http_host);

			ret = write(ya_mcu_ota_para.ota_server_socket, ya_request, strlen((char const*)ya_request));
			if(ret < 0)
			{
				ya_mcu_ota_para.ya_http_state = YA_ERROR_HANDLE;
			}else
			{
				ya_printf(C_LOG_INFO, "http ota get:\n");
				ya_printf(C_LOG_INFO, "%s\n", ya_request);
				
				memset(ya_mcu_ota_para.ota_buffer, 0, HTTP_BUF_SIZE);
				ya_mcu_ota_para.ya_http_state = YA_HTTP_HEADER_HANDLE;
			}

			if(ya_request)
				ya_hal_os_memory_free(ya_request);
		break;

		case YA_HTTP_HEADER_HANDLE:
			ret = ya_http_header_handle(&ya_http_mcu_reponse_result, ya_mcu_ota_para.ota_server_socket, ya_mcu_ota_para.ota_buffer);
			if(ret < 0)
			{
				ya_mcu_ota_para.ya_http_state = YA_ERROR_HANDLE;
			}else if(ret > 0)	
			{
				if(ya_http_mcu_reponse_result.body_len != ya_http_mcu_ota_url_para.ota_size)
				{
					ya_printf(C_LOG_ERROR, "ota size error: %d, %d\n", ya_http_mcu_reponse_result.body_len, ya_http_mcu_ota_url_para.ota_size);

					ya_mcu_ota_para.ya_http_state = YA_ERROR_HANDLE;
				}else
				{
					ya_mcu_ota_para.ya_http_state = YA_EARSE_FLASH;
					ya_http_prefile_header_init();
					memset(&ya_http_mcu_file, 0, sizeof(http_file_para_t));

					ya_http_mcu_file.body_file_header_enbable = 1;
					ya_http_mcu_file.body_read_len = ya_http_mcu_reponse_result.body_read_len;
					ya_http_mcu_file.file_len = ya_http_mcu_reponse_result.body_len;
					ya_http_mcu_file.header_len = ya_http_mcu_reponse_result.header_len;
				}
			}
		break;

		case YA_EARSE_FLASH:
			ya_printf(C_LOG_INFO, "ota_file_addr: %08x, ota len: %d\n", ya_mcu_ota_para.ota_file_addr, ya_http_mcu_file.file_len);

			ret = ya_http_ota_sector_erase(ya_mcu_ota_para.ota_file_addr, ya_http_mcu_file.file_len);
			if(ret != 0)
				ya_mcu_ota_para.ya_http_state = YA_ERROR_HANDLE;
			else
			{
				ya_printf(C_LOG_INFO, "erase flash success\n");
				ya_mcu_ota_para.ya_http_state = YA_HTTP_BODY_HANDLE;
				ya_mcu_ota_para.curdelta = 0;
				ya_mcu_ota_para.delta = 0;
			}
		break;

		case YA_HTTP_BODY_HANDLE:
			ret = ya_http_body_handle(&ya_http_mcu_file, ya_mcu_ota_para.ota_server_socket, ya_mcu_ota_para.ota_buffer, ya_mcu_ota_para.ota_file_addr);
			if(ret < 0)
				ya_mcu_ota_para.ya_http_state = YA_ERROR_HANDLE;
			else if(ret > 0)
			{
				ya_printf(C_LOG_INFO, "download the file success: %d\n", ya_http_mcu_file.body_cur_pos);
				ya_mcu_ota_para.ya_http_state = YA_HTTP_CHECKSUM;
			}
		break;

		case YA_HTTP_CHECKSUM:
			ret = ya_verify_mcu_file_checksum(ya_mcu_ota_para.ota_file_addr,ya_http_mcu_file.file_len,ya_http_mcu_ota_url_para.md5, ya_mcu_ota_para.ota_buffer);
			if(ret != 0)
			{
				ya_mcu_ota_para.eror_code = YA_CHECK_FILE_FAILED;
				ya_mcu_ota_para.ya_http_state = YA_ERROR_HANDLE;
			}else
			{
				ya_mcu_ota_para.ya_http_state = YA_NOTIFY_MCU;
			}
		break;

		case YA_ERROR_HANDLE:
			if(ya_mcu_ota_para.ota_server_socket >= 0)
			{
				close(ya_mcu_ota_para.ota_server_socket);
				ya_mcu_ota_para.ota_server_socket = -1;
			}

			if(ya_mcu_ota_para.ota_buffer)
			{
				ya_hal_os_memory_free(ya_mcu_ota_para.ota_buffer);
				ya_mcu_ota_para.ota_buffer = NULL;
			}
			
			if(ya_mcu_ota_para.count < ya_http_mcu_ota_url_para.retry_num)
			{
				ya_printf(C_LOG_INFO, "try again: %d, wait interval: %d\n", ya_mcu_ota_para.count, ya_http_mcu_ota_url_para.retry_interval);
				ya_mcu_ota_para.count++;
				ya_mcu_ota_para.ya_http_state = YA_HTTP_RETRY_WAIT;
				ya_init_timer(&ya_mcu_ota_para.ya_ota_retry_timer);
				ya_countdown_ms(&ya_mcu_ota_para.ya_ota_retry_timer, ya_http_mcu_ota_url_para.retry_interval);

			}else
			{
				ya_mcu_ota_para.ya_http_state = YA_HTTP_IDLE;
				//release the url host and resource
				ya_free_http_url_resource(&ya_http_mcu_ota_url_para);
				
				if(ya_mcu_ota_para.eror_code < 0)
					ya_report_mcu_ota_result(ya_mcu_ota_para.eror_code);
			}
		break;

		case YA_HTTP_RETRY_WAIT:
			if(ya_has_timer_expired(&ya_mcu_ota_para.ya_ota_retry_timer) == C_OK)
				ya_mcu_ota_para.ya_http_state = YA_HTTP_START;
		break;

		case YA_HTTP_IDLE:

		break;


		case YA_NOTIFY_MCU:
			// do job here
			ya_mcu_download_finish(ya_http_mcu_ota_url_para.ota_size, ya_http_mcu_ota_url_para.ota_ver);

			if(ya_mcu_ota_para.ota_server_socket >= 0)
			{
				close(ya_mcu_ota_para.ota_server_socket);
				ya_mcu_ota_para.ota_server_socket = -1;
			}

			if(ya_mcu_ota_para.ota_buffer)
			{
				ya_hal_os_memory_free(ya_mcu_ota_para.ota_buffer);
				ya_mcu_ota_para.ota_buffer = NULL;
			}

			ya_mcu_ota_para.ya_http_state = YA_HTTP_IDLE;
			//release the url host and resource
			ya_free_http_url_resource(&ya_http_mcu_ota_url_para);
		break;

		default:

		break;
	}
}

void ya_http_ota_app(void *arg)
{
	int32_t ret = -1;
	msg_t ms_msg;

	memset(&ya_wifi_ota_para, 0, sizeof(ya_module_ota_para_t));
	ya_wifi_ota_para.ota_server_socket = -1;
	
	memset(&ya_mcu_ota_para, 0, sizeof(ya_module_ota_para_t));
	ya_mcu_ota_para.ota_server_socket = -1;
	
	ya_wifi_ota_para.ota_file_addr = ya_get_ota_download_addr();
	ya_mcu_ota_para.ota_file_addr = ya_mcu_ota_download_addr();

	ya_read_ota_result_from_flash();

	// wifi ota para init
	memset(&ya_http_ota_url_para, 0, sizeof(ya_http_ota_url_para_t));
	memset(&ya_http_reponse_result, 0, sizeof(ya_http_response_result_t));

	// mcu ota para init
	memset(&ya_http_mcu_ota_url_para, 0, sizeof(ya_http_ota_url_para_t));
	memset(&ya_http_mcu_reponse_result, 0, sizeof(ya_http_response_result_t));

	printf("start ota app success ok\n");
	
	while(1)
	{
		ya_wifi_ota_download();
		ya_mcu_ota_download();

		memset(&ms_msg, 0, sizeof(msg_t));
		
		ret = ya_hal_os_queue_recv(&ya_ota_queue, &ms_msg, 20);
		if(ret == C_OK)
		{
			ya_printf(C_LOG_INFO, "start http ota\n");
			switch(ms_msg.type)
			{
				case YA_WIFI_OTA_REQUIRE:
				{
					ya_printf(C_LOG_INFO, "wifi ota\n");
					if (ya_wifi_ota_para.ota_file_addr == 0)
						ya_report_ota_result(-1);
				
					ya_wifi_ota_init();
					
					ret = ya_ota_require_handle(YA_WIFI, &ya_http_ota_url_para, ms_msg.addr, ms_msg.len);
					if(ret < 0 || ret == YA_HTTP_SUCCESS)
					{
						ya_free_http_url_resource(&ya_http_ota_url_para);
						ya_report_ota_result(ret);
					}else
					{
						ret = ya_write_ota_enable_into_flash(&ya_http_ota_url_para);
						if (ret != 0)
						{
							ya_report_ota_result(YA_HTTP_OTA_FAILED);
						}else
						{
							ya_wifi_ota_para.ya_http_state = YA_HTTP_START;
						}
					}
				}
				break;

				case YA_MCU_OTA_REQUIRE:
				{
					ya_printf(C_LOG_INFO, "mcu ota\n");
					if (ya_mcu_ota_para.ota_file_addr == 0 || ya_get_mcu_ota_enbale() == 0)
						ya_report_mcu_ota_result(-1);
						
					ya_mcu_ota_init();

					ret = ya_ota_require_handle(YA_MCU, &ya_http_mcu_ota_url_para, ms_msg.addr, ms_msg.len);
					if(ret < 0 || ret == YA_HTTP_SUCCESS)
					{
						ya_free_http_url_resource(&ya_http_mcu_ota_url_para);
						ya_report_mcu_ota_result(ret);
					}else
					{
						ya_mcu_ota_para.ya_http_state = YA_HTTP_START;
					}
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

int ya_start_ota_app(void)
{
	int32_t ret = 0;

	ret = ya_hal_os_queue_create(&ya_ota_queue, "ya_ota_queue queue", sizeof(msg_t), 5);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create os queue error\n");
		return -1;
	}

	ret = ya_hal_os_thread_create(&ya_ota_app, "ya_ota_hardware thread", ya_http_ota_app, 0, (2 * 1024), 5);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create ya_ota_hardware error\n");
		return -1;
	}		

	cloud_add_event_listener("ya_http_ota_app thread", CLOUD_UPGRADE_EVENT, ya_ota_handle_command);
	cloud_add_event_listener("ya_http_ota_app thread", CLOUD_ONOFF_EVENT, ya_ota_handle_onoffline);

	return 0;
}

