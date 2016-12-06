#include "uhttpmessage.h"
#include "uhttpdefs.h"

#include <stdio.h>
#include <string.h>

//public static members.
const char * uhttpmessage::HEADER_CONTENT_LENGTH    = "Content-Length";
const char * uhttpmessage::HEADER_CONTENT_TYPE      = "Content-Type";
const char * uhttpmessage::HEADER_CONTENT_ENCODING  = "Content-Encoding";
const char * uhttpmessage::HEADER_CONNECTION        = "Connection";
const char * uhttpmessage::HEADER_PROXY_CONNECTION  = "Proxy-Connection";
const char * uhttpmessage::HEADER_TRANSFER_ENCODING = "Transfer-Encoding";
const char * uhttpmessage::HEADER_DATE              = "Date";
const char * uhttpmessage::HEADER_SERVER            = "Server";
const char * uhttpmessage::HEADER_KEEPALIVE         = "Keep-Alive";
const char * uhttpmessage::HEADER_XFF               = "X-Forwarded-For";

uhttpmessage::uhttpmessage(int type)
    : type_(type)
    , version_(uhttp_version_1_0)
    , content_()
    , header_()
{
}

uhttpmessage::~uhttpmessage()
{
}

int uhttpmessage::version() const
{
    return version_;
}

void uhttpmessage::set_version(int version)
{
    version_ = version;
}

int uhttpmessage::type() const
{
    return type_;
}

size_t uhttpmessage::header_count() const
{
    return header_.size();
}

void uhttpmessage::set_header(const char * name, const char * value)
{
    if (name == NULL || strlen(name) == 0 || 
        value == NULL || strlen(value) == 0) {
        return;
    }

    header_[name] = value;
}

void uhttpmessage::set_header(const char * name, int value)
{
    if (name == NULL || strlen(name) == 0) {
        return;
    }

    char buf[32] = {0};
    snprintf(buf, 32, "%d", value);

    set_header(name, buf);
}

bool uhttpmessage::remove_header(const char * name)
{
    bool ret = false;
    
    if (name != NULL) {
        std::map<std::string, std::string>::iterator it;
        if ((it = header_.find(name)) != header_.end()) {
            header_.erase(it);
            ret = true;
        }
    }

    return ret;
}

const char * uhttpmessage::get_header(const char * name) const
{   
    const char * value = NULL;
    
    if (name != NULL) {
        std::map<std::string, std::string>::const_iterator it;
        if ((it = header_.find(name)) != header_.end()) {
            value = it->second.c_str();
        }
    }

    return value;
}

void uhttpmessage::output_header(std::ostream & stream, const char * name)
{
    if (header_.empty()) {
        return;
    }
    
    if (name == NULL) {
        std::map<std::string, std::string>::const_iterator it;
        const char * name;
        const char * value;
        
        for (it = header_.begin(); it != header_.end(); ++ it) {
            name = it->first.c_str();
            value = it->second.c_str();
            
            stream << name << ": " << value << UHTTP_LINE_END;
        }
    } else {
        const char * value = get_header(name);
        if (value != NULL) {
            stream << name << ": " << value << UHTTP_LINE_END;
        }
    }

    return;
}

bool uhttpmessage::keep_alive() const
{
    bool ret = false;
    
    const char * proxy = get_header(HEADER_PROXY_CONNECTION);
    const char * local = get_header(HEADER_CONNECTION);

    if ((NULL != proxy && 0 == strcasecmp(proxy, HEADER_KEEPALIVE))
            || (NULL != local && 0 == strcasecmp(local, HEADER_KEEPALIVE))) {
        ret = true;
    }

    return ret;
}

void uhttpmessage::append_content(const std::string & content)
{
    content_.append(content);
    set_header(HEADER_CONTENT_LENGTH, content_.size());
}

void uhttpmessage::append_content(const char * content, int length)
{
    if (content == NULL || length == 0) {
        return;
    }
    content_.append(content, length);
    set_header(HEADER_CONTENT_LENGTH, content_.size());
}

void uhttpmessage::set_content(const char * content, int length)
{
    content_.clear();

    append_content(content, length);
}

void uhttpmessage::set_content(const char * content)
{
    if (content != NULL) {
        set_content(content, strlen(content));
    }
}

const std::string & uhttpmessage::content() const
{
    return content_;
}

std::string & uhttpmessage::content()
{
    return content_;
}

void uhttpmessage::clear()
{
    content_.clear();
    header_.clear();
    version_ = uhttp_version_1_0;
}
