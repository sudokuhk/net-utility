#ifndef _BASIC_TCP_STREAM_H__
#define _BASIC_TCP_STREAM_H__

#include "basic_tcp_streambuf.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <errno.h>
#include <string.h>

class basic_tcp_stream
	: public std::iostream
{
public:
	basic_tcp_stream(size_t bufsize = 1024)
		: buf_size_(bufsize)
	{
	}

	virtual ~basic_tcp_stream()
	{
		delete rdbuf();
	}

	void new_rdbuf(basic_tcp_streambuf * buf)
	{
		std::streambuf * old = rdbuf(buf);
		delete old;
	}

	bool remote_info(char * ipb, size_t ipl, int * port)
	{	
		if (socket() <= 0) {
			return false;
		}
		
		struct sockaddr_in addr;
	    socklen_t slen = sizeof(addr);
	    memset(&addr, 0, sizeof(addr));

	    int ret = getpeername(socket(), (struct sockaddr*) &addr, &slen);

	    if (0 == ret) {
	        inet_ntop(AF_INET, &addr, ipb, ipl);
	        if (NULL != port)
	            *port = ntohs(addr.sin_port);
	    }

	    return 0 == ret;
	}

	virtual bool set_timeout(int ms) = 0;

	virtual int last_error() = 0;

  	virtual int socket() = 0;
    
protected:
	const size_t buf_size_;
};

#endif