#include "uthread.h"
#include "utcontext.h"

uthread::uthread(utid_t uid, size_t stacksize, 
    uthread_callback_t cb, bool protect)
    : context_(NULL)
    , status_(UTHREAD_SUSPEND)
    , uid_(uid)
    , init_(false)
{
    context_ = utcontext::create(stacksize, NULL, NULL, cb, protect);
}

uthread::~uthread()
{
    delete context_;
}

void uthread::init(uthread_func_t func, uthread_arg_t arg)
{
    init_   = true;
    status_ = UTHREAD_SUSPEND;
    context_->make(func, arg);
}

void uthread::done()
{
    init_   = false;
    status_ = UTHREAD_DONE;
}

bool uthread::resume()
{
    if (!init_ || status_ == UTHREAD_RUNNING) {
        return false;
    }
    
    status_ = UTHREAD_RUNNING;
    return context_->resume();
}

bool uthread::yield()
{
    if (!init_) {
        return false;
    }
    
    status_ = UTHREAD_SUSPEND;
    return context_->yield();
}

const utid_t uthread::uid() const
{
    return uid_;
}

const uthread::ustatus uthread::status() const
{
    return status_;
}

