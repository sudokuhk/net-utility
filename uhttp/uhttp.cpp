#include "uhttp.h"
#include "uhttpmessage.h"
#include "uhttprequest.h"
#include "uhttpresponse.h"
#include <ulog/ulog.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "zlib.h"

#define HTTP_RECV_MAXSIZE 8192

const static char * smethods[] = {
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "MKCOL",
    "COPY",
    "MOVE",
    "OPTIONS",
    "PROPFIND",
    "PROPPATCH",
    "LOCK",
    "UNLOCK",
    "TRACE",
    "CONNECT",
    NULL,
};

const static char * sversions[] = {
    "HTTP/1.0",
    "HTTP/1.1",
    NULL,
};

const static struct sreasonphrase_t
{
    int status_code;
    const char * reason_phrase;
} sreasonphrase[] = {
    //Informational 1xx
    {uhttp_status_continue, "Continue"}, 
    {uhttp_status_switch_protocol, "Switching Protocols"},
    {uhttp_status_processing, "Processing"},
    
    //Successful 2xx
    {uhttp_status_ok, "OK"},
    {uhttp_status_created, "Created"},
    {uhttp_status_accepted, "Accepted"},
    {uhttp_status_no_author_info, "Non-Authoritative Information"},
    {uhttp_status_no_content, "No Content"},
    {uhttp_status_reset_content, "Reset Content"},
    {uhttp_status_partial_content, "Partial Content"},
    {uhttp_status_multi_status, "Multi-Status"},
    {uhttp_status_already_reported, "Already Reported"},
    {uhttp_status_im_used, "IM Used"},
    
    //Redirection 3xx  
    {uhttp_status_multiple_choices, "Multiple Choices"},
    {uhttp_status_moved_permanently, "Moved Permanently"},
    {uhttp_status_found, "Found"},
    {uhttp_status_see_other, "See Other"},
    {uhttp_status_not_modified, "Not Modified"},        
    {uhttp_status_use_proxy, "Use Proxy"},
    //306 unused.   Switch Proxy
    {uhttp_status_temporary_redirect, "Temporary Redirect"},
    {uhttp_status_permanent_redirect, "Permanent Redirect"}, 
    
    //Client Error 4xx
    {uhttp_status_bad_request, "Bad Request"},
    {uhttp_status_unauthorized, "Unauthorized"},
    {uhttp_status_payment_required, "Payment Required"},
    {uhttp_status_forbidden, "Forbidden"},
    {uhttp_status_not_found, "Not Found"},
    {uhttp_status_method_not_allowed, "Method Not Allowed"},
    {uhttp_status_not_acceptable, "Not Acceptable"},
    {uhttp_status_proxy_auth_required, "Proxy Authentication Require"},
    {uhttp_status_request_timeout, "Request Timeout"},
    {uhttp_status_conflict, "Conflict"},
    {uhttp_status_gone, "Gone"},
    {uhttp_status_length_required, "Length Required"},
    {uhttp_status_precondition_failed, "Precondition Failed"},
    {uhttp_status_request_entity_too_large, "Payload Too Large"},
    {uhttp_status_request_uri_too_long, "URI Too Long"},
    {uhttp_status_unspport_media_type, "Unsupported Media Type"},
    {uhttp_status_requested_range_not_statisfiable, "Range Not Satisfiable"},
    {uhttp_status_expectation_failed, "Expectation Failed"},
    
    //Server Error 5xx
    {uhttp_status_internal_server_error, "Internal Server Error"},
    {uhttp_status_not_implemented, "Not Implemented"},
    {uhttp_status_bad_gateway, "Bad Gateway"},
    {uhttp_status_service_unavailable, "Service Unavailable"},
    {uhttp_status_gateway_timeout, "Gateway Timeout"},
    {uhttp_status_http_version_not_supported, "HTTP Version Not Supported"},
    {0, NULL},
};

uhttp::uhttp(std::iostream & stream, uhttphandler & handler, 
    bool gzip, bool chunk, bool discard)
    : stream_(stream)
    , handler_(handler)
    , linesize_(HTTP_RECV_MAXSIZE)
    , line_((char *)malloc(linesize_))
    , chunk_(chunk)
    , gzip_(gzip)
    , compress_buf_size_(HTTP_RECV_MAXSIZE * 2)
    , compress_buf_(gzip_ ? (char *)malloc(compress_buf_size_) : NULL)
    , decompress_buf_size_(HTTP_RECV_MAXSIZE)
    , decompress_buf_((char *)malloc(decompress_buf_size_))
    , discard_(discard)
{
}

uhttp::~uhttp()
{
    if (line_ != NULL) {
        free(line_);
    }

    if (compress_buf_ != NULL) {
        free(compress_buf_);
    }

    if (decompress_buf_ != NULL) {
        free(decompress_buf_);
    }
}

void uhttp::run()
{    
    uhttprequest * prequest = new uhttprequest();
    uhttpresponse * presponse = new uhttpresponse();

    uhttprequest & request = *prequest;
    uhttpresponse & response = *presponse;
    
    int result      = en_succeed;
    bool alive      = true;
    int handler_res = 0;

    while (alive) {
        
        result = recv_request(request);
        
        alive = request.keep_alive();
        //request.output_header(std::cout);

        //network down, callback return, break loop.
        if (!stream_.good()) {
            ulog(ulog_debug, "[%s:%d] socket error!\n", __FUNCTION__, __LINE__);
            result = en_socket_reset;
            break;
        }
        
        handler_res = handler_.onhttp(request, response, result);

        if (request.version() == uhttp_version_1_0) {
            response.set_version(uhttp_version_1_0);
        }
        
        if (request.version() == uhttp_version_1_0 ||
            response.version() == uhttp_version_1_0 ||
            !request.keep_alive() || 
            !response.keep_alive()) { //  ||
            //response.statuscode() != uhttp_status_ok) {
            ulog(ulog_debug, "[%s:%d] disconnect after send!\n",
                __FUNCTION__, __LINE__);
            response.set_header(uhttpresponse::HEADER_CONNECTION, "close");
            alive = false;
        }

        if (response.get_header(uhttpmessage::HEADER_DATE) == NULL) {
            char buffer[256] = { 0 };
            time_t t_time = time(NULL);
            struct tm tm_time;
            gmtime_r(&t_time, &tm_time);
            //localtime_r(&t_time, &tm_time);
            strftime(buffer, sizeof(buffer), 
                "%a, %d %b %Y %H:%M:%S %Z", &tm_time);
            response.set_header(uhttpresponse::HEADER_DATE, buffer);
        }
        if (response.get_header(uhttpmessage::HEADER_SERVER) == NULL) {
            response.set_header(uhttpresponse::HEADER_SERVER, "http/uhttp");
        }

        result = send_response(response);
        if (result != en_succeed || handler_res != 0) {
            if (!stream_.good()) {
                result = en_socket_reset;
            }
            
            break;
        }

        // http 1.0 or connection close, close active.
        // maybe need to wait a monent, to check it client close it.
        // TODO::  @20170113
        if (!alive) {
            break;
        }

        request.clear();
        response.clear();
    }

    delete prequest;
    delete presponse;
    
    return;
}

int uhttp::send_request(uhttprequest & request)
{
    std::string & content = request.content();
    request.set_header(uhttpmessage::HEADER_CONTENT_LENGTH, content.size());
    
    if (gzip_) {
        if (!compress(content)) {
            return en_send_req_comp_error;
        }
        request.set_header(uhttpmessage::HEADER_CONTENT_ENCODING, "gzip");
        request.set_header(uhttpmessage::HEADER_CONTENT_LENGTH, content.size());
    }

    if (chunk_ && content.size() > chunk_size_) {
        request.set_header(uhttpmessage::HEADER_TRANSFER_ENCODING, "chunked");
        request.set_header(uhttpmessage::HEADER_CONTENT_LENGTH, chunk_size_);
    }
    
    stream_ << smethods[request.method()] << " " 
            << request.uri().get() << " "
            << sversions[request.version()]
            << UHTTP_LINE_END; 

    //if (!stream_.flush().good()) {
    //    return en_send_req_start_error;
    //}
    
    request.output_header(stream_);
    stream_ << UHTTP_LINE_END;

    //if (!stream_.flush().good()) {
    //    return en_send_req_header_error;
    //}
    
    if (!content.empty()) {
        if (chunk_ && content.size() > chunk_size_) {
            send_by_chunk(content);
        } else {
            stream_.write(content.data(), content.size());
        }
    }

    if (!stream_.flush().good()) {
        return en_send_req_body_error;
    }
    
    return en_succeed;
}

int uhttp::recv_request(uhttprequest & request)
{
    if (!recv_start(request)) {
        //printf("recv recv_start failed!\n");
        return en_recv_req_start_error;
    }
    
    if (!recv_header(request)) {
        //printf("recv recv_header failed!\n");
        return en_recv_req_header_error;
    }

    return recv_body(request);
}

int uhttp::send_response(uhttpresponse & response)
{
    std::string & content = response.content();
    response.set_header(uhttpmessage::HEADER_CONTENT_LENGTH, content.size());
    
    if (gzip_) {
        if (!compress(content)) {
            return en_send_res_comp_error;
        }
        response.set_header(uhttpmessage::HEADER_CONTENT_ENCODING, "gzip");
        response.set_header(uhttpmessage::HEADER_CONTENT_LENGTH, content.size());
    } 

    if (chunk_ && content.size() > chunk_size_) {
        response.set_header(uhttpmessage::HEADER_TRANSFER_ENCODING, "chunked");
        response.set_header(uhttpmessage::HEADER_CONTENT_LENGTH, chunk_size_);
    }
    
    stream_ << sversions[response.version()] << " " 
            << response.statuscode() << " "
            << get_reasonphrase(response.statuscode())
            << UHTTP_LINE_END;

    //if (!stream_.flush().good()) {
    //    return en_send_res_start_error;
    //}

    //response.output_header(std::cout);
    response.output_header(stream_);
    stream_ << UHTTP_LINE_END;

    //if (!stream_.flush().good()) {
    //    return en_send_req_header_error;
    //}

    if (!content.empty()) {
        if (chunk_ && content.size() > chunk_size_) {
            send_by_chunk(content);
        } else {
            stream_.write(content.data(), content.size());
        }
    }

    if (!stream_.flush().good()) {
        return en_send_req_body_error;
    }
    
    return en_succeed;
}

int uhttp::recv_response(uhttpresponse & response)
{
    if (!recv_start(response)) {
        return en_recv_res_start_error;
    }

    if (!recv_header(response)) {
        return en_recv_res_header_error;
    }

    return recv_body(response);
}

bool uhttp::iswhitespace(char c)
{
    if (c == ' ' || c == '\0' || c == '\n' || c == '\r') {
        return true;
    }
    return false;
}

void uhttp::trimleft(char * & left, char * & right)
{
    while (left <= right && iswhitespace(*left))    left ++;
}

void uhttp::trimright(char * & left, char * & right)
{
    while (left <= right && iswhitespace(*right))   right --;
}

void uhttp::trim(char * & left, char * & right)
{
    //private function, assume left & right never be NULL.
    trimleft(left, right);
    trimright(left, right);
}

void uhttp::set(char * const p, char v)
{
    *p = v;
}

/*
 * split str by sep. skip left whitespace.
 * [a1, a2] represent the get one.
 * return next begin.
 */
char* uhttp::get(char *begin, char *end, char sep, char *& a1, char *&a2)
{
    char * pb = begin;
    char * pe = end;
    
    trimleft(pb, pe);
    if (pb > pe) {
        return NULL;
    }

    char * ps = strchr(pb, sep);
    if (ps == NULL) {
        return NULL;
    }

    a1 = pb;
    a2 = ps - 1;

    trimright(a1, a2);

    return ps + 1;
}

bool uhttp::recv_start(std::string & first, std::string & second, 
        std::string & third, bool stringsplit)
{
    if (!stream_.getline(line_, linesize_).good()) {
        return false;
    }

    std::streamsize rdn = stream_.gcount();
    if (rdn >= linesize_) {
        ulog(ulog_error, "uhttp:recv_start max size:%ld\n", rdn);
        return false;
    }
    
    char * begin    = line_;
    char * end      = line_ + rdn - 1;

    trimright(begin, end);
    if (begin > end) {
        return false;
    }
    set(end + 1, '\0');
    ulog(ulog_debug, "uhttp:recv_start:%s\n", begin);
    
    // format:  METHOD URI VERSION (CRLF)
    char * pb;
    char * pe;
    char * next;

    // get method
    next = get(begin, end, ' ', pb, pe);
    if (next == NULL) {
        return false;
    }
    std::string(pb, pe - pb + 1).swap(first);

    // get uri.
    next = get(next, end, ' ', pb, pe);
    if (next == NULL) {
        return false;
    }
    std::string(pb, pe - pb + 1).swap(second);

    // get version
    trimleft(next, end);
    std::string(next, end - next + 1).swap(third);
    
    return true;
}


bool uhttp::recv_start(uhttprequest & request)
{
    std::string method, uri, version;

    if (!recv_start(method, uri, version)) {
        return false;
    }
    
    ulog(ulog_debug, "uhttp:recv_request[%s][%s][%s]\n",
        method.c_str(), uri.c_str(), version.c_str());
    

    const int imethod = analyse_method(method.c_str());
    request.set_method(imethod);

    request.set_uri(uri.c_str());

    const int iversion = analyse_version(version.c_str());
    request.set_version(iversion);

    if(imethod == uhttp_method_unknown || iversion == uhttp_version_unknown) {
        return false;
    }

    return true;
}

bool uhttp::recv_start(uhttpresponse& response)
{
    std::string version, status, reason;

    if (!recv_start(version, status, reason)) {
        return false;
    }

    ulog(ulog_debug, "uhttp:recv_response[%s][%s][%s]\n",
        version.c_str(), status.c_str(), reason.c_str());
    
    const int iversion = analyse_version(version.c_str());
    response.set_version(iversion);
    response.set_statuscode(atoi(status.c_str()));

    if(iversion == uhttp_version_unknown) {
        return false;
    }

    return true;
}

bool uhttp::recv_header(uhttpmessage & msg)
{
    bool good = false;
    std::streamsize readn = 0;
    char * sep, * pb, * pe, * begin, * end;
    
    for ( ; ; ) {
        good = stream_.getline(line_, linesize_).good();
        if (!good) {
            break;
        }

        readn   = stream_.gcount();
        begin   = line_;
        end     = line_ + readn - 1;

        if (readn > linesize_) {
            return false;
        }

        trimright(begin, end);
        if (begin > end) {
            break;
        }
        set(end + 1, '\0');

        sep     = get(begin, end, ':', pb, pe);
        if (sep == NULL) {
            return false;
        }

        trimleft(sep, end);

        std::string key(pb, pe - pb + 1);
        std::string value(sep, end - sep + 1);
        
        msg.set_header(key, value);
    } 

    return good;
}

int uhttp::recv_body(uhttpmessage & msg)
{
    bool    good    = true;
    int     ret     = en_succeed;

    const char * encoding 
        = msg.get_header(uhttpmessage::HEADER_TRANSFER_ENCODING);

    if (encoding != NULL && strcasecmp(encoding, "chunked") == 0) {
        
        while (good) {
            good = stream_.getline(line_, linesize_).good();
            if (!good) {
                break;
            }

            char * end = line_ + stream_.gcount() - 1;
            while ((end != line_) && iswhitespace(*end)) {
                *end -- = '\0';
            }
            
            int size = strtol(line_, NULL, 16);

            //chunk end
            if (size == 0) {
                good = stream_.getline(line_, linesize_).good();
                break;
            } else if (msg.content().size() + (size_t)size > max_content_limit_
                && ret == en_succeed) {
                ret = msg.type() ==uhttpmessage::en_request 
                    ? en_recv_req_body_too_big_error
                    : en_recv_res_body_too_big_error;
                ulog(ulog_error, "client intended to send too large chunked body:"
                    "%ld+%d bytes\n", msg.content().size(), size);

                if (!discard_) {
                    break;
                }
            }
            
            int readn = 0;
            while (size > 0 && good) {
                readn = size > linesize_ ? linesize_ : size;
                good = stream_.read(line_, readn).good();

                if (good && ret == en_succeed) {
                    msg.append_content(line_, readn);
                } // else. discard directly when body too big.
                size -= readn;
            }

            if (good) {
                good = stream_.getline(line_, linesize_).good();
            }
        }
        
        msg.set_header(uhttpmessage::HEADER_CONTENT_LENGTH, msg.content().size());
    } else {
        const char * slength = 
            msg.get_header(uhttpmessage::HEADER_CONTENT_LENGTH);

        if (slength != NULL) {
            int length = atoi(slength);

            if (length > (int)max_content_limit_) {
                ret = msg.type() ==uhttpmessage::en_request 
                    ? en_recv_req_body_too_big_error
                    : en_recv_res_body_too_big_error;
                ulog(ulog_error, "client intended to send too large body:%d bytes\n",
                    length);
            }

            if (ret == en_succeed || (ret != en_succeed && !discard_)) {
                int readn = 0;
                while (length > 0 && good) {
                    readn = length > linesize_ ? linesize_ : length;
                    good = stream_.read(line_, readn).good();

                    if (good && ret == en_succeed) {
                        msg.append_content(line_, readn);
                    }
                    length -= readn;
                }
            }
        }
    }

    if (ret != en_succeed) {
        return ret;
    }

    if (!good) {
        ret = msg.type() ==uhttpmessage::en_request ? 
            en_recv_req_body_error : en_recv_res_body_error;
    } else {
    
        const char * content_type = 
            msg.get_header(uhttpmessage::HEADER_CONTENT_ENCODING);
        bool gzip = false;
        bool suc  = false;
        
        if (content_type != NULL) {
            gzip = strstr(content_type, "gzip") != NULL;
        }
        
        if (good && (gzip) && msg.content().size() > 0) {
            std::string content;
            
            z_stream dstream = {0};

            dstream.zalloc     = NULL; 
            dstream.zfree      = NULL; 
            dstream.opaque     = NULL; 

            if (inflateInit2(&dstream, MAX_WBITS + 16) != Z_OK) {
                //printf("inflate init failed!\n");
                gzip = false;
            }

            Bytef * next_in = (Bytef *)msg.content().c_str();
            size_t desize = msg.content().size();
            size_t consume = 0;

            while (dstream.total_in < desize) {
                dstream.avail_in    = desize - consume;
                dstream.next_in     = next_in + consume;
                dstream.next_out    = (Bytef *)decompress_buf_;
                dstream.avail_out   = decompress_buf_size_;

                int ret = inflate(&dstream, Z_NO_FLUSH);
                if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT) {
                    break;
                } 
                if (ret == Z_MEM_ERROR) {
                    break;
                }
                if (ret == Z_DATA_ERROR) {
                    break;
                }

                content.append(decompress_buf_, 
                    dstream.next_out - (Bytef *)decompress_buf_);
                consume = dstream.next_in - next_in;
            }
            
            if (dstream.total_in == desize) {
                msg.content().swap(content);
                msg.remove_header(uhttpmessage::HEADER_CONTENT_ENCODING);
                suc = true;
            }

            if (inflateEnd(&dstream) != Z_OK) {
            }
        }

        if (gzip && !suc) {
            ret = msg.type() ==uhttpmessage::en_request ? 
                en_recv_req_decomp_error : en_recv_res_decomp_error;
        }
    }

    return ret;
}

bool uhttp::send_by_chunk(const std::string & data)
{
    size_t size     = data.size();
    size_t loop     = size / chunk_size_;
    size_t off      = 0;
    
    const char * p  = data.data();
    
    for (size_t i = 0; i < loop; i++) {
        stream_ << std::hex << chunk_size_ << UHTTP_LINE_END;
        
        stream_.write(p + off, chunk_size_);
        off += chunk_size_;

        stream_ << UHTTP_LINE_END;

        if (!stream_.flush().good()) {
            return false;
        }
    }

    if (off < size) {
        stream_ << std::hex << size - off << UHTTP_LINE_END;
        stream_.write(p + off, size - off);
        stream_ << UHTTP_LINE_END;

        if (!stream_.flush().good()) {
            return false;
        }
    }

    stream_ << 0 << UHTTP_LINE_END;
    stream_ << UHTTP_LINE_END;
        
    return stream_.flush().good();
}

int uhttp::analyse_method(const char * method)
{
    int imethod = uhttp_method_unknown;

    for (int i = 0; smethods[i] != NULL; i++) {
        if (!strcasecmp(method, smethods[i])) {
            imethod = i;
            break;
        }
    }
    
    return imethod;
}

int uhttp::analyse_version(const char * version)
{
    int iversion = uhttp_version_unknown;

    for (int i = 0; sversions[i] != NULL; i++) {
        if (!strcasecmp(version, sversions[i])) {
            iversion = i;
            break;
        }
    }
    
    return iversion;
}

bool uhttp::compress(std::string & content, int type)
{
    //((void *)type); //fix unused!

    if (content.empty()) {
        return false;
    }

    z_stream cstream = {0};
    
    cstream.zalloc     = NULL; 
    cstream.zfree      = NULL; 
    cstream.opaque     = NULL; 

    if (deflateInit2(&cstream, Z_DEFAULT_COMPRESSION, 
        Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        //printf("deflateInit2 failed, %s\n", 
        //    cstream.msg == NULL ? "NONE" : cstream.msg);
        return false;
    }

    size_t remain   = compress_buf_size_ >> 1;
    Bytef * next_in = (Bytef *)content.c_str();
    size_t csize    = content.size();
    size_t consume  = 0;
    std::string swapc;
    size_t noflush  = 0;

    if (csize > remain) {
        noflush = csize - remain;
    }

    while (noflush > 0 && cstream.total_in < noflush) {
        cstream.next_in     = next_in + consume;
        cstream.avail_in    = noflush - consume;
        cstream.next_out    = (Bytef *)compress_buf_;
        cstream.avail_out   = compress_buf_size_;

        int ret = deflate(&cstream, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR) {
            break;
        }

        consume = cstream.next_in - next_in;
        swapc.append(compress_buf_, cstream.next_out - (Bytef *)compress_buf_);
    }

    bool ok = (cstream.total_in == noflush);
    int ret = Z_OK;
    next_in += noflush;

    if (ok) {
        while (ret != Z_STREAM_END) {
            cstream.next_in     = next_in + consume;
            cstream.avail_in    = csize - consume;
            cstream.next_out    = (Bytef *)compress_buf_;
            cstream.avail_out   = compress_buf_size_;

            ret = deflate(&cstream, Z_FINISH);
            if (ret == Z_STREAM_ERROR) {
                break;
            }
            //sleep(1);

            consume = cstream.next_in - next_in;
            swapc.append(compress_buf_, cstream.next_out - (Bytef *)compress_buf_);
        }
    }

    deflateEnd(&cstream);

    if (ret == Z_STREAM_END) {
        content.swap(swapc);
        return true;
    }

    return false;
}

const char * uhttp::get_reasonphrase(int statuscode) 
{
    const char * reasonphrease = "";

    for (int i = 0; sreasonphrase[i].status_code != 0; i++) {
        if (sreasonphrase[i].status_code == statuscode) {
            reasonphrease = sreasonphrase[i].reason_phrase;
            break;
        }
    }

    return reasonphrease;
}

const char * uhttp::get_version(int version)
{
    if (version > uhttp_version_unknown || version < 0) {
        version = uhttp_version_unknown;
    }

    return sversions[version];
}

const char * uhttp::get_methodname(int method)
{
    if (method < uhttp_method_get || method >= uhttp_method_unknown) {
        return NULL;
    }
    return smethods[method];
}

void uhttp::set_max_content_limit(size_t limit)
{
    max_content_limit_ = limit;
}

size_t uhttp::max_content_limit_ = 4 * 1024 * 1024; // 4M
size_t uhttp::chunk_size_        = 4096;