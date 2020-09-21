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
#include "ya_hal_wlan.h"
#include "system_wifi.h"
#include "system_event.h"
#include "ya_common.h"

ya_hal_wlan_event_handler_t wifi_event_handler;

static sys_err_t tr_wifi_event_handler(void *ctx, system_event_t *event)
{
	
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_CONNECTED:
        printf("SYSTEM_EVENT_STA_CONNECTED\n");
		
        break;

    case SYSTEM_EVENT_STA_START:
        printf("SYSTEM_EVENT_STA_START\n");

        break;

    case SYSTEM_EVENT_STA_GOT_IP:
		printf("SYSTEM_EVENT_STA_GOT_IP\n");
		wifi_event_handler(YA_HAL_EVT_STA_CONNECT_SUCCESS, NULL);
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
		printf("SYSTEM_EVENT_STA_DISCONNECTED\n");
		wifi_event_handler(YA_HAL_EVT_LINK_LOST, NULL);
        break;

    default:
        break;
    }

    return 0;
}

int32_t ya_hal_wlan_start(ya_hal_wlan_event_handler_t event_handler)
{	
	while (!wifi_is_ready()){
		system_printf("err! set AP/STA information must after wifi initialized!\n");
		vTaskDelay(pdMS_TO_TICKS(5));
	}

	wifi_event_handler = event_handler;
	sys_event_loop_set_cb(tr_wifi_event_handler, NULL);
	return 0;
}

int32_t ya_hal_wlan_stop(void)
{
	int32_t ret = 0;
	wifi_work_mode_e wifi_work_mode;
	wifi_work_mode = wifi_get_opmode();

	if (WIFI_MODE_STA == wifi_work_mode)
	{
		wifi_stop_station();
	}
	else if (WIFI_MODE_AP == wifi_work_mode)
	{
		wifi_stop_softap();
	}
	else if (WIFI_MODE_AP_STA == wifi_work_mode)
	{
		wifi_stop_station();
		wifi_stop_softap();
	}
	else
	{
		ret = -1;
	}
	
	return ret;
}


int32_t ya_disable_wifi_power_saving()
{
	return 0;
}

int32_t ya_hal_wlan_disconnnect_wifi(void)
{
	if(0 == wifi_disconnect())
		return 0;

	return -1;
}


int32_t ya_hal_set_sta_mode()
{
	if (0 != ya_hal_wlan_stop())
	{
		return -1;
	}
	vTaskDelay(200);
	wifi_set_opmode(WIFI_MODE_STA);
	wifi_set_status(STATION_IF, STA_STATUS_STOP);
	return 0;
}



int32_t ya_hal_wlan_start_ap(ya_hal_wlan_ap_start_param_t *ap_start_param)
{
	printf("ya_hal_wlan_start_ap1\n");
	wifi_config_u ap_conf;	
	ip_info_t ipInfoTmp;

	wifi_stop_station();
	wifi_set_opmode(WIFI_MODE_AP);
	//wifi_set_status(SOFTAP_IF, AP_STATUS_STOP);

	if (ap_start_param->ssid_length > WIFI_SSID_MAX_LEN || ap_start_param->password_length >WIFI_PWD_MAX_LEN)
	{
		return -1;
	}

	memset(&ap_conf, 0, sizeof(wifi_config_u));
	memcpy(ap_conf.ap.ssid, ap_start_param->ssid, ap_start_param->ssid_length);
	if(ap_start_param->password_length == 0)
	{
		memcpy(ap_conf.ap.password, ap_start_param->password, ap_start_param->password_length);
	}
	ap_conf.ap.channel = ap_start_param->channel;

	if(ap_start_param->password_length == 0)
		ap_conf.ap.authmode = AUTH_OPEN;
	else if(ap_start_param->password_length >= 8)
		ap_conf.ap.authmode = AUTH_WPA2_PSK;
	else
		ap_conf.ap.authmode = AUTH_WEP;


	if (0 != wifi_start_softap(&ap_conf))
	{
		return -1;
	}

	memset(&ipInfoTmp, 0, sizeof(ipInfoTmp));
	IP4_ADDR(&ipInfoTmp.ip, 192, 168, 1, 1);
	IP4_ADDR(&ipInfoTmp.gw, 192, 168, 1, 1);
	IP4_ADDR(&ipInfoTmp.netmask, 255, 255, 255, 0);
	set_softap_ipconfig(&ipInfoTmp);

	return 0;
}


int32_t ya_hal_wlan_stop_ap(void)
{
	wifi_stop_softap();
	wifi_set_opmode(WIFI_MODE_STA);
	wifi_set_status(STATION_IF, STA_STATUS_STOP);
	
	return 0;
}


int32_t ya_hal_wlan_get_mac_address(uint8_t *mac_addr)
{
	wifi_get_mac_addr(0,mac_addr);
	return 0;
}

int32_t ya_hal_wlan_set_mac_address(uint8_t *mac_addr)
{
	uint8_t mac_str[20]={0};
		
	snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
								   mac_addr[0],mac_addr[1],mac_addr[2],
								   mac_addr[3],mac_addr[4],mac_addr[5]);
														
	if( 0 != ef_set_env_blob(NV_WIFI_STA_MAC, mac_str, strlen(mac_str)))
	{
		return -1;
	}

	if( 0 != ef_set_env_blob(NV_WIFI_AP_MAC, mac_str, strlen(mac_str)))
	{
		return -1;
	}
	
	return wifi_set_mac_addr(0,mac_addr);

}

int32_t ya_hal_wlan_get_ip_address(uint32_t *ipv4_addr)
{
	wifi_get_ip_addr(0,ipv4_addr);
	return 0;
}

int32_t ya_hal_wlan_get_ip_address_with_string(uint8_t *ip, uint8_t len)
{
	unsigned int ip_tep;
	uint8_t *ip_temp = (uint8_t *)ip_tep;
	wifi_get_ip_addr(0,&ip_tep);
	snprintf(ip, len, "%d.%d.%d.%d", ip_temp[0], ip_temp[1], ip_temp[2], ip_temp[3]);
	return 0;
}



int32_t ya_hal_wlan_get_gateway_ip_address(uint32_t *gate_addr)
{
	wifi_get_gw_addr(0,gate_addr);
	return 0;
}

int32_t ya_hal_wlan_get_rssi(int32_t *rssi)
{
	return -1; 	
}

int32_t ya_hal_wlan_get_channel(uint32_t *channel)
{	
	unsigned char uc;
	uc=wifi_rf_get_channel();
	*channel=uc;
	return 0;
}


int32_t ya_hal_wlan_set_channel(uint32_t channel)
{
	wifi_rf_set_channel(channel);
	return 0;
}


int32_t ya_hal_wlan_scan_obj_ssid(ya_obj_ssid_result_t *obj_scan_ssid, uint8_t num)
{
	int i = 0;
	int ret = -1;
	uint8_t index = 0;
	ya_obj_ssid_result_t *p = NULL;
	
	if (0 != wifi_scan_start(1,NULL))
	{
		return ret;
	}

	int wifi_ap_num = wpa_get_scan_num();
	ya_printf(C_LOG_INFO, "wpa_get_scan_num = %d\r\n", wifi_ap_num);
	if (wifi_ap_num > 0)
	{
		wifi_info_t wifi_inf = {0};
		for (i = 0; i < wifi_ap_num; ++i) 
		{
			memset(&wifi_inf, 0, sizeof(wifi_inf));
			wpa_get_scan_result(i, &wifi_inf);

			ya_printf(C_LOG_INFO, "scan result: ssid:%s, rssi:%d\r\n", wifi_inf.ssid, wifi_inf.rssi);

			p = obj_scan_ssid;
			for	(index = 0; index < num; index++)
			{
				if(strcmp(wifi_inf.ssid, p->scan_ssid) == 0)
				{
					p->scan_result = 1;
					p->rssi = wifi_inf.rssi;
					ret = 0;
				}
				p++;
			}
				
		}
	}
	return ret;
}


int32_t ya_wlan_connect_ap(char *sSsid, char *sPassword, int iSsidLen, int iPasswordLen, void *pSemaphpre, uint8_t *bssid)
{
	int count = 0;
	int vif = 0;
	unsigned int sta_ip;
		
	if(iSsidLen == 0 || iSsidLen > 32 || iPasswordLen > 64)
	{
		wifi_event_handler(YA_HAL_EVT_LINK_LOST, NULL);
		return -1;
	}

	wifi_remove_config_all(vif);
	wifi_add_config(vif);
	wifi_config_ssid(STATION_IF, (unsigned char *)sSsid);
	
	if(iPasswordLen == 0 || sPassword == NULL)
		wifi_set_password(STATION_IF, NULL);
	else
		wifi_set_password(STATION_IF, sPassword);

	wifi_config_commit(0);

	wifi_get_ip_addr(STATION_IF, &sta_ip);
	while(sta_ip == 0 && count <= 300){
		vTaskDelay((portTickType)(100 / portTICK_RATE_MS));
		wifi_get_ip_addr(STATION_IF, &sta_ip);
		count++;
	}

	if(sta_ip == 0){
		wifi_event_handler(YA_HAL_EVT_AP_START_FAILED, NULL);
		return -1;
	}

	return 0;
}


ya_hal_wlan_ap_connect_param_t ya_wifi_info;
static void ya_wlan_connect(void *param)
{
	ya_wlan_connect_ap(ya_wifi_info.ssid, ya_wifi_info.pwd, strlen(ya_wifi_info.ssid), strlen(ya_wifi_info.pwd), NULL,ya_wifi_info.bssid);
	vTaskDelete(NULL);
}

int32_t ya_hal_wlan_connect_ap(ya_hal_wlan_ap_connect_param_t *connect_param)
{
	memset(&ya_wifi_info, 0, sizeof(ya_hal_wlan_ap_connect_param_t));
	memcpy(&ya_wifi_info, connect_param, sizeof(ya_hal_wlan_ap_connect_param_t));
	if(xTaskCreate(ya_wlan_connect, "ya_wlan_connect", (1024), NULL, tskIDLE_PRIORITY + 3, NULL) != pdPASS)
		return -1;

	return 0;
}


ya_monitor_data_cb_t g_monitor_callback = NULL;

int ya_wifi_stop_monitor(void)
{
	wifi_set_promiscuous(false);
	g_monitor_callback = NULL;
	return 0;
}

static void wifi_promisc_hdl(void *buf, int len, wifi_promiscuous_pkt_type_t type)
{
	wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
	int with_fcs = 1;
	if(type == WIFI_PKT_MGMT){
		with_fcs = 0;
	} else if(type == WIFI_PKT_DATA){
		if(pkt->payload[1] & 0x40){
			with_fcs = 1;
		} else {
			with_fcs = 0;
		}
		
	}

	if(with_fcs)
		len -= 4;

	if(g_monitor_callback){
		g_monitor_callback(pkt->payload, len); 
	 }
}


int ya_wifi_start_monitor(ya_monitor_data_cb_t fn)
{
	wifi_promiscuous_filter_t filter = {0};
	filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA | WIFI_PROMIS_FILTER_MASK_MGMT;
	wifi_set_promiscuous_filter(&filter);

	g_monitor_callback = fn;
	wifi_set_promiscuous_rx_cb(wifi_promisc_hdl);	
	
	wifi_set_promiscuous(true);
	return 0;
}


