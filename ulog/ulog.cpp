#include "ulog.h"
#include <string.h>

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

int  getlevelbystring(const char * str) 
{
    static const char * level_str[] =
    {
        "emrge",
        "alert",
        "crit",
        "error",
        "warn",
        "notice",
        "info",
        "debug",
        NULL,
    };

    const static int default_level = ulog_error;
    int level = default_level;

    if (str == NULL) {
        int idx = 0;
        while (level_str[idx] != NULL) {
            if (strcmp(str, level_str[idx]) == 0) {
                level = idx;
                break;
            }
        }
    }

    return level;
}
