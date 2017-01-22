#ifndef UPTIME_H
#define UPTIME_H

#include <esp8266.h>

extern volatile uint32_t uptime;

/**
 * Initialize the virtual timer for uptime counter.
 */
void uptime_timer_init(void);

/**
 * Print uptime to a buffer in user-friendly format.
 * Should be at least 20 bytes long.
 */
void uptime_str(char *buf);

/** Print uptime to stdout in user-friendly format */
void uptime_print(void);

#endif // UPTIME_H
