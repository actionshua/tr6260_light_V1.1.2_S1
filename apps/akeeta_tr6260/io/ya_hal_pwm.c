#include "FreeRTOS.h"
#include "soc_pin_mux.h"
#include "ya_hal_pwm.h"
#include "drv_gpio.h"
#include "ya_common.h"

#if (PWM_NUM > 0)

#include "pit.h"
#include "drv_pwm.h"
#include "soc_pin_mux.h"
#define PWM_CLK_BASE		40000000 //40Mhz

static int pwm_reload(uint32_t channel, uint32_t freq, uint32_t duty, uint8_t bit)
{
	uint32_t tick_h, tick_l;
	uint32_t base, num, flags;
	int32_t rnt = 0;

	if(channel >= PMW_CHANNEL_MAC || duty < 0 || duty >1000)
	{
		return DRV_ERR_INVALID_PARAM;
	} 
	else if(channel < PMW_CHANNEL_4)
	{
		base = PIT_NUM_0;
		num = channel;
	} 
	else
	{
		base = PIT_NUM_1;
		num = channel -PMW_CHANNEL_4;
	}

	tick_h = PWM_CLK_BASE / 1000 * duty / freq;
	tick_l = PWM_CLK_BASE / freq - tick_h;

	if(tick_h>=1){tick_h = tick_h - 1;}
	if(tick_l>=1){tick_l = tick_l - 1;}


	flags = system_irq_save();
	if (duty <= 1) 
	{
		rnt = pit_ch_ctrl(base, num, 0, PIT_CHCLK_APB, PIT_CHMODE_PWM);
	}
	else 
	{
		rnt = pit_ch_ctrl(base, num, bit, PIT_CHCLK_APB, PIT_CHMODE_PWM);
	}

	if (rnt == DRV_SUCCESS)
	{
		rnt = pit_ch_reload_value(base, num, (tick_h<<16)|tick_l);
	}
	system_irq_restore(flags); 
	return rnt;
}

static void pwm_ctrl(uint8_t channel, uint32_t freq, uint32_t duty, uint32_t pn) 
{
	int32_t rnt = pwm_reload(channel, freq, duty, pn); 
	if(rnt != DRV_SUCCESS)
	{
		ya_printf(C_LOG_ERROR,"pwm_ctrl reload error channel: %d", channel);
	}
	if (duty <= 0 || duty > 1000) 
	{
		rnt = pwm_stop(channel);
	}
	else
	{
		rnt = pwm_start(channel);
		if(rnt != DRV_SUCCESS)
		{
			ya_printf(C_LOG_INFO,"pwm_ctrl start error channel: %d", channel);
		}
	}
}

void pwm_cfg_init(uint8_t channel, uint32_t freq)
{
	if (channel == PMW_CHANNEL_0) 
	{
		pwm_init(PMW_CHANNEL_0);
		 PIN_FUNC_SET(IO_MUX0_GPIO0, FUNC_GPIO0_PWM_CTRL0);
		pwm_ctrl(channel, freq, 1, 0);
	} 
	else if (channel == PMW_CHANNEL_1) 
	{
		pwm_init(PMW_CHANNEL_1);
		PIN_FUNC_SET(IO_MUX0_GPIO1, FUNC_GPIO1_PWM_CTRL1);
		pwm_ctrl(channel, freq, 1, 0);
	}
	else if (channel == PMW_CHANNEL_2) 
	{
		pwm_init(PMW_CHANNEL_2);
		PIN_FUNC_SET(IO_MUX0_GPIO2, FUNC_GPIO2_PWM_CTRL2);
		pwm_ctrl(channel, freq, 1, 0);
	} 
	else if (channel == PMW_CHANNEL_3) 
	{
		pwm_init(PMW_CHANNEL_3);
		PIN_FUNC_SET(IO_MUX0_GPIO3, FUNC_GPIO3_PWM_CTRL3);
		pwm_ctrl(channel, freq, 1, 0);
	}
	else if (channel == PMW_CHANNEL_4) 
	{
		pwm_init(PMW_CHANNEL_4);
		PIN_FUNC_SET(IO_MUX0_GPIO4, FUNC_GPIO4_PWM_CTRL4);
		pwm_ctrl(channel, freq, 1, 0);
	} 
	else if (channel == PMW_CHANNEL_5) 
	{
		pwm_init(PMW_CHANNEL_5);
		pwm_ctrl(channel, freq, 1, 0);
	}
}

void ya_hal_pwm_init(void)
{
	pwm_cfg_init(PMW_CHANNEL_0,5000);
	pwm_cfg_init(PMW_CHANNEL_1,5000);
	pwm_cfg_init(PMW_CHANNEL_2,5000);
	pwm_cfg_init(PMW_CHANNEL_4,5000);
	pwm_cfg_init(PMW_CHANNEL_5,5000);
}

//duty_ratio:0~1000
void ya_hal_pwm_write(uint8_t index, uint32_t duty_ratio)
{
	if (duty_ratio >1000)
		return;
	
	if(index < PMW_CHANNEL_MAC)
	{
		pwm_ctrl(index,5000,duty_ratio,0);
	}
}

#endif

