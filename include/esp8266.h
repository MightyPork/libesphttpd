// Combined include file for esp8266
// Actually misnamed, as it also works for ESP32.
// ToDo: Figure out better name

#ifndef ESP8266_COMMON_H
#define ESP8266_COMMON_H

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

#ifndef DEBUG_MALLOC
#define DEBUG_MALLOC 0
#endif

#if DEBUG_MALLOC
static inline void* my_malloc(size_t size, int line, const char *fn) {
	void *p = os_malloc(size);
	mem_dbg("*A (%4d) -> %p @ "__BASE_FILE__":%d %s", size, p, line, fn);
	return p;
}

static inline void my_free(void *p, int line, const char *fn) {
	os_free(p);
	mem_dbg("*F %p @ "__BASE_FILE__":%d %s", p, line, fn);
}
#define malloc(siz) my_malloc(siz, __LINE__, __FUNCTION__)
#define free(p) my_free(p, __LINE__, __FUNCTION__)
#else
#define malloc(siz) os_malloc(siz)
#define free(p) os_free(p)
#endif



#define ESP_CONST_DATA __attribute__((aligned(4))) __attribute__((section(".irom.text")))

#endif // ESP8266_COMMON_H
