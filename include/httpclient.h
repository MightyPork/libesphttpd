/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Martin d'Allens <martin.dallens@gmail.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <esp8266.h>
#include <httpd.h>

#if DEBUG_HTTPC
#define httpc_warn warn
#define httpc_dbg dbg
#define httpc_error error
#define httpc_info info
#else
#define httpc_dbg(...)
#define httpc_warn(...)
#define httpc_error(...)
#define httpc_info(...)
#endif

#define HTTP_STATUS_GENERIC_ERROR  -1   // In case of TCP or DNS error the callback is called with this status.
#define BUFFER_SIZE_MAX            5000 // Size of http responses that will cause an error.

#define HTTP_REQUEST_TIMEOUT_MS 5000

/* Define this if ssl is needed. Also link the ssl lib */
//#define USE_SECURE

/**
 * "full_response" is a string containing all response headers and the response body.
 * "response_body and "http_status" are extracted from "full_response" for convenience.
 *
 * A successful request corresponds to an HTTP status code of 200 (OK).
 * More info at http://en.wikipedia.org/wiki/List_of_HTTP_status_codes
 */
typedef void (* httpclient_cb)(int http_status,
							   const char *response_headers,
							   const char *response_body,
							   size_t body_size);

typedef struct {
	const char* url;
	const char* headers;
	const char* body;
	httpd_method method;
	int timeout;
	size_t max_response_len;
} httpclient_args;

/**
 * Populate args struct with defaults
 * @param args
 */
void httpclient_args_init(httpclient_args *args);

/**
 * @brief Download a web page from its URL.
 *
 * Try:
 * http_get("http://wtfismyip.com/text", http_callback_example);
 */
bool http_get(const char * url, httpclient_cb user_callback);

/**
 * @brief Post data to a web form.
 *
 * The data should be encoded as application/x-www-form-urlencoded.
 *
 * Try:
 * http_post("http://httpbin.org/post", "first_word=hello&second_word=world", http_callback_example);
 */
bool http_post(const char *url, const char *post_data, httpclient_cb user_callback);

/** Like POST, but with the PUT method. */
bool http_put(const char *url, const char *body, httpclient_cb user_callback);

/**
 * @brief Send a HTTP request
 * @param url      : protocol://host[:port][/path]
 * @param method   : get, post, ...
 * @param body     : request body. If GET & body != NULL, method changes to POST.
 * @param headers  : additional headers string. Must end with \r\n
 * @param user_callback : callback for parsing the response
 * @return success (in sending)
 */
bool http_request(const httpclient_args *args, httpclient_cb user_callback);

/**
 * Output on the UART.
 */
void http_callback_example(int http_status, const char *response_headers, const char *response_body, size_t body_size);

/**
 * Show status code, and body on error. Error/warn log msg on error.
 */
void http_callback_showstatus(int code, const char *response_headers, const char *response_body, size_t body_size);

#endif
