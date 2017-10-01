//
// Created by MightyPork on 2017/10/01.
//

#ifndef ESP_UTILS_H
#define ESP_UTILS_H

#include <esp8266.h>

static inline int esp_isupper(char c)
{
	return (c >= 'A' && c <= 'Z');
}

static inline int esp_isalpha(char c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}


static inline int esp_isspace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static inline int esp_isdigit(char c)
{
	return (c >= '0' && c <= '9');
}

char * ICACHE_FLASH_ATTR esp_strdup(const char *str);

long ICACHE_FLASH_ATTR esp_strtol(const char *nptr, char **endptr, int base);

#endif //ESP_UTILS_H
