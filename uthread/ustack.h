#ifndef _USTACK_H__
#define _USTACK_H__

#include <unistd.h>

class ustack
{
public:
    virtual ~ustack() {}

    virtual void * top()     = 0;
    virtual size_t size()     = 0;

public:
    static ustack * create(size_t stacksize, bool protect);
};

#endif