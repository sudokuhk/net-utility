/*************************************************************************
    > File Name: uimgserverunit.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Nov 28 13:55:12 CST 2016
 ************************************************************************/

#include "uimgserverunit.h"
#include "uimgworker.h"

void * uimgserverunit::threadf(void * args)
{
    uimgserverunit * pobj = (uimgserverunit *)args;
    pobj->run();

    return NULL;
}

uimgserverunit::uimgserverunit(const uimg_conf & config)
    : config_(config)
    , lock_()
    , schedule_(8 * 1024, 100000, false, &lock_)
    , running_(false)
    , thread_(-1)
{
}

uimgserverunit::~uimgserverunit()
{
}

void uimgserverunit::start()
{
    pthread_create(&thread_, NULL, uimgserverunit::threadf, this);
}

void uimgserverunit::stop()
{
    schedule_.stop();
}

bool uimgserverunit::accept(int fd, const char * ip)
{
    uimgworker * work = new uimgworker(*this, config_, schedule_, fd, ip);
    schedule_.add_task(work);
    
    return true;
}

void uimgserverunit::run()
{
    schedule_.run();
}

void uimgserverunit::onworkerevent(int method, const char * uri, 
    int reqsize, int rescode, int ressize)
{
}

void uimgserverunit::onworkerfinish(int fd)
{
}


