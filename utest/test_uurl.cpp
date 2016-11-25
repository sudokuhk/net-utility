#include "uhttp/uurl.h"

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
    "https://www.baidu.com/s?ie=utf-8&f=8&rsv_bp=1&rsv_idx=2&tn=baiduhome_pg&wd=%E9%A2%86%E6%B6%A8%20%E5%8D%95%E8%AF%8D&rsv_spt=1&oq=url%20%E5%8D%8F%E8%AE%AE&rsv_pq=e0a914fe0000a885&rsv_t=cf67aITyWkUMNGxSzlg59WuEIPTC3ITQFz5m5iZbuh%2BopBNBjyBNUB7BKC5N%2FdfI3vIR&rqlang=cn&rsv_enter=1&rsv_sug3=24&rsv_sug1=17&rsv_sug7=100&rsv_sug2=0&inputT=3519&rsv_sug4=3520",
    "http://1.1.1.1/opac/reader/doLogin",
    "http://db2015.wstock.cn/wsDB_API/kline.php?symbol=SH510500&desc=1&q_type=0&r_type=2&etime=2016-11-17&stime=2016-11-17&u=test&p=8e6a",
    NULL,
};

void parse(const char * surl)
{
    std::cout << ">>>>>>>parse <" << surl << ">" << std::endl;
    uurl url(surl);

    if (url.protocol() == uprotocol_unknown) {
        std::cout << "parse failed! > " << surl << std::endl;
        return;
    }

    std::cout << "protocol:" << url.protocol() << std::endl;
    std::cout << "host:" << url.host() << std::endl;
    std::cout << "port:" << url.port() << std::endl;

    const uuri & uri = url.uri();
    std::cout << "path:" << uri.path() << std::endl;
    std::cout << "parameters:" << uri.parameters() << std::endl;

    const std::map<std::string, std::string> & query = uri.query();
    std::map<std::string, std::string>::const_iterator it;
    
    for (it = query.begin(); it != query.end(); ++ it) {
        std::cout << it->first << " = " << it->second << std::endl;
    }
    
    std::cout << "fragment:" << uri.fragment() << std::endl;
    std::cout << "uri: " << uri.get() << std::endl;
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
