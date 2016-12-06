#include "utimer.h"

#include <time.h>       //gettimeofday
#include <sys/time.h>   //clock_gettime, nanosleep
#include <errno.h>
//#include <stdio.h>

const uint64_t utimermgr::get_timestamp()
{
    struct timespec tp;  
    struct timeval tv;
    
    //ERRORS
    //   EFAULT tp points outside the accessible address space.
    //   EINVAL The clk_id specified is not supported on this system.
    //   EPERM  clock_settime() does not have permission to set the clock indicated.
    if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0) {
        return (uint64_t)tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
    } else {
        gettimeofday(&tv, NULL);
        return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec;
    }
}

void utimermgr::sleep(const int ms)
{
    timespec t;
    
    t.tv_sec    = ms / 1000; 
    t.tv_nsec   = (ms % 1000) * 1000000;
    int ret     = 0;
    
    do {
        ret = nanosleep(&t, &t);
    } while (ret == -1 && errno == EINTR); 
}

utimermgr::utimermgr()
    : heap_()
{
}

utimermgr::~utimermgr()
{
    heap_.clear();
}

void utimermgr::add(int timeo, utimer * tobj)
{   
    if (tobj == NULL) {
        return;
    }

    uint64_t now = get_timestamp();
    if (timeo == -1) {
        tobj->abs_time_ = -1LLU;
    } else {
        tobj->abs_time_ = (uint64_t)timeo + now;
    }

    heap_.push_back(tobj);

    heap_up(heap_.size());

    //printf("add, thread:%d, id:%zd, now:%lu, abstime:%lu, obj:%p\n", 
    //    tobj->thread_, tobj->timer_id_, now, tobj->abs_time_, tobj);
}

void utimermgr::remove(utimer * obj)
{
    if (obj == NULL || obj->timer_id_ == 0) {
        //printf("invalid remove timer! obj:%p, thread:%d, timerid:%zd\n", 
        //    obj, obj == NULL ? -1 : obj->thread_, 
        //obj == NULL ? -1 : obj->timer_id_);
        return;
    }
    //printf("remove timer, thread:%d, id:%zd, abstime:%lu\n", 
    //    obj->thread_, obj->timer_id_, obj->abs_time_);
    
    size_t remidx   = obj->timer_id_ - 1;   //heap_ vector idex.
    size_t heapsize = heap_.size();

    if (remidx >= heapsize) {
        return;
    }

    utimer * tobj = heap_[remidx];
    tobj->timer_id_ = 0;
    std::swap(heap_[remidx], heap_[heapsize - 1]);
    heap_.pop_back();

    if (heap_.size() == 0) { //empty heap, no need adjust.
        return;
    }

    //adjust heap.
    if (heap_[remidx]->compare(*tobj) == 0) {
        heap_[remidx]->timer_id_ = remidx + 1;
    } else if (heap_[remidx]->compare(*tobj) < 0) {
        heap_up(remidx + 1);
    } else {
        heap_down(remidx);
    }
}

const bool utimermgr::empty() const
{
    return heap_.empty();
}

const int utimermgr::next_timeo() const
{
    if (empty()) {
        return -1;
    }

    int min_timeo = 0;
    utimer * tobj = heap_[0];
    uint64_t now = get_timestamp();
    //printf("thread:%d, id:%zd, now:%lu, abstime:%lu\n", 
    //    tobj->thread_, tobj->timer_id_, now, tobj->abs_time_);

    if (tobj->abs_time_ > now) {
        min_timeo = tobj->abs_time_ - now;
    }

    return min_timeo;
}

utimer * utimermgr::pop()
{
    if (empty()) {
        return NULL;
    }

    utimer * tobj = heap_[0];
    tobj->timer_id_ = 0;

    std::swap(heap_[0], heap_[heap_.size() - 1]);
    heap_.pop_back();

    if (!empty()) {
        heap_down(0);
    }

    return tobj;
}

void utimermgr::heap_up(size_t end_idx)
{
    size_t now_idx = end_idx - 1;

    utimer * tobj = heap_[now_idx];
    size_t parent_idx = (now_idx - 1) >> 1;

    while (now_idx > 0 && parent_idx >= 0 && tobj->compare(*heap_[parent_idx]) < 0) {
        heap_[now_idx] = heap_[parent_idx];
        heap_[now_idx]->timer_id_ = now_idx + 1;

        now_idx = parent_idx;
        parent_idx = (now_idx - 1) >> 1;
    }

    heap_[now_idx] = tobj;
    heap_[now_idx]->timer_id_ = now_idx + 1;
}

void utimermgr::heap_down(size_t begin_idx)
{
    utimer * tobj = heap_[begin_idx];
    
    size_t now_idx   = begin_idx;
    size_t heapsize  = heap_.size();
    size_t child_idx = (now_idx + 1) << 1;

    while (child_idx <= heapsize) {
        if (child_idx == heapsize 
            || heap_[child_idx - 1]->compare(*heap_[child_idx]) < 0) {
            child_idx --;
        }

        if (tobj->compare(*heap_[child_idx]) <= 0) {
            break;
        }

        heap_[now_idx] = heap_[child_idx];
        heap_[now_idx]->timer_id_ = now_idx + 1;

        now_idx = child_idx;
        child_idx = (now_idx + 1) << 1;
    }

    heap_[now_idx] = tobj;
    heap_[now_idx]->timer_id_ = now_idx + 1;
}


