#ifndef HTTPD_LOGGING_H
#define HTTPD_LOGGING_H

#include <esp8266.h>
#include "uptime.h"

#ifndef VERBOSE_LOGGING
#define VERBOSE_LOGGING 1
#endif

#ifndef LOG_EOL
#define LOG_EOL "\n"
#endif

/**
 * Print a startup banner message (printf syntax)
 * Uses bright green color
 */
#define banner(fmt, ...) \
	do {  \
		printf(LOG_EOL "\x1b[32;1m");  \
		uptime_print();  \
		printf(" [i] " fmt "\x1b[0m" LOG_EOL LOG_EOL, ##__VA_ARGS__);  \
	} while(0)

/**
 * Same as 'info()', but enabled even if verbose logging is disabled.
 * This can be used to print version etc under the banner.
 */
#define banner_info(fmt, ...) \
	do { \
		printf("\x1b[32m"); \
		uptime_print(); \
		printf(" [i] " fmt "\x1b[0m"LOG_EOL, ##__VA_ARGS__); \
	} while(0)

#if VERBOSE_LOGGING
	/**
	 * Print a debug log message (printf format)
	 */
	#define dbg(fmt, ...) \
		do { \
			uptime_print(); \
			printf(" [ ] " fmt LOG_EOL, ##__VA_ARGS__); \
		} while(0)

	/**
	 * Print a info log message (printf format)
	 * Uses bright green color
	 */
	#define info(fmt, ...) \
		do { \
			printf("\x1b[32m"); \
			uptime_print(); \
			printf(" [i] " fmt "\x1b[0m"LOG_EOL, ##__VA_ARGS__); \
		} while(0)
#else
	#define dbg(fmt, ...)
	#define info(fmt, ...)
#endif

/**
 * Print a error log message (printf format)
 * Uses bright red color
 */
#define error(fmt, ...) \
	do { \
		printf("\x1b[31;1m"); \
		uptime_print(); \
		printf(" [E] " fmt "\x1b[0m"LOG_EOL, ##__VA_ARGS__); \
	} while(0)

/**
 * Print a warning log message (printf format)
 * Uses bright yellow color
 */
#define warn(fmt, ...) \
	do { \
		printf("\x1b[33;1m"); \
		uptime_print(); \
		printf(" [W] " fmt "\x1b[0m"LOG_EOL, ##__VA_ARGS__); \
	} while(0)

#endif // HTTPD_LOGGING_H


// --------------- logging categories --------------------



#ifndef DEBUG_ROUTER
#define DEBUG_ROUTER 1
#endif

#ifndef DEBUG_ESPFS
#define DEBUG_ESPFS 1
#endif

#ifndef DEBUG_WS
#define DEBUG_WS 1
#endif

#ifndef DEBUG_HTTP
#define DEBUG_HTTP 1
#endif

#ifndef DEBUG_CAPTDNS
#define DEBUG_CAPTDNS 1
#endif

#if DEBUG_ROUTER
#define router_warn warn
#define router_dbg dbg
#define router_error error
#define router_info info
#else
#define router_dbg(...)
#define router_warn(...)
#define router_error(...)
#define router_info(...)
#endif

#if DEBUG_ESPFS
#define espfs_warn warn
#define espfs_dbg dbg
#define espfs_error error
#define espfs_info info
#else
#define espfs_dbg(...)
#define espfs_warn(...)
#define espfs_error(...)
#define espfs_info(...)
#endif

#if DEBUG_WS
#define ws_warn warn
#define ws_dbg dbg
#define ws_error error
#define ws_info info
#else
#define ws_dbg(...)
#define ws_warn(...)
#define ws_error(...)
#define ws_info(...)
#endif

#if DEBUG_HTTP
#define http_warn warn
#define http_dbg dbg
#define http_error error
#define http_info info
#else
#define http_dbg(...)
#define http_warn(...)
#define http_error(...)
#define http_info(...)
#endif

#if DEBUG_CAPTDNS
#define cdns_warn warn
#define cdns_dbg dbg
#define cdns_error error
#define cdns_info info
#else
#define cdns_dbg(...)
#define cdns_warn(...)
#define cdns_error(...)
#define cdns_info(...)
#endif

