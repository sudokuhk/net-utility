#include "uurl.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char * surl_protocol[] =
{
    "http",
    "https",
    "ftp",
    "mailto",
    "ldap",
    "file",
    "news",
    "gopher",
    "telnet",
    NULL,
};

uurl::uurl(const char * url)
    : url_(url)
    , protocol_(uprotocol_unknown)
    , host_()
    , port_(80)
    , uri_()
{
    analyse();
}

uurl::~uurl()
{
}

const int uurl::protocol() const
{
    return protocol_;
}

const std::string & uurl::host() const
{
    return host_;
}

const int uurl::port() const
{
    return port_;
}

const uuri uurl::uri() const
{
    return uri_;
}

int uurl::get_protocol(const char * protocol)
{
    int iprotocol = uprotocol_unknown;

    int idx = 0;
    const char ** ptr = surl_protocol;
    
    while (*ptr != NULL) {
        if (strcasecmp(*ptr, protocol) == 0) {
            iprotocol = idx;
            break;
        }
        ++ ptr;
        ++ idx;
    }

    return iprotocol;
}

void uurl::analyse()
{
    //protocol://hostname[:port]/path/[;parameters][?query]#fragment
    if (url_.empty()) {
        return;
    }
    
    const char * begin = url_.data();
    const char * end   = begin + url_.size();
    const char * pos   = begin;

    // 1. get protocol.
    while (pos < end && (*pos != ':')) {
        ++ pos;
    }
    
    if (pos != begin && pos < end) {
        std::string protocol(begin, pos - begin);
        int iprocotol = get_protocol(protocol.c_str());
        if (iprocotol == uprotocol_unknown) {
            return;
        }
        protocol_ = iprocotol;
    } else {
        goto error;
    }

    if (pos + 2 > end || 
        *(pos + 1) != '/' || *(pos + 2) != '/') {
        goto error;
    }

    // 2. skip '://'
    pos += 2;

    // 3. get host.
    pos ++;
    begin = pos;
    
    while (pos < end && 
        (*pos != ':' && *pos != '/' && *pos != '?' 
            && *pos != ';' && *pos != '#')) {
        ++ pos;
    }

    if (pos != begin && pos < end) {
        std::string(begin, pos - begin).swap(host_);
    } else {
        goto error;
    }

    // 4. get port if had.
    if (*pos == ':') {
        pos ++;
        begin = pos;
        
        while (pos < end && 
            (*pos != '/' && *pos != '?' && *pos != ';' && *pos != '#')) {
            ++ pos;
        }

        if (pos != begin && pos < end) {
            std::string port(begin, pos - begin);
            port_ = atoi(port.c_str());
        }
    }

    // 5. get uri.
    begin = pos;
    uri_.set(begin);

    return;
    
error:
    protocol_ = uprotocol_unknown;
    return;
}
