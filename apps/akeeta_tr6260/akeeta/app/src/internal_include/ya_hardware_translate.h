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

#ifndef __YA_HARDWARE_TRANSLATE__
#define __YA_HARDWARE_TRANSLATE__

/**
 * @brief This function is the one used to handle the translate from cloud json to thing mode or other apps(such as timer).
 *
 * @return 0: sucess, -1: failed
 */
extern int32_t ya_handle_cloud_data(uint8_t *buf, uint16_t len);

#endif
