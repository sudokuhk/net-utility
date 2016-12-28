/*************************************************************************
    > File Name: ustring.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Wed Dec 14 14:49:27 CST 2016
 ************************************************************************/

#include "ustring.h"
#include <string.h>
#include <stdio.h>

void split(const char * str, char sep, std::vector<std::string> & out)
{
    typedef const char * const_char;
    const_char p = str;
    const_char np, pb, pe;
        
    while (*p && (np = strchr(p, sep)) != NULL) {
        pb = p;
        pe = np - 1;

        trim(pb, pe);
        if (pb <= pe) {
            std::string one(pb, pe - pb + 1);
            out.push_back(one);
        }

        p = np + 1;
    }

    if (*p) {
        pb = p;
        pe = p + strlen(p) - 1;
        trim(pb, pe);
        std::string one(pb, pe - pb + 1);
        out.push_back(one);
    }
}

bool iswhitespace(char c)
{
    if (c == ' ' || c == '\0' || c == '\n' || c == '\r') {
        return true;
    }
    return false;
}

void trimleft(const char * & left, const char * & right)
{
    while (left <= right && iswhitespace(*left))    left ++;
}

void trimright(const char * & left, const char * & right)
{
    while (left <= right && iswhitespace(*right))   right --;
}

void trim(const char * & left, const char * & right)
{
    trimleft(left, right);
    trimright(left, right);
}

void set(char * const p, char v)
{
    *p = v;
}

unsigned int str2ip(const char * ip)
{
    if (ip == NULL) {
        return 0;
    }
    
    unsigned int num[4];
    unsigned int ret = 0;
    int cnt = sscanf(ip, "%d.%d.%d.%d", &num[0], &num[1], &num[2], &num[3]);

    if (cnt == 4) {
        ret = ret | (num[0] << 24) | (num[1] << 16) | (num[2] << 8) | (num[3]);
    }

    return ret;
}

std::string ip2str(unsigned int ip)
{
    char buf[64];
    snprintf(buf, 64, "%d.%d.%d.%d",
        (ip >> 24) & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >> 8) & 0xFF,
        (ip >> 0) & 0xFF);

    return buf;
}

