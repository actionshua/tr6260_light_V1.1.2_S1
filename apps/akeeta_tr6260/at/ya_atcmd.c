#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "ya_common.h"
#include "ya_hal_os.h"
#include "cJSON.h"
#include "ya_config.h"
#include "ya_atcmd.h"


#define CMD_RET_SUCCESS  		0
#define CMD_RET_FAILURE  		1
#define CMD_RET_UNHANDLED  		2
#define CMD_RET_GET_HEADER  	3
#define CMD_RET_DATA_ERROR 		(-1)


#define RESP_HEAD "rsp:"
#define RESP_ERR "rsp:ERROR"
#define RESP_OK "rsp:OK"
#define GET_MAC_BACK "getmac"

#define CMD_BUFFER_SIZE  (8*1024)
static uint8_t *pcmd_buffer = NULL;
static uint8_t at_cmd_ret = CMD_RET_DATA_ERROR;
static uint8_t get_header_flag = 0;

#ifdef UART_POLLING_DATA
static uint32_t pcmd_buffer_index = 0;
static ya_hal_os_thread_t at_cmd_thread_handler = NULL;
static uint32_t arg1_len = 0;
static uint32_t arg2_len = 0;
static uint32_t arg3_len = 0;
static uint32_t json_len = 0;
static uint32_t crc16_arg = 0;
#endif

typedef enum
{
	AWS_CERT = 0,
	ALI_CERT,
	UNKNOWN_CERT
}ya_cert_t;

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

#ifdef UART_POLLING_DATA
void at_cmd_thread(void *param)
{
	uint8_t i = 0;
	uint16_t token_len = 0;
	uint8_t *token = NULL;
	uint16_t crc16_cal = 0;
	cJSON *license_ojb = NULL;
	cJSON *sub_object = NULL;
	ya_cert_t cert_type = UNKNOWN_CERT;
	uint16_t pos = 0;
	uint8_t index = 0;
	uint8_t mac[6]={0};
	char tmp_str[16]={0};
	static uint8_t mac_str_buf[24]={0};
	uint16_t crc_16;
	cJSON *JSObject = NULL;
	while(1)
	{
		if((pcmd_buffer != NULL)&&strlen(pcmd_buffer))
		{
			if(strstr(pcmd_buffer, "getmac"))
			{
				get_header_flag = 1;
				at_cmd_log_OFF();
				memset(mac,0,sizeof(mac));
				memset(tmp_str,0,sizeof(tmp_str));
				memset(mac_str_buf,0,sizeof(mac_str_buf));
				ya_hal_wlan_get_mac_address(mac);
		
				snprintf(tmp_str, sizeof(tmp_str), "%02x%02x%02x%02x%02x%02x", mac[0],mac[1],
														mac[2],mac[3],mac[4],mac[5]);
		
				crc_16 = crc_check(mac, 6);
		
				snprintf(mac_str_buf, sizeof(mac_str_buf), "%s%s%04x", RESP_HEAD, tmp_str, crc_16);
				at_response(GET_MAC_BACK);
				at_response(mac_str_buf);
				at_cmd_ret = CMD_RET_SUCCESS;
				
				pcmd_buffer_index = 0;
				if(strlen(pcmd_buffer) <= strlen("getmac"))
					memset(pcmd_buffer,0,CMD_BUFFER_SIZE);
			}
			else if(strstr(pcmd_buffer, "setmac"))
			{	
				token = mystrtok(pcmd_buffer, " ");
				if(token != NULL)
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
							at_cmd_ret = CMD_RET_DATA_ERROR;
							goto END;
						}	
					}
					else
					{
						ya_printf(C_LOG_ERROR,"cal crc16 error \r\n");
						at_cmd_ret = CMD_RET_DATA_ERROR;
						goto END;
					}	
					ya_printf(C_LOG_INFO,"crc16_arg===%x, crc16_cal===%x\r\n",crc16_arg,crc16_cal);
					if (crc16_arg != crc16_cal)
					{
						at_cmd_ret = CMD_RET_FAILURE;
						goto END;
					}	
					if(0 == ya_hal_wlan_set_mac_address(mac))
						at_cmd_ret = CMD_RET_SUCCESS;
					else
					{
						ya_printf(C_LOG_ERROR,"ya_hal_wlan_set_mac_address error \r\n");
						at_cmd_ret = CMD_RET_FAILURE;
						goto END;
					}	
				}
				else
				{
					ya_printf(C_LOG_ERROR,"cmd format error \r\n");
					at_cmd_ret = CMD_RET_DATA_ERROR;
					goto END;
				}	
			}
			else
			{
				get_header_flag = 0;
				if((arg1_len > 0)&&(arg2_len > 0)&&(arg3_len > 0)&&(crc16_arg > 0))
				{
					if(pcmd_buffer_index >= (arg1_len+arg2_len+arg3_len+json_len+3))
					{
						crc16_cal = crc_check(pcmd_buffer+arg1_len+arg2_len+arg3_len+3, json_len);
						if (crc16_cal != crc16_arg)
						{
							ya_printf(C_LOG_ERROR,"crc16_cal error \r\n");
							at_cmd_ret = CMD_RET_FAILURE;
							goto END;
						}
						JSObject = cJSON_Parse(pcmd_buffer+arg1_len+arg2_len+arg3_len+3);
						if (NULL == JSObject)
						{
							ya_printf(C_LOG_ERROR,"cJSON_Parse ERROR\r\n");
							at_cmd_ret = CMD_RET_FAILURE;
							goto END;
						}
						sub_object = cJSON_GetObjectItem(JSObject, "server");
						if (sub_object)
						{
							if (0 ==strcmp("CN", sub_object->valuestring))
								cert_type = ALI_CERT;
							else if (0 ==strcmp("US", sub_object->valuestring))
								cert_type = AWS_CERT;
						}
						else
						{
							ya_printf(C_LOG_ERROR,"get server obj error\r\n");
							at_cmd_ret = CMD_RET_FAILURE;
							goto END;
						}
						if (cert_type == ALI_CERT)
						{
							license_ojb = cJSON_GetObjectItem(JSObject, "license");
							if (NULL == license_ojb)
							{
								ya_printf(C_LOG_ERROR,"get license obj error\r\n");
								at_cmd_ret = CMD_RET_FAILURE;
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
											at_cmd_ret = CMD_RET_FAILURE;
											pos = 0;
											goto END;
										}
									}
								}
								else
								{
									ya_printf(C_LOG_ERROR,"get product_key obj error\n");
									at_cmd_ret = CMD_RET_FAILURE;
									goto END;
								}
							}
							at_cmd_ret = CMD_RET_SUCCESS;
						}
						else if (cert_type == AWS_CERT)
						{
							license_ojb = cJSON_GetObjectItem(JSObject, "license");
							if (NULL == license_ojb)
							{
								ya_printf(C_LOG_ERROR,"get license obj error\n");
								at_cmd_ret = CMD_RET_FAILURE;
								goto END;
							}
							sub_object = cJSON_GetObjectItem(license_ojb, "client_cert");
							if (sub_object)
							{
								if(0 != ya_write_check_aws_para_device_cert(sub_object->valuestring, strlen(sub_object->valuestring), YA_LICENSE_DEVICE_CERT_ADDR))
								{
									ya_printf(C_LOG_ERROR,"ya_write_check_aws_para_device_cert error\n");
									at_cmd_ret = CMD_RET_FAILURE;
									goto END;
								}							
							}
							else
							{
								ya_printf(C_LOG_ERROR,"get client_cert obj error\n");
								at_cmd_ret = CMD_RET_FAILURE;
								goto END;
							}								
							sub_object = cJSON_GetObjectItem(license_ojb, "client_id");
							if (sub_object)
							{
								if(0 != ya_write_check_aws_para_device_id(sub_object->valuestring, strlen(sub_object->valuestring), YA_LICENSE_DEVICE_ID_ADDR))
								{
									at_cmd_ret = CMD_RET_FAILURE;
									goto END;
								}						
							}
							else
							{
								ya_printf(C_LOG_ERROR,"get client_id obj error\n");
								at_cmd_ret = CMD_RET_FAILURE;
								goto END;
							}	
							sub_object = cJSON_GetObjectItem(license_ojb, "private_key");
							if (sub_object)
							{
								if(0 != ya_write_check_aws_para_device_rsa_private_key(sub_object->valuestring, strlen(sub_object->valuestring), YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR))
								{
									ya_printf(C_LOG_ERROR,"ya_write_check_aws_para_device_rsa_private_key error\n");
									at_cmd_ret = CMD_RET_FAILURE;
									goto END;
								}							
							}
							else
							{
								ya_printf(C_LOG_ERROR,"get private_key obj error\n");
								at_cmd_ret = CMD_RET_FAILURE;
								goto END;
							}						
							sub_object = cJSON_GetObjectItem(license_ojb, "thing_type");
							if (sub_object)
							{
								if(0 != ya_write_check_aws_para_thing_type(sub_object->valuestring, strlen(sub_object->valuestring), YA_LICENSE_THING_TYPE_ADDR))
								{
									ya_printf(C_LOG_ERROR,"ya_write_check_aws_para_thing_type error\n");
									at_cmd_ret = CMD_RET_FAILURE;
									goto END;
								}							
							}
							else
							{
								ya_printf(C_LOG_ERROR,"get thing_type obj error\n");
								at_cmd_ret = CMD_RET_FAILURE;
								goto END;
							}	
							at_cmd_ret = CMD_RET_SUCCESS;
						}
						else
						{
							ya_printf(C_LOG_ERROR,"cert_type error \r\n");
							at_cmd_ret = CMD_RET_DATA_ERROR;
							goto END;
						}	
					}				
				}
				else
				{
					token = mystrtok(pcmd_buffer, " ");
					if(token && (strcmp(token, "wrcert") == 0))
					{
						get_header_flag = 1;
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
								ya_printf(C_LOG_INFO,"crc16_arg = 0x%x,pcmd_buffer_index==%d,arg1_len==%d,arg2_len==%d,arg3_len==%d,json_len==%d\r\n", crc16_arg,pcmd_buffer_index,arg1_len,arg2_len,arg3_len,json_len);
							}
							else
							{
								ya_printf(C_LOG_ERROR,"json_len format error \r\n");
								at_cmd_ret = CMD_RET_DATA_ERROR;
								goto END;
							}	
						}
						else
						{
							ya_printf(C_LOG_ERROR,"cmd format error \r\n");
							at_cmd_ret = CMD_RET_DATA_ERROR;
							goto END;
						}			
					}				
				}
			}			
		}

		ya_delay(50);	
END:
		if (at_cmd_ret == CMD_RET_SUCCESS)
		{			
			if(JSObject)
			{
				cJSON_Delete(JSObject);
				JSObject = NULL;			
			}
			if(pcmd_buffer)
			{
				ya_hal_os_memory_free(pcmd_buffer);
				pcmd_buffer = NULL;
			}
			pcmd_buffer_index = 0;
			arg1_len = 0;
			arg2_len = 0;
			arg3_len = 0;
			crc16_arg = 0;
			at_cmd_ret = -1;
			json_len = 0;
			if(get_header_flag == 0)
			{
				at_response(RESP_OK);
				//goto out;
			}				
		}
		else if(at_cmd_ret == CMD_RET_FAILURE)
		{
			at_response(RESP_ERR);
			if(JSObject)
			{
				cJSON_Delete(JSObject);
				JSObject = NULL;			
			}
			if(pcmd_buffer)
			{
				ya_hal_os_memory_free(pcmd_buffer);
				pcmd_buffer = NULL;
			}
			pcmd_buffer_index = 0;
			arg1_len = 0;
			arg2_len = 0;
			arg3_len = 0;
			crc16_arg = 0;
			at_cmd_ret = -1;
			json_len = 0;
		}
	}
out:
	ya_hal_os_thread_delete(&at_cmd_thread_handler);
	at_cmd_thread_handler = NULL;
}

void at_cmd_thread_create(void)
{
	int32_t ret = 0;
	if(!at_cmd_thread_handler)
	{
		ret = ya_hal_os_thread_create(&at_cmd_thread_handler, "at_cmd_thread", at_cmd_thread, 0, (2*1024), 5);
		if(ret != C_OK)
		{
			ya_printf(C_LOG_ERROR, "create at_cmd_thread error\n");
		}			
	}
}
#endif

int ya_atcmd_handler(char *cmd)
{
#ifdef UART_POLLING_DATA
	int32_t i = 0;
	if(!cmd)
	{
		ya_printf(C_LOG_ERROR,"cmd NULL\r\n");
		at_cmd_ret = CMD_RET_DATA_ERROR;
	}
	ya_printf(C_AT_CMD,"cmd===%c\n",*cmd);
	if((!pcmd_buffer)&&(*cmd != '\n')&&(*cmd != '\r'))
	{
		pcmd_buffer = (char *)ya_hal_os_memory_alloc(CMD_BUFFER_SIZE);
		if(!pcmd_buffer)
		{
			ya_printf(C_LOG_ERROR,"pcmd_buffer malloc fail\r\n");
			at_cmd_ret = CMD_RET_DATA_ERROR;
		}
		memset(pcmd_buffer,0,CMD_BUFFER_SIZE);
		at_cmd_thread_create();
	}	
	if(pcmd_buffer)
	{
		pcmd_buffer[pcmd_buffer_index++] = *cmd;
	}
	if((get_header_flag == 1)||(pcmd_buffer_index > 100))
		at_cmd_ret = CMD_RET_GET_HEADER;
	return at_cmd_ret;	
#else
	uint8_t i = 0;
	uint16_t json_len = 0;
	uint16_t token_len = 0;
	uint16_t arg1_len = 0;
	uint16_t arg2_len = 0;
	uint16_t arg3_len = 0;
	uint8_t *token = NULL;
	uint16_t crc16_arg = 0;
	uint16_t crc16_cal = 0;
	cJSON *JSObject = NULL;
	cJSON *license_ojb = NULL;
	cJSON *sub_object = NULL;
	ya_cert_t cert_type = UNKNOWN_CERT;
	uint16_t pos = 0;
	uint8_t index = 0;
	uint8_t mac[6]={0};
	char tmp_str[16]={0};
	static uint8_t mac_str_buf[24]={0};
	uint16_t crc_16;
	if(!cmd)
	{
		ya_printf(C_LOG_ERROR,"cmd NULL\r\n");
		at_cmd_ret = CMD_RET_DATA_ERROR;
		goto END;
	}
	if((!pcmd_buffer)&&(*cmd != '\n')&&(*cmd != '\r'))
	{
		pcmd_buffer = (char *)ya_hal_os_memory_alloc(CMD_BUFFER_SIZE);
		if(!pcmd_buffer)
		{
			ya_printf(C_LOG_ERROR,"pcmd_buffer malloc fail\r\n");
			at_cmd_ret = CMD_RET_DATA_ERROR;
			goto END;
		}
	}	
	memset(pcmd_buffer,0,CMD_BUFFER_SIZE);
	memcpy(pcmd_buffer,cmd,CMD_BUFFER_SIZE);

	if(strcmp(pcmd_buffer, "getmac") == 0)
	{
		get_header_flag = 1;
		at_cmd_log_OFF();
		memset(mac,0,sizeof(mac));
		memset(tmp_str,0,sizeof(tmp_str));
		memset(mac_str_buf,0,sizeof(mac_str_buf));
		ya_hal_wlan_get_mac_address(mac);

		snprintf(tmp_str, sizeof(tmp_str), "%02x%02x%02x%02x%02x%02x", mac[0],mac[1],
												mac[2],mac[3],mac[4],mac[5]);

		crc_16 = crc_check(mac, 6);

		snprintf(mac_str_buf, sizeof(mac_str_buf), "%s%s%04x", RESP_HEAD, tmp_str, crc_16);
		at_response(GET_MAC_BACK);
		at_response(mac_str_buf);
		
	}
	else
	{
		token = mystrtok(pcmd_buffer, " ");
		if(token && (strcmp(token, "wrcert") == 0))
		{
			get_header_flag = 1;
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
							at_cmd_ret = CMD_RET_FAILURE;
							goto END;
						}
						JSObject = cJSON_Parse(pcmd_buffer+arg1_len+arg2_len+arg3_len+3);
						if (NULL == JSObject)
						{
							ya_printf(C_LOG_ERROR,"cJSON_Parse ERROR\r\n");
							at_cmd_ret = CMD_RET_FAILURE;
							goto END;
						}
						sub_object = cJSON_GetObjectItem(JSObject, "server");
						if (sub_object)
						{
							if (0 ==strcmp("CN", sub_object->valuestring))
								cert_type = ALI_CERT;
							else if (0 ==strcmp("US", sub_object->valuestring))
								cert_type = AWS_CERT;
						}
						else
						{
							ya_printf(C_LOG_ERROR,"get server obj error\r\n");
							at_cmd_ret = CMD_RET_FAILURE;
							goto END;
						}
						if (cert_type == ALI_CERT)
						{
							license_ojb = cJSON_GetObjectItem(JSObject, "license");
							if (NULL == license_ojb)
							{
								ya_printf(C_LOG_ERROR,"get license obj error\r\n");
								at_cmd_ret = CMD_RET_FAILURE;
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
											at_cmd_ret = CMD_RET_FAILURE;
											pos = 0;
										}
									}
								}
								else
								{
									ya_printf(C_LOG_ERROR,"get product_key obj error\n");
									at_cmd_ret = CMD_RET_FAILURE;
									goto END;
								}
							}
							at_cmd_ret = CMD_RET_SUCCESS;
						}
						else if (cert_type == AWS_CERT)
						{
							license_ojb = cJSON_GetObjectItem(JSObject, "license");
							if (NULL == license_ojb)
							{
								ya_printf(C_LOG_ERROR,"get license obj error\n");
								at_cmd_ret = CMD_RET_FAILURE;
								goto END;
							}
							sub_object = cJSON_GetObjectItem(license_ojb, "client_cert");
							if (sub_object)
							{
								if(0 != ya_write_check_aws_para_device_cert(sub_object->valuestring, strlen(sub_object->valuestring), YA_LICENSE_DEVICE_CERT_ADDR))
								{
									ya_printf(C_LOG_ERROR,"ya_write_check_aws_para_device_cert error\n");
									at_cmd_ret = CMD_RET_FAILURE;
									goto END;
								}							
							}
							else
							{
								ya_printf(C_LOG_ERROR,"get client_cert obj error\n");
								at_cmd_ret = CMD_RET_FAILURE;
								goto END;
							}								
							sub_object = cJSON_GetObjectItem(license_ojb, "client_id");
							if (sub_object)
							{
								if(0 != ya_write_check_aws_para_device_id(sub_object->valuestring, strlen(sub_object->valuestring), YA_LICENSE_DEVICE_ID_ADDR))
								{
									at_cmd_ret = CMD_RET_FAILURE;
									goto END;
								}						
							}
							else
							{
								ya_printf(C_LOG_ERROR,"get client_id obj error\n");
								at_cmd_ret = CMD_RET_FAILURE;
								goto END;
							}	
							sub_object = cJSON_GetObjectItem(license_ojb, "private_key");
							if (sub_object)
							{
								if(0 != ya_write_check_aws_para_device_rsa_private_key(sub_object->valuestring, strlen(sub_object->valuestring), YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR))
								{
									ya_printf(C_LOG_ERROR,"ya_write_check_aws_para_device_rsa_private_key error\n");
									at_cmd_ret = CMD_RET_FAILURE;
									goto END;
								}							
							}
							else
							{
								ya_printf(C_LOG_ERROR,"get private_key obj error\n");
								at_cmd_ret = CMD_RET_FAILURE;
								goto END;
							}						
							sub_object = cJSON_GetObjectItem(license_ojb, "thing_type");
							if (sub_object)
							{
								if(0 != ya_write_check_aws_para_thing_type(sub_object->valuestring, strlen(sub_object->valuestring), YA_LICENSE_THING_TYPE_ADDR))
								{
									ya_printf(C_LOG_ERROR,"ya_write_check_aws_para_thing_type error\n");
									at_cmd_ret = CMD_RET_FAILURE;
									goto END;
								}							
							}
							else
							{
								ya_printf(C_LOG_ERROR,"get thing_type obj error\n");
								at_cmd_ret = CMD_RET_FAILURE;
								goto END;
							}	
							at_cmd_ret = CMD_RET_SUCCESS;
						}
						else
						{
							ya_printf(C_LOG_ERROR,"cert_type error \r\n");
							at_cmd_ret = CMD_RET_DATA_ERROR;
							goto END;
						}	
					}
					else
					{
						ya_printf(C_LOG_ERROR,"crc16 format error \r\n");
						at_cmd_ret = CMD_RET_DATA_ERROR;
						goto END;
					}	
				}
				else
				{
					ya_printf(C_LOG_ERROR,"json_len format error \r\n");
					at_cmd_ret = CMD_RET_DATA_ERROR;
					goto END;
				}	
			}
			else
			{
				ya_printf(C_LOG_ERROR,"cmd format error \r\n");
				at_cmd_ret = CMD_RET_DATA_ERROR;
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
						at_cmd_ret = CMD_RET_DATA_ERROR;
						goto END;
					}	
				}
				else
				{
					ya_printf(C_LOG_ERROR,"cal crc16 error \r\n");
					at_cmd_ret = CMD_RET_DATA_ERROR;
					goto END;
				}	
				ya_printf(C_LOG_INFO,"crc16_arg===%x, crc16_cal===%x\r\n",crc16_arg,crc16_cal);
				if (crc16_arg != crc16_cal)
				{
					at_cmd_ret = CMD_RET_FAILURE;
					goto END;
				}	
				if(0 == ya_hal_wlan_set_mac_address(mac))
					at_cmd_ret = CMD_RET_SUCCESS;
				else
				{
					ya_printf(C_LOG_ERROR,"ya_hal_wlan_set_mac_address error \r\n");
					at_cmd_ret = CMD_RET_FAILURE;
					goto END;
				}	
			}
			else
			{
				ya_printf(C_LOG_ERROR,"cmd format error \r\n");
				at_cmd_ret = CMD_RET_DATA_ERROR;
				goto END;
			}	
		}
		else
		{
			//ya_printf(C_LOG_ERROR,"cmd format error \r\n");
			at_cmd_ret = CMD_RET_DATA_ERROR;
			goto END;
		}			
	}
END:
	if (JSObject)
	{
		cJSON_Delete(JSObject);
		JSObject = NULL;
		if (at_cmd_ret == CMD_RET_SUCCESS)
		{
			at_response(RESP_OK);
		}
		else
			at_response(RESP_ERR);
		if(pcmd_buffer)
		{
			ya_hal_os_memory_free(pcmd_buffer);
			pcmd_buffer = NULL;
		}
	}
	if(get_header_flag == 1)
		at_cmd_ret = CMD_RET_GET_HEADER;
	return at_cmd_ret;
#endif
}
