#include "uhttprequest.h"
#include "uhttpdefs.h"

uhttprequest::uhttprequest()
    : uhttpmessage(en_request)
    , method_(uhttp_method_unknown)
    , uri_()
{
}

uhttprequest::~uhttprequest()
{
}

void uhttprequest::set_method(int method)
{
    method_ = method;
}

const int uhttprequest::method() const
{
    return method_;
}

void uhttprequest::set_uri(const char * uri)
{
    uri_.set(uri);
}

void uhttprequest::set_uri(const uuri & uri)
{
    uri_ = uri;
}

const uuri & uhttprequest::uri() const
{
    return uri_;
}

void uhttprequest::clear()
{
    uhttpmessage::clear();

    uri_.clear();
    method_ = uhttp_method_unknown;
}


