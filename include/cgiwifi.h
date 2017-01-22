#ifndef CGIWIFI_H
#define CGIWIFI_H

#include "httpd.h"

httpd_cgi_state cgiWiFiScan(HttpdConnData *connData);
httpd_cgi_state tplWlan(HttpdConnData *connData, char *token, void **arg);
httpd_cgi_state cgiWiFi(HttpdConnData *connData);
httpd_cgi_state cgiWiFiConnect(HttpdConnData *connData);
httpd_cgi_state cgiWiFiSetMode(HttpdConnData *connData);
httpd_cgi_state cgiWiFiSetChannel(HttpdConnData *connData);
httpd_cgi_state cgiWiFiConnStatus(HttpdConnData *connData);

#endif
