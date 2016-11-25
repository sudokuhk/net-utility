/*************************************************************************
    > File Name: test_tcp_streambuf.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Tue Sep 20 10:21:53 CST 2016
 ************************************************************************/

#include "ustream/basic_tcp_streambuf.h"

//#include <stdio.h>

#ifndef _TEST_TCP_STREAMBUF_H__
#define _TEST_TCP_STREAMBUF_H__

class test_tcp_streambuf
	: public basic_tcp_streambuf
{
public:
	test_tcp_streambuf(size_t bufsize)
		: basic_tcp_streambuf(bufsize)
	{
	}

	~test_tcp_streambuf() {}

protected:
	virtual ssize_t precv(void * buf, size_t len, int flags)
	{
		ssize_t nrecv = read(0, buf, len);
		//printf("precv, nread:%zd!\n", nrecv);
		return nrecv;
	}
	
	virtual ssize_t psend(const void * buf, size_t len, int flags)
	{
		ssize_t nsend = write(1, buf, len);
		//printf("psend, nsend:%zd!\n", nsend);
		return nsend;
	}
};

#endif