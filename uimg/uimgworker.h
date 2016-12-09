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

    virtual int onhttp(const uhttprequest & request, 
        uhttpresponse & response, int errcode);
    
private:
    typedef int (uimgworker::*handler)(const uhttprequest &, uhttpresponse &, int);

    // can use lua script replace.
    int echo(const uhttprequest & request, uhttpresponse & response, int errcode);
    int notfind(const uhttprequest & request, uhttpresponse & response, int errcode);
    int favicon(const uhttprequest & request, uhttpresponse & response, int errcode);
    int upload(const uhttprequest & request, uhttpresponse & response, int errcode);
    int download(const uhttprequest & request, uhttpresponse & response, int errcode);
    int index(const uhttprequest & request, uhttpresponse & response, int errcode);

private:
    void add_extheaders(uhttpresponse & response);
    
    bool check_etag(const uhttprequest & request, uhttpresponse & response,
        const std::string & md5);
    
    std::string generate_json(int ret, const std::string & md5, int upsize);

private:
    enum en_post_result
    {
        en_post_succeed = 0,
        en_post_internal_error,
        en_post_file_type_not_support,
        en_post_req_method_error,
        en_post_access_refused,
        en_post_body_error,
        en_post_content_length_error,
        en_post_content_type_error,
        en_post_file_too_big,
        en_post_url_illegal,
        en_post_image_not_existed,
        en_post_max,
    };
    
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
