/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Martin d'Allens <martin.dallens@gmail.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

// FIXME: sprintf->snprintf everywhere.

#include <esp8266.h>
#include <httpd.h>
#include <httpclient.h>

#include "httpclient.h"
#include "esp_utils.h"

// Internal state.
typedef struct {
	char *path;
	int port;
	char *post_data;
	char *headers;
	char *hostname;
	char *buffer;
	int buffer_size;
	int max_buffer_size;
	bool secure;
	httpclient_cb user_callback;
	int timeout;
	ETSTimer timeout_timer;
	httpd_method method;
	void *userArg;
} request_args;

static int ICACHE_FLASH_ATTR
chunked_decode(char *chunked, int size)
{
	char *src = chunked;
	char *end = chunked + size;
	int i, dst = 0;

	do {
		char *endstr = NULL;
		//[chunk-size]
		i = (int) esp_strtol(src, &endstr, 16);
		httpc_dbg("Chunk Size:%d\r\n", i);
		if (i <= 0)
			break;
		//[chunk-size-end-ptr]
		src = strstr(src, "\r\n") + 2;
		//[chunk-data]
		memmove(&chunked[dst], src, (size_t) i);
		src += i + 2; /* CRLF */
		dst += i;
	} while (src < end);

	//
	//footer CRLF
	//

	/* decoded size */
	return dst;
}

static void ICACHE_FLASH_ATTR
receive_callback(void *arg, char *buf, unsigned short len)
{
	struct espconn *conn = (struct espconn *) arg;
	request_args *req = (request_args *) conn->reserve;

	if (req->buffer == NULL) {
		return;
	}

	// Let's do the equivalent of a realloc().
	const int new_size = req->buffer_size + len;
	char *new_buffer;
	if (new_size > BUFFER_SIZE_MAX || NULL == (new_buffer = (char *) malloc(new_size))) {
		httpc_error("Response too long (%d)", new_size);
		req->buffer[0] = '\0'; // Discard the buffer to avoid using an incomplete response.
		if (req->secure) {
#ifdef USE_SECURE
			espconn_secure_disconnect(conn);
#endif
		}
		else {
			espconn_disconnect(conn);
		}
		return; // The disconnect callback will be called.
	}

	memcpy(new_buffer, req->buffer, req->buffer_size);
	memcpy(new_buffer + req->buffer_size - 1 /*overwrite the null character*/, buf, len); // Append new data.
	new_buffer[new_size - 1] = '\0'; // Make sure there is an end of string.

	free(req->buffer);
	req->buffer = new_buffer;
	req->buffer_size = new_size;
}


static void ICACHE_FLASH_ATTR
sent_callback(void *arg)
{
	struct espconn *conn = (struct espconn *) arg;
	request_args *req = (request_args *) conn->reserve;

	if (req->post_data == NULL) {
		httpc_dbg("All sent");
	}
	else {
		// The headers were sent, now send the contents.
		httpc_dbg("Sending request body");
		if (req->secure) {
#ifdef USE_SECURE
			espconn_secure_sent(conn, (uint8_t *)req->post_data, strlen(req->post_data));
#endif
		}
		else {
			espconn_sent(conn, (uint8_t *) req->post_data, (uint16) strlen(req->post_data));
		}
		free(req->post_data);
		req->post_data = NULL;
	}
}

static void ICACHE_FLASH_ATTR
connect_callback(void *arg)
{
	httpc_info("Connected!");
	struct espconn *conn = (struct espconn *) arg;
	request_args *req = (request_args *) conn->reserve;

	espconn_regist_recvcb(conn, receive_callback);
	espconn_regist_sentcb(conn, sent_callback);

	char post_headers[32] = "";

	if (req->post_data != NULL) { // If there is data this is a POST request.
		sprintf(post_headers, "Content-Length: %d\r\n", strlen(req->post_data));

		if (req->method == HTTPD_METHOD_GET) {
			req->method = HTTPD_METHOD_POST;
		}
	}

	const char *method = httpdMethodName(req->method);

	if (req->headers == NULL) { /* Avoid NULL pointer, it may cause exception */
		req->headers = (char *) malloc(sizeof(char));
		req->headers[0] = '\0';
	}

	const size_t buflen = 69 + strlen(method)
						  + strlen(req->path)
						  + strlen(req->hostname)
						  + strlen(req->headers)
						  + strlen(post_headers) + 10; // safety margin
	char buf[buflen];

	int len = sprintf(buf,
					  "%s %s HTTP/1.1\r\n"
						  "Host: %s:%d\r\n"
						  "Connection: close\r\n"
						  "User-Agent: ESP8266\r\n"
						  "%s"
						  "%s"
						  "\r\n",
					  method, req->path, req->hostname, req->port, req->headers, post_headers);

	httpc_dbg("Sending request");
	if (req->secure) {
#ifdef USE_SECURE
		espconn_secure_sent(conn, (uint8_t *)buf, len);
#endif
	}
	else {
		espconn_sent(conn, (uint8_t *) buf, (uint16) len);
	}

	if (req->headers != NULL) {
		free(req->headers);
		req->headers = NULL;
	}
}

/**
 * @brief Free all that could be allocated in a request, including the struct itself.
 * @param req : req to free
 */
static ICACHE_FLASH_ATTR void
free_req(request_args *req)
{
	if (!req) return;

	if (req->buffer) free(req->buffer);
	if (req->hostname) free(req->hostname);
	if (req->path) free(req->path);
	if (req->post_data) free(req->post_data);
	if (req->headers) free(req->headers);
	free(req);
}

static void ICACHE_FLASH_ATTR
disconnect_callback(void *arg)
{
	httpc_dbg("Disconnected");
	struct espconn *conn = (struct espconn *) arg;

	if (conn == NULL) {
		httpc_dbg("conn is null!");
		return;
	}

	if (conn->reserve != NULL) {
		httpc_dbg("Processing response");

		request_args *req = (request_args *) conn->reserve;
		int http_status = HTTP_STATUS_GENERIC_ERROR;
		int body_size = 0;
		char *body = "";

		/* Turn off timeout timer */
		os_timer_disarm(&(req->timeout_timer));

		if (req->buffer == NULL) {
			httpc_error("Buffer shouldn't be NULL");
		}
		else if (req->buffer[0] != '\0') {
			// FIXME: make sure this is not a partial response, using the Content-Length header.

			const char *version10 = "HTTP/1.0 ";
			const char *version11 = "HTTP/1.1 ";
			if (!strstarts(req->buffer, version10) && !strstarts(req->buffer, version11)) {
				httpc_error("Invalid version in %s", req->buffer);
			}
			else {
				http_status = atoi(req->buffer + strlen(version10));
				/* find body and zero terminate headers */
				body = strstr(req->buffer, "\r\n\r\n") + 2;
				*body++ = '\0';
				*body++ = '\0';

				body_size = (int) (req->buffer_size - (body - req->buffer));

				if (strstr(req->buffer, "Transfer-Encoding: chunked")) {
					body_size = chunked_decode(body, body_size);
				}
			}
		}

		httpc_info("Request completed.");
		if (req->user_callback != NULL) { // Callback is optional.
			req->user_callback(http_status, req->buffer, body, body_size, req->userArg);
		}

		free_req(req);
	} else {
		httpc_error("Reserve is NULL!");
	}

	espconn_delete(conn);
	if (conn->proto.tcp != NULL) {
		free(conn->proto.tcp);
	}
	free(conn);
}

static void ICACHE_FLASH_ATTR
error_callback(void *arg, sint8 errType)
{
	(void) errType;

	httpc_error("Disconnected with error, type %d", errType);
	disconnect_callback(arg);
}

static void ICACHE_FLASH_ATTR
http_timeout_callback(void *arg)
{
	httpc_error("Connection timeout\n");
	struct espconn *conn = (struct espconn *) arg;

	if (conn == NULL) {
		return;
	}

	request_args *req = (request_args *) conn->reserve;
	if (req) {
		/* Call disconnect */
		if (req->secure) {
#ifdef USE_SECURE
			espconn_secure_disconnect(conn);
#endif
		}
		else {
			espconn_disconnect(conn);
		}

		if (req->user_callback != NULL) {
			// fire callback, so user can free the userArg
			req->user_callback(HTTP_STATUS_TIMEOUT, req->buffer, "", 0, req->userArg);
		}
		free_req(req);
	}

	// experimental - better cleanup
	if (conn->proto.tcp != NULL) {
		free(conn->proto.tcp);
	}
	free(conn);
}

static void ICACHE_FLASH_ATTR
dns_callback(const char *hostname, ip_addr_t *addr, void *arg)
{
	(void)hostname;
	request_args *req = (request_args *) arg;

	if (addr == NULL) {
		httpc_error("DNS failed for %s", hostname);
		if (req->user_callback != NULL) {
			req->user_callback(HTTP_STATUS_GENERIC_ERROR, "", "", 0, req->userArg);
		}
		free(req);
	}
	else {
		httpc_info("DNS found %s "IPSTR, hostname, IP2STR(addr));

		struct espconn *conn = (struct espconn *) malloc(sizeof(struct espconn));
		conn->type = ESPCONN_TCP;
		conn->state = ESPCONN_NONE;
		conn->proto.tcp = (esp_tcp *) malloc(sizeof(esp_tcp));
		conn->proto.tcp->local_port = espconn_port();
		conn->proto.tcp->remote_port = req->port;
		conn->reserve = req;

		memcpy(conn->proto.tcp->remote_ip, addr, 4);

		espconn_regist_connectcb(conn, connect_callback);
		espconn_regist_disconcb(conn, disconnect_callback);
		espconn_regist_reconcb(conn, error_callback);

		/* Set connection timeout timer */
		os_timer_disarm(&(req->timeout_timer));
		os_timer_setfn(&(req->timeout_timer), (os_timer_func_t *) http_timeout_callback, conn);
		os_timer_arm(&(req->timeout_timer), req->timeout, false);

		if (req->secure) {
#ifdef USE_SECURE
			espconn_secure_set_size(ESPCONN_CLIENT, 5120); // set SSL buffer size
			espconn_secure_connect(conn);
#endif
		}
		else {
			espconn_connect(conn);
		}
	}
}

bool ICACHE_FLASH_ATTR
http_request(const httpclient_args *args, httpclient_cb user_callback)
{
	// --- prepare port, secure... ---

	// FIXME: handle HTTP auth with http://user:pass@host/
	const char *url = args->url;

	char hostname[128] = "";
	int port = 80;
	bool secure = false;

	if (strstarts(url, "http://"))
		url += strlen("http://"); // Get rid of the protocol.
	else if (strstarts(url, "https://")) {
		port = 443;
		secure = true;
		url += strlen("https://"); // Get rid of the protocol.
	}
	else {
		httpc_error("Invalid URL protocol: %s", url);
		return false;
	}

	char *path = strchr(url, '/');
	if (path == NULL) {
		path = strchr(url, '\0'); // Pointer to end of string.
	}

	char *colon = strchr(url, ':');
	if (colon > path) {
		colon = NULL; // Limit the search to characters before the path.
	}

	if (colon == NULL) { // The port is not present.
		memcpy(hostname, url, (size_t) (path - url));
		hostname[path - url] = '\0';
	}
	else {
		port = atoi(colon + 1);
		if (port == 0) {
			httpc_error("Port error %s\n", url);
			return false;
		}

		memcpy(hostname, url, (size_t) (colon - url));
		hostname[colon - url] = '\0';
	}

	if (path[0] == '\0') { // Empty path is not allowed.
		path = "/";
	}

	// ---

	httpc_info("HTTP request: %s:%d%s", hostname, port, path);

	request_args *req = (request_args *) malloc(sizeof(request_args));
	req->hostname = esp_strdup(hostname);
	req->path = esp_strdup(path);

	// remove #anchor
	char *hash = strchr(req->path, '#');
	if (hash != NULL) *hash = '\0'; // remove the hash part

	req->port = port;
	req->secure = secure;
	req->headers = esp_strdup(args->headers);
	req->post_data = esp_strdup(args->body);
	req->buffer_size = 1;
	req->buffer = (char *) malloc(1);
	req->buffer[0] = '\0'; // Empty string.
	req->user_callback = user_callback;
	req->timeout = HTTP_REQUEST_TIMEOUT_MS;
	req->method = args->method;
	req->userArg = args->userArg;

	ip_addr_t addr;
	err_t error = espconn_gethostbyname((struct espconn *) req, // It seems we don't need a real espconn pointer here.
										hostname, &addr, dns_callback);

	if (error == ESPCONN_INPROGRESS) {
		httpc_dbg("DNS pending");
	}
	else if (error == ESPCONN_OK) {
		// Already in the local names table (or hostname was an IP address), execute the callback ourselves.
		dns_callback(hostname, &addr, req);
	}
	else {
		if (error == ESPCONN_ARG) {
			httpc_error("DNS arg error %s", hostname);
		}
		else {
			httpc_error("DNS error code %d", error);
		}
		dns_callback(hostname, NULL, req); // Handle all DNS errors the same way.
	}

	return true;
}

void ICACHE_FLASH_ATTR httpclient_args_init(httpclient_args *args)
{
	args->url = NULL;
	args->body = NULL;
	args->method = HTTPD_METHOD_GET;
	args->headers = NULL;
	args->max_response_len = BUFFER_SIZE_MAX;
	args->timeout = HTTP_REQUEST_TIMEOUT_MS;
	args->userArg = NULL;
}


bool ICACHE_FLASH_ATTR
http_post(const char *url, const char *body, void *userArg, httpclient_cb user_callback)
{
	httpclient_args args;
	httpclient_args_init(&args);

	args.url = url;
	args.body = body;
	args.method = HTTPD_METHOD_POST;
	args.userArg = userArg;

	return http_request(&args, user_callback);
}


bool ICACHE_FLASH_ATTR
http_get(const char *url, void *userArg, httpclient_cb user_callback)
{
	httpclient_args args;
	httpclient_args_init(&args);

	args.url = url;
	args.method = HTTPD_METHOD_GET;
	args.userArg = userArg;

	return http_request(&args, user_callback);
}


bool ICACHE_FLASH_ATTR
http_put(const char *url, const char *body, void *userArg, httpclient_cb user_callback)
{
	httpclient_args args;
	httpclient_args_init(&args);

	args.url = url;
	args.body = body;
	args.method = HTTPD_METHOD_PUT;
	args.userArg = userArg;

	return http_request(&args, user_callback);
}


void ICACHE_FLASH_ATTR
http_callback_example(int http_status,
					  const char *response_headers,
					  const char *response_body,
					  size_t body_size,
					  void *userArg)
{
	(void)userArg;
	dbg("Response: code %d", http_status);
	if (http_status != HTTP_STATUS_GENERIC_ERROR) {
		dbg("len(headers) = %d, len(body) = %d", (int) strlen(response_headers), body_size);
		dbg("body: %s<EOF>", response_body); // FIXME: this does not handle binary data.
	}
}


void ICACHE_FLASH_ATTR http_callback_showstatus(int code,
												const char *response_headers,
												const char *response_body,
												size_t body_size,
												void *userArg)
{
	(void) response_body;
	(void) response_headers;
	(void) body_size;
	(void)userArg;

	if (code == 200) {
		info("Response OK (200)");
	}
	else if (code >= 400) {
		error("Response ERROR (%d)", code);
		dbg("Body: %s<EOF>", response_body);
	}
	else {
		// ???
		warn("Response %d", code);
		dbg("Body: %s<EOF>", response_body);
	}
}
