
#ifndef _YA_MCU_OTA_USER_DEFINE_H_
#define _YA_MCU_OTA_USER_DEFINE_H_

/**
The following funciton is realized by the user according to mcu ota.
**/

/**
 * @brief This function is used to get the mcu ota download address.
 * @return: ota download address
**/

extern uint32_t ya_mcu_ota_download_addr(void);


/**
 * When the wifi module download the mcu ota file finish, it will use this function to notify the user. 
 * If the products do not need ota, just set the function to idle function.
 *
 * @param ota_size: the ota mcu file size
 * @param new_version: the ota mcu version
 *
**/	
extern void ya_mcu_download_finish(uint32_t ota_size, char *new_version);


#endif


