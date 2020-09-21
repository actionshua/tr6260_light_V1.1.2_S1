#ifndef __YA_ENV__
#define __YA_ENV__

typedef enum
{
	SWITCH_OFF = 0,
	SWITCH_ON,
	SWITCH_STA_FROM_SAVE
}pwr_on_relay_status_t;

extern int ya_save_pwrOn_switch_ctrMode(pwr_on_relay_status_t ctrType);
extern pwr_on_relay_status_t ya_get_pwrOn_switch_ctrMode(void);
extern int ya_save_switch_status(int val);
extern int ya_get_switch_status(void);
extern int ya_get_inching_time(void);
extern int ya_save_inching_time(int val);
extern int ya_get_inching_enable_val(void);
extern int ya_save_inching_enable_val(int val);

#endif

