#include "uhttpresponse.h"
#include "uhttpdefs.h"
#include "uhttp.h"

uhttpresponse::uhttpresponse()
    : uhttpmessage(en_response)
    , statuscode_(0)
{
    set_header(HEADER_CONNECTION, HEADER_KEEPALIVE);
}

uhttpresponse::~uhttpresponse()
{
}

void uhttpresponse::set_statuscode(int statuscode)
{
    statuscode_ = statuscode;
}

const int uhttpresponse::statuscode() const
{
    return statuscode_;
}

const char * uhttpresponse::reasonphrase() const
{
    return uhttp::get_reasonphrase(statuscode_);
}

void uhttpresponse::clear()
{
    uhttpmessage::clear();

    statuscode_ = 0;
    
    set_keepalive(true);
    set_version(uhttp_version_1_1);
}

