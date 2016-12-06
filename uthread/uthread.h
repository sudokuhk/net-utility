#ifndef _UTHREAD_H__
#define _UTHREAD_H__

#include "ulist.h"
#include "utcontext.h"
#include <unistd.h>

class utcontext;
typedef int utid_t;

class uthread
    : public ulist
{
public:
    enum ustatus {
        UTHREAD_RUNNING,
        UTHREAD_SUSPEND,
        UTHREAD_DONE
    };
    
public:
    uthread(utid_t uid, size_t stacksize, uthread_callback_t cb, bool protect);

    ~uthread();

    void init(uthread_func_t func, uthread_arg_t arg);

    void done();

    bool resume();
    
    bool yield();

    const utid_t uid() const;

    const ustatus status() const;
    
private:
    utcontext *         context_;
    ustatus             status_;
    utid_t              uid_;
    bool                init_;
};

#endif