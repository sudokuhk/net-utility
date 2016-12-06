#ifndef _U_URI_H__
#define _U_URI_H__

#include <map>
#include <string>

// url format:protocol://hostname[:port]/path/[;parameters][?query]#fragment
class uuri
{
public:
    typedef std::map<std::string, std::string> query_map;
    
public:
    uuri();

    uuri(const char * uri);

    ~uuri();

    void set(const char * uri);

    std::string get() const;

    const std::string & path() const;

    const std::string & parameters() const;

    const std::map<std::string, std::string> & query() const;

    const std::string & fragment() const;

    void set_path(const char * path);

    void set_parameters(const char * parameters);

    void set_query(const char * name, const char * value);

    void set_query(const char * name, const int value);

    void set_fragment(const char * fragment);

    void set_fragment(int fragment);

    void clear();
private:
    void analyse();

    void set_string(std::string & str, const char * value);

    std::string conbine() const;
    
private:
    std::string uri_;
    std::string path_;
    std::string parameters_;
    std::map<std::string, std::string> query_;
    std::string fragment_;
};

#endif
