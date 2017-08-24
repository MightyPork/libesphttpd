// Combined include file for esp8266
// Actually misnamed, as it also works for ESP32.
// ToDo: Figure out better name


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef FREERTOS
#include <stdint.h>

#ifdef ESP32
#include <esp_common.h>
#else
#include <espressif/esp_common.h>
#endif

#else
#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include <ets_sys.h>
#include <gpio.h>
#include <mem.h>
#include <osapi.h>
#include <user_interface.h>
#include <upgrade.h>
#endif

#include "logging.h"
#include "platform.h"
#include "espmissingincludes.h"

#define TIMER_START(pTimer, func, ms, repeated) do { \
	os_timer_disarm((pTimer)); \
	os_timer_setfn((pTimer), (func), NULL); \
	os_timer_arm((pTimer), (ms), (repeated)); \
} while (0)
