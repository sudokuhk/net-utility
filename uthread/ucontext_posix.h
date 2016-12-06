#ifndef _UCONTEXT_POSIX_H__
#define _UCONTEXT_POSIX_H__

#include "utcontext.h"

#include <ucontext.h>
#include <stdint.h>  

class ustack;

class ucontext_posix
    : public utcontext
{
public:
    static utcontext * do_create(size_t stacksize, uthread_func_t func, 
        uthread_arg_t arg, uthread_callback_t cb, bool protect);
    
private:
    ucontext_posix(size_t stacksize, uthread_func_t func, uthread_arg_t arg, 
        uthread_callback_t cb, bool protect);

public:
    virtual ~ucontext_posix();
    
    virtual void make(uthread_func_t func, uthread_arg_t arg);
    virtual bool resume();
    virtual bool yield();

private:
    ucontext_t * main_context();

    static void main_context_destroy(void * ctx);
    static void uthread_wrapper(uint64_t addr);
    
private:
    ucontext_t             ctx_;
    uthread_func_t         func_;
    uthread_arg_t          arg_;
    ustack *             stack_;
    uthread_callback_t     callback_;

    static pthread_key_t key_;
};

#endif
