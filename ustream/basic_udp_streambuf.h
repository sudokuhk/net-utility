/*************************************************************************
    > File Name: basic_udp_streambuf.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Tue Sep 20 07:38:05 CST 2016
 ************************************************************************/
 
#ifndef __BASIC_UDP_STREAMBUF_H__
#define __BASIC_UDP_STREAMBUF_H__

#include <iostream>
#include <unistd.h>

class basic_udp_streambuf
	: public std::streambuf
{
public:
	basic_udp_streambuf(ssize_t buf_size)
		: buf_size_(buf_size)
	{
		/*
		 * we need two buffer: put buffer & get buffer.
		 * gback() return start of get buffer.
		 * egptr() return end of get buffer.
		 * pbase() return start of put buffer.
		 * epptr() return end of put buffer.
		 * pptr() and gptr() return current position of put/get buffer.
		 */
		char * gbuf = new char[buf_size_];
		char * pbuf = new char[buf_size_];

		//void setg (char* gbeg, char* gnext, char* gend);
		//gback(), gptr(), egptr()
		setg(gbuf, gbuf, gbuf);

		//void setp (char* new_pbase, char* new_epptr);
		//pbase(), epptr().
		//pptr() equals to pbase().
		setp(pbuf, pbuf + buf_size_);
	}

	virtual ~basic_udp_streambuf()
	{
		delete [] eback();
		delete [] pbase();
	}

	/*
	 * put buffer comes full.
	 * call if sputs failed.
	 */
	virtual traits_type::int_type 
		overflow(traits_type::int_type c = traits_type::eof())
	{
		if (sync() == -1) {
			return traits_type::eof();
		} 
		
		if (traits_type::eq_int_type(traits_type::eof(), c)) {
			return traits_type::not_eof(c);
		}

		sputc(traits_type::to_char_type(c));

		return c;
	}

	virtual traits_type::int_type underflow()
	{
		ssize_t nrecv = precv(eback(), buf_size_, 0);
		if (nrecv <= 0) {
			return traits_type::eof();
		}
		
		setg(eback(), eback(), eback() + nrecv);
		
		return traits_type::to_int_type(*gptr());
	}

	virtual int sync()
	{
		ssize_t nsend = 0;
		ssize_t total = pptr() - pbase();
		ssize_t once  = 0;

		//flush all data to network.
		while (nsend < total) {
			once = psend(pbase() + nsend, (total - nsend), 0);
            
			if (once <= 0) {
				return -1;
			}
			nsend += once;
		}
        
		//clear.
		setp(pbase(), pbase() + buf_size_);
		pbump(0);

		return 0;
	}

protected:
	virtual ssize_t precv(void * buf, size_t len, int flags) = 0;
	
	virtual ssize_t psend(const void * buf, size_t len, int flags) = 0;

protected:
	const ssize_t buf_size_;
};

#endif
