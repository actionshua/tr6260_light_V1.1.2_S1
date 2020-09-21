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
#include "mbedtls_md5.h"
#include "aes.h"
#include "ya_aes_md5.h"

#define AES_BLOCK_SIZE 16

int ya_mbedtls_md5(uint8_t *input, uint8_t *output, uint32_t len)
{
	if((!input)||(!output)||(len <= 0))
		return -1;
	
	mbedtls_md5_context md5_ctx;
	mbedtls_md5_init(&md5_ctx);
	mbedtls_md5_starts(&md5_ctx);
	mbedtls_md5_update(&md5_ctx, input, len);
	mbedtls_md5_finish(&md5_ctx, output);
	mbedtls_md5_free(&md5_ctx);	

	return 0;
}

uint32_t AES_CBC_PKCS5Padding_encrypt(uint8_t *p_aes_key, uint8_t *iv, uint8_t *plaintext, uint8_t *ciphertext, uint32_t len) 
{
	uint32_t nBei = len / AES_BLOCK_SIZE + 1;
	uint32_t nTotal = nBei * AES_BLOCK_SIZE;

	uint8_t *enc_s = (uint8_t*)ya_hal_os_memory_alloc(nTotal);
	uint8_t nNumber = 0;
	if (len % AES_BLOCK_SIZE > 0)
	{
		nNumber = nTotal - len;
	}
	else
	{
		nNumber = AES_BLOCK_SIZE;
	}
	memset(enc_s, nNumber, nTotal);
	memcpy(enc_s, plaintext, len);

    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);

    mbedtls_aes_setkey_enc(&aes_ctx, p_aes_key, 128);
    mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, nTotal, iv, enc_s, ciphertext);

	mbedtls_aes_free(&aes_ctx);
	if(enc_s)
	{	
		ya_hal_os_memory_free(enc_s);
		enc_s=NULL;		
	}
	return nTotal;
}

uint32_t AES_CBC_PKCS5Padding_decrypt(uint8_t *p_aes_key, uint8_t *iv, uint8_t *ciphertext, uint8_t *plaintext, uint32_t len) 
{
	int ret = -1;
	uint32_t decrypt_data_len = 0;

	uint8_t *dec_s = (uint8_t*)ya_hal_os_memory_alloc(len);
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_dec(&aes_ctx, p_aes_key, 128);
    ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, len, iv, ciphertext, dec_s);

	if (ret != 0)
		ya_printf(C_LOG_INFO,"mbedtls_aes_crypt_cbc decode: %d\n",ret);

	memcpy(plaintext,dec_s,len);
    mbedtls_aes_free(&aes_ctx);
	if(dec_s)
		ya_hal_os_memory_free(dec_s);

	if(len > plaintext[len-1])
		decrypt_data_len = len - plaintext[len-1];

	return decrypt_data_len;
}

uint32_t AES_CFB_NoPadding_decrypt(uint8_t *p_aes_key, uint8_t *iv, uint8_t *ciphertext, uint8_t *plaintext, uint32_t len) 
{
	uint32_t decrypt_data_len = 0;	
	size_t offset;

	offset = 0;

	uint8_t *dec_s = (uint8_t*)ya_hal_os_memory_alloc(len);
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);

   // mbedtls_aes_setkey_dec(&aes_ctx, p_aes_key, 128);
	mbedtls_aes_setkey_enc(&aes_ctx, p_aes_key, 128);

	mbedtls_aes_crypt_cfb128( &aes_ctx, MBEDTLS_AES_DECRYPT, len, &offset, iv, ciphertext, dec_s );
	
	memcpy(plaintext,dec_s,len);
    mbedtls_aes_free(&aes_ctx);

	if(dec_s)
		ya_hal_os_memory_free(dec_s);		

	decrypt_data_len = len;

	return decrypt_data_len;
}

