/*************************************************************************
    > File Name: uimgio.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Dec  5 13:38:23 CST 2016
 ************************************************************************/

#ifndef __UIMG_IO_H__
#define __UIMG_IO_H__

#include <string>
#include <map>

#define     MD5_LEN             32
#define     DEFAULT_PATHSIZE    3
#define     DEFAULT_PATH_DEPTH  3

class uimgio
{
public:
    uimgio(const std::string & rootpath, int pathdepth = DEFAULT_PATH_DEPTH, 
        int spathsize = DEFAULT_PATHSIZE);

    ~uimgio();

    int save(const std::string & md5, const char * data, size_t len);

    int get(const std::string & md5, 
        const std::map<std::string, std::string> & params, 
        std::string & content);

private:
    std::string getpath(const std::string & md5);

    int write(const std::string & fname, const char * data, size_t len);
    int read(const std::string & fname, std::string & buf);
private:
    std::string rootpath_;
    int         pathdepth_;
    int         pathsize_;
};

#endif
