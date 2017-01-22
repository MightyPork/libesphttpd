#ifndef HTTPDESPFS_H
#define HTTPDESPFS_H

#include "httpd.h"

/**
 * The template substitution callback.
 * Returns CGI_MORE if more should be sent within the token, CGI_DONE otherwise.
 */
typedef httpd_cgi_state (* TplCallback)(HttpdConnData *connData, char *token, void **arg);

httpd_cgi_state cgiEspFsHook(HttpdConnData *connData);
httpd_cgi_state cgiEspFsTemplate(HttpdConnData *connData);

#endif