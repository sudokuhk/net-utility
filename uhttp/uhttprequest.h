#ifndef _U_HTTP_REQUEST_H__
#define _U_HTTP_REQUEST_H__

#include "uhttpmessage.h"
#include "uuri.h"

class uhttprequest
    : public uhttpmessage
{
public:
    uhttprequest();

    virtual ~uhttprequest();

    void set_method(int method);

    const int method() const;

    const char * methodname() const;

    void set_uri(const char * uri);

    void set_uri(const uuri & uri);

    const uuri & uri() const;

    virtual void clear();
    
private:
    int method_;
    
    uuri uri_;
};

#endif
