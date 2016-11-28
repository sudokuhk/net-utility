/*************************************************************************
    > File Name: uconfig_default.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Nov 28 15:09:30 CST 2016
 ************************************************************************/

#ifndef __U_CONFIG_DEFAULT_H__
#define __U_CONFIG_DEFAULT_H__

#include "uconfig.h"

class uconfig_default
    : public uconfig
{
public:
    uconfig_default();

    virtual ~uconfig_default();

    virtual bool load(const char * filename);

private:
    bool istrim(char c);

    void skip_blank(char *& begin, char *& end);
};

#endif 