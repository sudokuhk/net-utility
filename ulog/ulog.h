#ifndef _ULOG_H__
#define _ULOG_H__

#include <stdarg.h>

enum
{
    ulog_emrge = 0,
    ulog_alert,
    ulog_crit,
    ulog_error,
    ulog_warn,
    ulog_notice,
    ulog_info,
    ulog_debug,
};

void ulog(int priority, const char * fmt, ...)  
    __attribute__((format(printf, 2, 3)));

typedef void (*ulog_t)(int, const char *, va_list);

void setulog(ulog_t log);

int  getlevelbystring(const char * str);

const char * getstringbylevel(int level);

#endif
