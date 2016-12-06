#include "uhttpresponse.h"
#include "uhttpdefs.h"
#include "uhttp.h"

uhttpresponse::uhttpresponse()
    : uhttpmessage(en_response)
    , statuscode_(0)
{
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
}

