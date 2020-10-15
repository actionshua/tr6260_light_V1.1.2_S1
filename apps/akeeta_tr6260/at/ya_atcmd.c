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

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "ya_common.h"
#include "ya_hal_os.h"
#include "cJSON.h"
#include "ya_config.h"
#include "ya_atcmd.h"
#include "ya_common_func.h"
#include "infra_defs.h"


typedef enum{
	CMD_RET_DATA_ERROR   = -1,
	ya_CMD_RET_SUCCESS,
	ya_CMD_RET_FAILURE,
}cmd_hanle_t;

#define RESP_HEAD 		"rsp:"
#define RESP_ERR 		"rsp:ERROR"
#define RESP_OK 		"rsp:OK"
#define GET_MAC_BACK 	"getmac"
#define RESP_NO_LICENSE "rsp:0,0,0"

#define CMD_BUFFER_SIZE  (8*1024)
static uint8_t *pcmd_buffer = NULL;
static uint32_t pcmd_buffer_index = 0;
static uint8_t md5_string[33] = {0};
static uint8_t get_md5_string_flag = 0;
static uint8_t enter_get_rssi_mode = 0;
void set_enter_get_rssi_mode(void)
{
	enter_get_rssi_mode = 1;
}
void clear_enter_get_rssi_mode(void)
{
	enter_get_rssi_mode = 0;
}
uint8_t check_enter_get_rssi_mode(void)
{
	return enter_get_rssi_mode;
}

extern int32_t ya_set_factory_rssi_info(uint8_t *rssi_name);
typedef enum
{
	AWS_CERT = 0,
	ALI_CERT,
	UNKNOWN_CERT
}ya_cert_t;
uint8_t license_md5_string(uint8_t *md5_string)
{
	if(!md5_string)
	{	
		ya_printf(C_LOG_ERROR,"md5_string error\n");
		return C_ERROR;
	}
	if(get_md5_string_flag == 1)
		return C_OK;
	cJSON *license_key_US = NULL, *license_US_info = NULL, *license_key_CN = NULL, *license_CN_info = NULL;
	uint8_t *buf_US = NULL, *buf_CN = NULL, *buf_all = NULL;
	uint8_t return_ret = C_OK;
	license_key_US = cJSON_CreateObject();
	if(!license_key_US)
	{	
		ya_printf(C_LOG_ERROR,"cJSON_CreateObject license_key_US error\n");
		return_ret =  C_ERROR;
		goto exit;
	}
	license_US_info = cJSON_CreateObject();
	if(!license_US_info)
	{
		ya_printf(C_LOG_ERROR,"cJSON_CreateObject license_US_info error\n");
		return_ret =  C_ERROR;
		goto exit;
	}
	
	license_key_CN = cJSON_CreateObject();
	if(!license_key_CN)
	{	
		ya_printf(C_LOG_ERROR,"cJSON_CreateObject license_key_CN error\n");
		return_ret =  C_ERROR;
		goto exit;
	}
	license_CN_info = cJSON_CreateObject();
	if(!license_CN_info)
	{
		ya_printf(C_LOG_ERROR,"cJSON_CreateObject license_CN_info error\n");
		return_ret =  C_ERROR;
		goto exit;
	}

	cJSON_AddStringToObject(license_key_US, "server", "US");
	cJSON_AddItemToObject(license_key_US, "license", license_US_info);
	if((ya_get_client_cert() != NULL)&&(ya_aws_get_client_id() != NULL)&&(ya_get_private_key() != NULL)&&(ya_aws_get_thing_type() != NULL))
	{
		cJSON_AddStringToObject(license_US_info, "client_cert", ya_get_client_cert());
		cJSON_AddStringToObject(license_US_info, "client_id", ya_aws_get_client_id());
		cJSON_AddStringToObject(license_US_info, "private_key", ya_get_private_key());
		cJSON_AddStringToObject(license_US_info, "thing_type", ya_aws_get_thing_type());
	}
	else
	{	
		ya_printf(C_LOG_ERROR,"license_key_US not exit\n");
		return_ret =  C_ERROR;
		goto exit;
	}

	cJSON_AddStringToObject(license_key_CN, "server", "CN");
	cJSON_AddItemToObject(license_key_CN, "license", license_CN_info);
	uint8_t product_key[IOTX_PRODUCT_KEY_LEN + 1] = {0};
	uint16_t get_productKey_ret = ya_get_productKey(product_key);
	if(get_productKey_ret > 0)
	{
		cJSON_AddStringToObject(license_CN_info, "product_key", product_key);
	}
	else
	{	
		ya_printf(C_LOG_ERROR,"get_productKey_ret <= 0\n");
		return_ret =  C_ERROR;
		goto exit;
	}
	uint8_t device_id[YA_DEVICE_ID_LEN + 1] = {0};
	uint16_t get_deviceID_ret = ya_get_deviceID(device_id);
	if(get_deviceID_ret > 0)
	{
		cJSON_AddStringToObject(license_CN_info, "device_id", device_id);
	}
	else
	{	
		ya_printf(C_LOG_ERROR,"get_deviceID_ret <= 0\n");
		return_ret =  C_ERROR;
		goto exit;
	}

	uint8_t device_secret[YA_DEVICE_SECRET_LEN + 1] = {0};
	uint16_t get_device_secret_ret = ya_get_deviceSecret(device_secret);
	if(get_device_secret_ret > 0)
	{
		cJSON_AddStringToObject(license_CN_info, "device_secret", device_secret);
	}	
	else
	{	
		ya_printf(C_LOG_ERROR,"get_device_secret_ret <= 0\n");
		return_ret =  C_ERROR;
		goto exit;
	}

	buf_US = cJSON_PrintUnformatted(license_key_US);

	buf_CN = cJSON_PrintUnformatted(license_key_CN);

	buf_all = (uint8_t *)ya_hal_os_memory_alloc(strlen(buf_CN)+strlen(buf_US)+1);
	if(!buf_all)
	{
		ya_printf(C_LOG_ERROR,"ya_hal_os_memory_alloc error\n");
		return_ret =  C_ERROR;
		goto exit;
	}
	memset(buf_all,0,strlen(buf_CN)+strlen(buf_US)+3);
	memcpy(buf_all,buf_CN,strlen(buf_CN));
	memcpy(buf_all+strlen(buf_CN),buf_US,strlen(buf_US));
	buf_all[strlen(buf_CN)+strlen(buf_US)] = '\0';
	uint8_t license_md5[16] = {0};
	ya_mbedtls_md5(buf_all,license_md5,strlen(buf_CN)+strlen(buf_US));
	HexArrayToString(license_md5, 16, md5_string);	
	get_md5_string_flag = 1;
exit:
	if(license_key_US)
	{
		cJSON_Delete(license_key_US);
		license_key_US = NULL;
	}
	if(license_key_CN)
	{
		cJSON_Delete(license_key_CN);
		license_key_CN = NULL;
	}
	if(buf_US)
	{
		ya_hal_os_memory_free(buf_US);
		buf_US = NULL;
	}
	if(buf_CN)
	{
		ya_hal_os_memory_free(buf_CN);
		buf_CN = NULL;
	}
	if(buf_all)
	{
		ya_hal_os_memory_free(buf_all);
		buf_all = NULL;
	}
	return return_ret;
}

static int at_cmd_hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}
uint8_t at_cmd_LOG_FLAG = 1;

void at_cmd_log_ON(void)
{
	at_cmd_LOG_FLAG = 1;
}
void at_cmd_log_OFF(void)
{
	at_cmd_LOG_FLAG = 0;
}

void at_response(char * str)
{
	ya_printf(C_AT_CMD,"%s\n", str);
}
static uint16_t crc_check(uint8_t *puchMsg, uint16_t usDataLen)
{
	uint16_t wCRCin = 0xFFFF;
	uint16_t wCPoly = 0x1021;
	uint8_t wChar = 0;
	int i = 0;

	while(usDataLen--) 
	{
		wChar = *(puchMsg++);
		wCRCin ^= (wChar << 8);
		for(i = 0;i < 8;i++)
		{
			if(wCRCin & 0x8000)
				wCRCin = (wCRCin << 1) ^ wCPoly;
		else
			wCRCin = wCRCin << 1;
		}
	}
	return (wCRCin) ;
}
static char *mystrtok(char *s,const char *delim) 
{
    static char *last;
    char *tok;
    char *ucdelim;
    char *spanp;
    int c,sc;
    if (s == NULL && (s = last) == NULL)
        return NULL;
    int found = 0;
    cont:
    c=*s++;
    for (spanp = (char *)delim;(sc = *spanp++) != 0;)
    {
        if (c == sc)
            goto cont;
    }
    if (c == 0) 
    {
        last = NULL;
        return NULL;
    }

    tok = s-1;
    while (!found && *s != '\0') 
    {
        ucdelim = (char *) delim;
        while (*ucdelim) 
        {
            if (*s == *ucdelim) 
            {
                found = 1;
                *s = '\0';
                last = s + 1;
                break;
            }
            ucdelim++;
        }
        if (!found)
        {
            s++;
            if(*s=='\0')
                last = NULL;
        }
    }

    return tok;
}

uint8_t ya_at_config_flag = 0;

int ya_data_atcmd_handler(void)
{
	int32_t at_cmd_ret = CMD_RET_DATA_ERROR;
	uint8_t i = 0;
	uint16_t json_len = 0;
	uint16_t token_len = 0;
	uint16_t arg1_len = 0;
	uint16_t arg2_len = 0;
	uint16_t arg3_len = 0;
	uint8_t *token = NULL;
	uint32_t crc16_arg = 0;
	uint32_t crc16_cal = 0;
	cJSON *JSObject = NULL;
	cJSON *license_ojb = NULL;
	cJSON *sub_object = NULL;
	ya_cert_t cert_type = UNKNOWN_CERT;
	uint16_t pos = 0;
	uint8_t index = 0;
	uint8_t mac[6]={0};
	char tmp_str[16]={0};
	uint8_t str_buf[48]={0};
	uint16_t crc_16;
		
	if((strcmp(pcmd_buffer, "getmac") == 0))
	{
		at_cmd_log_OFF();
		memset(mac,0,sizeof(mac));
		memset(tmp_str,0,sizeof(tmp_str));
		memset(str_buf,0,sizeof(str_buf));
		ya_hal_wlan_get_mac_address(mac);

		snprintf(tmp_str, sizeof(tmp_str), "%02x%02x%02x%02x%02x%02x", mac[0],mac[1],
												mac[2],mac[3],mac[4],mac[5]);

		crc_16 = crc_check(mac, 6);

		snprintf(str_buf, sizeof(str_buf), "%s%s%04x", RESP_HEAD, tmp_str, crc_16);
		at_response(GET_MAC_BACK);
		at_response(str_buf);

		at_cmd_ret = ya_CMD_RET_SUCCESS; 
	}	
	else if((strcmp(pcmd_buffer, "getversion") == 0))
	{
		at_cmd_log_OFF();
		memset(str_buf,0,sizeof(str_buf));
		strcpy(str_buf, RESP_HEAD);
		ya_get_ver_string(str_buf + strlen(RESP_HEAD));
		at_response(str_buf);
		at_cmd_ret = ya_CMD_RET_SUCCESS; 
	}
	else if((strcmp(pcmd_buffer, "yasoftap") == 0))
	{
		at_response(RESP_OK);
		ya_set_ap_mode();
		at_cmd_ret = ya_CMD_RET_SUCCESS; 
	}
	else if((strcmp(pcmd_buffer, "yasniffer") == 0))
	{
		at_response(RESP_OK);
		ya_set_sniffer_mode();
		at_cmd_ret = ya_CMD_RET_SUCCESS; 
	}
	else if((strcmp(pcmd_buffer, "yareset") == 0))
	{
		at_response(RESP_OK);
		ya_hal_sys_reboot();
		at_cmd_ret = ya_CMD_RET_SUCCESS; 
	}
	else if((strcmp(pcmd_buffer, "yafac") == 0))
	{
		at_response(RESP_OK);
		ya_reset_module_to_factory();
		at_cmd_ret = ya_CMD_RET_SUCCESS; 
	}
	else if(strcmp(pcmd_buffer, "getlicense") == 0)
	{
		int ret_us = ya_check_us_license_exit();
		int ret_cn = ya_check_cn_license_exit();

		at_cmd_ret = ya_CMD_RET_SUCCESS; 
		if (ret_us == 0 && ret_cn == 0)
		{
			uint8_t product_key[IOTX_PRODUCT_KEY_LEN + 1] = {0};
			uint8_t device_ID[YA_DEVICE_ID_LEN + 1] = {0};
			uint16_t get_productKey_ret = ya_get_productKey(product_key);
			if(get_productKey_ret > 0)
			{
				uint8_t response_buf[IOTX_PRODUCT_KEY_LEN + 1 +YA_DEVICE_ID_LEN + 1 + 5 + 34] = {0};
				memcpy(response_buf,"rsp:",strlen("rsp:"));
				memcpy(response_buf+4,product_key,get_productKey_ret);
				memcpy(response_buf+4+get_productKey_ret,",",1);
				uint16_t get_deviceID_ret = ya_get_deviceID(device_ID);
				if(get_deviceID_ret > 0)
				{
					memcpy(response_buf+4+get_productKey_ret+1,device_ID,get_deviceID_ret);
					uint8_t get_license_md5_ret = license_md5_string(md5_string);
					if(get_license_md5_ret == C_OK)
					{
						memcpy(response_buf+4+get_productKey_ret+1+get_deviceID_ret,",",1);
						memcpy(response_buf+4+get_productKey_ret+1+get_deviceID_ret+1,md5_string,strlen(md5_string));
						at_response(response_buf);
						goto END;
					}
					else
					{
						ya_printf(C_LOG_ERROR,"get_license_md5_ret error \r\n");
						at_response(RESP_NO_LICENSE);
						goto END;
					}					
				}
				else
				{
					ya_printf(C_LOG_ERROR,"get_productKey_ret error \r\n");
					at_response(RESP_NO_LICENSE);
					goto END;
				}							
			}
			else
			{
				ya_printf(C_LOG_ERROR,"get_deviceID_ret error \r\n");
				at_response(RESP_NO_LICENSE);
				goto END;
			}	
		}
		else
		{
			ya_printf(C_LOG_ERROR,"license not exit \r\n");
			at_response(RESP_NO_LICENSE);
			goto END;
		}		
	}
	else if(strstr(pcmd_buffer, "getrssi ") > 0)
	{
		at_cmd_ret = ya_CMD_RET_SUCCESS; 
		
		if(check_enter_get_rssi_mode())
		{
			ya_printf(C_LOG_ERROR,"enter_get_rssi_mode \r\n");
			at_cmd_ret = ya_CMD_RET_FAILURE;
			goto END;
		}
		uint8_t getrssi_name_len = strlen(pcmd_buffer)-strlen("getrssi ");
		if(getrssi_name_len <= 0 || getrssi_name_len > 32)
		{
			ya_printf(C_LOG_ERROR,"rssi_name_len error \r\n");
			at_cmd_ret = ya_CMD_RET_FAILURE;
			goto END;
		}
		set_enter_get_rssi_mode();
		uint8_t getrssi_name[33] = {0};
		memcpy(getrssi_name,pcmd_buffer+strlen("getrssi "),strlen(pcmd_buffer)-strlen("getrssi "));
		ya_set_factory_rssi_info(getrssi_name);
	}
	else if (strstr(pcmd_buffer, "wrcert") > 0||strstr(pcmd_buffer, "setmac") > 0)
	{
		token = mystrtok(pcmd_buffer, " ");
		if(token && (strcmp(token, "wrcert") == 0))
		{
			arg1_len = strlen("wrcert");
			token = mystrtok(NULL, " ");
			if(token != NULL)
			{
				arg2_len = strlen(token);
				for(i=0;i<strlen(token);i++)
					json_len += at_cmd_hex2num(token[i])<<(4*(strlen(token)-i-1));
				ya_printf(C_LOG_INFO,"json_len = 0x%x\r\n", json_len);
				if(json_len > 0)
				{
					token = mystrtok(NULL, " ");
					arg3_len = strlen(token);
					sscanf(token,"%x",&crc16_arg);
					if(crc16_arg)
					{
						crc16_cal = crc_check(pcmd_buffer+arg1_len+arg2_len+arg3_len+3, json_len);
						ya_printf(C_LOG_INFO,"crc16_cal==%x,crc16_arg==%x\n",crc16_cal,crc16_arg);
						if (crc16_cal != crc16_arg)
						{
							ya_printf(C_LOG_ERROR,"crc16_cal error \r\n");
							at_cmd_ret = ya_CMD_RET_FAILURE;
							goto END;
						}
						JSObject = cJSON_Parse(pcmd_buffer+arg1_len+arg2_len+arg3_len+3);
						if (NULL == JSObject)
						{
							ya_printf(C_LOG_ERROR,"cJSON_Parse ERROR\r\n");
							at_cmd_ret = ya_CMD_RET_FAILURE;
							goto END;
						}
						sub_object = cJSON_GetObjectItem(JSObject, "server");
						if (sub_object)
						{
							if (0 ==strcmp("CN", sub_object->valuestring))
								cert_type = ALI_CERT;
							else if (0 ==strcmp("US", sub_object->valuestring))
								cert_type = AWS_CERT;
							memset(md5_string,0,sizeof(md5_string));
						}
						else
						{
							ya_printf(C_LOG_ERROR,"get server obj error\r\n");
							at_cmd_ret = ya_CMD_RET_FAILURE;
							goto END;
						}
						if (cert_type == ALI_CERT)
						{
							license_ojb = cJSON_GetObjectItem(JSObject, "license");
							if (NULL == license_ojb)
							{
								ya_printf(C_LOG_ERROR,"get license obj error\r\n");
								at_cmd_ret = ya_CMD_RET_FAILURE;
								goto END;
							}
							pos = 0;
							memset(pcmd_buffer,0,(json_len+1));
							for (index = 0; index < 3; index++)
							{
								if (index == 0)
									sub_object = cJSON_GetObjectItem(license_ojb, "product_key");
								else if (index == 1)
									sub_object = cJSON_GetObjectItem(license_ojb, "device_id");
								else if (index == 2)
									sub_object = cJSON_GetObjectItem(license_ojb, "device_secret");
								if (sub_object)
								{
									pcmd_buffer[pos++] = strlen(sub_object->valuestring);
									memcpy(pcmd_buffer + pos, sub_object->valuestring, strlen(sub_object->valuestring));
									pos = pos + strlen(sub_object->valuestring);
						
									if(index == 2)
									{
										if(ya_write_aliyun_cert(pcmd_buffer, pos, YA_LICENSE_ALIYUUN) != 0)
										{
											ya_printf(C_LOG_ERROR,"ya_write_aliyun_cert error\n");
											at_cmd_ret = ya_CMD_RET_FAILURE;
											pos = 0;
										}
									}
								}
								else
								{
									ya_printf(C_LOG_ERROR,"get product_key obj error\n");
									at_cmd_ret = ya_CMD_RET_FAILURE;
									goto END;
								}
							}
							at_response(RESP_OK);
							at_cmd_ret = ya_CMD_RET_SUCCESS;
						}
						else if (cert_type == AWS_CERT)
						{
							license_ojb = cJSON_GetObjectItem(JSObject, "license");
							if (NULL == license_ojb)
							{
								ya_printf(C_LOG_ERROR,"get license obj error\n");
								at_cmd_ret = ya_CMD_RET_FAILURE;
								goto END;
							}
							sub_object = cJSON_GetObjectItem(license_ojb, "client_cert");
							if (sub_object)
							{
								if(0 != ya_write_check_aws_para_device_cert(sub_object->valuestring, strlen(sub_object->valuestring), YA_LICENSE_DEVICE_CERT_ADDR))
								{
									ya_printf(C_LOG_ERROR,"ya_write_check_aws_para_device_cert error\n");
									at_cmd_ret = ya_CMD_RET_FAILURE;
									goto END;
								}							
							}
							else
							{
								ya_printf(C_LOG_ERROR,"get client_cert obj error\n");
								at_cmd_ret = ya_CMD_RET_FAILURE;
								goto END;
							}								
							sub_object = cJSON_GetObjectItem(license_ojb, "client_id");
							if (sub_object)
							{
								if(0 != ya_write_check_aws_para_device_id(sub_object->valuestring, strlen(sub_object->valuestring), YA_LICENSE_DEVICE_ID_ADDR))
								{
									at_cmd_ret = ya_CMD_RET_FAILURE;
									goto END;
								}						
							}
							else
							{
								ya_printf(C_LOG_ERROR,"get client_id obj error\n");
								at_cmd_ret = ya_CMD_RET_FAILURE;
								goto END;
							}	
							sub_object = cJSON_GetObjectItem(license_ojb, "private_key");
							if (sub_object)
							{
								if(0 != ya_write_check_aws_para_device_rsa_private_key(sub_object->valuestring, strlen(sub_object->valuestring), YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR))
								{
									ya_printf(C_LOG_ERROR,"ya_write_check_aws_para_device_rsa_private_key error\n");
									at_cmd_ret = ya_CMD_RET_FAILURE;
									goto END;
								}							
							}
							else
							{
								ya_printf(C_LOG_ERROR,"get private_key obj error\n");
								at_cmd_ret = ya_CMD_RET_FAILURE;
								goto END;
							}						
							sub_object = cJSON_GetObjectItem(license_ojb, "thing_type");
							if (sub_object)
							{
								if(0 != ya_write_check_aws_para_thing_type(sub_object->valuestring, strlen(sub_object->valuestring), YA_LICENSE_THING_TYPE_ADDR))
								{
									ya_printf(C_LOG_ERROR,"ya_write_check_aws_para_thing_type error\n");
									at_cmd_ret = ya_CMD_RET_FAILURE;
									goto END;
								}							
							}
							else
							{
								ya_printf(C_LOG_ERROR,"get thing_type obj error\n");
								at_cmd_ret = ya_CMD_RET_FAILURE;
								goto END;
							}	
							at_response(RESP_OK);
							at_cmd_ret = ya_CMD_RET_SUCCESS;
						}
						else
						{
							ya_printf(C_LOG_ERROR,"cert_type error \r\n");
							at_cmd_ret = ya_CMD_RET_FAILURE;
							goto END;
						}	
					}
					else
					{
						ya_printf(C_LOG_ERROR,"crc16 format error \r\n");
						at_cmd_ret = ya_CMD_RET_FAILURE;
						goto END;
					}	
				}
				else
				{
					ya_printf(C_LOG_ERROR,"json_len format error \r\n");
					at_cmd_ret = ya_CMD_RET_FAILURE;
					goto END;
				}	
			}
			else
			{
				ya_printf(C_LOG_ERROR,"cmd format error \r\n");
				at_cmd_ret = ya_CMD_RET_FAILURE;
				goto END;
			}			
		}
		else if(token && (strcmp(token, "setmac") == 0))
		{
			token = mystrtok(NULL, " ");
			if(token != NULL)
			{
				ya_printf(C_LOG_INFO,"\r\n");
				for (i=0; i<6; i++)
				{
					mac[i] = at_cmd_hex2num(token[i*2]) * 0x10 + at_cmd_hex2num(token[i*2+1]);
					ya_printf(C_LOG_INFO,"%02x ",mac[i]);
				}
				ya_printf(C_LOG_INFO,"\r\n");
				crc16_cal = crc_check(mac, 6);
				if(crc16_cal)
				{
					token = mystrtok(NULL, "\0");
					if(token != NULL)
						sscanf(token,"%x",&crc16_arg);
					else
					{
						ya_printf(C_LOG_ERROR,"get crc16_cal error \r\n");
						at_cmd_ret = ya_CMD_RET_FAILURE;
						goto END;
					}	
				}
				else
				{
					ya_printf(C_LOG_ERROR,"cal crc16 error \r\n");
					at_cmd_ret = ya_CMD_RET_FAILURE;
					goto END;
				}	
				ya_printf(C_LOG_INFO,"crc16_arg===%x, crc16_cal===%x\r\n",crc16_arg,crc16_cal);
				if (crc16_arg != crc16_cal)
				{
					at_cmd_ret = ya_CMD_RET_FAILURE;
					goto END;
				}	
				if(0 == ya_hal_wlan_set_mac_address(mac))
				{
					at_response(RESP_OK);
					at_cmd_ret = ya_CMD_RET_SUCCESS;
				} else
				{
					ya_printf(C_LOG_ERROR,"ya_hal_wlan_set_mac_address error \r\n");
					at_cmd_ret = ya_CMD_RET_FAILURE;
					goto END;
				}	
			}
			else
			{
				ya_printf(C_LOG_ERROR,"cmd format error \r\n");
				at_cmd_ret = ya_CMD_RET_FAILURE;
				goto END;
			}	
		}
		else
		{
			at_cmd_ret = CMD_RET_DATA_ERROR;
			goto END;
		}			
	}
END:
	if (JSObject)
	{
		cJSON_Delete(JSObject);
	}

	if (at_cmd_ret == ya_CMD_RET_FAILURE)
		at_response(RESP_ERR);

	return at_cmd_ret;
}

uint8_t ya_at_frame_get = 0;

void at_cmd_thread(void *param)
{

	while(1)
	{
		if (ya_at_frame_get)
		{
			ya_data_atcmd_handler();
			if (pcmd_buffer)
			{
				ya_hal_os_memory_free(pcmd_buffer);
				pcmd_buffer = NULL;
			}
			ya_at_frame_get = 0;
		}

		ya_delay(20);
	}
}

void at_cmd_thread_create(void)
{
	int32_t ret = 0;
	ret = ya_hal_os_thread_create(NULL, "at_cmd_thread", at_cmd_thread, 0, (2*1024), 5);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create at_cmd_thread error\n");
	}			
}

int ya_atcmd_handler(char *cmd)
{

	#ifdef UART_POLLING_DATA
	static uint8_t init_ya_at_flag = 0;

	if (ya_at_frame_get || !cmd)
		return CMD_RET_DATA_ERROR;

	if (!init_ya_at_flag)
	{
		if (cmd[0] == '\r' || cmd[0] == '\n')
			return CMD_RET_DATA_ERROR;

		at_cmd_thread_create();
		init_ya_at_flag = 1;
	}

	if (pcmd_buffer == NULL)
	{
		pcmd_buffer = (char *)ya_hal_os_memory_alloc(CMD_BUFFER_SIZE);
		pcmd_buffer_index = 0;
		if (!pcmd_buffer)
		{
			ya_printf(C_LOG_ERROR,"pcmd_buffer malloc fail\r\n");
			return CMD_RET_DATA_ERROR;
		}
	}
	
	pcmd_buffer[pcmd_buffer_index++] = *cmd;

	if (pcmd_buffer[pcmd_buffer_index - 1] == '\r' || pcmd_buffer[pcmd_buffer_index - 1] == '\n')
	{
		pcmd_buffer[pcmd_buffer_index - 1] = 0;
		
		if (strncmp (pcmd_buffer, "getmac", strlen("getmac")) == 0
			|| strncmp (pcmd_buffer, "setmac", strlen("setmac")) == 0
			|| strncmp (pcmd_buffer, "getrssi", strlen("getrssi")) == 0
			|| strncmp (pcmd_buffer, "getversion", strlen("getversion")) == 0)
		{
			ya_at_config_flag = 1;
		}
		
		ya_at_frame_get = 1;
		pcmd_buffer_index = 0;
	}

	if (pcmd_buffer_index >= CMD_BUFFER_SIZE)
		pcmd_buffer_index = 0;

	if (ya_at_config_flag)
		return 0;

	return CMD_RET_DATA_ERROR;

	#else

	if (!cmd)
		return CMD_RET_DATA_ERROR;

	pcmd_buffer = cmd;
	return ya_data_atcmd_handler();
	#endif
}


