#ifndef _BLOCK_TCP_STREAMBUF__
#define _BLOCK_TCP_STREAMBUF__

#include "basic_tcp_streambuf.h"

class block_tcp_streambuf
	: public basic_tcp_streambuf
{
public:
	block_tcp_streambuf(int socket, size_t bufsize) 
		: basic_tcp_streambuf(bufsize)
		, socket_(socket)
	{
	}

	virtual ~block_tcp_streambuf() {}

protected:
	virtual ssize_t precv(void * buf, size_t len, int flags) 
	{
		ssize_t nrecv = ::recv(socket_, buf, len, flags);
		return nrecv;
	}
	
	virtual ssize_t psend(const void * buf, size_t len, int flags)
	{
		return ::send(socket_, buf, len, flags);
	}

private:
	int socket_;
};

#endif