#include "uhttp.h"
#include "uhttpmessage.h"
#include "uhttprequest.h"
#include "uhttpresponse.h"

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

uhttp::uhttp(std::iostream & stream, uhttphandler & handler, bool gzip)
    : stream_(stream)
    , handler_(handler)
    , line_(NULL)
    , linesize_(HTTP_RECV_MAXSIZE)
    , compress_buf_size_(HTTP_RECV_MAXSIZE * 2)
    , compress_buf_(NULL)
    , decompress_buf_size_(HTTP_RECV_MAXSIZE)
    , decompress_buf_(NULL)
    , gzip_(gzip)
{
    line_           = (char *)malloc(linesize_);

    if (gzip_) {
        compress_buf_   = (char *)malloc(compress_buf_size_);
        decompress_buf_ = (char *)malloc(decompress_buf_size_);
    }
}

uhttp::~uhttp()
{
    if (line_ != NULL) {
        free(line_);
        line_ = NULL;
    }

    if (compress_buf_ != NULL) {
        free(compress_buf_);
        compress_buf_ = NULL;
    }

    if (decompress_buf_ != NULL) {
        free(decompress_buf_);
        decompress_buf_ = NULL;
    }
}

void uhttp::run()
{    
    uhttprequest * prequest = new uhttprequest();
    uhttpresponse * presponse = new uhttpresponse();

    uhttprequest & request = *prequest;
    uhttpresponse & response = *presponse;
    
    bool good = false;
    bool alive = true;

    while (alive) {
        
        good = recv_request(request);
        if (!good) {
            break;
        }
        alive = request.keep_alive();
        //request.output_header(std::cout);

        int res = handler_.onhttp(request, response);

        if (request.version() == uhttp_version_1_0 ||
            response.version() == uhttp_version_1_0 ||
            !request.keep_alive() || 
            !response.keep_alive() ||
            response.statuscode() != uhttp_status_ok) {
            response.set_header(uhttpresponse::HEADER_CONNECTION, "close");
        }

        if (response.get_header(uhttpmessage::HEADER_DATE) == NULL) {
            char buffer[256] = { 0 };
            time_t t_time = time(NULL);
            struct tm tm_time;
            gmtime_r(&t_time, &tm_time);
            strftime(buffer, sizeof(buffer), 
                "%a, %d %b %Y %H:%M:%S %Z", &tm_time);
            response.set_header(uhttpresponse::HEADER_DATE, buffer);
        }
        if (response.get_header(uhttpmessage::HEADER_SERVER) == NULL) {
            response.set_header(uhttpresponse::HEADER_SERVER, "http/uhttp");
        }
        
        good = send_response(response);
        if (!good || res != 0) { // || response.statuscode() != uhttp_status_ok) {
            break;
        }
    }

    delete prequest;
    delete presponse;
    
    return;
}

bool uhttp::send_request(uhttprequest & request)
{
    stream_ << smethods[request.method()] << " " 
            << request.uri().get() << " "
            << sversions[request.version()]
            << UHTTP_LINE_END;

    std::string & content = request.content();
    if (gzip_ && compress(content)) {
        request.set_header(uhttpmessage::HEADER_CONTENT_ENCODING, "gzip");
        request.set_header(uhttpmessage::HEADER_CONTENT_LENGTH, content.size());
    }
    
    request.output_header(stream_);
    stream_ << UHTTP_LINE_END;

    if (!content.empty()) {
        stream_.write(content.data(), content.size());
    }

    if (stream_.flush().good()) {
        return true;
    }
    
    return false;
}

bool uhttp::recv_request(uhttprequest & request)
{
    if (!recv_start(request)) {
        //printf("recv recv_start failed!\n");
        return false;
    }
    
    if (!recv_header(request)) {
        //printf("recv recv_header failed!\n");
        return false;
    }

    if (!recv_body(request)) {
        //printf("recv recv_body failed!\n");
        return false;
    }

    return true;
}

bool uhttp::send_response(uhttpresponse & response)
{
    stream_ << sversions[response.version()] << " " 
            << response.statuscode() << " "
            << get_reasonphrase(response.statuscode())
            << UHTTP_LINE_END;

    std::string & content = response.content();
    if (gzip_ && compress(content)) {
        response.set_header(uhttpmessage::HEADER_CONTENT_ENCODING, "gzip");
        response.set_header(uhttpmessage::HEADER_CONTENT_LENGTH, content.size());
    }

    //response.output_header(std::cout);
    response.output_header(stream_);
    stream_ << UHTTP_LINE_END;

    //const std::string & content = response.content();
    if (!content.empty()) {
        stream_.write(content.data(), content.size());
    }

    if (stream_.flush().good()) {
        return true;
    }
    
    return false;
}

bool uhttp::recv_response(uhttpresponse & response)
{
    if (!recv_start(response)) {
        return false;
    }

    if (!recv_header(response)) {
        return false;
    }

    if (!recv_body(response)) {
        return false;
    }

    return true;
}

bool uhttp::istrim(char c)
{
    if (c == ' ' || c == '\0' || c == '\n' || c == '\r') {
        return true;
    }
    return false;
}

bool uhttp::recv_start(std::string & first, std::string & second, 
        std::string & third, bool stringsplit)
{
    if (!stream_.getline(line_, linesize_).good()) {
        return false;
    }

    if (stringsplit) {
        //std::string line(line_, stream_.gcount() - 2);

        //int begin = 0;
        //int pos = line.find(begin, ' ');
        return true;
    }
    
    char * end = line_ + stream_.gcount() - 1;
    while ((end != line_) && istrim(*end)) {
        *end -- = '\0';
    }

    //printf("start:%s\n", line_); 
    
    // format:  METHOD URI VERSION (CRLF)
    char * method;
    char * uri;
    char * version;
    
    // get method
    method = line_;
    while (method < end && istrim(*method)) method ++;   //trim
    //printf("method:%s\n", method);

    // find first whitespace.
    uri = method + 1;
    while (uri < end && *uri != ' ') uri ++;
    *uri = '\0';

    // get uri.
    uri ++;
    while (uri < end && istrim(*uri)) uri ++;   //trim
    //printf("uri:%s\n", uri);

    // find second whitespace.
    version = uri + 1;
    while (version < end && *version != ' ') version ++;
    *version = '\0';

    //get version.
    version ++;
    while (version < end && istrim(*version)) version ++;   //trim
    //printf("version:%s\n", version);

    std::string(method).swap(first);
    std::string(uri).swap(second);
    std::string(version).swap(third);

    return true;
}


bool uhttp::recv_start(uhttprequest & request)
{
    std::string method, uri, version;

    if (!recv_start(method, uri, version)) {
        return false;
    }

    const int imethod = analyse_method(method.c_str());
    request.set_method(imethod);

    request.set_uri(uri.c_str());

    const int iversion = analyse_version(version.c_str());
    request.set_version(iversion);

    //printf("method:%2d:%s, uri:%s, version:%d:%s\n",
    //    imethod, method.c_str(), uri.c_str(), iversion, version.c_str());
    
    //invalid.
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

    
    const int iversion = analyse_version(version.c_str());
    response.set_version(iversion);

    response.set_statuscode(atoi(status.c_str()));

    //printf("verison:%2d:%s, status:%s\n",
    //    iversion, version.c_str(), (status.c_str()));
    
    //invalid.
    if(iversion == uhttp_version_unknown) {
        return false;
    }

    return true;
}

bool uhttp::recv_header(uhttpmessage & msg)
{
    bool good = false;
    std::streamsize readn = 0;
    char * end;
    char * name;
    char * value;
    char * sep;
    
    for ( ; ; ) {
        good = stream_.getline(line_, linesize_).good();
        if (!good) {
            break;
        }

        readn = stream_.gcount();
        
        end = line_ + readn - 1;
        while ((end >= line_) && istrim(*end)) {
            *end -- = '\0';
        }
        //printf("read line:%s\n", line_);

        if (*line_ == '\0') {
            break;
        }
        
        //get name.
        name = line_;
        while (name < end && istrim(*name)) name ++; //trim name left.

        //get separate ':'
        sep = name;
        while (sep < end && *sep != ':') sep ++;
        *sep = '\0';

        //get value.
        value = sep + 1;
        while (value < end && istrim(*value)) value ++; //trim value left.

        //trim name right.
        sep --;
        while (sep >= name && istrim(*sep)) {
            *sep -- = '\0';
        }

        msg.set_header(name, value);
        //printf("header==>  '%s' = '%s'\n", name, value);
    } 

    return good;
}

bool uhttp::recv_body(uhttpmessage & msg)
{
    bool good = true;

    const char * encoding 
        = msg.get_header(uhttpmessage::HEADER_TRANSFER_ENCODING);

    if (encoding != NULL && strcasecmp(encoding, "chunked") == 0) {

        while (good) {
            good = stream_.getline(line_, linesize_).good();
            if (!good) {
                break;
            }

            char * end = line_ + stream_.gcount() - 1;
            while ((end != line_) && istrim(*end)) {
                *end -- = '\0';
            }
            
            int size = strtol(line_, NULL, 16);

            //chunk end
            if (size == 0) {
                good = stream_.getline(line_, linesize_).good();
                break;
            } else if (size > (int)max_content_limit_) {
                good = false;
                break;
            }
            
            int readn = 0;
            while (size > 0 && good) {
                readn = size > linesize_ ? linesize_ : size;
                good = stream_.read(line_, readn).good();

                if (good) {
                    msg.append_content(line_, readn);
                    size -= readn;

                    if (msg.content().size() > max_content_limit_) {
                        good = false;
                        break;
                    }
                }
            }

            if (good) {
                good = stream_.getline(line_, linesize_).good();
            }
        }
    } else {
        const char * slength = 
            msg.get_header(uhttpmessage::HEADER_CONTENT_LENGTH);

        if (slength != NULL) {
            int length = atoi(slength);

            if (length <= (int)max_content_limit_) {
                int readn = 0;
                while (length > 0 && good) {
                    readn = length > linesize_ ? linesize_ : length;
                    good = stream_.read(line_, readn).good();

                    if (good) {
                        msg.append_content(line_, readn);
                        length -= readn;
                    }
                }
            } else {
                good = false;
            }
        }
    }

    if (good && gzip_ && msg.content().size() > 0) {
        std::string content;
        
        const char * dataencoding 
            = msg.get_header(uhttpmessage::HEADER_CONTENT_ENCODING);
        bool gzip = false;
        if (dataencoding != NULL && strstr(dataencoding, "gzip") != NULL) {
            gzip = true;
        }

        if (gzip) {
            z_stream dstream = {0};
    
            dstream.zalloc     = NULL; 
            dstream.zfree      = NULL; 
            dstream.opaque     = NULL; 
    
            if (inflateInit2(&dstream, MAX_WBITS + 16) != Z_OK) {
                printf("inflate init failed!\n");
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
            }

            if (inflateEnd(&dstream) != Z_OK) {
            }
        }
    }

    return good;
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
        printf("deflateInit2 failed, %s\n", 
            cstream.msg == NULL ? "NONE" : cstream.msg);
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

void uhttp::set_max_content_limit(size_t limit)
{
    max_content_limit_ = limit;
}

size_t uhttp::max_content_limit_ = 4 * 1024 * 1024; // 4M

