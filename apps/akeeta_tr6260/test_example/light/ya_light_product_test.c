#include "ya_common.h"
#include "ya_app_main.h"
#include "ya_light_io.h"
#include "ya_light_product_test.h"

typedef enum
{	
	LICENSE_TEST = 0,
	RGBCW_TEST,
	AGING_TEST,

	LIGHT_ROUTER_FAILED,
	LIGHT_ROUTER_LOW_RSSI,
	LIGHT_LICENSE_FAILED,

	LIGHT_TEST_OK,
}light_test_machine_t;


typedef struct
{
	uint8_t test_state;
	uint8_t aging_time;
}ya_light_test_para_t;


ya_light_test_para_t ya_light_test_para;

int32_t ya_rgbcw_test(uint8_t test_type)
{
	uint16_t count = 0;

	if(test_type == 0)
	{
		for (count = 0; count < 24; count++)
		{
			ya_pwm_led_write(255,0,0,0,0);
			ya_delay(1000);		
			ya_pwm_led_write(0,255,0,0,0);
			ya_delay(1000);		
			ya_pwm_led_write(0,0,255,0,0);
			ya_delay(1000);		
			ya_pwm_led_write(0,0,0,255,0);
			ya_delay(1000);		
			ya_pwm_led_write(0,0,0,0,255);
			ya_delay(1000);		
		}
	}else
	{
		while(1)
		{
			ya_pwm_led_write(255,0,0,0,0);
			ya_delay(1000);		
			ya_pwm_led_write(0,255,0,0,0);
			ya_delay(1000);		
			ya_pwm_led_write(0,0,255,0,0);
			ya_delay(1000);		
			ya_pwm_led_write(0,0,0,255,0);
			ya_delay(1000);		
			ya_pwm_led_write(0,0,0,0,255);
			ya_delay(1000);
		}
	}
	return 0;
}

int32_t ya_aging_poweron_rgbcw_test()
{
	uint16_t count = 0;
	for (count = 0; count < 5; count++)
	{
		ya_pwm_led_write(255,0,0,0,0);
		ya_delay(500);		
		ya_pwm_led_write(0,255,0,0,0);
		ya_delay(500);		
		ya_pwm_led_write(0,0,255,0,0);
		ya_delay(500);		
		ya_pwm_led_write(0,0,0,255,0);
		ya_delay(500);		
		ya_pwm_led_write(0,0,0,0,255);
		ya_delay(500);		
	}

	return 0;
}

int32_t ya_aging_rgbcw_test()
{
	if(ya_light_test_para.aging_time >= 50)
	{
		ya_light_test_para.aging_time = 0;
		return 0;
	}
	
	for (; ya_light_test_para.aging_time < 50; ya_light_test_para.aging_time++)
	{
		if(ya_light_test_para.aging_time < 20)
		{
			ya_printf(C_LOG_INFO, "warm long time test\r\n");
			ya_pwm_led_write(0,0,0,255,0);
			ya_delay(60 *1000);		
		}else if (ya_light_test_para.aging_time < 40)
		{
			ya_printf(C_LOG_INFO, "green long time test\r\n");
			ya_pwm_led_write(0,0,0,0,255);
			ya_delay(60 *1000);		
		}else 
		{
			ya_printf(C_LOG_INFO, "RGB long time test\r\n");
			ya_pwm_led_write(255,255,255,0,0);
			ya_delay(60 *1000);		
		}
	}
	return 0;

}

int32_t ya_ligth_rgbcw_router_failed()
{
	ya_pwm_led_write(0,0,0,255,0);
	while(1)
	{
		ya_delay(1000);
	}
}

int32_t ya_ligth_rgbcw_router_low_rssi()
{
	uint16_t brightness;
	int  i = 0;
	
	while(1)
	{
		//dim to bright
		for (i = 10; i <= 255; i=i+24)
		{
			ya_pwm_led_write(0,0,0,i,0);
			vTaskDelay(150);
		}
		//bright to dim
		for (i = 255; i>=0; i = i - 24)
		{
			ya_pwm_led_write(0,0,0,i,0);
			vTaskDelay(150);
		}
		
		ya_pwm_led_write(0,0,0,0,0);
		ya_delay(250);

		ya_pwm_led_write(0,0,0,255,0);
		ya_delay(250);

		ya_pwm_led_write(0,0,0,0,0);
		ya_delay(250);

	}

	return 0;
}

int32_t ya_ligth_rgbcw_router_license_failed()
{
	uint16_t brightness = 1;
	while(1)
	{
		//on
		ya_pwm_led_write(10,0,0,0,0);
		ya_delay(3000);

		//off
		ya_pwm_led_write(0,0,0,0,0);
		ya_delay(3000);
	}
	return 0;
}

int32_t ya_ligth_test_ok()
{
	ya_pwm_led_write(0,10,0,0,0);
	while(1)
	{
		ya_delay(1000);
	}
}

void ya_light_factory_test(int32_t router_rssi, uint8_t test_type, CLOUD_T cloud_type)
{
	int32_t ret = 0;

	if (router_rssi < -60)
		ya_light_test_para.test_state = LIGHT_ROUTER_LOW_RSSI;
	else
		ya_light_test_para.test_state = LICENSE_TEST;

	ya_light_test_para.aging_time = 0;
	
	while(1)
	{
		switch(ya_light_test_para.test_state)
		{
			case LICENSE_TEST:
			{
				ya_printf(C_LOG_INFO, "LICENSE_TEST\r\n");
				if (cloud_type == AKEETA_OVERSEA)
				{
					if (ya_check_us_license_exit()==0)
						ya_light_test_para.test_state = RGBCW_TEST;
					else
						ya_light_test_para.test_state = LIGHT_LICENSE_FAILED;
					
				}else if (cloud_type == AKEETA_CN)
				{
					if (ya_check_cn_license_exit()==0)
						ya_light_test_para.test_state = RGBCW_TEST;
					else
						ya_light_test_para.test_state = LIGHT_LICENSE_FAILED;

				}else if (cloud_type == UNKNOWN_CLOUD_TYPE)
				{
					if (ya_check_cn_license_exit()==0 && ya_check_us_license_exit()==0)
						ya_light_test_para.test_state = RGBCW_TEST;
					else
						ya_light_test_para.test_state = LIGHT_LICENSE_FAILED;
					
				}else
				{
					ya_light_test_para.test_state = LIGHT_LICENSE_FAILED;
				}
			}
			break;


			case RGBCW_TEST:
			{
				ya_printf(C_LOG_INFO, "RGBCW_TEST\r\n");
			
				//2 miniues;
				ret = ya_rgbcw_test(test_type);
				if(ret == 0)
					ya_light_test_para.test_state = AGING_TEST;
			}	
			break;

			case AGING_TEST:
			{
				ya_printf(C_LOG_INFO, "AGING_TEST, %d\r\n", ya_light_test_para.aging_time);
				ret = ya_aging_rgbcw_test();
				if(ret == 0)
				{
					ya_ligth_test_ok();
					ya_light_test_para.test_state = LIGHT_TEST_OK;
				}
			}	
			break;

			case LIGHT_ROUTER_FAILED:
			ya_printf(C_LOG_INFO, "LIGHT_ROUTER_FAILED\r\n");
			ya_ligth_rgbcw_router_failed();
			break;


			case LIGHT_ROUTER_LOW_RSSI:
			ya_printf(C_LOG_INFO, "LIGHT_ROUTER_LOW_RSSI\r\n");
			ya_ligth_rgbcw_router_low_rssi();
			break;

			case LIGHT_LICENSE_FAILED:
			ya_printf(C_LOG_INFO, "LIGHT_LICENSE_FAILED\r\n");
			ya_ligth_rgbcw_router_license_failed();
			break;

			case LIGHT_TEST_OK:

			break;

			default:

			break;
		}

	}
}


