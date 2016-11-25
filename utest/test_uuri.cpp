#include "uhttp/uuri.h"

#include <iostream>

/*
./test_uuri "/index?a=1&%20b=2&c=3&d=http://192.168.124.128&e=#1234"
./test_uuri "/index;abcd?a=1&%20b=2&c=3&d=http://192.168.124.128&e=#1234"
./test_uuri "/index;?a=1&%20b=2&c=3&d=http://192.168.124.128&e=#1234"
./test_uuri "/index;?#1234"
./test_uuri "/index;?a=1&%20b=2&c=3&d=http://192.168.124.128&e=#1234"
./test_uuri ";?#"
./test_uuri ";?a=#"
*/
const char * cases[] =
{
    "/",
    "/index",
    "/index.jsp",
    "/;",
    "/;parameters",
    "/;?",
    "/;?a",
    "/;?a=",
    "/;?a=1",
    "/;?a=1&b",
    "/;?a=1&b=",
    "/;?a=1&b=c=",
    "/;?a=1&b=c=2",
    "/;?a=1&b=#",
    "/;?a=1&b=#fragment",
    ";?a=1&b=#fragment",
    "?a=1&b=#fragment",
    "a=1&b=#fragment",
    "#fragment",
    NULL,
};

void parse(const char * suri)
{
    std::cout << ">>>>>>>parse <" << suri << ">" << std::endl;
    uuri uri;
    uri.set(suri);

    std::cout << "path:" << uri.path() << std::endl;
    std::cout << "parameters:" << uri.parameters() << std::endl;

    const std::map<std::string, std::string> & query = uri.query();
    std::map<std::string, std::string>::const_iterator it;
    
    for (it = query.begin(); it != query.end(); ++ it) {
        std::cout << it->first << " = " << it->second << std::endl;
    }
    
    std::cout << "fragment:" << uri.fragment() << std::endl;
    std::cout << "<<<<================" << std::endl;
}

int main(int argc, char * argv[])
{
    const char ** ptr = cases;
    while (*ptr != NULL) {
        parse(*ptr);
        ++ ptr;
    }
    
    return 0;
}
