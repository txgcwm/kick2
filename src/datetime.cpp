
#include <time.h>
#include <stdio.h>

#include "datetime.h"

std::string DateTime::Now()
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    char str_time[21] = { 0 };
    strftime(str_time, 21, "%Y-%m-%d %H:%M:%S", tm_now);
    return std::string(str_time);
}

std::string DateTime::NowMs()
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    struct timespec xtime;
    clock_gettime(CLOCK_REALTIME, &xtime);

    char buf[22] = { 0 };
    snprintf(buf, sizeof(buf), "%02d/%02d/%02d %02d:%02d:%02d.%03d",
        tm_now->tm_mon + 1, tm_now->tm_mday, tm_now->tm_year % 100,
        tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec,
        (int)xtime.tv_nsec / 1000000);
        
    return std::string(buf);
}

uint64_t DateTime::UnixTime()
{
#ifdef WIN32
    SYSTEMTIME	st;
    _int64		ft;
    ::GetSystemTime(&st);
    ::SystemTimeToFileTime(&st, (FILETIME *)&ft);
    return	(long)((ft - UNIXTIME_BASE) / 10000000);
#else
    return time(NULL);
#endif
}

uint64_t DateTime::UnixTimeMs()
{
#ifdef WIN32
    SYSTEMTIME	st;
    _int64		ft;
    ::GetSystemTime(&st);
    ::SystemTimeToFileTime(&st, (FILETIME *)&ft);
    return ((ft - UNIXTIME_BASE) / 10000);
#else
    struct timespec xtime;
    clock_gettime(CLOCK_REALTIME, &xtime);
    return (xtime.tv_nsec) / 1000000 + xtime.tv_sec * 1000;
#endif
}

uint32_t DateTime::DayOfWeek()
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    return tm_now->tm_wday;
}

uint32_t DateTime::DayOfYear()
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    return tm_now->tm_yday;
}
