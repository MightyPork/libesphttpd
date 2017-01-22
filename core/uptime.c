#include <esp8266.h>

volatile uint32_t uptime = 0;

static ETSTimer prUptimeTimer;

static void uptimeTimerCb(void *arg)
{
	uptime++;
}

void uptime_timer_init(void)
{
	os_timer_disarm(&prUptimeTimer);
	os_timer_setfn(&prUptimeTimer, uptimeTimerCb, NULL);
	os_timer_arm(&prUptimeTimer, 1000, 1);
}

void uptime_str(char *buf)
{
	u32 a = uptime;
	u32 days = a / 86400;
	a -= days * 86400;

	u32 hours = a / 3600;
	a -= hours * 3600;

	u32 mins = a / 60;
	a -= mins * 60;

	u32 secs = a;

	if (days > 0) {
		sprintf(buf, "%ud %02u:%02u:%02u", days, hours, mins, secs);
	} else {
		sprintf(buf, "%02u:%02u:%02u", hours, mins, secs);
	}
}

void uptime_print(void)
{
	u32 a = uptime;
	u32 days = a / 86400;
	a -= days * 86400;

	u32 hours = a / 3600;
	a -= hours * 3600;

	u32 mins = a / 60;
	a -= mins * 60;

	u32 secs = a;

	if (days > 0) {
		printf("%ud %02u:%02u:%02u", days, hours, mins, secs);
	} else {
		printf("%02u:%02u:%02u", hours, mins, secs);
	}
}
