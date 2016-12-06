#include "utcontext.h"

utcontext::utcontext()
{
}

utcontext::~utcontext()
{
}

utcontext * utcontext::create(size_t stacksize, uthread_func_t func, 
    uthread_arg_t arg, uthread_callback_t cb, bool protect)
{
    utcontext * ctx = NULL;

    if (create_func_ != NULL) {
        ctx = create_func_(stacksize, func, arg, cb, protect);
    }

    return ctx;
}

void utcontext::set_createf(ucontext_create_func_t createf)
{
    create_func_ = createf;
}

ucontext_create_func_t utcontext::get()
{
    return create_func_;
}

ucontext_create_func_t utcontext::create_func_ = NULL;