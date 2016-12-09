#ifndef _UHTTP_H__
#define _UHTTP_H__

#include "uhttpdefs.h"
#include "uhttpmessage.h"
#include "uhttprequest.h"
#include "uhttpresponse.h"
#include "uboundaryparser.h"

#include <iostream>

class uhttphandler
{
public:
    virtual ~uhttphandler() {}

    //handle request, and fill response
    virtual int onhttp(const uhttprequest & request,
        uhttpresponse & response) = 0;

    virtual int onerror(int errcode, uhttpresponse & response) = 0;
};

class uhttp
{
public:
    static const char * get_reasonphrase(int statuscode) ;
    static const char * get_version(int version);
    static void set_max_content_limit(size_t limit);
    static const char * get_methodname(int method);

public:
    enum en_uhttp_errcode {
        en_succeed                  = 0,
            
        en_socket_reset             = 1,
        
        en_recv_req_start_error,
        en_recv_req_header_error,
        en_recv_req_body_error,
        
        en_send_req_start_error,
        en_send_req_header_error,
        en_send_req_body_error,

        en_recv_res_start_error,
        en_recv_res_header_error,
        en_recv_res_body_error,

        en_send_res_start_error,
        en_send_res_header_error,
        en_send_res_body_error,

        en_recv_req_decomp_error,
        en_send_req_comp_error,
        en_recv_res_decomp_error,
        en_send_res_comp_error,
    };
    
public:
    uhttp(std::iostream & stream, uhttphandler & handler, bool gzip = true);

    virtual ~uhttp();

    void run();

public:
    int send_request(uhttprequest & request);
    
    int recv_request(uhttprequest & request);
    
    int send_response(uhttpresponse & response);

    int recv_response(uhttpresponse & response);

private:
    inline bool iswhitespace(char c);
    inline void trimleft(char * & left, char * & right);
    inline void trimright(char * & left, char * & right);
    inline void trim(char * & left, char * & right);
    inline void set(char * const p, char v);
    inline char* get(char *begin, char *end, char sep, char *& a1, char *&a2);
    
    bool recv_start(uhttprequest & request);

    bool recv_start(uhttpresponse& response);
    
    bool recv_header(uhttpmessage & msg);

    int recv_body(uhttpmessage & msg);

    int analyse_method(const char * method);

    int analyse_version(const char * version);

    bool recv_start(std::string & first, std::string & second, 
        std::string & third, bool stringsplit = false);

    bool compress(std::string & content, int type = 0);
    
private:
    std::iostream & stream_;
    uhttphandler &  handler_;
    
    int             linesize_;
    char * const    line_;
    
    bool            gzip_;
    int             compress_buf_size_;
    char * const    compress_buf_;
    int             decompress_buf_size_;
    char * const    decompress_buf_;

    static size_t   max_content_limit_;
};

#endif
