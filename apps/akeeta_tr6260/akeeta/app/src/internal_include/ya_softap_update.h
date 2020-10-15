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


#ifndef __YA_SOFTAP_UPDATE_H__
#define __YA_SOFTAP_UPDATE_H__ 

typedef struct 
{
	char router_ssid[32+1];
	char router_pwd[64+1];
	uint8_t router_bssid[6];
	char random[32+1];
	uint8_t cloud_type;
} ya_ap_router_para_t;

typedef struct _tag_softap_param_st
{
	int timeout;						// 超时时间（秒）
	char ap_ssid[33];           		// 设备进入AP的ssid
	char ap_password[65];       		// 设备进入AP的password
	uint8_t cloud_support_type;			// 0：cn, 1: us, 0xff: unknown
	void (*p_softap_config_cb)(uint8_t code, ya_ap_router_para_t *ya_ap_router_para);  
} softap_param_st; 

enum
{
	SOFTAP_CONFIG_FINISH = 0,
	SOFTAP_TIME_OUT,
	SOFTAP_ERROR,
};

#define ENABLE_FACTORY_TEST	

#define ENABLE_LICENSE_ONE_WRITE

int32_t ya_start_softap(softap_param_st * p_softap_param);

#endif

