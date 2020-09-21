/**
 * This implementation CAN NOT be called from the ISR method
 * Meanwhile, this implementation also be effective after the vTaskStartScheduler method
 * If need to call from ISR, the APIs should have another implementation
 */

#include "timer_interface.h"
#include "FreeRTOS.h"
#include "task.h"

bool has_timer_expired(Timer *timer)
{
	uint32_t cur_timer =  (uint32_t)(xTaskGetTickCount() * portTICK_RATE_MS);
	uint32_t init_timer = (uint32_t)(timer->init_tick * portTICK_RATE_MS);
	uint32_t delta_timer = 0;

	if(cur_timer >= init_timer)
		delta_timer = cur_timer - init_timer;
	else
		delta_timer = 0xffffffff - init_timer + cur_timer;

	if (timer->type == TIMER_TYPE_MS)
	{
		if (delta_timer > timer->time_out)
			return 1;
	}else
	{
		if (delta_timer/1000 > timer->time_out)
			return 1;
	}
		
	return 0;

}

void countdown_ms(Timer *timer, uint32_t timeout)
{
    if (timer == NULL || timeout <= 0) {
        return;
    }
    timer->init_tick = xTaskGetTickCount();
    timer->time_out = timeout;
    timer->type = TIMER_TYPE_MS;
}

void countdown_sec(Timer *timer, uint32_t timeout)
{
    if (timer == NULL || timeout <= 0) {
        return;
    }
    timer->init_tick = (uint32_t) xTaskGetTickCount();
    timer->time_out = timeout;
    timer->type = TIMER_TYPE_S;
}

uint32_t left_ms(Timer *timer)
{
	uint32_t result_ms = 0;
	uint32_t cur_tick = (uint32_t) xTaskGetTickCount();
	uint32_t delta_tick = 0;

	if(cur_tick >= timer->init_tick)
		delta_tick = cur_tick - timer->init_tick;
	else
		delta_tick = 0xffffffff - timer->init_tick + cur_tick;

    if (timer->type == TIMER_TYPE_MS) {
        result_ms = timer->time_out - (delta_tick * portTICK_RATE_MS);
    } else {
       result_ms = (timer->time_out * 1000) - (delta_tick * portTICK_RATE_MS);
    }

    return result_ms;
}

void aws_init_timer(Timer *timer)
{
    timer->time_out = 0;
    timer->init_tick = 0;
    timer->type = TIMER_TYPE_MS;
}

//weishan add
extern uint64_t ya_hal_os_msec_to_ticks(uint32_t msecs);
void aws_delay(uint32_t milliseconds)
{
	vTaskDelay(ya_hal_os_msec_to_ticks(milliseconds));
}


