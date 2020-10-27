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
#include "ya_common_func.h"
#include "ya_softap_update.h"
#include "ya_aes_md5.h"
#include "ya_aliyun_cloud.h"

extern int ya_os_get_random(unsigned char *output, size_t output_len);

#define AP_UDP_LISTEN_PORT 		38880
#define AP_UDP_RECE_TIMEOUT		1000
#define SOFTAP_BUF_SIZE 		2*1024

typedef enum
{
	SOFTAP_CONFIG_HEADER_LOW_POS 	= 0,
	SOFTAP_CONFIG_HEADER_HIGH_POS  	= 1,
	SOFTAP_CONFIG_VERSION_POS     	= 2,
	SOFTAP_CONFIG_CMD_LOW_POS    	= 3,
	SOFTAP_CONFIG_CMD_HIGH_POS    	= 4,
	SOFTAP_CONFIG_SN_LOW_POS     	= 5,
	SOFTAP_CONFIG_SN_HIGH_POS    	= 6,
	SOFTAP_CONFIG_DATA_LEN_LOW_POS 	= 7,
	SOFTAP_CONFIG_DATA_LEN_HIGH_POS = 8,
	SOFTAP_CONFIG_DATA_POS          = 9,
}softap_config_pos_t;
typedef enum
{
	SOFTAP_CONFIG_CMD_GET_RANDOM_RESPONSE 		= 0x8001,
	SOFTAP_CONFIG_CMD_GET_DEVICE_MSG_RESPONSE 	= 0x8002,
	SOFTAP_CONFIG_CMD_GET_NET_MSG_RESPONSE 		= 0x8003
}softap_config_cmd_t;

typedef struct _tag_softap_socket_st
{
	int fd;
	struct sockaddr sock_addr;
	socklen_t sock_addr_len;
} softap_socket_st;

uint8_t *ya_softap_decode_buf = NULL;
uint8_t *ya_softap_rece_buf = NULL;

static softap_socket_st g_softap_socket; 
softap_param_st g_softap_param; 
ya_ap_router_para_t ya_router_para;
ya_hal_os_thread_t soft_ap_config = NULL;

static uint8_t aes_key[16];
static uint8_t iv[16];
static uint8_t random_data[32];
static uint8_t init_aes_key_and_iv_flag = 0;

#ifdef ENABLE_FACTORY_TEST
extern void ya_get_ver_string(char* obj);

int32_t ya_factory_response_device_para(void)
{
	uint8_t device_mac[6];
	char device_string[64+1];
	cJSON *cmd = NULL;
	char *data = NULL;

	memset(device_string, 0, 64+1);
	ya_hal_wlan_get_mac_address(device_mac);
	snprintf(device_string, 50, "%02x:%02x:%02x:%02x:%02x:%02x", device_mac[0],device_mac[1],device_mac[2], device_mac[3],device_mac[4],device_mac[5]);
	
	cmd = cJSON_CreateObject();
	if(!cmd)
		return -1;

	cJSON_AddStringToObject(cmd, "code", "0");
	cJSON_AddStringToObject(cmd, "mac", device_string);

	memset(device_string, 0, 64+1);
	ya_get_ver_string(device_string);
	cJSON_AddStringToObject(cmd, "version", device_string);

	data = cJSON_PrintUnformatted(cmd);	

	if(data)
	{
		ya_hal_socket_sendto(g_softap_socket.fd, data, strlen(data), 0, (struct sockaddr*)&g_softap_socket.sock_addr, g_softap_socket.sock_addr_len); 
		ya_hal_os_memory_free(data);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return 0;

}

int32_t ya_factory_test_query_response(char *json_key, char *buffer, char *code)
{
	cJSON *cmd = NULL;
	char *data = NULL;
	
	cmd = cJSON_CreateObject();
	if(!cmd)
		return -1;

	if(buffer && json_key)
		cJSON_AddStringToObject(cmd, json_key, buffer);

	cJSON_AddStringToObject(cmd, "code", code);

	data = cJSON_PrintUnformatted(cmd);	

	if(data)
	{
		ya_hal_socket_sendto(g_softap_socket.fd, data, strlen(data), 0, (struct sockaddr*)&g_softap_socket.sock_addr, g_softap_socket.sock_addr_len); 
		ya_hal_os_memory_free(data);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return 0;
}

void ya_response_aliyun_cert(void)
{
	cJSON *cmd = NULL, *aliyun = NULL;
	uint16_t pos = 0;
	uint8_t buf_cert[128], value_string[64 + 1];
	char *data = NULL;
	uint32_t read_len = 0;
	
	read_len = 128;
	memset(buf_cert, 0, 128);
	
	if(ya_read_aliyun_cert(buf_cert, &read_len, YA_LICENSE_ALIYUUN) != 0)
		goto err;

	cmd = cJSON_CreateObject();
	if(!cmd) 
		goto err;

	aliyun = cJSON_CreateObject();
	if(!aliyun)
		goto err;

	cJSON_AddItemToObject(cmd, "aliyun", aliyun);
	cJSON_AddStringToObject(cmd, "code", "0");

	memset(value_string, 0, 65);
	if(buf_cert[pos] > YA_PRODUCT_KEY_LEN)
		goto err;

	pos = 0;
	memcpy(value_string, buf_cert + pos + 1, buf_cert[pos]);

	cJSON_AddStringToObject(aliyun, "product_key", (char *)value_string);

	pos = pos + 1 + buf_cert[pos];
	
	memset(value_string, 0, 65);
	if(buf_cert[pos] > YA_DEVICE_ID_LEN)
		goto err;

	memcpy(value_string, buf_cert + pos + 1, buf_cert[pos]);

	cJSON_AddStringToObject(aliyun, "device_id", (char *)value_string);

	pos = pos + 1 + buf_cert[pos];
	
	memset(value_string, 0, 65);
	if(buf_cert[pos] > YA_DEVICE_SECRET_LEN)
		goto err;

	memcpy(value_string, buf_cert + pos + 1, buf_cert[pos]);

	cJSON_AddStringToObject(aliyun, "device_secret", (char *)value_string);

	data = cJSON_PrintUnformatted(cmd);	

	if(data)
	{
		ya_hal_socket_sendto(g_softap_socket.fd, data, strlen(data), 0, (struct sockaddr*)&g_softap_socket.sock_addr, g_softap_socket.sock_addr_len); 
		ya_hal_os_memory_free(data);
	}	

	return; 
	
	err:
	ya_factory_test_query_response(NULL, NULL, "1");

	if(cmd) cJSON_Delete(cmd);
	
	return;
}

int32_t ya_factory_test_cmd_response(char *code)
{
	cJSON *cmd = NULL;
	char *data = NULL;
	
	cmd = cJSON_CreateObject();
	if(!cmd)
		return -1;

	cJSON_AddStringToObject(cmd, "code", code);

	data = cJSON_PrintUnformatted(cmd);	

	if(data)
	{
		ya_hal_socket_sendto(g_softap_socket.fd, data, strlen(data), 0, (struct sockaddr*)&g_softap_socket.sock_addr, g_softap_socket.sock_addr_len); 
		ya_hal_os_memory_free(data);
	}

	if(cmd)
		cJSON_Delete(cmd);

	return 0;
}

int32_t ya_softap_factory_license_parse(char *buffer, uint32_t len)
{
	uint8_t index = 0;
	uint16_t pos = 0;
	cJSON *JSObject = NULL;
	uint32_t read_data_len = 0;

	cJSON *Command_JSObject = NULL, *sub_object = NULL;

	if ((JSObject = cJSON_Parse(buffer)) == NULL)
		return -1;
	
	Command_JSObject = cJSON_GetObjectItem(JSObject, "query");
	if(Command_JSObject)
	{
		memset(buffer, 0, SOFTAP_BUF_SIZE);
		read_data_len = SOFTAP_BUF_SIZE;
		if(strcmp(Command_JSObject->valuestring,"device") == 0)
		{
			ya_factory_response_device_para();
		}else if(strcmp(Command_JSObject->valuestring,"cert") == 0)
		{
			if(ya_read_flash_with_var_len(YA_LICENSE_DEVICE_CERT_ADDR, (uint8_t *)buffer, &read_data_len, 1) != 0)
				ya_factory_test_query_response("cert", NULL, "1");
			else
				ya_factory_test_query_response("cert", buffer, "0");	

		}else if(strcmp(Command_JSObject->valuestring,"key") == 0)
		{
			if(ya_read_flash_with_var_len(YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR, (uint8_t *)buffer, &read_data_len, 1) != 0)
				ya_factory_test_query_response("key", NULL, "1");
			else
				ya_factory_test_query_response("key", buffer, "0");
		}else if(strcmp(Command_JSObject->valuestring,"clientid") == 0)
		{
			if(ya_read_flash_with_var_len(YA_LICENSE_DEVICE_ID_ADDR, (uint8_t *)buffer, &read_data_len, 1) != 0)
				ya_factory_test_query_response("clientid", NULL, "1");
			else
				ya_factory_test_query_response("clientid", buffer, "0");
		}else if(strcmp(Command_JSObject->valuestring,"thingtype") == 0)
		{
			if(ya_read_flash_with_var_len(YA_LICENSE_THING_TYPE_ADDR, (uint8_t *)buffer, &read_data_len, 1) != 0)
				ya_factory_test_query_response("thingtype", NULL, "1");
			else
				ya_factory_test_query_response("thingtype", buffer, "0");
		}else if(strcmp(Command_JSObject->valuestring,"aliyun") == 0)
		{
			ya_response_aliyun_cert();
		}
	}

	Command_JSObject = cJSON_GetObjectItem(JSObject, "cert");
	if(Command_JSObject)
	{
		if(ya_write_check_aws_para_device_cert(Command_JSObject->valuestring, strlen(Command_JSObject->valuestring), YA_LICENSE_DEVICE_CERT_ADDR) != 0)
			ya_factory_test_cmd_response("1");
		else
			ya_factory_test_cmd_response("0");
	}

	Command_JSObject = cJSON_GetObjectItem(JSObject, "key");
	if(Command_JSObject)
	{
		if(ya_write_check_aws_para_device_rsa_private_key(Command_JSObject->valuestring, strlen(Command_JSObject->valuestring), YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR) != 0)
			ya_factory_test_cmd_response("1");
		else
			ya_factory_test_cmd_response("0");
	}

	Command_JSObject = cJSON_GetObjectItem(JSObject, "clientid");
	if(Command_JSObject)
	{
		if(ya_write_check_aws_para_device_id(Command_JSObject->valuestring, strlen(Command_JSObject->valuestring), YA_LICENSE_DEVICE_ID_ADDR) != 0)
			ya_factory_test_cmd_response("1");
		else
			ya_factory_test_cmd_response("0");
	}

	Command_JSObject = cJSON_GetObjectItem(JSObject, "thingtype");
	if(Command_JSObject)
	{
		if(ya_write_check_aws_para_thing_type(Command_JSObject->valuestring, strlen(Command_JSObject->valuestring), YA_LICENSE_THING_TYPE_ADDR) != 0)
			ya_factory_test_cmd_response("1");
		else
			ya_factory_test_cmd_response("0");
	}

	Command_JSObject = cJSON_GetObjectItem(JSObject, "aliyun");
	if(Command_JSObject)
	{
		pos = 0;
		for (index = 0; index < 3; index++)
		{
			if (index == 0)
				sub_object = cJSON_GetObjectItem(Command_JSObject, "product_key");
			else if (index == 1)
				sub_object = cJSON_GetObjectItem(Command_JSObject, "device_id");
			else if (index == 2)
				sub_object = cJSON_GetObjectItem(Command_JSObject, "device_secret");

			if (sub_object)
			{
				buffer[pos++] = strlen(sub_object->valuestring);
				memcpy(buffer + pos, sub_object->valuestring, strlen(sub_object->valuestring));
				pos = pos + strlen(sub_object->valuestring);

				if(index == 2)
				{
					printf("pos: %d\n", pos);
					if(ya_write_aliyun_cert((uint8_t *)buffer, pos, YA_LICENSE_ALIYUUN) != 0)
					{
						ya_printf(C_LOG_ERROR, "ya_write_aliyun_cert error\n");
						pos = 0;
					}
				}
			}else
			{
				pos = 0;
				break;
			}
		}

		if (pos)
			ya_factory_test_cmd_response("0");
		else
			ya_factory_test_cmd_response("1");
		
	}
	cJSON_Delete(JSObject);
	return 0;
}

#endif


void ya_hal_debug_array(char *string_start, uint8_t *buf, uint16_t buf_len)
{
#if 1
	uint16_t i = 0;
	ya_printf(C_LOG_INFO, "%s\r\n", string_start);

	for(i=0; i<buf_len;	i++)
	{
		ya_printf(C_LOG_INFO, "%02x ",	buf[i]);
	}

	ya_printf(C_LOG_INFO, "\r\n");
#endif
}

void set_init_aes_key_and_iv_flag(uint8_t flag)
{
	init_aes_key_and_iv_flag = flag;
}

void create_aes_key_and_iv(void)
{
	uint8_t ssid_tmp[100];
	
	memset(ssid_tmp, 0, 100);
	if (init_aes_key_and_iv_flag == 0)
	{	
		memcpy(ssid_tmp,g_softap_param.ap_ssid,strlen(g_softap_param.ap_ssid));
		memcpy(ssid_tmp+strlen(g_softap_param.ap_ssid),AES_KEY_ADD_STRING, strlen(AES_KEY_ADD_STRING));
		memset(aes_key,0,sizeof(aes_key));

		ya_mbedtls_md5(ssid_tmp,aes_key,strlen(ssid_tmp));
		memcpy(iv,aes_key,sizeof(aes_key));			
	} else if (init_aes_key_and_iv_flag == 1)
	{
		memcpy(iv,random_data,sizeof(iv));
		memcpy(aes_key,random_data+sizeof(iv),sizeof(aes_key));
	} 
}

static int softap_set_ap(char *ssid, char *pwd)
{
	int ret = -1;
	uint16_t ssid_len = 0, pwd_len = 0;
	ya_hal_wlan_ap_start_param_t ap_para;

	if(!ssid || !pwd)  return -1;
	
	ssid_len = strlen(ssid);
	pwd_len = strlen(pwd);

	if(ssid_len == 0 || ssid_len > 32 || pwd_len > 64)
	{
		ya_printf(C_LOG_ERROR, "softap_ap error\n");
		return -1;
	}
	
	ap_para.ssid = ssid;
	ap_para.password = pwd;
	ap_para.ssid_length = ssid_len;
	ap_para.password_length = pwd_len;
	ap_para.channel = 6;

	ret = ya_hal_wlan_start_ap(&ap_para);
	
	if (ret != 0)
	{
		ya_printf(C_LOG_ERROR, "softap_set_ap error\n");
		return C_ERROR;
	}

	ya_os_get_random(random_data,32);
	
	return C_OK; 
}

static uint32_t softap_feedback_encrypt_pack(uint16_t cmd, uint16_t len, uint8_t *data)
{
	uint32_t encrypt_data_len;
	uint8_t data_md5[16]={0};

	ya_softap_decode_buf[SOFTAP_CONFIG_CMD_LOW_POS] = cmd;
	ya_softap_decode_buf[SOFTAP_CONFIG_CMD_HIGH_POS] = cmd >> 8;

	ya_mbedtls_md5(data,data_md5,len);	
	create_aes_key_and_iv();

	encrypt_data_len = AES_CBC_PKCS5Padding_encrypt(aes_key,iv,data,ya_softap_decode_buf+SOFTAP_CONFIG_DATA_POS,len);
	ya_softap_decode_buf[SOFTAP_CONFIG_DATA_LEN_LOW_POS] = encrypt_data_len;
	ya_softap_decode_buf[SOFTAP_CONFIG_DATA_LEN_HIGH_POS] = encrypt_data_len >> 8;

	memcpy(ya_softap_decode_buf+SOFTAP_CONFIG_DATA_POS+encrypt_data_len, data_md5, sizeof(data_md5));
	return (encrypt_data_len+25);
}

int32_t ya_softap_random_cmd(void)
{
	int32_t socket_data_len = 0;
	cJSON *root = NULL;
	char *buf = NULL;

	char random_data_string[128] = {0};
	HexArrayToString(random_data, 32, random_data_string);

	root = cJSON_CreateObject();
	if(!root)	return -1;

	cJSON_AddStringToObject(root, "Random", random_data_string);	

	buf = cJSON_PrintUnformatted(root);

	if(root)
		cJSON_Delete(root);

	
	if (buf)
	{
		ya_printf(C_LOG_INFO, "softap_random_cmd: %s\n", buf);
		socket_data_len = softap_feedback_encrypt_pack(SOFTAP_CONFIG_CMD_GET_RANDOM_RESPONSE,strlen(buf),(uint8_t *)buf);
		ya_hal_socket_sendto(g_softap_socket.fd, ya_softap_decode_buf, socket_data_len, 0, (struct sockaddr*)&g_softap_socket.sock_addr, g_softap_socket.sock_addr_len); 

		ya_hal_os_memory_free(buf);		
		return 0;
	}
	return -1;
}

int32_t ya_softap_get_cmd(cJSON *JSObject)
{
	uint32_t wificloud = 0;
	int32_t socket_data_len = 0, cloud_type = 0, ret = 0;
	
	char device_string[64+1];
	char *buf = NULL, *product_key = NULL, *client_id = NULL;

	cJSON *CloudType = NULL;
	cJSON *root = NULL;

	ret = 0;

	CloudType = cJSON_GetObjectItem(JSObject, "CloudType");
	
	wificloud = ya_get_wifi_cloud_support();

	if (CloudType->type == cJSON_String) 
	{
		cloud_type = atoi(CloudType->valuestring);
		if (cloud_type >= WIFI_MAX)
		{
			ret = 2;
			goto out;
		}
	} else
	{
		ret = 2;
		goto out;
	}

	if ((g_softap_param.cloud_support_type == AKEETA_CN && cloud_type != WIFI_CN) ||
		 	(g_softap_param.cloud_support_type == AKEETA_OVERSEA && cloud_type == WIFI_CN))
	{
		ret = 1;
		goto out;
	} 

	if (cloud_type >= WIFI_MAX)
	{
		ret = 1;
		goto out;
	} 
	

	if (cloud_type == WIFI_CN)
	{
		product_key = ya_aly_get_thing_name();
		client_id = ya_aly_get_client_id();
	} else 
	{
		product_key = ya_aws_get_thing_type();
		client_id = ya_aws_get_client_id();
	}

	if (product_key == NULL || client_id == NULL) 
	{
		ret = 1;
		goto out;
	}

	if (strlen(product_key) == 0 || strlen(client_id) == 0)
	{
		ret = 1;
		goto out;
	}

	ya_router_para.cloud_type = cloud_type;


	out:
	root = cJSON_CreateObject();
	if(!root)	return -1;

	cJSON_AddStringToObject(root, "Command", "Return");	

	
	if (ret == 0)
	{
		cJSON_AddStringToObject(root, "DeviceId", client_id);	
		cJSON_AddStringToObject(root, "ProductKey", product_key);
		cJSON_AddStringToObject(root, "Code", "0");
		cJSON_AddStringToObject(root, "WiFiCloud", ya_int_to_string(wificloud));
	}else 
	{
		cJSON_AddStringToObject(root, "DeviceId", "");	
		cJSON_AddStringToObject(root, "ProductKey", "");
		if (ret == 1)
			cJSON_AddStringToObject(root, "Code", "1");
		else
			cJSON_AddStringToObject(root, "Code", "2");

		cJSON_AddStringToObject(root, "WiFiCloud", ya_int_to_string(wificloud));
	}

	memset(device_string, 0, 64+1);
	ya_get_ver_string(device_string);
	cJSON_AddStringToObject(root, "Version", device_string);
	
	buf = cJSON_PrintUnformatted(root);

	if(root)
		cJSON_Delete(root);

	if (buf)
	{
		ya_printf(C_LOG_INFO, "softap_get_cmd: %s\n", buf);	
		socket_data_len = softap_feedback_encrypt_pack(SOFTAP_CONFIG_CMD_GET_DEVICE_MSG_RESPONSE, strlen(buf), (uint8_t *)buf);
		ya_hal_socket_sendto(g_softap_socket.fd, ya_softap_decode_buf, socket_data_len, 0, (struct sockaddr*)&g_softap_socket.sock_addr, g_softap_socket.sock_addr_len); 

		ya_hal_os_memory_free(buf);	

		if (ret == 0)
			return 0;
	}

	return -1;
}

int32_t ya_softap_apconfig_cmd(cJSON *JSObject)
{
	cJSON *Ssid = NULL, *Password = NULL, *Bssid = NULL, *SsidGBK = NULL, *Random = NULL;
	int32_t code = 0;
	int32_t socket_data_len = 0;
	cJSON *root = NULL;
	char *buf = NULL;
	uint8_t resend_flag = 0;

	code = 0;

	Ssid  = cJSON_GetObjectItem(JSObject, "Ssid");
	if (!Ssid || Ssid->type != cJSON_String)
	{
		code = 1;
		goto out;
	}
	
	if (strlen(Ssid->valuestring) > 32)
	{
		code = 1;
		goto out;
	}

	Password  = cJSON_GetObjectItem(JSObject, "Password");
	if (!Password || Password->type != cJSON_String)
	{
		code = 1;
		goto out;
	}
	
	if (strlen(Password->valuestring) > 64)
	{
		code = 1;
		goto out;
	}

	Bssid  = cJSON_GetObjectItem(JSObject, "Bssid");
	if (!Bssid || Bssid->type != cJSON_String)
	{
		code = 1;
		goto out;
	}
	
	if (strlen(Bssid->valuestring) > 12)
	{
		code = 1;
		goto out;
	}

	Random  = cJSON_GetObjectItem(JSObject, "Random");
	if (!Random || Random->type != cJSON_String)
	{
		code = 1;
		goto out;
	}
	
	if (strlen(Random->valuestring) > 32)
	{
		code = 1;
		goto out;
	}

	strcpy(ya_router_para.router_ssid, Ssid->valuestring);
	strcpy(ya_router_para.router_pwd, Password->valuestring);
	StringToHexArray(ya_router_para.router_bssid, 6, Bssid->valuestring);
	strcpy(ya_router_para.random, Random->valuestring);

	out:

	root = cJSON_CreateObject();

	if (!root) return -1;

	if (code == 0)
		cJSON_AddStringToObject(root, "Code", "0");
	else
		cJSON_AddStringToObject(root, "Code", "1");

	buf = cJSON_PrintUnformatted(root);

	if (buf)
	{
		ya_printf(C_LOG_INFO, "softap_apconfig_cmd: %s\n", buf);	
		socket_data_len = softap_feedback_encrypt_pack(SOFTAP_CONFIG_CMD_GET_NET_MSG_RESPONSE,strlen(buf),(uint8_t *)buf);
		for(resend_flag = 0;resend_flag < 3;resend_flag++)
			ya_hal_socket_sendto(g_softap_socket.fd, ya_softap_decode_buf, socket_data_len, 0, (struct sockaddr*)&g_softap_socket.sock_addr, g_softap_socket.sock_addr_len); 

		ya_hal_os_memory_free(buf);	

		ya_delay(1000);
		if (code == 0)
		{
			g_softap_param.p_softap_config_cb(SOFTAP_CONFIG_FINISH, &ya_router_para);
			return 1;
		}
	}

	return -1;
}

int32_t ya_softap_comm_parse(uint8_t *buffer, uint32_t len)
{	
	int ret = -1;
	uint32_t cipher_data_len = 0, decrypt_data_len = 0;
	uint8_t data_decrypt_md5[16]={0};
	cJSON *JSObject_decrypt = NULL, *Command_JSObject_decrypt = NULL;
	uint16_t md5_pos = 0;
	uint16_t cmd = 0;
	
	if((buffer[SOFTAP_CONFIG_HEADER_HIGH_POS] != 0x5A) || (buffer[SOFTAP_CONFIG_HEADER_LOW_POS] != 0x5A))
	{
		ya_printf(C_LOG_ERROR, "softap data header invalid\n");
		return -1;
	}

	memset(ya_softap_decode_buf, 0, SOFTAP_BUF_SIZE);	
	cmd = (((uint16_t)buffer[SOFTAP_CONFIG_CMD_HIGH_POS]) <<8) + buffer[SOFTAP_CONFIG_CMD_LOW_POS];

	if (cmd == 0x0001)
		set_init_aes_key_and_iv_flag(0);
	else
		set_init_aes_key_and_iv_flag(1);

	create_aes_key_and_iv();

	cipher_data_len = (((uint16_t)buffer[SOFTAP_CONFIG_DATA_LEN_HIGH_POS]) <<8) + buffer[SOFTAP_CONFIG_DATA_LEN_LOW_POS];

	memcpy(ya_softap_decode_buf, buffer, SOFTAP_CONFIG_DATA_POS);
	
	decrypt_data_len = AES_CBC_PKCS5Padding_decrypt(aes_key,iv,buffer+SOFTAP_CONFIG_DATA_POS,ya_softap_decode_buf + SOFTAP_CONFIG_DATA_POS,cipher_data_len);
	if(!decrypt_data_len)
	{
		ya_hal_debug_array("error decode buffer ", buffer, (cipher_data_len+25));
		return -1;
	}

	ret = ya_mbedtls_md5(ya_softap_decode_buf+SOFTAP_CONFIG_DATA_POS,data_decrypt_md5,decrypt_data_len);
	if (ret != 0)
	{
		ya_printf(C_LOG_ERROR, "ya_mbedtls_md5 error,%d\n",__LINE__);
		return -1;
	}

	//ya_hal_debug_array("After decrypt", ya_softap_decode_buf + SOFTAP_CONFIG_DATA_POS, decrypt_data_len);

	md5_pos = SOFTAP_CONFIG_DATA_POS + cipher_data_len;

	if (0 == memcmp(buffer+md5_pos,data_decrypt_md5, 16))
	{
		ya_softap_decode_buf[SOFTAP_CONFIG_DATA_POS + decrypt_data_len] = '\0';
		if ((JSObject_decrypt = cJSON_Parse(ya_softap_decode_buf+SOFTAP_CONFIG_DATA_POS)) == NULL)
		{
			ya_printf(C_LOG_ERROR,"json data error\n");
			return -1;
		}

		Command_JSObject_decrypt = cJSON_GetObjectItem(JSObject_decrypt, "Command");
		if(Command_JSObject_decrypt)
		{
			if (memcmp(Command_JSObject_decrypt->valuestring, "Random", strlen("Random")) == 0)
			{
				ya_printf(C_LOG_INFO,"get device random from app: %s\n", ya_softap_decode_buf+SOFTAP_CONFIG_DATA_POS);
				ret =  ya_softap_random_cmd();
			}
			else if (memcmp(Command_JSObject_decrypt->valuestring, "Get", strlen("Get")) == 0)
			{
				ya_printf(C_LOG_INFO,"get devicce info from app: %s\n", ya_softap_decode_buf+SOFTAP_CONFIG_DATA_POS);
				ret =  ya_softap_get_cmd(JSObject_decrypt);
			}
			else if (memcmp(Command_JSObject_decrypt->valuestring, "Apconfig", strlen("Apconfig")) == 0) 
			{
				ya_printf(C_LOG_INFO,"send router info from app: %s\n", ya_softap_decode_buf+SOFTAP_CONFIG_DATA_POS);
				ret =  ya_softap_apconfig_cmd(JSObject_decrypt);
			}					
		}
	}
	else
	{
		ya_hal_debug_array("md5 cacl", data_decrypt_md5, 16);
		ya_printf(C_LOG_ERROR, "softap data MD5 invalid\n");
		return -1;
	}

	if(JSObject_decrypt)
		cJSON_Delete(JSObject_decrypt);
	
	return ret;
}

static void ya_softap_update(void *param)
{
	int fd = -1, ret = -1, ret1 = -1;
	uint32_t package_data_len;
	Ya_Timer ya_soft_ap_timer;

	#ifdef ENABLE_LICENSE_ONE_WRITE
	uint8_t enable_license_write = 0;

	if (g_softap_param.cloud_support_type == AKEETA_CN)
	{
		if (ya_check_cn_license_exit_internal() != 0)
			enable_license_write = 1;
	} else if (g_softap_param.cloud_support_type == AKEETA_OVERSEA)
	{
		if (ya_check_us_license_exit_internal() != 0)
			enable_license_write = 1;
	} else 
	{
		if (ya_check_cn_license_exit_internal() != 0 || ya_check_us_license_exit_internal() != 0)
			enable_license_write = 1;
	}

	if (enable_license_write)
		ya_printf(C_LOG_INFO, "no license in it, enable write\r\n");
	#endif
	
	ret = softap_set_ap(g_softap_param.ap_ssid, g_softap_param.ap_password);
	if (ret < 0)
		goto out; 

	fd = ya_udp_server(AP_UDP_LISTEN_PORT, AP_UDP_RECE_TIMEOUT);
	if (fd < 0)
	{
		ya_printf(C_LOG_ERROR, "ap udp socket error\n");
		goto out; 
	}

	if(NULL == ya_softap_rece_buf)
		ya_softap_rece_buf = (uint8_t*)ya_hal_os_memory_alloc(SOFTAP_BUF_SIZE);

	if (!ya_softap_rece_buf)
	{
		ya_printf(C_LOG_ERROR, "ya_hal_os_memory_alloc fail %d\n",__LINE__);
		goto out; 
	}
	
	if(NULL == ya_softap_decode_buf)
		ya_softap_decode_buf = (uint8_t*)ya_hal_os_memory_alloc(SOFTAP_BUF_SIZE);

	if (!ya_softap_decode_buf)
	{
		ya_printf(C_LOG_ERROR, "ya_hal_os_memory_alloc fail %d\n",__LINE__);
		goto out; 
	}


	g_softap_socket.fd = fd;

	ya_init_timer(&ya_soft_ap_timer);
	ya_countdown_ms(&ya_soft_ap_timer, g_softap_param.timeout*1000);
	memset(&ya_router_para, 0, sizeof(ya_ap_router_para_t));

	while(1)
	{
		if(ya_has_timer_expired(&ya_soft_ap_timer) == 0)
		{
			g_softap_param.p_softap_config_cb(SOFTAP_TIME_OUT, NULL);
			goto out; 
		}

		memset(ya_softap_rece_buf, 0, SOFTAP_BUF_SIZE);

		g_softap_socket.sock_addr_len = sizeof(struct sockaddr);
		ret = ya_hal_socket_recvform(fd, ya_softap_rece_buf, SOFTAP_BUF_SIZE-1, 0, (struct sockaddr*)&g_softap_socket.sock_addr, &g_softap_socket.sock_addr_len);
		if (ret > 25 )
		{		
			ya_printf(C_LOG_INFO, "client ip:%s\n", inet_ntoa(g_softap_socket.sock_addr.sa_data));

			if((ya_softap_rece_buf[SOFTAP_CONFIG_HEADER_HIGH_POS] == 0x5A) && (ya_softap_rece_buf[SOFTAP_CONFIG_HEADER_LOW_POS] == 0x5A))
			{
				ret1 = 0;
				package_data_len = (ya_softap_rece_buf[SOFTAP_CONFIG_DATA_LEN_HIGH_POS]<<8)|(ya_softap_rece_buf[SOFTAP_CONFIG_DATA_LEN_LOW_POS]);

				if (ret >= package_data_len)
				{
					ret1 = ya_softap_comm_parse(ya_softap_rece_buf, ret);
					if(ret1 == 1)			
						goto out; 
					
					if(ret1 == -1)	
					{
						ya_delay(2000);
						g_softap_param.p_softap_config_cb(SOFTAP_ERROR, NULL);
						goto out;
					}
				}
			}
			#ifdef ENABLE_FACTORY_TEST
			else 
			{
				#ifdef ENABLE_LICENSE_ONE_WRITE
				if (enable_license_write)
				#endif
				ya_softap_factory_license_parse((char *)ya_softap_rece_buf, SOFTAP_BUF_SIZE);
			}
			#endif
		}
		#ifdef ENABLE_FACTORY_TEST
		else
		{
			#ifdef ENABLE_LICENSE_ONE_WRITE
			if (enable_license_write)
			#endif
			ya_softap_factory_license_parse(ya_softap_rece_buf, SOFTAP_BUF_SIZE);
		}		
		#endif
	}
	
out:
	if(fd >= 0)
	{
		close(fd);
		fd = -1;
	}
	
	if(ya_softap_rece_buf)
	{
		ya_hal_os_memory_free(ya_softap_rece_buf);
		ya_softap_rece_buf = NULL;
	}

	if(ya_softap_decode_buf)
	{
		ya_hal_os_memory_free(ya_softap_decode_buf);
		ya_softap_decode_buf = NULL;
	}
	
	ya_printf(C_LOG_INFO, "softap stop\n");
	soft_ap_config = NULL;
	ya_hal_os_thread_delete(NULL);
	return;
}

int32_t ya_start_softap(softap_param_st * p_softap_param)
{
	if (ya_softap_rece_buf)
	{
		ya_hal_os_memory_free(ya_softap_rece_buf);
		ya_softap_rece_buf = NULL;
	}

	if(ya_softap_decode_buf)
	{
		ya_hal_os_memory_free(ya_softap_decode_buf);
		ya_softap_decode_buf = NULL;
	}
	
	if(!soft_ap_config)
	{
		memset(&g_softap_param, 0, sizeof(softap_param_st));
		memcpy(&g_softap_param, p_softap_param, sizeof(softap_param_st));		
		
		memset(&g_softap_socket, 0, sizeof(softap_socket_st));
		g_softap_socket.fd = -1;

		if(ya_hal_os_thread_create(&soft_ap_config, "softap_task", ya_softap_update, 0, (2*1024), 5) != C_OK)
		{
			ya_printf(C_LOG_ERROR, "create ya_app_main error\n");
			return C_ERROR;
		}

	}
	return C_OK;
}

