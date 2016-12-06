#include "uruntime.h"
#include "ucontext_posix.h"

//#include <stdio.h>

uruntime::uruntime(int stacksize/* = 4096*/, bool protect_stack/* = false*/)
    : uthreads_()
    , stack_size_(stacksize)
    , current_uid_(list_end)
    , next_valid_thread_(list_end)
    , unfinished_thread_(0)
    , stack_protect_(protect_stack)
{
    utcontext::set_createf(ucontext_posix::do_create);
}

uruntime::~uruntime()
{
    uthread_iterator it = uthreads_.begin();
    while (it != uthreads_.end()) {
        delete *it;
        ++ it;
    }
    uthreads_.clear();
}

int uruntime::new_thread(uthread_func_t func, uthread_arg_t arg)
{
    if (func == NULL) {
        return -1;
    }

    uthread * t = NULL;
    
    if (next_valid_thread_ == list_end) {
        utid_t id = uthreads_.size();
        t = new uthread(id, stack_size_, this, stack_protect_);
        uthreads_.push_back(t);
    } else {
        t = uthreads_[next_valid_thread_];
        next_valid_thread_ = t->next();
    }

    //printf("runtime, thread:%d, next:%d, %p, new thread, func:%p, arg:%p\n", 
    //    t->uid(), next_valid_thread_, t, func, arg);
    t->init(func, arg);
    unfinished_thread_ ++;

    return t->uid();
}

bool uruntime::resume(utid_t uid)
{
    if (uid > (ssize_t)uthreads_.size()) {
        return false;
    }

    uthread * t  = uthreads_[uid];
    current_uid_ = uid;
    
    return t->resume();
}

bool uruntime::yield()
{
    if (current_uid_ != -1) {
        uthread * t  = uthreads_[current_uid_];
        current_uid_ = -1;
        
        return t->yield();
    }

    return false;
}

utid_t uruntime::current()
{
    return current_uid_;
}

bool uruntime::finished()
{
    return unfinished_thread_ == 0;
}

void uruntime::done()
{
    if (current_uid_ != list_end) {
        uthread * t = uthreads_[current_uid_];
        current_uid_ = list_end;
        //printf("runtime, thread:%d, next:%d, %p, done!\n", 
        //    t->uid(), next_valid_thread_, t);

        t->done();
        t->set_next(next_valid_thread_);
        next_valid_thread_ = t->uid();

        unfinished_thread_ --;
    }
}