/*******************************************************************************
 * Copyright by Transa Semi.
 *
 * File Name:    drv_timer.h
 * File Mark:    
 * Description:  timer
 * Others:        
 * Version:       V1.0
 * Author:        wangc
 * Date:          2018-12-21
 * History 1:      
 *     Date: 
 *     Version:
 *     Author: 
 *     Modification:  
 * History 2: 
  ********************************************************************************/

/****************************************************************************
* 	                                           Include files
****************************************************************************/

/****************************************************************************
* 	                                           Local Macros
****************************************************************************/

/****************************************************************************
* 	                                           Local Types
****************************************************************************/

/****************************************************************************
* 	                                           Local Constants
****************************************************************************/

/****************************************************************************
* 	                                           Local Function Prototypes
****************************************************************************/

/****************************************************************************
* 	                                          Global Constants
****************************************************************************/

/****************************************************************************
* 	                                          Global Variables
****************************************************************************/

/****************************************************************************
* 	                                          Global Function Prototypes
****************************************************************************/

/****************************************************************************
* 	                                          Function Definitions
****************************************************************************/
int hal_timer_init(void);
int hal_timer_config(unsigned int us, unsigned int reload);
int hal_timer_start(void);
int hal_timer_stop(void);
 void hal_timer_callback_register(void (* user_timer_cb_fun)(void *), void *data);
 void hal_timer_callback_unregister(void);

