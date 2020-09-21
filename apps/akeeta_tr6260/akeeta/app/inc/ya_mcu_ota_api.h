
#ifndef _YA_MCU_OTA_API_H_
#define _YA_MCU_OTA_API_H_

/**
 * the user can use this api to notify the ota process and result
 *
 * @param code:

		100: ota success
		0~99: ota process (consider the file has been download, the process should begin from 10%)
		-1: ota fail
  @param new_version: the obj new version
 *
**/	

int32_t ya_report_mcu_ota_result(int32_t code);

#endif


