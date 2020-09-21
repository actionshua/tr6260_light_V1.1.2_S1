/*
reatlek 8710cx
Note
    1, Supported GPIO pins, PA_0, PA_1, PA_2, PA_3, PA_4, PA_7, PA_8, PA_9, PA_10, PA_11, PA_12, PA_13, PA_14, PA_15, PA_16, PA_17, PA_18, PA_19, PA_20, and PA_23.
    2, Please off JTAG/SWD, when using PA_0, PA_1, PA_2, PA_3, and PA_4.
    3, Please off log UART, when using PA_15, and PA_16.
    4, PA_7, PA_8, PA_9, PA_10, PA_11 and PA_12 only available on RTL8720CF.

*/


#ifndef _YA_HAL_BUTTON_H_
#define _YA_HAL_BUTTON_H_

#define BUTTON1_PUSH_VALUE			0
#define BUTTON1_RELEASE_VALUE		1


#define BUTTON_NUM				1		

#define YA_BUTTON_PARA_INIT	 {{IO_MUX0_GPIO0_REG, IO_MUX0_GPIO0_BITS, IO_MUX0_GPIO0_OFFSET, FUNC_GPIO0_GPIO0,DRV_GPIO_0, DRV_GPIO_DIR_INPUT,BUTTON1_PUSH_VALUE, NULL}}

typedef enum
{
	BUTTON1 = 0,
}YA_BUTTON_NAME;

typedef void (*ya_button_push_time_func)(uint32_t button_push_ms);

#if (BUTTON_NUM > 0)

extern int ya_button_init(void);

extern void ya_set_button_callback(uint8_t index, ya_button_push_time_func ya_button_push_time_callback, uint32_t overtime);

#endif

#endif




