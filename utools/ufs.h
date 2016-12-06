/*************************************************************************
    > File Name: ufs_tools.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Nov 28 21:37:09 CST 2016
 ************************************************************************/

#ifndef __U_FS_TOOLS_H__
#define __U_FS_TOOLS_H__

#include <unistd.h>
#include <string>

int makedir(const char * newdir);

int isdir(const char * dirname);

off_t isreg(const char * fname);

std::string combine_path(const std::string & first, const std::string & second);

#endif
