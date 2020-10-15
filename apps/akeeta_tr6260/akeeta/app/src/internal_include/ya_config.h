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

#ifndef _YA_CONFIG_H_
#define _YA_CONFIG_H_

#define US_CLOUD_SUPPORT   		 0
#define CN_CLOUD_SUPPORT   		 1
#define US_CN_CLOUD_SUPPORT      2      

#define CLOUD_SUPPORT			US_CN_CLOUD_SUPPORT

//==============================chips choose==============================================

#define	 YA_8710_BX	   		0
#define  YA_8710_CX		 	1
#define  YA_6260_S1			2
#define  YA_7231U			3

#define YA_CHIP   		YA_6260_S1

#if (YA_CHIP == YA_8710_BX)
#define YA_BASE_ADDR										(0x08000000)

#define 	FLASH_SIZE		1
#if FLASH_SIZE == 0
#define YA_USER_DATA_ADDR									(0x080F5000 - YA_BASE_ADDR)
#define OTA2_START_ADDR										0x08080000
#else
#define YA_USER_DATA_ADDR									(0x0814B000 - YA_BASE_ADDR)
#define OTA2_START_ADDR										0x080AB000
#endif

#define YA_USER_DATA_BACK_ADDR 								(YA_USER_DATA_ADDR + 0x1000)
#define YA_LICENSE_DEVICE_CERT_ADDR							(YA_USER_DATA_BACK_ADDR + 0x1000)
#define YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR   			(YA_LICENSE_DEVICE_CERT_ADDR + 0x1000)
#define YA_LICENSE_DEVICE_ID_ADDR							(YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR + 0x1000)
#define YA_LICENSE_THING_TYPE_ADDR							(YA_LICENSE_DEVICE_ID_ADDR + 0x1000)

#define YA_LICENSE_ALIYUUN									(YA_LICENSE_THING_TYPE_ADDR + 0x1000)
#define YA_OTA_DATA_ADDR 									(YA_LICENSE_ALIYUUN + 0x1000)
#define YA_DEVICE_TIMER_ADDR								(YA_OTA_DATA_ADDR + 0x1000)

#endif

#if (YA_CHIP == YA_8710_CX)

// 8K  user data
#define YA_USER_DATA_ADDR									(0x180000)
#define YA_USER_DATA_BACK_ADDR 								(YA_USER_DATA_ADDR + 0x1000)

// 16K aws data
#define YA_LICENSE_DEVICE_CERT_ADDR							(YA_USER_DATA_BACK_ADDR + 0x1000)
#define YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR   			(YA_LICENSE_DEVICE_CERT_ADDR + 0x1000)
#define YA_LICENSE_DEVICE_ID_ADDR							(YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR + 0x1000)
#define YA_LICENSE_THING_TYPE_ADDR							(YA_LICENSE_DEVICE_ID_ADDR + 0x1000)

//4K ali data
#define YA_LICENSE_ALIYUUN									(YA_LICENSE_THING_TYPE_ADDR + 0x1000)

// 4K ota data
#define YA_OTA_DATA_ADDR 									(YA_LICENSE_ALIYUUN + 0x1000)

// 8K timer data
#define YA_DEVICE_TIMER_ADDR								(YA_OTA_DATA_ADDR + 0x1000)
#endif

#if (YA_CHIP == YA_6260_S1)
#define YA_USER_DATA_ADDR									(0xAD000)
#define YA_USER_DATA_BACK_ADDR 								(YA_USER_DATA_ADDR + 0x1000)

#define YA_LICENSE_DEVICE_CERT_ADDR							(YA_USER_DATA_BACK_ADDR + 0x1000)
#define YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR   			(YA_LICENSE_DEVICE_CERT_ADDR + 0x1000)
#define YA_LICENSE_DEVICE_ID_ADDR							(YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR + 0x1000)
#define YA_LICENSE_THING_TYPE_ADDR							(YA_LICENSE_DEVICE_ID_ADDR + 0x1000)

#define YA_LICENSE_ALIYUUN									(YA_LICENSE_THING_TYPE_ADDR + 0x1000)
#define YA_OTA_DATA_ADDR 									(YA_LICENSE_ALIYUUN + 0x1000)
#define YA_DEVICE_TIMER_ADDR								(YA_OTA_DATA_ADDR + 0x1000)
#endif

#if (YA_CHIP == YA_7231U)
#define YA_USER_DATA_ADDR									0x1F0000 //(YA_USER_DATA_BACK_ADDR - 0x1000)//(0x1E2000)
#define YA_USER_DATA_BACK_ADDR 								(YA_USER_DATA_ADDR + 0x1000)//(YA_LICENSE_DEVICE_CERT_ADDR - 0x1000)

#define YA_LICENSE_DEVICE_CERT_ADDR							(YA_USER_DATA_BACK_ADDR + 0x1000)
#define YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR   			(YA_LICENSE_DEVICE_CERT_ADDR + 0x1000)
#define YA_LICENSE_DEVICE_ID_ADDR							(YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR + 0x1000)
#define YA_LICENSE_THING_TYPE_ADDR							(YA_LICENSE_DEVICE_ID_ADDR + 0x1000)

#define YA_LICENSE_ALIYUUN									(YA_LICENSE_THING_TYPE_ADDR + 0x1000)//(YA_OTA_DATA_ADDR - 0x1000)
#define YA_OTA_DATA_ADDR 									(YA_LICENSE_ALIYUUN + 0x1000)//(YA_DEVICE_TIMER_ADDR - 0x1000)
#define YA_DEVICE_TIMER_ADDR								(YA_OTA_DATA_ADDR + 0x1000) //0x1EA000 (YA_FLASH_ADDR_END - 0x8000)
//#define MAX_TIMER_DEVICE_DATA		0x4000	<===¡À¡ê¡ä?¨º¡À?? ¡Á?¡ä¨®¡ä?¡ä¡é????
//left 4k for reserved
#define YA_FLASH_ADDR_END									(0x200000)//2M add by shixian
#endif


#define 	WIF_SDK_VERSION			"akeeta.sdk.207"
typedef enum{
	WIFI_CN = 0,
	WIFI_US,
	WIFI_EU,

	WIFI_MAX,
}MODULE_DOMAIN;


#define AWS_DOMAIN_SUPPORT					((1<<WIFI_US) + (1<<WIFI_EU))

#define AWS_IOT_MQTT_HOST_US              "a1q3ywz7wfeo4i-ats.iot.us-west-2.amazonaws.com" ///< Customer specific MQTT HOST. The same will be used for Thing Shadow
#define AWS_IOT_MQTT_HOST_EU			  "a1q3ywz7wfeo4i-ats.iot.eu-central-1.amazonaws.com"

#define AWS_IOT_MQTT_PORT              443  ///< default port for MQTT/S


#include "ya_app_main.h"
#include "cloud.h"
#include "ya_mode_api.h"
#include "ya_cert_flash_api.h"
#include "ya_api_thing_uer_define.h"

extern bool ya_get_debind_enable(void);

extern void ya_debind_enable(void);

extern void ya_debind_disable(void);

extern int ya_get_randomnum(char *num);

extern void ya_clear_randomnum(void);

extern void ya_get_ver_string(char* obj);

extern void ya_get_mcu_ver_string(char* obj);

extern void ya_set_mcu_ver_string(char* obj);

extern bool ya_get_mcu_ota_enbale(void);

extern int ya_get_cloud_support(void);

/**
 * @brief This function is used to get the mac string.
 * @param obj is the mac string.
 * @return 0: sucess, -1: failed
 */
extern void ya_get_mac_addr_string(char* obj);


extern void ya_callback_servertimer_set(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, uint8_t week);

extern uint32_t ya_get_wifi_cloud_support(void);

#endif

