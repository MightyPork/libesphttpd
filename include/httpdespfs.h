#ifndef HTTPDESPFS_H
#define HTTPDESPFS_H

#include "httpd.h"

httpd_cgi_state cgiEspFsHook(HttpdConnData *connData);
httpd_cgi_state cgiEspFsTemplate(HttpdConnData *connData);

#endif