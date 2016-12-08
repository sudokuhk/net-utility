/*************************************************************************
    > File Name: umd5sum.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Nov 28 21:37:09 CST 2016
 ************************************************************************/

#ifndef __U_MD5SUM_H__
#define __U_MD5SUM_H__

#include <string>
#include <fstream>

/* Type define */
typedef unsigned char byte;
typedef unsigned int uint32;

/* umd5sum declaration. */
class umd5sum {
public:
    static bool ismd5(const std::string & md5);
    
public:
	umd5sum();
	umd5sum(const void *input, size_t length);
	umd5sum(const std::string &str);
	umd5sum(std::ifstream &in);
    
	void update(const void *input, size_t length);
	void update(const std::string &str);
	void update(std::ifstream &in);
	const byte* digest();
	std::string toString();
	void reset();
    
private:
	void update(const byte *input, size_t length);
	void final();
	void transform(const byte block[64]);
	inline void encode(const uint32 *input, byte *output, size_t length);
	inline void decode(const byte *input, uint32 *output, size_t length);
	std::string bytesToHexString(const byte *input, size_t length);

	/* class uncopyable */
	umd5sum(const umd5sum&);
	umd5sum& operator=(const umd5sum&);
private:
	uint32 _state[4];	/* state (ABCD) */
	uint32 _count[2];	/* number of bits, modulo 2^64 (low-order word first) */
	byte _buffer[64];	/* input buffer */
	byte _digest[16];	/* message digest */
	bool _finished;		/* calculate finished ? */

	static const byte PADDING[64];	/* padding for calculate */
	static const char HEX[16];
	static const size_t BUFFER_SIZE = 1024;
};

#endif/*MD5_H*/

