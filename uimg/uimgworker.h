/*************************************************************************
    > File Name: uimgworker.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Tue Nov 29 14:26:21 CST 2016
 ************************************************************************/

#ifndef __UIMG_WORKER_H__
#define __UIMG_WORKER_H__

#include <uhttp/uhttp.h>
#include <unetwork/uschedule.h>
#include <unetwork/utcpsocket.h>
#include <utools/umd5sum.h>

#include <map>
#include "uimg.h"
#include "uimgio.h"

class uimgworker
    : public utask
    , public uhttphandler
{
public:
    class umonitor {
    public:
        virtual ~umonitor() {}

        virtual void onworkerevent(int method, const char * uri, 
            int reqsize, int rescode, int ressize) = 0;
        virtual void onworkerfinish(int fd) = 0;
    };

public:
    uimgworker(umonitor & monitor, const uimg_conf & config,
        uschedule & schudule, int fd, const char * ip);

    virtual ~uimgworker();

    virtual void run();

    virtual int onhttp(const uhttprequest & request, uhttpresponse & response);

private:
    typedef int (uimgworker::*handler)(const uhttprequest &, uhttpresponse &);

    // can use lua script replace.
    int echo(const uhttprequest & request, uhttpresponse & response);
    int notfind(const uhttprequest & request, uhttpresponse & response);
    int favicon(const uhttprequest & request, uhttpresponse & response);
    int upload(const uhttprequest & request, uhttpresponse & response);
    int download(const uhttprequest & request, uhttpresponse & response);
    int index(const uhttprequest & request, uhttpresponse & response);

private:
    void add_extheaders(uhttpresponse & response);
    bool check_etag(const uhttprequest & request, uhttpresponse & response,
        const std::string & md5);
    
private:
    umonitor &  monitor_;
    const uimg_conf & config_;
    uschedule & schudule_;
    utcpsocket  socket_;
    uhttp       http_;
    umd5sum     md5_;
    uimgio      io_;
    std::string peerip_;

    struct handler_type {
        const char *    cmd;
        handler         func;
    };

    const static handler_type handlers[];
};

#endif
