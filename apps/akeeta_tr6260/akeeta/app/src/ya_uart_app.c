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
#include "ya_config.h"
#include "ya_hal_uart.h"
#include "ya_uart_app.h"
#include "ya_flash.h"

#define MAX_CERT_BUF_LEN	   				(3*1024)


#define UART_RETRY_NUM						3
#define UART_RETRY_INTERVAL					1000
#define UART_MAX_FRAME_LEN					540

#define CMD_GET_PRODUCT_ID 					0x01
#define CMD_REPORT_NET_STATUS 				0X02
#define CMD_RESET_WIFI 						0x03
#define CMD_RESET_WIFE_CONFIG				0x04
#define CMD_GET_LOCAL_T 					0x06
#define CMD_WIFE_FUNC_TEST 					0x07
#define CMD_MCU_REPORT_STATUS 				0x08
#define CMD_SET_COMMAND 					0x09
#define CMD_GET_CURRENT_WIFI_INTENTITY 		0x0b

//new define cmd
#define CMD_GET_MAC							0xF0
#define CMD_WRITE_CERT 						0XF1
#define CMD_READBACK_CERT 					0xF2
#define CMD_SET_MAC							0xF3

#define SUB_CMD_WRITE_CERT 					0x01
#define SUB_CMD_WRITE_KEY 					0x02
#define SUB_CMD_WRITE_DEV_ID 				0x03
#define SUB_CMD_WRITE_THING_NAME			0x04

#define SUB_CMD_READBACK_CERT 				0x01
#define SUB_CMD_READBACK_KEY 				0x02
#define SUB_CMD_READBACK_DEV_ID 			0x03
#define SUB_CMD_READBACK_THING_NAME			0x04


#define CMD_HEADER_HIGH  					0X55
#define CMD_HEADER_LOW						0xAA

#define UART_HEADER_LEN						4	
#define UART_HEADER_WITH_LEN  				6

typedef enum{

	UART_HEADER_HIGH_POS = 0,
	UART_HEADER_LOW_POS,
	UART_VERSION_POS,
	UART_CMD_POS,
	UART_LEN_HIGH_POS,
	UART_LEN_LOW_POS,
	UART_DATA_POS,
}uart_pos_t;


typedef enum
{
	UART_DATA_CERT_TYPE_POS = 6,
	UART_DATA_SUB_CMD_POS,
	UART_DATA_TOTAL_SUB_FRAME_NUM_CMD_POS,
	UART_DATA_TOTAL_SUB_FRAME_INDEX_POS,
	UART_DATA_TOTAL_SUB_FRAME_DATA_POS,
}uart_cert_pos_t;

ya_hal_os_thread_t ya_uart = NULL;
ya_hal_os_queue_t ya_uart_queue = NULL;


uint8_t *cert_buffer = NULL;
uint8_t *rx_buf = NULL;
uint8_t *tx_buf = NULL;

uint8_t get_checksum(uint8_t *buf, int len)
{
	int i;
	uint32_t sum=0;

	for (i=0; i<len; i++)
	{
		sum +=buf[i];
	}
	
	return (sum%256);
}

uint16_t crc_check(uint8_t *puchMsg, uint16_t usDataLen)
{
	uint16_t wCRCin = 0xFFFF;
	uint16_t wCPoly = 0x1021;
	uint8_t wChar = 0;
	int i = 0;

	while(usDataLen--) 
	{
		wChar = *(puchMsg++);
		wCRCin ^= (wChar << 8);
		for(i = 0;i < 8;i++)
		{
			if(wCRCin & 0x8000)
				wCRCin = (wCRCin << 1) ^ wCPoly;
		else
			wCRCin = wCRCin << 1;
		}
	}
	return (wCRCin) ;
}

uint8_t write_certificate(uint8_t *buf, int dataLen)
{
	int32_t ret = -1;
	int efficient_data_len;

	uint8_t sub_cmd = buf[UART_DATA_SUB_CMD_POS];
	uint8_t totalFrame = buf[UART_DATA_TOTAL_SUB_FRAME_NUM_CMD_POS];
	uint8_t current_frame_index = buf[UART_DATA_TOTAL_SUB_FRAME_INDEX_POS];

	uint32_t cert_len=0;
	uint32_t read_cert_len = 0;
	uint32_t flash_addr = 0;
	uint16_t crc_get = 0;
	
	switch(sub_cmd)
	{
		case SUB_CMD_WRITE_CERT:
		case SUB_CMD_WRITE_KEY:
			
			efficient_data_len = dataLen - 4;
		
			if(current_frame_index == 0)
			{
				if(cert_buffer == NULL)
					cert_buffer = (uint8_t *)ya_hal_os_memory_alloc(MAX_CERT_BUF_LEN);
				
				memset(cert_buffer, 0, MAX_CERT_BUF_LEN);	
			}

			if(cert_buffer)
			{
				memcpy(cert_buffer+current_frame_index*512, buf + UART_DATA_TOTAL_SUB_FRAME_DATA_POS, efficient_data_len);
				
				if((current_frame_index+1) == totalFrame) 
				{	
					ya_printf(C_LOG_INFO,"cert_buffer: %s\n", cert_buffer);
					cert_len = efficient_data_len + current_frame_index*512;
					crc_get =((uint16_t)cert_buffer[cert_len-2] <<8) + cert_buffer[cert_len-1]; 
					
					ret = -1;
					if (sub_cmd == SUB_CMD_WRITE_CERT)
					{
						flash_addr = YA_LICENSE_DEVICE_CERT_ADDR;
						ret = ya_write_check_aws_para_device_cert(cert_buffer, (cert_len-2), YA_LICENSE_DEVICE_CERT_ADDR);
					}else if (sub_cmd == SUB_CMD_WRITE_KEY)
					{
						flash_addr = YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR;
						ret = ya_write_check_aws_para_device_rsa_private_key(cert_buffer, (cert_len-2), YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR);
					}else
					{	
						goto err;
					}
					
					if(ret == 0)
					{
						read_cert_len = MAX_CERT_BUF_LEN;
						
						if(ya_read_flash_with_var_len(flash_addr, cert_buffer, &read_cert_len, 1) != 0)
							goto err;

						if(read_cert_len != (cert_len-2))
							goto err;
						
						if(crc_check(cert_buffer, read_cert_len) != crc_get)
							goto err;

						ya_printf(C_LOG_INFO,"write cert ok\n");
						
					}else{
						goto err;
					}

					if(cert_buffer) 
					{
						ya_hal_os_memory_free(cert_buffer);
						cert_buffer = NULL;
					}
				}else if((current_frame_index+1) > totalFrame)
					goto err;
			}else
			{
				goto err;
			}
			break;
			
		case SUB_CMD_WRITE_DEV_ID:
			printf("write SUB_CMD_WRITE_DEV_ID\n ");
			efficient_data_len = dataLen - 2;		
			ya_write_check_aws_para_device_id(buf + 8, efficient_data_len, YA_LICENSE_DEVICE_ID_ADDR);
			break;

		case SUB_CMD_WRITE_THING_NAME:
			printf("write SUB_CMD_WRITE_THING_NAME\n ");
			efficient_data_len = dataLen - 2;		
			ya_write_check_aws_para_thing_type(buf+8, efficient_data_len, YA_LICENSE_THING_TYPE_ADDR);
			break;
				
		default:
			goto err;
	}

	return 1;

	err:
	if(cert_buffer) 
	{
		ya_hal_os_memory_free(cert_buffer);
		cert_buffer = NULL;
	}
	return 0;	
}

uint16_t read_certificate(uint8_t *rx_buf, uint8_t *tx_buf, uint8_t sub_cmd)
{
	uint16_t frame_len = 0, tmp_len = 0;

	uint8_t total_frame = 0, sub_frame_index = 0;
	uint32_t flash_addr = 0;
	uint32_t data_len=0;
	
	static uint32_t read_data_len = 0;

	switch(sub_cmd)
	{
		case SUB_CMD_READBACK_CERT:
		case SUB_CMD_READBACK_KEY:

			if(sub_cmd == SUB_CMD_READBACK_CERT)
				flash_addr = YA_LICENSE_DEVICE_CERT_ADDR;
			else
				flash_addr = YA_LICENSE_DEVICE_RSA_PRIVATE_KEY_ADDR;

			sub_frame_index = rx_buf[8];
			if(sub_frame_index == 0)
			{
				if(cert_buffer == NULL)
					cert_buffer = (uint8_t *)ya_hal_os_memory_alloc(MAX_RSA_LEN);

				if(cert_buffer)
				{
					memset(cert_buffer, 0, MAX_RSA_LEN);

					read_data_len = MAX_RSA_LEN;
					if(ya_read_flash_with_var_len(flash_addr, cert_buffer, &read_data_len, 1) != 0)
					{
						read_data_len = 0;
						goto err;
					}
					printf("read license: %s\n", cert_buffer);
				}else
					goto err;
			}
			else
			{
				if(cert_buffer == NULL)
					goto err;
			}

			//get total frame
			if(read_data_len%512 == 0)
				total_frame = read_data_len/512;
			else
				total_frame = read_data_len/512 + 1;

			//get the sub frame
			if((sub_frame_index+1) == total_frame)
				data_len = read_data_len%512;
			else if((sub_frame_index+1) < total_frame)
				data_len = 512;
			else
				goto err;
		
			tmp_len = data_len+4;
			tmp_len = (tmp_len>>8)|(tmp_len<<8);
			memcpy(tx_buf+4, &tmp_len, 2);
			tx_buf[8] = total_frame;
			tx_buf[9] = sub_frame_index;
			memcpy(tx_buf+10, cert_buffer+sub_frame_index*512, data_len);

			frame_len = data_len + 10 + 1;
			tx_buf[frame_len-1] = get_checksum(tx_buf, frame_len-1);

			if((sub_frame_index+1) == total_frame)
			{
				if(cert_buffer) 
				{
					ya_hal_os_memory_free(cert_buffer);
					cert_buffer = NULL;
				}
				
				read_data_len = 0;
			}
			break;
			
		case SUB_CMD_READBACK_DEV_ID:
		case SUB_CMD_READBACK_THING_NAME:
			data_len = 128;
		
			if(sub_cmd == SUB_CMD_READBACK_DEV_ID)
				flash_addr = YA_LICENSE_DEVICE_ID_ADDR;
			else
				flash_addr = YA_LICENSE_THING_TYPE_ADDR;
	
			if(ya_read_flash_with_var_len(flash_addr, tx_buf+8, &data_len, 1) != 0)
				goto err;

			tmp_len = data_len+2;
			tmp_len = (tmp_len>>8)|(tmp_len<<8);	
			memcpy(tx_buf+4, &tmp_len, 2);
			frame_len = data_len+8+1;
			
			tx_buf[frame_len-1] = get_checksum(tx_buf, frame_len-1);
			break;
	}

	return frame_len;

	err:
	read_data_len = 0;
	
	if(cert_buffer) 
	{
		ya_hal_os_memory_free(cert_buffer);
		cert_buffer = NULL;
	}
	return 0;
}

void ya_uart_handle_cloud_onff_event(uint8_t *data_buf, uint16_t data_len)
{

}



int32_t ya_handle_uart_msg_send_to_queue(uint8_t *data, uint16_t data_len, uint8_t reply_flag)
{
	int32_t ret = -1;
	uint8_t *buf = NULL;
	msg_t ms_msg;
	
	buf = (uint8_t *)ya_hal_os_memory_alloc(data_len);
	memset( buf, 0, data_len);
	memcpy( buf, data, data_len);
	
	memset(&ms_msg, 0, sizeof(msg_t));
	ms_msg.addr = buf;
	ms_msg.len = data_len;

	if(reply_flag == 0)
		ms_msg.type = UART_WITHOUT_REPLY;
	else
		ms_msg.type = UART_WITH_REPLY;
	
	ret = ya_hal_os_queue_send(&ya_uart_queue, &ms_msg, 10);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR,"cloud queue error\n");
	
		if(ms_msg.addr)
			ya_hal_os_memory_free(ms_msg.addr);
	
		return -1;
	}
	
	return 0;
}

int32_t ya_handle_uart_body_send_to_queue(uint8_t cmd, uint8_t *data, uint16_t data_len)
{
	int32_t ret = -1;
	uint8_t *buf = NULL;
	uint16_t pos  = 0;
	
	buf = (uint8_t *)ya_hal_os_memory_alloc(data_len + UART_HEADER_WITH_LEN + 1);

	if(!buf)
		return -1;

	buf[pos++] = CMD_HEADER_HIGH;
	buf[pos++] = CMD_HEADER_LOW;
	buf[pos++] = 0x00;
	buf[pos++] = cmd;

	buf[pos++] = (uint8_t)((data_len & 0xFF00) >> 8);
	buf[pos++] = (uint8_t)(data_len & 0x00FF);

	memcpy(buf + pos, data, data_len);
	pos = pos + data_len;

	buf[pos] = get_checksum(buf, pos - 1);

	ret = ya_handle_uart_msg_send_to_queue(buf, pos++, UART_WITHOUT_REPLY);

	return ret;
}


void ya_handle_uart_data(uint8_t *buf, uint16_t buf_len)
{
	uint8_t crc = 0;
	uint8_t sub_cmd = 0;
	uint16_t pos = 0, reply_buf_len = 0;
	uint8_t ret;
	uint8_t *buf_cert = NULL;

	uint32_t cert_len = 0;

	crc = get_checksum(buf, buf_len - 1);

	if(crc != buf[buf_len - 1])
	{
		ya_printf(C_LOG_ERROR, "crc error, rec: %02x, cac: %02x\n", buf[buf_len - 1], crc);
		return;
	}
	
	switch(buf[UART_CMD_POS])
	{
		case CMD_RESET_WIFI:
			memcpy(tx_buf, buf, UART_HEADER_LEN);
			tx_buf[UART_LEN_HIGH_POS] = 0x00;
			tx_buf[UART_LEN_LOW_POS] = 0x00;

			pos = pos + UART_HEADER_WITH_LEN;
			tx_buf[pos++] = get_checksum(tx_buf, UART_HEADER_WITH_LEN);
			reply_buf_len = pos;

			ya_reset_module_to_factory();
			break;
				
		case CMD_RESET_WIFE_CONFIG:
			memcpy(tx_buf, buf, UART_HEADER_LEN);
			tx_buf[UART_LEN_HIGH_POS] = 0x00;
			tx_buf[UART_LEN_LOW_POS] = 0x00;
			
			pos = pos + UART_HEADER_WITH_LEN;
			tx_buf[pos++] = get_checksum(tx_buf, UART_HEADER_WITH_LEN);
			reply_buf_len = pos;

			if(buf[UART_DATA_POS] == 0)
				ya_set_sniffer_mode();
			else
				ya_set_ap_mode();

			break;

		case CMD_GET_MAC:
			memcpy(tx_buf, buf, UART_HEADER_LEN);
			
			tx_buf[UART_LEN_HIGH_POS] = 0x00;
			tx_buf[UART_LEN_LOW_POS] = 0x06;
			
			ya_hal_wlan_get_mac_address(tx_buf + UART_DATA_POS);
			pos = pos + UART_HEADER_WITH_LEN + 6;
			
			tx_buf[pos++] = get_checksum(tx_buf, UART_HEADER_WITH_LEN + 6);
			reply_buf_len = pos;
		break;

		case CMD_SET_MAC:		
			memcpy(tx_buf, buf, UART_HEADER_LEN);
			tx_buf[UART_LEN_HIGH_POS] = 0x00;
			tx_buf[UART_LEN_LOW_POS] = 0x01;

			if(buf_len >= (UART_DATA_POS + 1 + 6))
			{
				if(ya_hal_wlan_set_mac_address(buf + UART_DATA_POS) != 0)
					tx_buf[UART_DATA_POS] = 0;
				else
					tx_buf[UART_DATA_POS] = 1;
			}else
			{
				tx_buf[UART_DATA_POS] = 0;
			}
			
			pos = pos + UART_HEADER_WITH_LEN + 1;
			tx_buf[pos++] = get_checksum(tx_buf, UART_HEADER_WITH_LEN + 1);
			reply_buf_len = pos;
			break;			
				
		case CMD_WRITE_CERT:
			if (buf[UART_DATA_CERT_TYPE_POS] == 0x01)
			{
				sub_cmd = buf[UART_DATA_SUB_CMD_POS];
				ret = write_certificate(buf, buf_len - UART_HEADER_WITH_LEN - 1);

				memcpy(tx_buf, buf, 10);
				if (sub_cmd == SUB_CMD_WRITE_DEV_ID || sub_cmd == SUB_CMD_WRITE_THING_NAME)
				{
					tx_buf[UART_LEN_HIGH_POS] = 0x00;
					tx_buf[UART_LEN_LOW_POS] = 0x03;
					
					tx_buf[8] = ret;
					tx_buf[9] = get_checksum(tx_buf, 9);
					reply_buf_len = 10;
				}
				else
				{
					tx_buf[UART_LEN_HIGH_POS] = 0x00;
					tx_buf[UART_LEN_LOW_POS] = 0x05;

					tx_buf[10] = ret;
					tx_buf[11] = get_checksum(tx_buf, 11);
					reply_buf_len = 12;
				}
			}else if (buf[UART_DATA_CERT_TYPE_POS] == 0x02)
			{
				if(ya_write_aliyun_cert(&(buf[UART_DATA_CERT_TYPE_POS + 1]), buf_len - UART_HEADER_WITH_LEN - 1 - 1, YA_LICENSE_ALIYUUN) != 0)
					ret = 0;
				else
					ret = 1;

				memcpy(tx_buf, buf, UART_HEADER_LEN);
				tx_buf[UART_LEN_HIGH_POS] = 0x00;
				tx_buf[UART_LEN_LOW_POS] = 0x02;
				tx_buf[UART_DATA_CERT_TYPE_POS] = 0x02;
				tx_buf[UART_DATA_CERT_TYPE_POS + 1] = ret;				
				
				pos = pos + UART_DATA_CERT_TYPE_POS + 1 + 1;
				tx_buf[pos] = get_checksum(tx_buf, pos);
				reply_buf_len = pos + 1;
			}
		break;
			
		case CMD_READBACK_CERT:
			sub_cmd = buf[UART_DATA_SUB_CMD_POS];
			if (buf[UART_DATA_CERT_TYPE_POS] == 0x01)
			{
				memcpy(tx_buf, buf, 8);
				reply_buf_len = read_certificate(buf, tx_buf, sub_cmd);
			}else if (buf[UART_DATA_CERT_TYPE_POS] == 0x02)
			{
				buf_cert = (uint8_t *)ya_hal_os_memory_alloc(128);
				if(!buf_cert) return;

				cert_len = 128;
				if(ya_read_aliyun_cert(buf_cert, &cert_len, YA_LICENSE_ALIYUUN) == 0)
				{
					memcpy(tx_buf, buf, UART_HEADER_LEN);
					tx_buf[UART_LEN_HIGH_POS] = 0x00;
					tx_buf[UART_LEN_LOW_POS] = cert_len + 1;
					tx_buf[UART_DATA_CERT_TYPE_POS] = 0x02;

					memcpy(tx_buf + UART_DATA_CERT_TYPE_POS + 1, buf_cert, cert_len);
					
					pos = pos + UART_DATA_CERT_TYPE_POS + 1 + cert_len;
					tx_buf[pos] = get_checksum(tx_buf, pos);
					reply_buf_len = pos + 1;
				}else
				{
					reply_buf_len = 0;
				}

				ya_hal_os_memory_free(buf_cert);
			}
		break;

		default:
			break;
	}

	if(reply_buf_len)
		ya_hal_uart_write(tx_buf, reply_buf_len);
}


void ya_uart_app(void *param)
{
	int32_t ret = -1;	
	uint16_t msg_len = 0;
	uint8_t queue_rece_enable = 0;
	uint8_t cur_requeire_cmd  = 0;
	uint8_t count = 0;

	Ya_Timer ya_uart_resend_timer;
	msg_t msg;

	rx_buf = (uint8_t *)ya_hal_os_memory_alloc(UART_MAX_FRAME_LEN);
	if(!rx_buf)
		return;

	tx_buf = (uint8_t *)ya_hal_os_memory_alloc(UART_MAX_FRAME_LEN);
	if(!tx_buf)
		return;
	
	ret = ya_hal_uart_open();
	if(ret != 0)
		return;

	queue_rece_enable = 1;
	ya_init_timer(&ya_uart_resend_timer);

	ya_printf(C_LOG_INFO, "start ya_uart\n");
	
	while (1) 
	{
		ret = ya_hal_uart_recv(rx_buf, UART_HEADER_WITH_LEN, 20);
		
		if(ret == UART_HEADER_WITH_LEN)
		{
			ya_printf(C_LOG_INFO, "header: %02x, %02x, %02x, %02x, %02x, %02x\n", rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4], rx_buf[5]);
			if(rx_buf[UART_HEADER_HIGH_POS] == CMD_HEADER_HIGH && rx_buf[UART_HEADER_LOW_POS] == CMD_HEADER_LOW)
			{
				msg_len = (uint16_t)(rx_buf[UART_LEN_HIGH_POS] << 8) + rx_buf[UART_LEN_LOW_POS];
				if(msg_len < UART_MAX_FRAME_LEN)
				{
					ret = ya_hal_uart_recv(rx_buf+UART_HEADER_WITH_LEN, (msg_len+1), 200);					
					if(ret == (msg_len+1))
					{
						ya_handle_uart_data(rx_buf, (UART_HEADER_WITH_LEN + msg_len + 1));
						//cmd compare
						if(queue_rece_enable)
						{
							if(rx_buf[UART_CMD_POS] == cur_requeire_cmd)
							{
								count = 0;
								queue_rece_enable = 1;
								if(msg.addr) ya_hal_os_memory_free(msg.addr);

							}
						}
					}
				}
			}			
		}

		if(queue_rece_enable)
		{
			memset(&msg, 0, sizeof(msg_t));
			ret = ya_hal_os_queue_recv(&ya_uart_queue, &msg, 0);
			if(ret == C_OK)
			{
				ya_printf(C_LOG_INFO, "[U-T]: %02x\n",msg.type);

				switch(msg.type)
				{
					case UART_WITH_REPLY:
						count = 0;
						queue_rece_enable = 0;
						ret = ya_hal_uart_write(msg.addr, msg.len);

						if(ret != msg.len)
							ya_printf(C_LOG_ERROR,"uart send error\n");
						else
							ya_countdown_ms(&ya_uart_resend_timer, UART_RETRY_INTERVAL);

						break;

					case UART_WITHOUT_REPLY:
						queue_rece_enable = 1;
						ret = ya_hal_uart_write(msg.addr, msg.len);

						if(ret != msg.len)
							ya_printf(C_LOG_ERROR,"uart send error\n");	

						if(msg.addr)
							ya_hal_os_memory_free(msg.addr);
						break;

					default:
						if(msg.addr)
							ya_hal_os_memory_free(msg.addr);
					break;
				}
			}
		}

		if(queue_rece_enable == 0)
		{
			if(ya_has_timer_expired(&ya_uart_resend_timer) == 0)
			{
				if(count < UART_RETRY_NUM)
				{
					ret = ya_hal_uart_write(msg.addr, msg.len);
					if(ret != C_OK)
						ya_printf(C_LOG_ERROR,"uart re-send error\n"); 

					count++;
				}else
				{
					count = 0;
					queue_rece_enable = 1;
				}
			}
		}
    }

	if(rx_buf)
		ya_hal_os_memory_free(rx_buf);

	rx_buf = NULL;

	if(tx_buf)
		ya_hal_os_memory_free(tx_buf);

	tx_buf = NULL;
    vTaskDelete(NULL);
}


void ya_add_uart_listen_event_from_cloud(void)
{
	cloud_add_event_listener("ya_uart_thread", CLOUD_ONOFF_EVENT, ya_uart_handle_cloud_onff_event);	
}

int32_t ya_start_uart_app(void)
{
	int32_t ret = 0;

	if(ya_uart)
		return 0;

	ret = ya_hal_os_queue_create(&ya_uart_queue, "ya uart queue", sizeof(msg_t), MSG_QUEUE_LENGTH);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create os queue error\n");
		return -1;
	}

	ret = ya_hal_os_thread_create(&ya_uart, "ya_uart thread", ya_uart_app, 0, (2*1024), 5);
	if(ret != C_OK)
	{
		ya_printf(C_LOG_ERROR, "create ya_uart error\n");
		return -1;
	}

	return 0;
}

