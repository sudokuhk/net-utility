/*************************************************************************
    > File Name: uimg.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Nov 28 13:54:43 CST 2016
 ************************************************************************/

#ifndef __UIMG_H__
#define __UIMG_H__

#include <stdarg.h>
#include <string>
#include <time.h>
#include <vector>
#include <map>
#include <set>

typedef struct uimg_conf
{
    //server
    std::string     server_ip;
    int             server_port;
    std::string     server_datapath;
    int             threads;
    int             deamon;
    std::string     root_path;
    int             limitsize;
    std::set<std::string>   allowtypes;
    bool            allowall;

    //log
    int             log_level;
    std::string     log_path;
    std::string     log_filename;

    //
    std::string     cwd;

    //http
    int             etag;
    std::map<std::string, std::string> add_header;
} uimg_conf_t;

class uimgserverunit;

class uimg_server
{   
public:
    uimg_server(uimg_conf_t & config);

    ~uimg_server();
    
    bool run();

    static void uimg_log_hook(int level, const char * fmt, va_list valist); 

private:
    void run_forever();

    void log(int level, const char * fmt, va_list valist);

    bool reopen_log();

    bool listen();
    
private:
    uimg_conf_t & config_;
    int listen_fd_;

    static uimg_server * sinstance;

    //for log
    int     log_fd_;
    char *  log_buf_;
    int     log_buf_size_;
    time_t  last_logfile_t_;
    pthread_mutex_t mutex_;

    std::vector<uimgserverunit *> server_units_;
};

#endif
