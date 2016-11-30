#ifndef _UHTTP_H__
#define _UHTTP_H__

#include "uhttpdefs.h"
#include "uhttpmessage.h"
#include "uhttprequest.h"
#include "uhttpresponse.h"

#include <iostream>

class uhttphandler
{
public:
    virtual ~uhttphandler() {}

    //handle request, and fill response
    virtual int onhttp(const uhttprequest & request,
        uhttpresponse & response) = 0;
};

class uhttp
{
public:
    static const char * get_reasonphrase(int statuscode) ;
    static const char * get_version(int version);
    static void set_max_content_limit(size_t limit);
    
public:
    uhttp(std::iostream & stream, uhttphandler & handler, bool gzip = true);

    virtual ~uhttp();

    void run();

public:
    bool send_request(uhttprequest & request);
    
    bool recv_request(uhttprequest & request);
    
    bool send_response(uhttpresponse & response);

    bool recv_response(uhttpresponse & response);

private:
    inline bool istrim(char c);

    bool recv_start(uhttprequest & request);

    bool recv_start(uhttpresponse& response);
    
    bool recv_header(uhttpmessage & msg);

    bool recv_body(uhttpmessage & msg);

    int analyse_method(const char * method);

    int analyse_version(const char * version);

    bool recv_start(std::string & first, std::string & second, 
        std::string & third, bool stringsplit = false);

    bool compress(std::string & content, int type = 0);
    
private:
    std::iostream & stream_;
    uhttphandler & handler_;

    char *  line_;
    int     linesize_;

    int     compress_buf_size_;
    char *  compress_buf_;

    int     decompress_buf_size_;
    char *  decompress_buf_;

    bool    gzip_;

    static size_t max_content_limit_;
};

#endif
