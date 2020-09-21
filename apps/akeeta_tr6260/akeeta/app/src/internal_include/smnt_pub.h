#ifndef _SMNT_PUB_H_
#define _SMNT_PUB_H_
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


typedef enum{
	smnt_result_ok,			/*解析出正确的结果*/
	smnt_result_decrypt_error,	/*解密错误*/
	smnt_result_ssidlen_error,	/*ssid 长度错误*/
	smnt_result_pwdlen_error,	/*password 长度错误*/
	smnt_result_overtime,
}smnt_result_flag_t;

typedef struct{
	smnt_result_flag_t smnt_result_status;
	uint8_t ssid_len;
	uint8_t password_len;
	char ssid[33];
	char password[65];
	uint8_t bssid[6];
	uint8_t cloud_select;
	char rand[33];
}smnt_result_t;



typedef struct{

	uint32_t timeout;
	void (*get_result_callback)(smnt_result_t * );
} smnt_param_t;



/**
 * @brief  启动smartconfig配网 内部函数，用户调用
 * 
 * @param  param 结构体，结果输出的回调函数
 * 
 * @author songcf (2020/04/01)
 *
 * @return 0 成功 -1失败
 */
extern int start_smnt(smnt_param_t param);


/**
 * @brief  停止smartconfig配网 内部函数，用户调用
 * 
 * @param  None
 * 
 * @author songcf (2020/04/01)
 *
 * @return 0 成功 -1失败
 */
extern int stop_smnt(void);



#endif


