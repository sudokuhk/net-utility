/*************************************************************************
    > File Name: udnsresolver.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Friday, January 06, 2017 PM04:59:58 CST
 ************************************************************************/

#ifndef __U_DNS_RESOLVER_H__
#define __U_DNS_RESOLVER_H__

//#include "uschedule.h"

#include <map>
#include <string>
#include <vector>
#include <list>
#include <set>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

class uudpsocket;
class uschedule;

class udnsresolver
    //: public utask
{
private:
    typedef struct record {
        uint32_t     ttl;
        uint32_t     query;
        std::string  ip;

        friend bool operator<(const record & lhs, const record & rhs) {
            return lhs.ip < rhs.ip;
        }
    } record_t;
    
public:
    typedef std::vector<std::string> iparray_type;
    /*
    class ucallback {
    public:
        virtual ~ucallback() {}

        virtual void on_resolved(int code, const std::string & domain, 
            const iparray_type & ips) = 0;
    };
    */
public:
    udnsresolver(uschedule & sched, int timeo = 30 * 1000/*ms*/);

    //virtual ~udnsresolver();
    ~udnsresolver();

    //virtual void run();

    //void query(const std::string & domain, ucallback & cb);
    iparray_type query(const std::string & domain);

private:
    bool send_request(const std::string & domain);
    iparray_type recv_request(const std::string & domain);
    int get_record(const uint8_t * pb, ssize_t & off, 
        std::string & name, record_t & info);
    void get_name(const uint8_t * pb, ssize_t & off, std::string & name);
    void get_name(const uint8_t * pb, int len, std::string & name);
    
private:
    uschedule & sched_;
    uudpsocket * sock_;
    int timeo_;
    //std::string domain_;
    
private:
    static bool get_cache(const std::string & domain, iparray_type & ips);
    static std::string get_server();
    static void load_config();
    static void update_dns(const std::string & domain, std::set<record_t> & res);
    
private:    
    static pthread_mutex_t lock_;
    static std::map<std::string, std::set<record_t> > cache_;
    static std::list<std::string> server_;
    static struct stat sb_;     // /etc/resolve.conf
    //std::map<std::string, ucallback &>   cb_;
};

#endif
