#ifndef _YA_HAL_PWM_H_
#define _YA_HAL_PWM_H_

#define PWM_NUM					5

#define PWM0_CHANNEL			PMW_CHANNEL_0
#define PWM0_FREQUENCY			5000
#define PWM0_DUTY_RATIO			500

#define PWM1_CHANNEL			PMW_CHANNEL_1
#define PWM1_FREQUENCY			5000
#define PWM1_DUTY_RATIO			500

#define PWM2_CHANNEL			PMW_CHANNEL_2
#define PWM2_FREQUENCY			5000
#define PWM2_DUTY_RATIO			500

#define PWM3_CHANNEL			PMW_CHANNEL_3
#define PWM3_FREQUENCY			5000
#define PWM3_DUTY_RATIO			500

#define PWM4_CHANNEL			PMW_CHANNEL_4
#define PWM4_FREQUENCY			5000
#define PWM4_DUTY_RATIO			500


#define YA_PWM_PARA_INIT	 {{PWM0_CHANNEL, PWM0_FREQUENCY, PWM0_DUTY_RATIO},\
							  {PWM1_CHANNEL, PWM1_FREQUENCY, PWM1_DUTY_RATIO},\
							  {PWM2_CHANNEL, PWM2_FREQUENCY, PWM2_DUTY_RATIO},\
							  {PWM3_CHANNEL, PWM3_FREQUENCY, PWM3_DUTY_RATIO},\
							  {PWM4_CHANNEL, PWM4_FREQUENCY, PWM4_DUTY_RATIO}}
typedef enum
{
	PWM_R = 4,
	PWM_G = 1,
	PWM_B = 2,

	PWM_COOL =5 ,
	PWM_WARM = 0,
}YA_PWM_NAME;



#if (PWM_NUM > 0)

extern void ya_hal_pwm_init(void);

extern void ya_hal_pwm_write(uint8_t index, uint32_t duty_ratio);

#endif

#endif

