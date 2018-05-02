#include "settime.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

int rtcSetTime(const struct tm* tm_time)
{
    int rtc_handle = -1;
    int ret = 0;
    struct rtc_time rtc_tm;
    if (tm_time == NULL) {
        return -1;
    }
    rtc_handle = open("/dev/rtc0", O_RDWR, 0);
    if (rtc_handle < 0) {
        //      db_error("open /dev/rtc0 fail");
        return -1;
    }
    memset(&rtc_tm, 0, sizeof(rtc_tm));
    rtc_tm.tm_sec = tm_time->tm_sec;
    rtc_tm.tm_min = tm_time->tm_min;
    rtc_tm.tm_hour = tm_time->tm_hour;
    rtc_tm.tm_mday = tm_time->tm_mday;
    rtc_tm.tm_mon = tm_time->tm_mon;
    rtc_tm.tm_year = tm_time->tm_year;
    rtc_tm.tm_wday = tm_time->tm_wday;
    rtc_tm.tm_yday = tm_time->tm_yday;
    rtc_tm.tm_isdst = tm_time->tm_isdst;
    ret = ioctl(rtc_handle, RTC_SET_TIME, &rtc_tm);
    if (ret < 0) {
        //       db_error("rtcSetTime fail");
        close(rtc_handle);
        return -1;
    }

    //    ALOGV("rtc_set_time ok");
    close(rtc_handle);
    return 0;
}

int setDateTime(struct tm* ptm)
{
    time_t timep;
    struct timeval tv;
    //    open("/dev/rtc0",O_RDWR);
    timep = mktime(ptm);
    tv.tv_sec = timep;
    tv.tv_usec = 0;

    if (settimeofday(&tv, NULL) < 0) {
        //      db_error("Set system date and time error.");
        return -1;
    }

    time_t t = time(NULL);
    struct tm* local = localtime(&t);
    rtcSetTime(local);
    return 0;
}
