#ifndef _USTACK_UNPROTECTED_H__
#define _USTACK_UNPROTECTED_H__

#include "ustack.h"

class ustack_unprotected
    : public ustack
{
public:
    ustack_unprotected(size_t stacksize);
    
    virtual ~ustack_unprotected();

    virtual void * top();
    virtual size_t size();

private:
    void * top_;
    size_t size_;
};

#endif