/*************************************************************************
    > File Name: uimgio.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Dec  5 13:38:44 CST 2016
 ************************************************************************/

#include "uimgio.h"

#include <utools/ufs.h>
#include <ulog/ulog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

uimgio::uimgio(const std::string & rootpath, int pathdepth, int spathsize)
    : rootpath_(rootpath)
    , pathdepth_(pathdepth)
    , pathsize_(spathsize)
{
    if (pathsize_ > MD5_LEN) {
        pathsize_ = MD5_LEN;
    } else if (pathsize_ <= 0) {
        pathsize_ = DEFAULT_PATHSIZE;
    }

    if (pathdepth_ > MD5_LEN) {
        pathdepth_ = MD5_LEN;
    } else if (pathdepth_ <= 0) {
        pathdepth_ = DEFAULT_PATH_DEPTH;
    }

    while (pathdepth_ * pathsize_ > MD5_LEN) 
        pathsize_--;
}

uimgio::~uimgio()
{
}

int uimgio::save(const std::string & md5, const char * data, size_t len)
{
    if (data == NULL || len == 0) {
        ulog(ulog_error, "save, invalid input param!!, md5:[%s], len:%ld\n",
            md5.c_str(), len);
        return -1;
    }

    std::string subpath(getpath(md5));
    std::string realpath(rootpath_);
    realpath.append("/").append(subpath);

    ulog(ulog_debug, "md5:%s, save path:%s.\n", md5.c_str(), realpath.c_str());

    /*if (isdir(realpath.c_str()) < 0) {
        ulog(ulog_error, "save path:%s not directory.\n", realpath.c_str());
        return -1;
    } */    
    
    if (makedir(realpath.c_str()) < 0) {
        ulog(ulog_error, "makedir [%s] failed!\n", realpath.c_str());
        return -1;
    }

    realpath.append("0*0");
    ulog(ulog_debug, "origin file:%s.\n", realpath.c_str());

    if (isreg(realpath.c_str()) > 0) {
        ulog(ulog_info, "file[%s] exits!.\n", realpath.c_str());
        return 0;
    }

    if (write(realpath, data, len) != 0) {
        ulog(ulog_error, "write file[%s] failed!.\n", realpath.c_str());
        return -1;
    }

    return 0;    
}

int uimgio::get(const std::string & md5, 
    const std::map<std::string, std::string> & params, std::string & content)
{
    std::string subpath(getpath(md5));
    std::string realpath(rootpath_);
    realpath.append("/").append(subpath).append("0*0");;
    int ret = -1;

    ulog(ulog_debug, "md5:%s, get path:%s.\n", md5.c_str(), realpath.c_str());

    off_t size = isreg(realpath.c_str());
    if (size < 0) {
        ulog(ulog_error, "file[%s] not exits!.\n", realpath.c_str());
        ret = -1;
    } else {
        content.resize(size);
        ret = read(realpath, content);
    }
    
    return ret;
}

std::string uimgio::getpath(const std::string & md5)
{
    int off = 0;
    std::string dirs;
    
    for (int i = 0; i < pathdepth_; i++) {
        dirs.append(md5.substr(off, pathsize_)).append("/");
        off += pathsize_;
    }
    dirs.append(md5).append("/");

    return dirs;
}

int uimgio::write(const std::string & fname, const char * data, size_t len)
{
    int ret      = 0;
    ssize_t wret = 0;
    
    int fd = open(fname.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd < 0) {
        ret = -1;
        goto out;
    }

    if(flock(fd, LOCK_EX | LOCK_NB) == -1) {
        ret = 0;
        goto out;
    }

    wret = ::write(fd, data, len);
    if (wret < 0) {
        ulog(ulog_error, "write (%s) failed!\n", fname.c_str());
        ret = -1;
    } else if ((size_t)wret != len) {
        ulog(ulog_error, "write (%s) part.., as failed!\n", fname.c_str());
        ret = -1;
    }

    flock(fd, LOCK_UN | LOCK_NB);
    ulog(ulog_info, "write (%s) done!\n", fname.c_str());
    
    ret = 0;
    
out:
    if (fd > 0) {
        close(fd);
    }
    return ret;
}

int uimgio::read(const std::string & fname, std::string & buf)
{
    int ret = -1;
    int fd = open(fname.c_str(), O_RDONLY);
    
    if (fd < 0) {
        ulog(ulog_error, "file[%s] open failed!!\n", fname.c_str());
        ret = -2;
        goto out;
    }
    
    if (::read(fd, &buf[0], buf.size()) != (ssize_t)buf.size()) {
        ulog(ulog_error, "file[%s] read part, failed!!\n", fname.c_str());
        ret = -3;
        goto out;
    }

    ret = 0;
out:
    if (fd > 0) {
        close(fd);
    }

    if (ret != 0) {
        buf.clear();
    }

    return ret;
}
