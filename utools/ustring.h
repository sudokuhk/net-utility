/*************************************************************************
    > File Name: ustring.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Wed Dec 14 14:49:06 CST 2016
 ************************************************************************/

#ifndef __U_STRING_H__
#define __U_STRING_H__

#include <string>
#include <vector>

/*
 * split str by sep. trim whitespace.
 * put result into out.
 */
void split(const char * str, char sep, std::vector<std::string> & out);

/*
 * is character is ' ' or '\0' or '\n' or '\r'.
 */
bool iswhitespace(char c);

void trimleft(const char * & left, const char * & right);
void trimright(const char * & left, const char * & right);
void trim(const char * & left, const char * & right);
void set(char * const p, char v);

#endif
