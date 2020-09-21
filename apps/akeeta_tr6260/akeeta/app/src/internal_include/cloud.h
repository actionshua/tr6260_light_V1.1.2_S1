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

#ifndef _CLOUD_H_
#define _CLOUD_H_

typedef enum
{
	CLOUD_ONOFF_EVENT				= 0x00,	
	CLOUD_SET_EVENT,

	CLOUD_UPGRADE_EVENT,
}cloud_event_type_t;


typedef void (*cloud_event_callback)(uint8_t *data, uint16_t data_len);


typedef int32_t (*cloud_add_listener_t)(char *listener_name, cloud_event_type_t event_type, cloud_event_callback cloud_event_handler);
typedef int32_t (*cloud_remove_listener_t)(char *listener_name, cloud_event_type_t event_type);
typedef int32_t (*cloud_send_msg_t)(uint8_t *data, uint16_t data_len);
typedef int32_t (*cloud_start_t)(void);
typedef char *(*cloud_get_thing_name_t)(void);
typedef char *(*cloud_get_client_id_t)(void);
typedef int32_t (*cloud_get_status_t)(void);
typedef int32_t (*cloud_unregister_t)(void);

typedef struct
{
	// class functions
	cloud_add_listener_t 		add_listener;
	cloud_remove_listener_t 	remove_listener;
	cloud_send_msg_t	 		send_msg;
	cloud_start_t 				start_cloud;
	cloud_get_client_id_t		cloud_get_client_id;
	cloud_get_thing_name_t		cloud_get_thing_name;
	cloud_get_status_t			cloud_get_status;
	cloud_unregister_t			cloud_unregister;
}cloud_context_t;

/**
 * @brief This function is the one used to perform add the event listener action into the cloud events.
 *
 * @param listener_name: the obj add listener name
 * @param event_type: the obj add cloud event name
 * @return 0: sucess, -1: failed
 */
extern int32_t cloud_add_event_listener(char *listener_name, cloud_event_type_t event_type, cloud_event_callback ya_cloud_event_chandler);

/**
 * @brief This function is the one used to perform remove the event listener action from the cloud events.
 *
 * @param listener_name: the obj remove listener name
 * @param event_type: the obj cloud event name
 * @return 0: sucess, -1: failed
 */
extern int32_t cloud_remove_event_listener(char *listener_name, cloud_event_type_t event_type);

/**
 * @brief This function is the one used to report data to aws cloud
 *
 * @param data: msg data addr
 * @param data_len: msg data len
 * @return 0: sucess, -1: failed
 */
extern int32_t cloud_report_msg(uint8_t *data, uint16_t data_len);

/**
 * @brief This function is used to create the cloud object.
 *
 * @param cloud_type: select the cloud type. AWS/ALIyun
 * @return 0: sucess, -1: failed
 */
extern int32_t cloud_create(MODULE_DOMAIN cloud_type);

/**
 * @brief This function is used to start the cloud 
 *
 * @param cloud_type: select the cloud type. AWS/ALIyun
 * @return 0: sucess, -1: failed
 */
extern int32_t cloud_start(CLOUD_T cloud_type);

/**
 * @brief This function is used to get the thing name.
 *
 * @return tihing name pointer
 */
extern char *ya_get_thing_type(void);

/**
 * @brief This function is used to get the thing name.
 *
 * @return client id pointer
 */

extern char *ya_get_client_id(void);


/**
 * @brief This function is used to get the cloud status
 *
 * @return cloud status
 */
int32_t ya_get_cloud_status(void);


/**
 * @brief This function is used to unregister the user.
 *
 * @return error or success
 */
int32_t ya_cloud_unregister(void);

#endif

