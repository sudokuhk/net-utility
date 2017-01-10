/*************************************************************************
    > File Name: udnsresolver.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Friday, January 06, 2017 PM05:00:01 CST
 ************************************************************************/

#include "udnsresolver.h"
#include "uudpsocket.h"
#include "uschedule.h"
#include "utools/ustring.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include <iostream>
#include <arpa/inet.h>
#include <stdio.h>

#define U_DNS_CONFIG_FILE   "/etc/resolv.conf"
#define U_DNS_DEF_SERVER    "8.8.8.8"
#define U_DNS_SERVER_PORT   53

pthread_mutex_t udnsresolver::lock_ = PTHREAD_MUTEX_INITIALIZER;
std::map<std::string, std::set<udnsresolver::record_t> > udnsresolver::cache_;
std::list<std::string> udnsresolver::server_;
struct stat udnsresolver::sb_;

struct dns_proto_header
{
    uint16_t    id;

    uint8_t     rd : 1;
    uint8_t     tc : 1;
    uint8_t     aa : 1;
    uint8_t     opcode : 4;
    uint8_t     qr : 1;

    uint8_t     rcode : 4;
    uint8_t     z : 3;
    uint8_t     ra : 1;

    uint16_t    qdcount;
    uint16_t    ancount;
    uint16_t    nscount;
    uint16_t    arcount;
};

udnsresolver::udnsresolver(uschedule & sched, int timeo)
    //: utask()
    : sched_(sched)
    , sock_(new uudpsocket(sched_, timeo_))
    //, cache_()
    //, cb_()
{

}

udnsresolver::~udnsresolver()
{
    delete sock_;
    sock_ = NULL;
}

udnsresolver::iparray_type udnsresolver::query(const std::string & domain)
{
    iparray_type ips;
    if (!get_cache(domain, ips)) {
        //domain_ = domain;
        //sched_.add_task(this);
        std::string server = get_server();
        sock_->connect(server.c_str(), U_DNS_SERVER_PORT);

        if (send_request(domain)) {
            ips = recv_request(domain);
        }
    }
    //get_cache(domain, ips);

    return ips;
}

bool udnsresolver::send_request(const std::string & domain)
{
    std::vector<uint8_t> data;
    data.resize(1024);
    size_t size = 0;

    uint8_t * pbuf = &data[0];
    dns_proto_header * h = (dns_proto_header *)pbuf;
    memset(h, 0, sizeof(dns_proto_header));

    h->id       = time(NULL) & 0xFFFF;
    h->rd       = 1;
    
    h->qdcount  = htons(1);
    
    size += sizeof(dns_proto_header);
    
    std::vector<std::string> vbuf;
    split(domain.c_str(), '.', vbuf);

    for (size_t i = 0; i < vbuf.size(); i++) {
        size_t cnt = vbuf[i].size();
        *(pbuf + size) = cnt;
        size ++;
        
        memcpy((pbuf + size), vbuf[i].c_str(), cnt);
        size += cnt;
    }
    *(pbuf + size) = 0;
    size ++;

    // qtype = ipv4
    *(uint16_t *)(pbuf + size) = htons(1);
    size += 2;

    //qclass = inet
    *(uint16_t *)(pbuf + size) = htons(1);
    size += 2;

    return sock_->send(pbuf, size, 0) == (ssize_t)size;
}

udnsresolver::iparray_type udnsresolver::recv_request(const std::string & domain)
{
    iparray_type ips;
    
    std::vector<uint8_t> buf;
    buf.resize(4096);
    uint8_t * pbuf = &buf[0];
    ssize_t off = 0;
    time_t now = time(NULL);
    std::string name;
    record_t r;
    r.query= now;

    if (sock_->recv(pbuf, buf.size(), 0) > 0) {
        dns_proto_header * h = (dns_proto_header *)pbuf;
        off += sizeof(dns_proto_header);

        if (h->qr == 1 && h->rcode == 0) { // no error!
            std::set<record_t> result;

            h->qdcount = ntohs(h->qdcount);
            h->ancount = ntohs(h->ancount);
            h->nscount = ntohs(h->nscount);
            h->arcount = ntohs(h->arcount);

            if (h->qdcount > 0) {
                for (uint16_t i = 0; i < h->qdcount; i++) {
                    get_name(pbuf, off, name);
                    off += 2; //skip qtype
                    off += 2; //skip qclass
                }
            }
            
            if (h->ancount > 0) {
                for (uint16_t i = 0; i < h->ancount; i++) {
                    get_record(pbuf, off, name, r);
                    result.insert(r);
                    ips.push_back(r.ip);
                }
            }

            if (h->nscount > 0) {
                for (uint16_t i = 0; i < h->nscount; i++) {
                    get_record(pbuf, off, name, r);
                    result.insert(r);
                    ips.push_back(r.ip);
                }
            }

            if (h->arcount > 0) {
                for (uint16_t i = 0; i < h->arcount; i++) {
                    get_record(pbuf, off, name, r);
                    result.insert(r);
                    ips.push_back(r.ip);
                }
            }
			
            update_dns(domain, result);
        }
    }    

    return ips;
}

void udnsresolver::get_record(const uint8_t * pb, ssize_t & off, 
        std::string & name, record_t & r)
{
    get_name(pb, off, name);

    off += 2; // type
    off += 2; // class
    
    uint32_t ttl  = ntohl(*(uint32_t *)(pb + off));
    off += 4; // ttl
    
    //uint16_t rdlength = ntohs(*(uint16_t *)(pb + off));
    off += 2; //rdlength

    uint32_t rdata = ntohl(*(uint32_t *)(pb + off));
    off += 4;

    r.ip  = ip2str(rdata);
    r.ttl = ttl;

    //printf("get [%s] ip[%s] ttl[%u]\n", name.c_str(), r.ip.c_str(), r.ttl);
}

void udnsresolver::get_name(const uint8_t * pb, ssize_t & off, 
    std::string & name)
{
    uint8_t pflag = *(pb + off);
    const uint8_t * pbegin = NULL;
    bool isptr = false;

    if (pflag & 0xC0) { //ptr
        uint16_t poff = ntohs(*(uint16_t *)(pb + off));
        off += 2;
        pbegin = pb + poff;
        isptr = true;
    } else {
        pbegin = pb + off;
    }
    
    uint8_t size = 0;
    
    while (*pbegin != 0) {
        size = *pbegin ++;
        name.append((const char *)pbegin, size).append(".");
        pbegin += size;
    }
    name[name.size() - 1] = 0;

    if (!isptr) {
        off += name.size() + 1;
    }
}

/*
void udnsresolver::run()
{
    std::string server = get_server();
    sock_->connect(server.c_str(), U_DNS_SERVER_PORT);

    if (send_request(domain_)) {
        recv_request(domain_);
    }
}
*/

bool udnsresolver::get_cache(const std::string & domain, iparray_type & ips)
{
    typedef std::map<std::string, 
        std::set<udnsresolver::record_t> >::iterator cacheit;

    pthread_mutex_lock(&lock_);
    bool ok = false;
    cacheit it = cache_.find(domain);
    if (it != cache_.end()) {
        std::set<record_t> & records = it->second;
        time_t now = time(NULL);

        if (!records.empty()) {
            std::set<record_t>::const_iterator rit = records.begin();
            while (rit != records.end()) {
                const record_t & r = *rit;
                if (now - r.query >= r.ttl) {
                    records.erase(rit ++);
                    continue;
                }
                ips.push_back(r.ip);
                ++ rit;
                ok = true;
            }
        }
    }
    pthread_mutex_unlock(&lock_);
    
    return ok;
}

void udnsresolver::update_dns(const std::string & domain, std::set<record_t> & res)
{
    if (domain.empty() || res.empty()) {
        return;
    }
    
    pthread_mutex_lock(&lock_);
    cache_[domain] = res;
    pthread_mutex_unlock(&lock_);
}

std::string udnsresolver::get_server()
{
    std::string server;

    struct stat sb;
    int iret = ::stat(U_DNS_CONFIG_FILE, &sb);

    pthread_mutex_lock(&lock_);
    if (server_.empty() || (iret == 0 && sb_.st_mtime != sb.st_mtime)) {
        sb_ = sb;
        load_config();
    } 

    if (server_.empty()) {
        server = U_DNS_DEF_SERVER;
    } else {
        server = *server_.begin();
    }
    pthread_mutex_unlock(&lock_);
    
    return server;
}

void udnsresolver::load_config()
{
    ssize_t ret;
    
    int fd = open(U_DNS_CONFIG_FILE, O_RDONLY);
    if (fd < 0) {
        return;
    }

    std::string buf;
    buf.resize(sb_.st_size + 1);
    ret = read(fd, &buf[0], sb_.st_size);
    close(fd);

    if (ret > 0) {
        std::stringstream stream(buf/*, ios_base::in*/);

        std::string line;
        std::string server;
        
        while (stream >> line) {
            if (line == "nameserver") {
                stream >> server;
                server_.push_back(server);
            }
        }
    }
}
