#include "FreeRTOS.h"
#include "drv_gpio.h"
#include "soc_pin_mux.h"
#include "ya_hal_gpio.h"
#include "ya_common.h"

#if (GPIO_NUM > 0)
typedef struct
{
	uint32_t io_mux_reg;
	uint32_t io_mux_bits;
	uint32_t io_mux_offset;
	uint32_t io_mux_func;
	
	uint32_t pin_num;
	uint32_t pin_dir;
}ya_gpio_para_t;


ya_gpio_para_t ya_gpio_para[GPIO_NUM] = YA_GPIO_PARA_INIT;

void ya_gpio_write(uint8_t index, int value)
{
	if(GPIO_NUM == 0 || index > GPIO_NUM)
	{
		return;
	}
	if(index == RELAY)
		gpio17_write(value);
	else
		gpio_write(ya_gpio_para[index].pin_num, value);
}

int ya_gpio_read(uint8_t index)
{
	uint32_t value=-1;
	if(GPIO_NUM == 0 || index > GPIO_NUM)
	{
		return -1;
	}
	if(index == RELAY)
		gpio17_read(&value);
	else
		gpio_read(ya_gpio_para[index].pin_num,&value);
	return value;
}


void ya_gpio_init(void)
{
	uint8_t index = 0;
	gpio_wakeup_config(DRV_GPIO_17, DRV_GPIO_WAKEUP_DIS);
	for(index = 0; index < GPIO_NUM - 1; index++)
	{
		PIN_MUX_SET(ya_gpio_para[index].io_mux_reg, ya_gpio_para[index].io_mux_bits,
					ya_gpio_para[index].io_mux_offset, ya_gpio_para[index].io_mux_func);
		gpio_set_dir(ya_gpio_para[index].pin_num,ya_gpio_para[index].pin_dir);
	}
}

#endif

