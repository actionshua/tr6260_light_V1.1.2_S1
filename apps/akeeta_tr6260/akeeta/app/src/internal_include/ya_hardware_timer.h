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

#ifndef _YA_HARDWARE_TIMER_H_
#define _YA_HARDWARE_TIMER_H_

#define SEVER_TIME_UPDATE_PERIOD  (2*60*60*1000)

extern int32_t device_read_timer_into_flash(void);

extern int32_t device_handle_cloud_timer(uint8_t *data_buf, uint8_t data_len);

extern int32_t device_handle_severtime_set(uint8_t *data_buf, uint8_t data_len);

extern void sys_abstime_init(void);

extern void sys_start_abstime(void);

extern void sys_run_abstime_1s(void);

extern void ya_hardware_timer_init(void);
extern void ya_clear_timer_flash(void);
#endif

