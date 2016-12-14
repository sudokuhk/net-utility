/*************************************************************************
    > File Name: ustring.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Wed Dec 14 14:49:27 CST 2016
 ************************************************************************/

#include "ustring.h"
#include <string.h>

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

