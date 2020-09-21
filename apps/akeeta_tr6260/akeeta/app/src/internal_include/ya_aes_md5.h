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
 

#ifndef __AES_MD5_H_
#define __AES_MD5_H_


#define AES_KEY_ADD_STRING "nm123##&"

#define AES_KEY_SMARTCONFIG_STRING "ABKFnm123##&"

extern int ya_mbedtls_md5(uint8_t *input, uint8_t *output, uint32_t len);

extern uint32_t AES_CBC_PKCS5Padding_encrypt(uint8_t *p_aes_key, uint8_t *iv, uint8_t *plaintext, uint8_t *ciphertext, uint32_t len);

extern uint32_t AES_CBC_PKCS5Padding_decrypt(uint8_t *p_aes_key, uint8_t *iv, uint8_t *ciphertext, uint8_t *plaintext, uint32_t len);

extern uint32_t AES_CFB_NoPadding_decrypt(uint8_t *p_aes_key, uint8_t *iv, uint8_t *ciphertext, uint8_t *plaintext, uint32_t len);

extern void HexArrayToString(uint8_t *hexarray, uint16_t length, char *string);

extern int StringToHexArray(uint8_t *hexarray,uint16_t length, char *string);

#endif


