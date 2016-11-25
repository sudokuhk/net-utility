#include "ulog.h"

static void default_log(int, const char *, va_list)
{
}

static ulog_t glog = default_log;

void ulog(int priority, const char * fmt, ...)  
{
    va_list args;
    va_start(args, fmt);
    glog(priority, fmt, args);
    va_end(args);
}

void setulog(ulog_t log)
{
    glog = log;
}

