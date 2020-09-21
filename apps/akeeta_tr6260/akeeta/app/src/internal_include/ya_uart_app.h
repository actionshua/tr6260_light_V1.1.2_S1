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
 

#ifndef __UART_CMD_H_
#define __UART_CMD_H_

typedef enum
{
	UART_WITHOUT_REPLY = 0,
	UART_WITH_REPLY,
}uart_msg_pos_t;
	

extern int32_t ya_handle_uart_body_send_to_queue(uint8_t cmd, uint8_t *data, uint16_t data_len);

extern int32_t ya_handle_uart_msg_send_to_queue(uint8_t *data, uint16_t data_len, uint8_t reply_flag);

extern int32_t ya_start_uart_app(void);

extern void ya_add_uart_listen_event_from_cloud(void);


#endif

