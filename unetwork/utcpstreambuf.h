#ifndef _U_TCP_STREAMBUF_H__
#define _U_TCP_STREAMBUF_H__

#include "ustream/basic_tcp_streambuf.h"

class uschedule;
class utcpsocket;

class utcpstreambuf 
    : public basic_tcp_streambuf
{
public:
    utcpstreambuf(utcpsocket * socket, uschedule * sched, size_t buf_size);

    virtual ~utcpstreambuf();

    virtual ssize_t precv(void * buf, size_t len, int flags);
	
	virtual ssize_t psend(const void * buf, size_t len, int flags);

private:
    utcpsocket * socket_;
    uschedule * schedule_;
};

#endif