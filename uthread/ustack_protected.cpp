#include "ustack_protected.h"

#include <sys/mman.h>
#include <assert.h>

ustack_protected::ustack_protected(size_t stacksize)
    : map_addr_(NULL)
    , map_size_(0)
    , top_(NULL)
    , size_(stacksize)
    , page_size_(4096)
{
    page_size_     = getpagesize();

    size_         = (size_ + page_size_ -1) & ~(page_size_ - 1);
    map_size_    = size_ + page_size_ + page_size_;
    
    map_addr_      = ::mmap(NULL, 
        map_size_, 
        PROT_READ | PROT_WRITE, 
        MAP_ANONYMOUS | MAP_PRIVATE, 
        -1, 
        0);

    assert(map_addr_ != NULL);

    mprotect(map_addr_, page_size_, PROT_NONE);
    mprotect((void *)((char *)map_addr_ + size_ + page_size_), 
        page_size_, PROT_NONE);

    top_     = (void *)((char *)map_addr_ + page_size_);
}

ustack_protected::~ustack_protected() 
{
    mprotect(map_addr_, page_size_, PROT_READ | PROT_WRITE);
    mprotect((void *)((char *)map_addr_ + size_ + page_size_), 
            page_size_, PROT_READ | PROT_WRITE);
    ::munmap(map_addr_, map_size_);

    top_     = NULL;
    size_     = 0;
}

void * ustack_protected::top()
{
    return top_;
}

size_t ustack_protected::size()
{
    return size_;
}
