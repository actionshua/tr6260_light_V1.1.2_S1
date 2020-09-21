
#ifndef _YA_CONFIG_PLATFORM_H_
#define _YA_CONFIG_PLATFORM_H_

#include "FreeRTOS.h"
#include "task.h"
#include "string.h"
#include "stdlib.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

extern void system_printf(const char *f, ...);
#define ya_mqtt_printf(format, ...)  system_printf(format, ##__VA_ARGS__)
#define printf(format, ...)  system_printf(format, ##__VA_ARGS__)

#endif

