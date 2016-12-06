#ifndef _URUNTIME_H__
#define _URUNTIME_H__

#include "uthread.h"
#include "utcontext.h"

#include <vector>

class uruntime
    : public uthread_callback
{
public:
    uruntime(int stacksize = 4096, bool protect_stack = false);

    virtual ~uruntime();

    int new_thread(uthread_func_t func, uthread_arg_t arg);

    bool resume(utid_t uid);

    bool yield();

    utid_t current();

    bool finished();

    virtual void done();

private:
    typedef std::vector<uthread *>         uthread_container;
    typedef uthread_container::iterator uthread_iterator;
    
    uthread_container uthreads_;

    size_t  stack_size_;
    utid_t  current_uid_;
    int     next_valid_thread_;
    int     unfinished_thread_;
    bool    stack_protect_;
};

#endif