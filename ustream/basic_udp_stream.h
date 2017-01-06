#ifndef _BASIC_UDP_STREAM_H__
#define _BASIC_UDP_STREAM_H__

#include "basic_udp_streambuf.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <errno.h>
#include <string.h>

class basic_udp_stream
	: public std::iostream
{
public:
	basic_udp_stream(size_t bufsize = 1024)
		: buf_size_(bufsize)
	{
	}

	virtual ~basic_udp_stream()
	{
		delete rdbuf();
	}

	void new_rdbuf(basic_udp_streambuf * buf)
	{
		std::streambuf * old = rdbuf(buf);
		delete old;
	}

	virtual void remote_info(char * ipb, size_t ipl, int * port) = 0;

	virtual bool set_timeout(int ms) = 0;

	virtual int last_error() = 0;

  	virtual int socket() = 0;
    
protected:
	const size_t buf_size_;
};

#endif