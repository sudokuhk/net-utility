#ifndef _BLOCK_TCP_STREAM_H__
#define _BLOCK_TCP_STREAM_H__

#include "basic_tcp_stream.h"
#include "block_tcp_streambuf.h"

class block_tcp_stream
	: public basic_tcp_stream
{
public:
	block_tcp_stream(size_t bufsize)
		: basic_tcp_stream(bufsize)
	{
	}

	virtual ~block_tcp_stream() {}

	void attach(int socket)
	{
		socket_ = socket;

		delete rdbuf(new block_tcp_streambuf(socket, buf_size_));
	}

	virtual bool set_timeout(int ms)
	{
		if (socket() < 0)
	        return false;

	    struct timeval to;
	    to.tv_sec 	= ms / 1000;
	    to.tv_usec 	= (ms % 1000) * 1000;

	    if (setsockopt(socket(), SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) < 0) {
	        return false;
	    }

	    if (setsockopt(socket(), SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to)) < 0) {
	        return false;
	    }

	    return true;
	}

	virtual int last_error()
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return 1;
		}
		return 2;
	}

protected:
  	virtual int socket() 
	{
		return socket_;
	}
	
private:
	int socket_;
};

#endif