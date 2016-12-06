#ifndef _U_HTTP_RESPONSE_H__
#define _U_HTTP_RESPONSE_H__

#include "uhttpmessage.h"

class uhttpresponse
    : public uhttpmessage
{
public:
    uhttpresponse();

    virtual ~uhttpresponse();

    void set_statuscode(int statuscode);

    const int statuscode() const;

    const char * reasonphrase() const;

    virtual void clear();
private:
    int statuscode_;
};

#endif
