#include "ustack_unprotected.h"

#include <sys/mman.h>
#include <assert.h>

ustack_unprotected::ustack_unprotected(size_t stacksize)
    : top_(NULL)
    , size_(stacksize)
{
    int page_size = getpagesize();

    size_ = (size_ + page_size -1) & ~(page_size - 1);
    top_  = ::mmap(NULL, 
        size_, 
        PROT_READ | PROT_WRITE, 
        MAP_ANONYMOUS | MAP_PRIVATE, 
        -1, 
        0);

    assert(top_ != NULL);
}

ustack_unprotected::~ustack_unprotected() 
{
    ::munmap(top_, size_);

    top_     = NULL;
    size_     = 0;
}

void * ustack_unprotected::top()
{
    return top_;
}

size_t ustack_unprotected::size()
{
    return size_;
}
