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
#include "ya_flash.h"
#include "ya_config.h"
#include "mbedtls/ssl.h"

#define MBEDCHECK
//#define MBEDCHECK_READ

int32_t ya_aws_read_check_device_cert(char *device_cert, uint32_t cert_addr)
{
	uint32_t read_data_len = 0;
	int32_t ret = -1;
	uint8_t *buf = NULL;

	buf = (uint8_t *)ya_hal_os_memory_alloc(MAX_CERT_LEN);
	if(buf == NULL)
		return C_ERROR;

	memset(buf, 0, MAX_CERT_LEN);
	
	read_data_len = MAX_CERT_LEN - 1;
	ret = ya_read_flash_with_var_len(cert_addr, buf, &read_data_len, 1);

	if(ret != C_OK || read_data_len == 0)
		goto err;

	//check the device cert
	#ifdef MBEDCHECK_READ
	mbedtls_x509_crt chain;
	mbedtls_platform_set_calloc_free(ya_hal_os_memory_calloc, ya_hal_os_memory_free);
	mbedtls_x509_crt_init(&chain);	
    ret = mbedtls_x509_crt_parse(&chain, (char *)buf, read_data_len + 1);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "mbedtls_x509_crt_parse error: %d\n", ret);
		mbedtls_x509_crt_free(&chain);
		goto err;
	}
	mbedtls_x509_crt_free(&chain);
	#endif

	memset(device_cert, 0, read_data_len + 1);
	memcpy(device_cert, buf, read_data_len);

	#ifdef LICENSE_PRINT_ENABLE
	ya_printf(C_LOG_INFO, "aws_certificate: \n");
	ya_printf(C_LOG_INFO, "%s", device_cert);
	ya_printf(C_LOG_INFO, "\n");
	#endif

	if(buf)
		ya_hal_os_memory_free(buf);

	return C_OK;

	err:

	if(buf)
		ya_hal_os_memory_free(buf);

	return C_ERROR;
}

int32_t ya_aws_read_check_device_rsa_private_key(char *private_key, uint32_t private_key_addr)
{
	uint32_t read_data_len = 0;
	int32_t ret = -1;
	uint8_t *buf = NULL;

	buf = (uint8_t *)ya_hal_os_memory_alloc(MAX_RSA_LEN);
	if(buf == NULL)
		return C_ERROR;

	memset(buf, 0, MAX_RSA_LEN);
	read_data_len = MAX_RSA_LEN - 1;
	ret = ya_read_flash_with_var_len(private_key_addr, buf, &read_data_len, 1);

	if(ret != C_OK || read_data_len == 0)
		goto err;

	//check the device cert
#ifdef MBEDCHECK_READ
	mbedtls_pk_context pk;	
	mbedtls_platform_set_calloc_free(ya_hal_os_memory_calloc, ya_hal_os_memory_free); 
	mbedtls_pk_init(&pk);	
	ret = mbedtls_pk_parse_key(&pk, (char *)buf, read_data_len+ 1, (const unsigned char *)"", strlen(""));
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "mbedtls_pk_parse_key error: %d\n", ret);
		mbedtls_pk_free(&pk);
		goto err;
	}
	mbedtls_pk_free(&pk);
#endif
	
	memset(private_key, 0, read_data_len + 1);
	memcpy(private_key, buf, read_data_len);

#ifdef LICENSE_PRINT_ENABLE
	ya_printf(C_LOG_INFO, "aws_privateKey: \n");
	ya_printf(C_LOG_INFO, "%s", private_key);
	ya_printf(C_LOG_INFO, "\n");
#endif

	if(buf)
		ya_hal_os_memory_free(buf);

	return C_OK;

	err:

	if(buf)
		ya_hal_os_memory_free(buf);

	return C_ERROR;
}

int32_t ya_aws_read_check_device_id(char *device_id, uint32_t device_id_addr)
{
	uint32_t read_data_len = 0;
	int32_t ret = -1;
	uint8_t *buf = NULL;
	
	buf = (uint8_t *)ya_hal_os_memory_alloc(MAX_CLIENT_ID_LEN);
	if(buf == NULL)
		return C_ERROR;

	memset(buf, 0, MAX_CLIENT_ID_LEN);
	
	read_data_len = MAX_CLIENT_ID_LEN - 1;
	ret = ya_read_flash_with_var_len(device_id_addr, buf, &read_data_len, 1);

	if(ret != C_OK || read_data_len == 0)
		goto err;
	
	memset(device_id, 0, read_data_len + 1);
	memcpy(device_id, buf, read_data_len);

#ifdef LICENSE_PRINT_ENABLE
	ya_printf(C_LOG_INFO, "aws_client_id: \n");
	ya_printf(C_LOG_INFO, "%s", device_id);
	ya_printf(C_LOG_INFO, "\n");
#endif

	if(buf)
		ya_hal_os_memory_free(buf);

	return C_OK;

	err:

	if(buf)
		ya_hal_os_memory_free(buf);

	return C_ERROR;
}

int32_t ya_aws_read_check_thing_type(char *thing_type, uint32_t thing_type_addr)
{
	uint32_t read_data_len = 0;
	int32_t ret = -1;
	uint8_t *buf = NULL;

	buf = (uint8_t *)ya_hal_os_memory_alloc(MAX_THING_TYPE_LEN);
	if(buf == NULL)
		return C_ERROR;

	memset(buf, 0, MAX_THING_TYPE_LEN);
	
	read_data_len = MAX_THING_TYPE_LEN - 1;
	ret = ya_read_flash_with_var_len(thing_type_addr, buf, &read_data_len, 1);

	if(ret != C_OK || read_data_len == 0)
		goto err;

	memset(thing_type, 0, read_data_len + 1);
	memcpy(thing_type, buf, read_data_len);

#ifdef LICENSE_PRINT_ENABLE
	ya_printf(C_LOG_INFO, "aws_thing_type: \n");
	ya_printf(C_LOG_INFO, "%s", thing_type);
	ya_printf(C_LOG_INFO, "\n");
#endif

	if(buf)
		ya_hal_os_memory_free(buf);

	return C_OK;

	err:

	if(buf)
		ya_hal_os_memory_free(buf);

	return C_ERROR;
}

int32_t ya_write_check_aws_para_device_cert(char *buf, uint16_t data_len, uint32_t cert_addr)
{
	int32_t ret = -1;
	
	//check the device cert
#ifdef MBEDCHECK
	mbedtls_x509_crt chain;
	mbedtls_platform_set_calloc_free(ya_hal_os_memory_calloc, ya_hal_os_memory_free);
	mbedtls_x509_crt_init(&chain);	
	buf[data_len] = '\0';
	ret = mbedtls_x509_crt_parse(&chain, (const unsigned char *)buf, data_len + 1);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "mbedtls_x509_crt_parse error: %d\n", ret);
		mbedtls_x509_crt_free(&chain);
		return C_ERROR;
	}
	mbedtls_x509_crt_free(&chain);
#endif
	
	ret= ya_write_flash(cert_addr, (uint8_t *)buf, data_len, 1);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "ya_write_flash cert error\n");
		return C_ERROR;
	}
	return C_OK;
}

int32_t ya_write_check_aws_para_device_rsa_private_key(char *buf, uint16_t data_len, uint32_t private_key_addr)
{
	int32_t ret = -1;
	
	//check the device cert
#ifdef MBEDCHECK
	mbedtls_pk_context pk;
	mbedtls_platform_set_calloc_free(ya_hal_os_memory_calloc, ya_hal_os_memory_free);
	mbedtls_pk_init(&pk);
	buf[data_len] = '\0';
	ret = mbedtls_pk_parse_key(&pk, (const unsigned char *)buf, data_len+ 1, (const unsigned char *)"", strlen(""));
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "mbedtls_pk_parse_key error: %d\n", ret);
		mbedtls_pk_free(&pk);
		return C_ERROR;
	}
	mbedtls_pk_free(&pk);
#endif
	
	ret= ya_write_flash(private_key_addr, (uint8_t *)buf, data_len, 1);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "ya_write_flash rsa error\n");
		return C_ERROR;
	}
	return C_OK;
}


int32_t ya_write_check_aws_para_device_id(char *buf, uint16_t data_len, uint32_t device_id_addr)
{
	int32_t ret = -1;

	ret= ya_write_flash(device_id_addr, (uint8_t *)buf, data_len, 1);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "ya_write_flash error\n");
		return C_ERROR;
	}
	return C_OK;
}


int32_t ya_write_check_aws_para_thing_type(char *buf, uint16_t data_len, uint32_t thing_type_addr)
{
	int32_t ret = -1;

	ret= ya_write_flash(thing_type_addr, (uint8_t *)buf, data_len, 1);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "ya_write_flash error\n");
		return C_ERROR;
	}

	return C_OK;
}

char *aws_certificateBuff = NULL;    //device cert
char *aws_privateKeyBuff = NULL;     //device rsa
char *aws_client_id = NULL; 			//device client id
char *aws_thing_type = NULL; 		//device thing type

int ya_aws_para_free(void)
{
	if (aws_certificateBuff)
	{
		ya_hal_os_memory_free(aws_certificateBuff);
		aws_certificateBuff = NULL;
	}

	if (aws_privateKeyBuff)
	{
		ya_hal_os_memory_free(aws_privateKeyBuff);
		aws_privateKeyBuff = NULL;
	}

	if (aws_client_id)
	{
		ya_hal_os_memory_free(aws_client_id);
		aws_client_id = NULL;
	}

	if (aws_thing_type)
	{
		ya_hal_os_memory_free(aws_thing_type);
		aws_thing_type = NULL;
	}

	return 0;
}


int ya_aws_para_init(void)
{
	int32_t ret = -1;

	if (aws_certificateBuff == NULL)
	{
		aws_certificateBuff = (char *)ya_hal_os_memory_alloc(MAX_CERT_LEN);
		if(!aws_certificateBuff) goto err;
		memset(aws_certificateBuff, 0, MAX_CERT_LEN);

		ret = ya_aws_read_check_device_cert(aws_certificateBuff, YA_LICENSE_DEVICE_CERT_ADDR);
		if(ret != C_OK)
		{
			ya_printf(C_LOG_ERROR, "ya_aws_read_check_device_cert error\n");
			goto err;
		}

		ya_printf(C_LOG_INFO, "aws device cert ok\n");
	}

	if (aws_privateKeyBuff == NULL)
	{
		aws_privateKeyBuff = (char *)ya_hal_os_memory_alloc(MAX_RSA_LEN);
		if(!aws_privateKeyBuff) goto err;
		memset(aws_privateKeyBuff, 0, MAX_RSA_LEN);

		ret = ya_aws_read_check_device_rsa_private_key(aws_privateKeyBuff, YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR);
		if(ret != C_OK)
		{
			ya_printf(C_LOG_ERROR, "ya_aws_read_check_device_cert error\n");
			goto err;
		}

		ya_printf(C_LOG_INFO, "aws private key ok\n");
	}

	if (aws_client_id == NULL)
	{
		aws_client_id = (char *)ya_hal_os_memory_alloc(MAX_CLIENT_ID_LEN);
		if(!aws_client_id) goto err;
		memset(aws_client_id, 0, MAX_CLIENT_ID_LEN);

		ret = ya_aws_read_check_device_id(aws_client_id, YA_LICENSE_DEVICE_ID_ADDR);
		if(ret != C_OK)
		{
			ya_printf(C_LOG_ERROR, "ya_aws_read_check_device_id error\n");
			goto err;
		}

		ya_printf(C_LOG_INFO, "aws client id ok\n");
	}

	if (aws_thing_type == NULL)
	{
		aws_thing_type = (char *)ya_hal_os_memory_alloc(MAX_THING_TYPE_LEN);
		if(!aws_thing_type) goto err;
		memset(aws_thing_type, 0, MAX_THING_TYPE_LEN);

		ret = ya_aws_read_check_thing_type(aws_thing_type, YA_LICENSE_THING_TYPE_ADDR);
		if(ret != C_OK)
		{
			ya_printf(C_LOG_ERROR, "ya_aws_read_check_thing_type error\n");
			goto err;
		}

		ya_printf(C_LOG_INFO, "aws thing type ok\n");
	}
	return C_OK;

	err:

	ya_aws_para_free();

	return C_ERROR;
}

char *ya_get_private_key(void)
{
	return aws_privateKeyBuff;
}

char *ya_get_client_cert(void)
{
	return aws_certificateBuff;
}

char *ya_aws_get_client_id(void)
{
	return aws_client_id;
}

char *ya_aws_get_thing_type(void)
{
	return aws_thing_type;
}

char *aliyun_productkey = NULL;    
char *aliyun_deviceid = NULL;     
char *aliyun_productsecret = NULL; 	

int32_t ya_read_aliyun_cert(uint8_t *buf, uint32_t *data_len, uint32_t aliyun_addr)
{
	return ya_read_flash_with_var_len(aliyun_addr, buf, data_len, 1);
}

int32_t ya_read_parse_aliyun_cert(uint32_t aliyun_addr)
{
	uint32_t read_data_len = 0, index = 0;
	int32_t ret = -1;
	uint8_t *buf = NULL, count = 0;

	read_data_len = 1024;
	buf = (uint8_t *)ya_hal_os_memory_alloc(read_data_len);
	if(buf == NULL)
		return C_ERROR;

	memset(buf, 0, read_data_len);
	ret = ya_read_flash_with_var_len(aliyun_addr, buf, &read_data_len, 1);

	if(ret != C_OK || read_data_len == 0)
		return C_ERROR;

	for (index = 0; index < read_data_len;)
	{
		if (count == 0)
		{
			if (buf[index] > YA_PRODUCT_KEY_LEN) goto err;
			if (aliyun_productkey == NULL)
			{
				aliyun_productkey = (char *)ya_hal_os_memory_alloc(buf[index] + 1);
				if(!aliyun_productkey) goto err;
			}

			memset(aliyun_productkey, 0, buf[index] + 1);
			memcpy(aliyun_productkey, &buf[index + 1], buf[index]);
		}else if (count == 1)
		{
			if(buf[index] > YA_DEVICE_ID_LEN) goto err;
			if (aliyun_deviceid == NULL)
			{
				aliyun_deviceid = (char *)ya_hal_os_memory_alloc(buf[index] + 1);
				if(!aliyun_deviceid) goto err;
			}
			
			memset(aliyun_deviceid, 0, buf[index] + 1);
			memcpy(aliyun_deviceid, &buf[index + 1], buf[index]);
		}else if (count == 2)
		{
			if(buf[index] > YA_DEVICE_SECRET_LEN) goto err;
			if (aliyun_productsecret == NULL)
			{
				aliyun_productsecret = (char *)ya_hal_os_memory_alloc(buf[index] + 1);
				if(!aliyun_productsecret) goto err;
			}		

			memset(aliyun_productsecret, 0, buf[index] + 1);	
			memcpy(aliyun_productsecret, &buf[index + 1], buf[index]);
		}else
		{
			break;
		}

		index = index + 1 + buf[index];
		count++;
	}
	

#if 1
	ya_printf(C_LOG_INFO, "aliyun_productkey: \n");
	ya_printf(C_LOG_INFO, "%s", aliyun_productkey);
	ya_printf(C_LOG_INFO, "\n");
	ya_printf(C_LOG_INFO, "aliyun_deviceid: \n");
	ya_printf(C_LOG_INFO, "%s", aliyun_deviceid);
	ya_printf(C_LOG_INFO, "\n");
	ya_printf(C_LOG_INFO, "aliyun_productsecret: \n");
	ya_printf(C_LOG_INFO, "%s", aliyun_productsecret);
	ya_printf(C_LOG_INFO, "\n");
#endif

	if(buf)
		ya_hal_os_memory_free(buf);

	return C_OK;

	err:

	if (buf)
		ya_hal_os_memory_free(buf);

	return C_ERROR;
}

int32_t ya_write_aliyun_cert(uint8_t *buf, uint16_t data_len, uint32_t aliyun_addr)
{
	int32_t ret = -1;
	uint32_t index = 0;
	uint8_t count = 0;

	for (index = 0; index < data_len;)
	{
		if (count == 0)
		{
			if(buf[index] > YA_PRODUCT_KEY_LEN) return C_ERROR;
		}else if (count == 1)
		{
			if(buf[index] > YA_DEVICE_ID_LEN) return C_ERROR;
		}else if (count == 2)
		{
			if(buf[index] > YA_DEVICE_SECRET_LEN) return C_ERROR;
		}else
		{
			break;
		}
		
		index = index + 1 + buf[index];
		count++;
	}

	ret= ya_write_flash(aliyun_addr, (uint8_t *)buf, data_len,1);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "ya_write_flash error\n");
		return C_ERROR;
	}
	return C_OK;	
}

int ya_get_productKey(char product_key[YA_PRODUCT_KEY_LEN + 1])
{
	if (aliyun_productkey)
	{
		strcpy(product_key, aliyun_productkey);
		return strlen(product_key);
	} else
	{
		memset(product_key, 0, YA_PRODUCT_KEY_LEN + 1);
		return 0;
	}
}

int ya_get_deviceID(char device_id[YA_DEVICE_ID_LEN + 1])
{
	if (aliyun_deviceid)
	{
		strcpy(device_id, aliyun_deviceid);
		return strlen(device_id);
	} else
	{
		memset(device_id, 0, YA_DEVICE_ID_LEN + 1);
		return 0;
	}
}

int ya_get_deviceSecret(char device_secret[YA_DEVICE_SECRET_LEN + 1])
{
	if (aliyun_productsecret)
	{
		strcpy(device_secret, aliyun_productsecret);
		return strlen(device_secret);
	} else 
	{
		memset(device_secret, 0, YA_DEVICE_SECRET_LEN + 1);
		return 0;
	}
}

int ya_aliyun_para_init(void)
{
	if (aliyun_productkey == NULL || aliyun_deviceid == NULL || aliyun_productsecret == NULL)
	{
		return ya_read_parse_aliyun_cert(YA_LICENSE_ALIYUUN);
	}else
	{
		return 0;
	}
}

int ya_aliyun_para_free(void)
{
	if (aliyun_productkey)
	{
		ya_hal_os_memory_free(aliyun_productkey);
		aliyun_productkey = NULL;
	}

	if (aliyun_deviceid)
	{
		ya_hal_os_memory_free(aliyun_deviceid);
		aliyun_deviceid = NULL;
	}

	if (aliyun_productsecret)
	{
		ya_hal_os_memory_free(aliyun_productsecret);
		aliyun_productsecret = NULL;
	}

	return 0;
}

int ya_test_akeeta_cn_with_obj_cert(char *productkey, char *deviceid, char *secret)
{
	if(productkey == NULL || deviceid == NULL || secret == NULL)
		return C_ERROR;

	if(strlen(productkey) > YA_PRODUCT_KEY_LEN || strlen(deviceid) > YA_DEVICE_ID_LEN || strlen(secret) > YA_DEVICE_SECRET_LEN )
		return C_ERROR;

	if(aliyun_productkey == NULL)
		aliyun_productkey = (char *)ya_hal_os_memory_alloc(YA_PRODUCT_KEY_LEN + 1);
	
	memset(aliyun_productkey, 0, YA_PRODUCT_KEY_LEN + 1);

	if(aliyun_deviceid == NULL)
		aliyun_deviceid = (char *)ya_hal_os_memory_alloc(YA_DEVICE_ID_LEN + 1);
	
	memset(aliyun_deviceid, 0, YA_DEVICE_ID_LEN + 1);

	if(aliyun_productsecret == NULL)
		aliyun_productsecret = (char *)ya_hal_os_memory_alloc(YA_DEVICE_SECRET_LEN + 1);
	
	memset(aliyun_productsecret, 0, YA_DEVICE_SECRET_LEN + 1);

	strcpy(aliyun_productkey, productkey);
	strcpy(aliyun_deviceid, deviceid);
	strcpy(aliyun_productsecret, secret);

	return C_OK;
}

int ya_test_akeeta_us_with_obj_cert(char *certificateBuff, char *privateKeyBuff, char *client_id,  char *thing_type)
{
	if(certificateBuff == NULL || privateKeyBuff == NULL || client_id == NULL || thing_type == NULL)
		return C_ERROR;

	if(aws_certificateBuff == NULL)
		aws_certificateBuff = (char *)ya_hal_os_memory_alloc(MAX_CERT_LEN);
	
	memset(aws_certificateBuff, 0, MAX_CERT_LEN);

	if(aws_privateKeyBuff == NULL)
		aws_privateKeyBuff = (char *)ya_hal_os_memory_alloc(MAX_RSA_LEN);
	
	memset(aws_privateKeyBuff, 0, MAX_RSA_LEN);

	if(aws_client_id == NULL)
		aws_client_id = (char *)ya_hal_os_memory_alloc(MAX_CLIENT_ID_LEN);
	
	memset(aws_client_id, 0, MAX_CLIENT_ID_LEN);

	if(aws_thing_type == NULL)
		aws_thing_type = (char *)ya_hal_os_memory_alloc(MAX_THING_TYPE_LEN);
	
	memset(aws_thing_type, 0, MAX_THING_TYPE_LEN);


	strcpy(aws_certificateBuff, certificateBuff);
	strcpy(aws_privateKeyBuff, privateKeyBuff);
	strcpy(aws_client_id, client_id);
	strcpy(aws_thing_type, thing_type);

	return C_OK;
}


int ya_check_us_license_exit(void)
{
	return ya_aws_para_init();
}

int ya_check_cn_license_exit(void)
{
	return ya_aliyun_para_init();
}


