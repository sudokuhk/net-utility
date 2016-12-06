#include "ustack.h"
#include "ustack_unprotected.h"
#include "ustack_protected.h"

ustack * ustack::create(size_t stacksize, bool protect)
{
    ustack * s = NULL;

    if (protect) {
        s = new ustack_protected(stacksize);
    } else {
        s = new ustack_unprotected(stacksize);
    }
    
    return s;
}