#ifndef CGIFLASH_H
#define CGIFLASH_H

#include "httpd.h"

#define CGIFLASH_TYPE_FW 0
#define CGIFLASH_TYPE_ESPFS 1

typedef struct {
	int type;
	int fw1Pos;
	int fw2Pos;
	int fwSize;
	char *tagName;
} CgiUploadFlashDef;

httpd_cgi_state cgiGetFirmwareNext(HttpdConnData *connData);
httpd_cgi_state cgiUploadFirmware(HttpdConnData *connData);
httpd_cgi_state cgiRebootFirmware(HttpdConnData *connData);

#endif
