#ifndef _UCONTEXT_H__
#define _UCONTEXT_H__

#include <unistd.h>

class utcontext;

class uthread_callback
{
public:
    virtual ~uthread_callback() {}

    virtual void done() = 0;
};

typedef void * uthread_arg_t;
typedef void (*uthread_func_t)(void *);

#ifdef CALLBACK_FUNC
typedef void (*uthread_callback_t)(void);
#else
typedef uthread_callback * uthread_callback_t;
#endif

typedef utcontext * (*ucontext_create_func_t)(size_t, 
    uthread_func_t, uthread_arg_t, uthread_callback_t, bool);

class utcontext
{
public:
    utcontext();

    virtual ~utcontext();
    
    virtual void make(uthread_func_t, uthread_arg_t) = 0;
    virtual bool resume()     = 0;
    virtual bool yield()     = 0;

public:
    static utcontext * create(size_t, 
        uthread_func_t, uthread_arg_t, uthread_callback_t, bool);

    static void set_createf(ucontext_create_func_t);
    static ucontext_create_func_t get();
    
private:
    static ucontext_create_func_t create_func_;
};

#endif