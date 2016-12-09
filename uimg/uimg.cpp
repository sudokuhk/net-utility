/*************************************************************************
    > File Name: uimg.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Nov 28 13:55:12 CST 2016
 ************************************************************************/

#include "uimg.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/time.h>

#include <ulog/ulog.h>
#include <utools/ufs.h>
#include "uimgserverunit.h"

uimg_server * uimg_server::sinstance = NULL;

void uimg_server::uimg_log_hook(int level, const char * fmt, va_list valist)
{
    sinstance->log(level, fmt, valist);
}

uimg_server::uimg_server(uimg_conf_t & config)
    : config_(config)
    , listen_fd_(-1)
    , log_fd_(-1)
    , log_buf_(NULL)
    , log_buf_size_(2048)
    , last_logfile_t_(0)
    , server_units_()
{
    sinstance   = this;
    log_buf_    = (char *)malloc(log_buf_size_);
}

uimg_server::~uimg_server()
{
    sinstance = NULL;
    free(log_buf_);

    if (log_fd_ > 0) {
        close(log_fd_);
    }

    if (listen_fd_ > 0) {
        close(listen_fd_);
    }
}

bool uimg_server::run()
{
    //init log.
    std::string logfile = config_.log_path; 
    if (makedir(logfile.c_str())) {
        fprintf(stderr, "create log director:%s failed!\n", logfile.c_str());
        return false;
    }
    
    setulog(uimg_server::uimg_log_hook);
    ulog(ulog_info, "init log done!\n");

    if (!listen()) {
        ulog(ulog_error, "listen %s:%d failed!\n", 
            config_.server_ip.c_str(), config_.server_port);
        return false;
    }

    server_units_.resize(config_.threads);
    for (int i = 0; i < config_.threads; i++) {
        server_units_[i] = new uimgserverunit(config_);
        server_units_[i]->start();
    }

    run_forever();

    return true;
}

void uimg_server::run_forever()
{
    struct sockaddr_in  addr;
    socklen_t           addr_len = sizeof(addr);
    int fd;
    int idx = 0;
    
    while (1) {
        fd = accept(listen_fd_, (struct sockaddr *)&addr, &addr_len);

        if (fd > 0) {
            ulog(ulog_info, "accept client, fd:%d, info:[%s:%d]\n",
                fd, inet_ntoa(addr.sin_addr) , ntohs(addr.sin_port));

            int workerid = idx ++ % server_units_.size();
            server_units_[workerid]->accept(fd, inet_ntoa(addr.sin_addr));
        }
    }
}

bool uimg_server::listen()
{
    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(config_.server_ip.c_str());
    server_addr.sin_port        = htons(config_.server_port);
 
    listen_fd_= socket(PF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        return false;
    }

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

     
    if (bind(listen_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        return false;
    }
    
    if (::listen(listen_fd_, 100)) {
        return false;
    }

    return true;
}

bool uimg_server::reopen_log()
{
    if (log_fd_ > 0) {
        close(log_fd_);
        log_fd_ = -1;
    }

    std::string logfile = config_.log_path;
    logfile.append("/").append(config_.log_filename);

    int openflag = O_CREAT;
    
    if (access(logfile.c_str(), F_OK) == 0) {
        
        time_t now = time(NULL);
        
        struct stat sb;
        if ((last_logfile_t_ != 0 && last_logfile_t_ / 86400 != now / 86400) ||
            (!stat(logfile.c_str(), &sb) && 
                now / 86400 != sb.st_mtime / 86400 && 
                now > sb.st_mtime)) {
            
            struct tm tm_time;
            gmtime_r(&sb.st_mtime, &tm_time);
        
            char buf[20];
            strftime(buf, sizeof(buf), "%Y%m%d", &tm_time); 

            std::string renamefile = logfile;
            renamefile.append(".").append(buf);

            int trytimes = 3;
            
            do {
                if (rename(logfile.c_str(), renamefile.c_str()) == 0) {
                    break;
                }
            } while (trytimes --);
            
            last_logfile_t_ = now;
        } else {
            openflag = 0;
        }
    }
    
    log_fd_ = open(logfile.c_str(), openflag | O_WRONLY, 0666);

    if (log_fd_ >= 0) {
        lseek(log_fd_, 0, SEEK_END);
    }
    
    return log_fd_ >= 0;
}

void uimg_server::log(int level, const char * fmt, va_list valist)
{
    if (config_.log_level < level) {
        return;
    }
    
    size_t off = 0;
    
    time_t t_time = time(NULL);

    if (log_fd_ < 0 || last_logfile_t_ / 86400 != t_time / 86400) {
        if (!reopen_log()) {
            return;
        }
    }
    
    struct timeval tv;
    gettimeofday(&tv , NULL);
    
    struct tm tm_time;
    localtime_r(&t_time, &tm_time);
    //gmtime_r(&t_time, &tm_time);
    //tm_time.tm_hour += 8; // GMT -> CCT
    
    off = strftime(log_buf_, log_buf_size_, "%Y-%m-%d %H:%M:%S", 
        &tm_time); 
    
    off += snprintf(log_buf_ + off, log_buf_size_ - off, ":%.6d [P:%ld][%s] ", 
        (int)tv.tv_usec, pthread_self(), getstringbylevel(level));
        
    off += vsnprintf(log_buf_ + off, log_buf_size_ - off, fmt, valist);

    write(log_fd_, log_buf_, off);
}
