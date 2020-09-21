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
#include "ya_config.h"

#define ROUTER_BROADCAST_DATA_LEN   256

uint8_t broadcast_buffer[ROUTER_BROADCAST_DATA_LEN];

#define YA_BROADCAST_PORT  				 38890   
#define YA_LISTEN_PORT					 5356
#define YA_LISTEN_PORT_STRING			"5356"
#define YA_LISTEN_TIMEOUT				1000

xTaskHandle ya_sniffer_broadcast = NULL;

int32_t ya_sniffer_broadcast_content(uint8_t *send_buffer, uint16_t *len)
{
	uint32_t wificloud = 0;
	char *buf = NULL;
	cJSON *cmd = NULL;
	uint8_t ip[64];
	
	cmd = cJSON_CreateObject();
	if(!cmd)
		return C_ERROR;

	wificloud = ya_get_wifi_cloud_support();

	memset(ip, 0, 64);
	ya_hal_wlan_get_ip_address_with_string(ip, 64);

	cJSON_AddStringToObject(cmd, "Command", "SmartGatewaySetUpNet");

	if (ya_get_cloud_support() == 1)
	{
		cJSON_AddStringToObject(cmd, "DeviceId", ya_get_client_id());
		cJSON_AddStringToObject(cmd, "ProductKey", ya_get_thing_type());
		cJSON_AddStringToObject(cmd, "Code", "0");
		cJSON_AddStringToObject(cmd, "WiFiCloud", ya_int_to_string(wificloud));
	}else
	{
		cJSON_AddStringToObject(cmd, "DeviceId", "");
		cJSON_AddStringToObject(cmd, "ProductKey", "");
		cJSON_AddStringToObject(cmd, "Code", "1");
		cJSON_AddStringToObject(cmd, "WiFiCloud", ya_int_to_string(wificloud));
	}
	
	cJSON_AddStringToObject(cmd, "Ip", (char *)ip);
	cJSON_AddStringToObject(cmd, "Port", YA_LISTEN_PORT_STRING);

	buf = cJSON_PrintUnformatted(cmd);
	if(strlen(buf) > (*len))
	{
		if(cmd)
			cJSON_Delete(cmd);

		if(buf)
			ya_hal_os_memory_free(buf);

		return C_ERROR;
	}

	memcpy(send_buffer, buf, strlen(buf));
	(*len) = strlen(buf);

	if(cmd)
		cJSON_Delete(cmd);

	if(buf)
		ya_hal_os_memory_free(buf);

	return C_OK;
}

int32_t ya_parse_listen_data(char *buffer, uint16_t len)
{
	int32_t ret = -1;
	cJSON *cmd = NULL, *Type = NULL;

	cmd = cJSON_Parse(buffer);
	if(!cmd)
		goto err;

	//ya_printf(C_LOG_INFO,"rece buffer: %s\n", buffer);

	Type = cJSON_GetObjectItem(cmd, "Type");
	if(!Type)
		goto err;

	ret = strcmp(Type->valuestring, "BackSuccess");
	if(ret != 0)
		goto err;

	if(cmd)
		cJSON_Delete(cmd);

	return C_OK;
	
	err:

	if(cmd)
		cJSON_Delete(cmd);

	return C_ERROR;
}

static void ya_snifer_broadcast_task(void *param)
{
	int listen_fd = -1, ret = -1;
	uint8_t flag = 0;
	
	struct sockaddr_in client_addr;
	socklen_t client_addr_length;

	struct sockaddr_in addrto;
	int nlen;

	uint16_t send_len = 0, count = 0;
	memset(&addrto, 0, sizeof(struct sockaddr_in));
	addrto.sin_family=AF_INET;
	addrto.sin_addr.s_addr= INADDR_BROADCAST;
	addrto.sin_port=htons(YA_BROADCAST_PORT);

	client_addr_length = sizeof(client_addr);
	nlen=sizeof(addrto);

	listen_fd = ya_udp_server(YA_LISTEN_PORT, YA_LISTEN_TIMEOUT);
	if (listen_fd < 0)
		goto out; 

	ya_printf(C_LOG_INFO, "ya_udp_server ok\n");

	while(count < 120)
	{
		if(ya_get_app_status() != YA_APP_CONNECTED)
			goto out;
	
		memset(broadcast_buffer, 0, ROUTER_BROADCAST_DATA_LEN);
		send_len = ROUTER_BROADCAST_DATA_LEN;
		ret = ya_sniffer_broadcast_content(broadcast_buffer, &send_len);
		if(ret != C_OK)
			ya_printf(C_LOG_ERROR, "ya_sniffer_broadcast error\n");
		
		ret = ya_hal_socket_sendto(listen_fd, broadcast_buffer, send_len, 0, (struct sockaddr*)&addrto, nlen); 
		if(ret < 0)
		{
			ya_printf(C_LOG_ERROR, "sendto error\n");
			goto out; 
		}else{
			ya_printf(C_LOG_INFO, "send notification to app ...ok\n");
			if (flag == 0)
			{
				flag = 1;
				ya_printf(C_LOG_INFO, "%s\n", broadcast_buffer);
			}
		}

		memset(broadcast_buffer, 0, ROUTER_BROADCAST_DATA_LEN);
		ret = ya_hal_socket_recvform(listen_fd, broadcast_buffer, ROUTER_BROADCAST_DATA_LEN, 0, (struct sockaddr*) &client_addr, &client_addr_length);
		if (ret > 0)
		{
			ya_printf(C_LOG_INFO, "recvfrom packet len: %d\n", ret);
			if(ret > (ROUTER_BROADCAST_DATA_LEN - 1))
				goto out;

			broadcast_buffer[ret] = '\0';
			ret = ya_parse_listen_data((char *)broadcast_buffer, ret);
			if(ret == C_OK){	
				ya_printf(C_LOG_INFO, "recvfrom packet from app and exit notification process\n");
				goto out; 
			}
		}
		count++;
	}
out:
	if(listen_fd >= 0)
	{
		close(listen_fd);
	}
	
	ya_printf(C_LOG_INFO, "ya_snifer_broadcast_task stop\n");
	ya_sniffer_broadcast = NULL;
	ya_hal_os_thread_delete(NULL);
	return;
}



int32_t ya_start_sniffer_broadcast(void)
{
	int ret = C_ERROR;
	
	if(!ya_sniffer_broadcast)
	{
		ret = ya_hal_os_thread_create(&ya_sniffer_broadcast, "ya_snifer_broadcast task", ya_snifer_broadcast_task, 0, (1024), 5);
		if(ret != C_OK)
		{
			ya_printf(C_LOG_ERROR, "create ya_start_sniffer_broadcast error\n");
			return C_ERROR;
		}

	}
	return C_OK;
}

