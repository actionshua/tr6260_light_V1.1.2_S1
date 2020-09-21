#ifndef _YA_HAL_GPIO_H_
#define _YA_HAL_GPIO_H_



#define GPIO_NUM			6

#define YA_GPIO_PARA_INIT	 {{IO_MUX0_GPIO4_REG, IO_MUX0_GPIO4_BITS, IO_MUX0_GPIO4_OFFSET, FUNC_GPIO4_GPIO4,DRV_GPIO_4,DRV_GPIO_DIR_OUTPUT},\
							  {IO_MUX0_GPIO2_REG, IO_MUX0_GPIO2_BITS, IO_MUX0_GPIO2_OFFSET, FUNC_GPIO2_GPIO2,DRV_GPIO_2,DRV_GPIO_DIR_OUTPUT},\
							  {IO_MUX0_GPIO1_REG, IO_MUX0_GPIO1_BITS, IO_MUX0_GPIO1_OFFSET, FUNC_GPIO1_GPIO1,DRV_GPIO_1,DRV_GPIO_DIR_OUTPUT},\
							  {IO_MUX0_GPIO13_REG, IO_MUX0_GPIO13_BITS, IO_MUX0_GPIO13_OFFSET, FUNC_GPIO13_GPIO13,DRV_GPIO_13,DRV_GPIO_DIR_OUTPUT},\
							  {IO_MUX0_GPIO5_REG, IO_MUX0_GPIO5_BITS, IO_MUX0_GPIO5_OFFSET, FUNC_GPIO5_GPIO5,DRV_GPIO_5,DRV_GPIO_DIR_OUTPUT}}
 

typedef enum
{
	WIFI_LIGHT = 0,
	INDICATOR_LIGHT_2,
	INDICATOR_LIGHT_3,
	INDICATOR_LIGHT_4,
	INDICATOR_LIGHT_5,
	RELAY
}YA_GPIO_NAME;

#define WIFI_LIGHT_ON		0
#define WIFI_LIGHT_OFF		1

#define RELAY_ON			0
#define RELAY_OFF 			1

#define INDICATOR_LIGHT_ON			0
#define INDICATOR_LIGHT_OFF 		1

#if (GPIO_NUM > 0)

extern void ya_gpio_write(uint8_t index, int value);

extern int ya_gpio_read(uint8_t index);

extern void ya_gpio_init(void);

#endif

#endif
