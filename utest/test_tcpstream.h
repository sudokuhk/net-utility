/*************************************************************************
    > File Name: test_tcp_stream.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Tue Sep 20 10:21:53 CST 2016
 ************************************************************************/

#include "ustream/basic_tcp_stream.h"

#include <stdio.h>

#ifndef _TEST_TCP_STREAM_H__
#define _TEST_TCP_STREAM_H__

class test_tcp_stream
	: public basic_tcp_stream
{
public:
	test_tcp_stream(size_t bufsize)
		: basic_tcp_stream(bufsize)
	{
	}

	~test_tcp_stream() {}

	virtual bool set_timeout(int ms)
	{
		return true;
	}

	virtual int last_error()
	{
		return 0;
	}

protected:
  	virtual int socket() 
	{
		return 0;
	}
};

#endif