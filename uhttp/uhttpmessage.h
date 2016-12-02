#ifndef _U_HTTP_MESSAGE_H__
#define _U_HTTP_MESSAGE_H__

#include <map>
#include <string>
#include <iostream>

class uhttpmessage
{
public:
    enum en_type {
        en_request,
        en_response,
    };

public:
    static const char * HEADER_CONTENT_LENGTH;
    static const char * HEADER_CONTENT_TYPE;
    static const char * HEADER_CONTENT_ENCODING;
    static const char * HEADER_CONNECTION;
    static const char * HEADER_PROXY_CONNECTION;
    static const char * HEADER_TRANSFER_ENCODING;
    static const char * HEADER_DATE;
    static const char * HEADER_SERVER;
    static const char * HEADER_KEEPALIVE;
    static const char * HEADER_XFF;
    
public:
    uhttpmessage(int type);

    virtual ~uhttpmessage();

    int version() const;
    void set_version(int version);
    
    int type() const;
    size_t header_count() const;

    void set_header(const char * name, const char * value);
    void set_header(const char * name, int value);
    bool remove_header(const char * name);

    const char * get_header(const char * name) const;
    void output_header(std::ostream & stream, const char * name = NULL);

    bool keep_alive() const;

    void append_content(const char * content, int length);
    void set_content(const char * content, int length);
    void set_content(const char * content);
    const std::string & content() const;
    std::string & content();

    virtual void clear();

protected:
    const int type_;

    int version_;

    std::string content_;

    std::map<std::string, std::string> header_;
};

#endif
