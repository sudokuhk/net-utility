/*************************************************************************
    > File Name: uimgserverunit.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Nov 28 13:54:43 CST 2016
 ************************************************************************/

#ifndef __UIMG_SERVER_UNIT_H__
#define __UIMG_SERVER_UNIT_H__

#include <pthread.h>
#include <unetwork/uschedule.h>
#include "uimg.h"
#include "uimgworker.h"

class uimgserverunit
    : public uimgworker::umonitor
{    
public:
    static void * threadf(void * args);

private:
    class uimglock : public ulock
    {
    public:
        uimglock()              {   pthread_mutex_init(&mutex_, NULL);  }
        virtual ~uimglock()     {   pthread_mutex_destroy(&mutex_);     }
        virtual void lock()     {   pthread_mutex_lock(&mutex_);        }
        virtual void unlock()   {   pthread_mutex_unlock(&mutex_);      }
    private:
        pthread_mutex_t mutex_;
    };
    
public:
    uimgserverunit(const uimg_conf & config);

    ~uimgserverunit();

    void start();

    void stop();

    bool accept(int fd, const char * ip);

public:
    virtual void onworkerevent(int method, const char * uri, 
            int reqsize, int rescode, int ressize);
    virtual void onworkerfinish(int fd);
    
private:
    void run();

private:
    const uimg_conf & config_;

    uimglock    lock_;
    uschedule   schedule_;
    bool        running_;
    pthread_t   thread_;
};

#endif
