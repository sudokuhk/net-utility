#ifndef _U_URL_H__
#define _U_URL_H__

#include <string>

#include "uuri.h"

enum uprotocol
{
    uprotocol_http = 0,
    uprotocol_https,
    uprotocol_ftp,
    uprotocol_mailto,
    uprotocol_ldap,
    uprotocol_file,
    uprotocol_news,
    uprotocol_gopher,
    uprotocol_telnet,
    uprotocol_unknown,
};

//protocol://hostname[:port]/path/[;parameters][?query]#fragment
class uurl
{
public:
    uurl(const char * url);

    ~uurl();

    const int protocol() const;

    const std::string & host() const;

    const int port() const;

    const uuri uri() const;
private:
    int get_protocol(const char * protocol);

    void analyse();
    
private:
    std::string url_;
    int         protocol_;
    std::string host_;
    int         port_;
    uuri        uri_;
};

#endif
