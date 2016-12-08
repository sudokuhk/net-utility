#include "uuri.h"

#include <string.h>
#include <stdio.h>

uuri::uuri()
    : uri_()
    , path_()
    , parameters_()
    , query_()
    , fragment_()
{
}

uuri::uuri(const char * uri)
    : uri_(uri)
    , path_()
    , parameters_()
    , query_()
    , fragment_()
{
}

uuri::~uuri()
{
}

void uuri::set(const char * uri)
{
    clear();
    
    if (uri == NULL) {
        return;
    }
    
    uri_.append(uri);

    analyse();
}

std::string uuri::get() const
{
    if (uri_.empty()) {
        return conbine();
    }
    return uri_;
}

const std::string & uuri::path() const
{
    return path_;
}

const std::string & uuri::parameters() const
{
    return parameters_;
}

const std::map<std::string, std::string> & uuri::query() const
{
    return query_;
}

const std::string & uuri::fragment() const
{
    return fragment_;
}

void uuri::set_path(const char * path)
{
    set_string(path_, path);
}

void uuri::set_parameters(const char * parameters)
{
    set_string(parameters_, parameters);
}

void uuri::set_query(const char * name, const char * value)
{
    if (name == NULL || strlen(name) == 0 ||
        value == NULL || strlen(value) == 0) {
        return;
    }
    query_[name] = value;
}

void uuri::set_query(const char * name, const int value)
{
    if (name == NULL || strlen(name) == 0) {
        return;
    }

    char buf[32];
    snprintf(buf, 31, "%d", value);

    set_query(name, buf);
}

void uuri::set_fragment(const char * fragment)
{
    set_string(fragment_, fragment);
}

void uuri::set_fragment(int fragment)
{
    char buf[32];
    snprintf(buf, 31, "%d", fragment);

    set_string(fragment_, buf);
}

void uuri::set_string(std::string & str, const char * value)
{
    if (value == NULL) {
        return;
    }
    std::string(value).swap(str);
}

std::string uuri::conbine() const
{
    std::string uri;
    
    uri.reserve(1024);
    
    if (!path_.empty()) {
        if (path_[0] != '/') {
            uri.append(("/"));
        }
        uri.append(path_);
    } else {
        uri.append("/");
    }

    if (!parameters_.empty()) {
        uri.append(";").append(parameters_);
    }

    if (!query_.empty()) {
        std::map<std::string, std::string>::const_iterator it;

        uri.append("?");

        for (it = query_.begin(); it != query_.end(); ++it) {
            uri.append(it->first).append("=").append(it->second).append("&");
        }

        //uri_.pop_back();
        uri.erase(uri.size() - 1);
    }

    if (!fragment_.empty()) {
        uri.append("#").append(fragment_);
    }

    return uri;
}

void uuri::analyse()
{
    //url format:protocol://hostname[:port]/path/[;parameters][?query]#fragment
    //uri analyse part: /path/[;parameters][?query]#fragment

    if (uri_.empty()) {
        return;
    }
    
    const char * begin = uri_.data();
    const char * end   = begin + uri_.size();
    const char * pos   = begin;
    
    // 1. get path.
    while (pos < end && (*pos != ';' && *pos != '?' && *pos != '#')) {
        ++ pos;
    }
    
    if (pos != begin) {
        std::string(begin, pos - begin).swap(path_);
    } else {
        set_string(path_, "/");
    }

    // 2. get parameters
    if (pos < end && *pos == ';') {
        begin = pos + 1;
        pos   = begin;
        
        while (pos < end && (*pos != '?' && *pos != '#')) {
            ++ pos;
        }
        if (pos != begin) {
            std::string(begin, pos - begin).swap(parameters_);
        }
    }

    // 3. get query
    if (pos < end && *pos == '?') {
        
        while (pos < end && (*pos != '#')) {

            begin = pos + 1;
            pos   = begin;
        
            while (pos < end && (*pos != '=' && *pos != '#')) {
                ++ pos;
            }

            if (*pos == '#') {
                break;
            }
            
            std::string name(begin, pos - begin);

            begin = pos + 1;
            pos   = begin;
            
            while (pos < end && (*pos != '&' && *pos != '#')) {
                ++ pos;
            }

            std::string value(begin, pos - begin);

            //printf("find query: (%s) = (%s)\n", name.c_str(), value.c_str());
            if (!name.empty()) {
                query_[name] = value;
            }
        }
    }

    // 4. get query
    if (pos < end && *pos == '#') {
        begin = pos + 1;
        pos   = end;
        
        if (pos != begin) {
            std::string(begin, pos - begin).swap(fragment_);
        }
    }
}

void uuri::clear()
{
    std::string().swap(uri_);
    std::string().swap(path_);
    std::string().swap(parameters_);
    std::map<std::string, std::string>().swap(query_);
    std::string().swap(fragment_);
}

