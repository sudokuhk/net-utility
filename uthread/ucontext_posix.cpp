#include "ucontext_posix.h"
#include "ustack_protected.h"
#include "ustack_unprotected.h"

#include <pthread.h>
#include <stdlib.h>

ucontext_posix::ucontext_posix(size_t stacksize, uthread_func_t func, 
    uthread_arg_t arg, uthread_callback_t cb, bool protect)
    : utcontext()
    , stack_(ustack::create(stacksize, protect))
    , callback_(cb) 
{   
    if (func != NULL) {
        make(func, arg);
    }
}

ucontext_posix::~ucontext_posix()
{
    delete stack_;
}

void ucontext_posix::make(uthread_func_t func, uthread_arg_t arg)
{
    func_     = func;
    arg_    = arg;

    getcontext(&ctx_);
    ctx_.uc_stack.ss_sp     = stack_->top();
    ctx_.uc_stack.ss_size   = stack_->size();
    ctx_.uc_stack.ss_flags  = 0;
    ctx_.uc_link            = main_context();

    uint64_t addr = (uint64_t)this;
    makecontext(&ctx_, (void (*)(void))ucontext_posix::uthread_wrapper, 
            1, addr);
}

bool ucontext_posix::resume()
{
    swapcontext(main_context(), &ctx_);
    return true;
}

bool ucontext_posix::yield()
{
    swapcontext(&ctx_, main_context());
    return true;
}

ucontext_t * ucontext_posix::main_context()
{
    void * pctx = pthread_getspecific(key_); 
    if (pctx == NULL) {
        pctx = malloc(sizeof(ucontext_t));
        pthread_setspecific(key_, pctx);
    }
    return (ucontext_t *)pctx;
}

utcontext * ucontext_posix::do_create(size_t stacksize, uthread_func_t func, 
        uthread_arg_t arg, uthread_callback_t cb, bool protect)
{    
    static bool bkey = true;
    if (bkey) {
        pthread_key_create(&key_, ucontext_posix::main_context_destroy); 
        bkey     = false;
    }

    return new ucontext_posix(stacksize, func, arg, cb, protect);
}

pthread_key_t ucontext_posix::key_;

void ucontext_posix::main_context_destroy(void * ctx)
{
    if (ctx != NULL) {
        free(ctx);
        pthread_setspecific(key_, NULL);
    }
}

void ucontext_posix::uthread_wrapper(uint64_t addr)
{    
    ucontext_posix * uctx = (ucontext_posix *)addr;

    uctx->func_(uctx->arg_);
    if (uctx->callback_ != NULL) {
        #ifdef CALLBACK_FUNC
        uctx->callback_();
        #else
        uctx->callback_->done();
        #endif
    }
}