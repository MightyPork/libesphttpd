#ifndef HTTPD_H
#define HTTPD_H

// we must not use this macro outside the library, as the git hash is not defined there
#define HTTPDVER "0.4+MightyPork/libesphttpd#" GIT_HASH

#ifndef SERVERNAME_PREFIX
#define SERVERNAME_PREFIX "esp8266-httpd "
#endif

//Max length of request head. This is statically allocated for each connection.
#ifndef HTTPD_MAX_HEAD_LEN
#define HTTPD_MAX_HEAD_LEN		1024
#endif

//Max post buffer len. This is dynamically malloc'ed if needed.
#ifndef HTTPD_MAX_POST_LEN
#define HTTPD_MAX_POST_LEN		2048
#endif

//Max send buffer len. This is allocated on the stack.
#ifndef HTTPD_MAX_SENDBUFF_LEN
#define HTTPD_MAX_SENDBUFF_LEN	2048
#endif

//If some data can't be sent because the underlaying socket doesn't accept the data (like the nonos
//layer is prone to do), we put it in a backlog that is dynamically malloc'ed. This defines the max
//size of the backlog.
#ifndef HTTPD_MAX_BACKLOG_SIZE
#define HTTPD_MAX_BACKLOG_SIZE	(4*1024)
#endif

//Max len of CORS token. This is allocated in each connection
#ifndef HTTPD_MAX_CORS_TOKEN_LEN
#define HTTPD_MAX_CORS_TOKEN_LEN      256
#endif

/**
 * CGI handler state / return value
 */
typedef enum {
	HTTPD_CGI_MORE = 0,
	HTTPD_CGI_DONE = 1,
	HTTPD_CGI_NOTFOUND = 2,
	HTTPD_CGI_AUTHENTICATED = 3,
} httpd_cgi_state;

/**
 * HTTP method (verb) used for the request
 */
typedef enum {
	HTTPD_METHOD_GET = 1,
	HTTPD_METHOD_POST = 2,
	HTTPD_METHOD_OPTIONS = 3,
	HTTPD_METHOD_PUT = 4,
	HTTPD_METHOD_DELETE = 5,
	HTTPD_METHOD_PATCH = 6,
	HTTPD_METHOD_HEAD = 7,
} httpd_method;

/**
 * Transfer mode
 */
typedef enum {
	HTTPD_TRANSFER_CLOSE = 0,
	HTTPD_TRANSFER_CHUNKED = 1,
	HTTPD_TRANSFER_NONE = 2,
} httpd_transfer_opt;

typedef struct HttpdPriv HttpdPriv;
typedef struct HttpdConnData HttpdConnData;
typedef struct HttpdPostData HttpdPostData;

typedef httpd_cgi_state (* cgiSendCallback)(HttpdConnData *connData);
typedef httpd_cgi_state (* cgiRecvHandler)(HttpdConnData *connData, char *data, int len);

//A struct describing a http connection. This gets passed to cgi functions.
struct HttpdConnData {
	ConnTypePtr conn;		// The TCP connection. Exact type depends on the platform.
	httpd_method requestType; // One of the HTTPD_METHOD_* values
	char *url;				// The URL requested, without hostname or GET arguments
	char *getArgs;			// The GET arguments for this request, if any.
	const void *cgiArg;		// Argument to the CGI function, as stated as the 3rd argument of
							// the builtInUrls entry that referred to the CGI function.
	const void *cgiArg2;	// 4th argument of the builtInUrls entries, used to pass template file to the tpl handler.
	void *cgiData;			// Opaque data pointer for the CGI function
	char *hostName;			// Host name field of request
	HttpdPriv *priv;		// Opaque pointer to data for internal httpd housekeeping
	cgiSendCallback cgi;	// CGI function pointer
	cgiRecvHandler recvHdl;	// Handler for data received after headers, if any
	HttpdPostData *post;	// POST data structure
	int remote_port;		// Remote TCP port
	uint8 remote_ip[4];		// IP address of client
	uint8 slot;				// Slot ID
};

//A struct describing the POST data sent inside the http connection.  This is used by the CGI functions
struct HttpdPostData {
	int len;				// POST Content-Length
	int buffSize;			// The maximum length of the post buffer
	int buffLen;			// The amount of bytes in the current post buffer
	int received;			// The total amount of bytes received so far
	char *buff;				// Actual POST data buffer
	char *multipartBoundary; //Text of the multipart boundary, if any
};

//A struct describing an url. This is the main struct that's used to send different URL requests to
//different routines.
typedef struct {
	const char *url;
	cgiSendCallback cgiCb;
	const void *cgiArg;
	const void *cgiArg2;
} HttpdBuiltInUrl;

// macros for defining HttpdBuiltInUrl's

/** Route with a CGI handler and two arguments */
#define ROUTE_CGI_ARG2(path, handler, arg1, arg2)  {(path), (handler), (void *)(arg1), (void *)(arg2)}

/** Route with a CGI handler and one arguments */
#define ROUTE_CGI_ARG(path, handler, arg1)         ROUTE_CGI_ARG2((path), (handler), (arg1), NULL)

/** Route with an argument-less CGI handler */
#define ROUTE_CGI(path, handler)                   ROUTE_CGI_ARG2((path), (handler), NULL, NULL)

/** Static file route (file loaded from espfs) */
#define ROUTE_FILE(path, filepath)                 ROUTE_CGI_ARG((path), cgiEspFsHook, (const char*)(filepath))

/** Static file as a template with a replacer function */
#define ROUTE_TPL(path, replacer)                  ROUTE_CGI_ARG((path), cgiEspFsTemplate, (TplCallback)(replacer))

/** Static file as a template with a replacer function, taking additional argument connData->cgiArg2 */
#define ROUTE_TPL_FILE(path, replacer, filepath)   ROUTE_CGI_ARG2((path), cgiEspFsTemplate, (TplCallback)(replacer), (filepath))

/** Redirect to some URL */
#define ROUTE_REDIRECT(path, target)               ROUTE_CGI_ARG((path), cgiRedirect, (const char*)(target))

/** Following routes are basic-auth protected */
#define ROUTE_AUTH(path, passwdFunc)               ROUTE_CGI_ARG((path), authBasic, (AuthGetUserPw)(passwdFunc))

/** Websocket endpoint */
#define ROUTE_WS(path, callback)                   ROUTE_CGI_ARG((path), cgiWebsocket, (WsConnectedCb)(callback))

/** Catch-all filesystem route */
#define ROUTE_FILESYSTEM()                         ROUTE_CGI("*", cgiEspFsHook)

#define ROUTE_END() {NULL, NULL, NULL, NULL}

const char *httpdGetVersion(void);

httpd_cgi_state cgiRedirect(HttpdConnData *connData);
httpd_cgi_state cgiRedirectToHostname(HttpdConnData *connData);
httpd_cgi_state cgiRedirectApClientToHostname(HttpdConnData *connData);

void httpdRedirect(HttpdConnData *conn, char *newUrl);
int httpdUrlDecode(char *val, int valLen, char *ret, int retLen);
int httpdFindArg(char *line, char *arg, char *buff, int buffLen);
void httpdInit(HttpdBuiltInUrl *fixedUrls, int port);
const char *httpdGetMimetype(const char *url);
const char *httpdMethodName(httpd_method m);
void httdSetTransferMode(HttpdConnData *conn, int mode);
void httpdStartResponse(HttpdConnData *conn, int code);
void httpdHeader(HttpdConnData *conn, const char *field, const char *val);
void httpdEndHeaders(HttpdConnData *conn);
int httpdGetHeader(HttpdConnData *conn, char *header, char *ret, int retLen);
int httpdSend(HttpdConnData *conn, const char *data, int len);
int httpdSend_js(HttpdConnData *conn, const char *data, int len);
int httpdSend_html(HttpdConnData *conn, const char *data, int len);
void httpdFlushSendBuffer(HttpdConnData *conn);
void httpdContinue(HttpdConnData *conn);
void httpdConnSendStart(HttpdConnData *conn);
void httpdConnSendFinish(HttpdConnData *conn);
void httpdAddCacheHeaders(HttpdConnData *connData, const char *mime);

//Platform dependent code should call these.
void httpdSentCb(ConnTypePtr conn, char *remIp, int remPort);
void httpdRecvCb(ConnTypePtr conn, char *remIp, int remPort, char *data, unsigned short len);
void httpdDisconCb(ConnTypePtr conn, char *remIp, int remPort);
int httpdConnectCb(ConnTypePtr conn, char *remIp, int remPort);


#endif
