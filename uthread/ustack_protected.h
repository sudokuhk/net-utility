#ifndef _USTACK_PROTECTED_H__
#define _USTACK_PROTECTED_H__

#include "ustack.h"

class ustack_protected
    : public ustack
{
public:
    ustack_protected(size_t stacksize);
    
    virtual ~ustack_protected();

    virtual void * top();
    virtual size_t size();

private:
    void * map_addr_;
    size_t map_size_;
    
    void * top_;
    size_t size_;

    int    page_size_;
};

#endif