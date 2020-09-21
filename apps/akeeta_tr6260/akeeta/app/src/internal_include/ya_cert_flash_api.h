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

#ifndef _YA_CERT_FLASH_API_H_
#define _YA_CERT_FLASH_API_H_


#define MAX_CERT_LEN 			(1024+512)
#define MAX_RSA_LEN  			(2048+512)
#define MAX_CLIENT_ID_LEN		 (128)
#define MAX_THING_TYPE_LEN		 (128)

/**
 * @brief This function is the one used to read and check the device cert from flash.
 *
 * @param device_cert: device cert buf
 * @param cert_addr: devvice cert read flash addr
 * @return 0: sucess, -1: failed
 */
extern int32_t ya_aws_read_check_device_cert(char *device_cert, uint32_t cert_addr);

/**
 * @brief This function is the one used to read and check rsa private key from flash.
 *
 * @param private_key: private key buf
 * @param private_key_addr: private key read addr
 * @return 0: sucess, -1: failed
 */

extern int32_t ya_aws_read_check_device_rsa_private_key(char *private_key, uint32_t private_key_addr);

/**
 * @brief This function is the one used to read and check device id from flash.
 *
 * @param device_cert: device id buf
 * @param cert_addr: devvice cert read addr
 * @return 0: sucess, -1: failed
 */

extern int32_t ya_aws_read_check_device_id(char *device_id, uint32_t device_id_addr);

/**
 * @brief This function is the one used to read and check thing type from flash.
 *
 * @param thing_type: thing_type buf
 * @param thing_type_addr: thing type read addr
 * @return 0: sucess, -1: failed
 */
extern int32_t ya_aws_read_check_thing_type(char *thing_type, uint32_t thing_type_addr);

/**
 * @brief This function is the one used to write the device cert into flash.
 *
 * @param buf: cert buf
 * @param data_len: buf len
 * @param cert_addr: cert addr
 * @return 0: sucess, -1: failed
 */
extern int32_t ya_write_check_aws_para_device_cert(char *buf, uint16_t data_len, uint32_t cert_addr);

/**
 * @brief This function is the one used to write the device rsa private key into flash.
 *
 * @param buf: key buf
 * @param data_len: buf len
 * @param cert_addr: private key addr
 * @return 0: sucess, -1: failed
 */
extern int32_t ya_write_check_aws_para_device_rsa_private_key(char *buf, uint16_t data_len, uint32_t private_key_addr);


/**
 * @brief This function is the one used to write the device client id into flash.
 *
 * @param buf: client id buf
 * @param data_len: buf len
 * @param device_id_addr: device id addr
 * @return 0: sucess, -1: failed
 */
extern int32_t ya_write_check_aws_para_device_id(char *buf, uint16_t data_len, uint32_t device_id_addr);


/**
 * @brief This function is the one used to write the thing type para into flash.
 *
 * @param buf: thing name buf
 * @param data_len: buf len
 * @param thing_type_addr: thing type addr
 * @return 0: sucess, -1: failed
 */
extern int32_t ya_write_check_aws_para_thing_type(char *buf, uint16_t data_len, uint32_t thing_type_addr);


/**
 * @brief This function is the one used to get the private key pointer
 *
 * @return private key pointer
 */
extern char *ya_get_private_key(void);

/**
 * @brief This function is the one used to get the client id pointer
 *
 * @return client id pointer
 */
extern char *ya_get_client_cert(void);

/**
 * @brief This function is the one used to get the client id pointer
 *
 * @return client id pointer
 */
extern char *ya_aws_get_client_id(void);

/**
 * @brief This function is the one used to get the thingname pointer
 *
 * @return client id pointer
 */
extern char *ya_aws_get_thing_type(void);

/**
 * @brief: aws cert para init
 *
 * @return 0: sucess, -1: failed
 */
 extern int ya_aws_para_init(void);

 
/**
* @brief: free aws para
*
* @return 0: sucess, -1: failed
*/
extern int ya_aws_para_free(void);

/**
 * @brief Get product key from user's system persistent storage
 *
 * @param [ou] product_key: array to store product key, max length is YA_PRODUCT_KEY_LEN
 * @return  the actual length of product key
 */
#define YA_PRODUCT_KEY_LEN (20)
extern int ya_get_productKey(char product_key[YA_PRODUCT_KEY_LEN + 1]);
/**
 * @brief Get device name from user's system persistent storage
 *
 * @param [ou] device_name: array to store device name, max length is YA_DEVICE_NAME_LEN
 * @return the actual length of device name
 */
 #define YA_DEVICE_ID_LEN (32)
extern int ya_get_deviceID(char device_id[YA_DEVICE_ID_LEN + 1]);

/**
 * @brief Get device secret from user's system persistent storage
 *
 * @param [ou] device_secret: array to store device secret, max length is YA_DEVICE_SECRET_LEN
 * @return the actual length of device secret
 */
#define YA_DEVICE_SECRET_LEN (64)
int ya_get_deviceSecret(char device_secret[YA_DEVICE_SECRET_LEN + 1]);

/**
 * @brief Get device aliyun cert from flash
 *
 * @param buf: cert buf
 * @param data_len: read buf len
 * @param aliyun_addr: aliyun cert addr
 * @return 0: sucess, -1: failed

 * @return 0: sucess, -1: failed
 */
extern int32_t ya_read_aliyun_cert(uint8_t *buf, uint32_t *data_len, uint32_t aliyun_addr);

/**
 * @brief write device aliyun cert into flash
 *
 * @param buf: cert buf
 * @param data_len: buf len
 * @param aliyun_addr: aliyun cert addr
 * @return 0: sucess, -1: failed
 */
extern int32_t ya_write_aliyun_cert(uint8_t *buf, uint16_t data_len, uint32_t aliyun_addr);

/**
 * @brief: aliyun cert para init
 *
 * @return 0: sucess, -1: failed
 */
extern int ya_aliyun_para_init(void);


/**
 * @brief: aliyun cert para free
 *
 * @return 0: sucess, -1: failed
 */
extern int ya_aliyun_para_free(void);


#endif

