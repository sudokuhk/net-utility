/*************************************************************************
    > File Name: a.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Friday, November 25, 2016 PM05:14:47 CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <sys/types.h>

#include "uimg.h"
#include <uconfig/uconfig.h>
#include <ulog/ulog.h>
#include <utools/ufs.h>
#include <utools/ustring.h>

#define MAJOR   0
#define MINOR   0
#define BUILD   1

const char * get_version()
{   
    #define VERSION_BUF_SIZE 30
    static char version[VERSION_BUF_SIZE] = {0};
    if (version[0] == 0) {
        snprintf(version, VERSION_BUF_SIZE, "%d.%d.%d", MAJOR, MINOR, BUILD);
    }
    return version;
    #undef VERSION_BUF_SIZE
}

std::string get_appname(const char * arg0)
{
    std::string pathname(arg0);

    std::string::size_type npos = pathname.rfind("/");
    if (npos != std::string::npos) {
        return pathname.substr(npos + 1);
    }
    return pathname;
}

std::string get_cwd()
{
    pid_t pid = getpid();
    char buf[1024];
    char linkname[4096];
    
    snprintf(buf, 1024, "/proc/%d/cwd", pid);

    ssize_t size = readlink(buf, linkname, 4096);
    if (size < 0) {
        fprintf(stderr, "readlink(%s) failed! reason:%s\n", buf, strerror(errno));
        exit(0);
    }

    linkname[size] = '\0';
    return linkname;
}

void show_usage(const char * appname)
{
    fprintf(stderr,        
"Usage: %s [-f conf]\n"
"\n"
"Mandatory arguments to long options are mandatory for short options too.\n"
"\n"
"   -f          configure file\n"
"   -v          show version\n"
"   -h          show this manual\n"
"\n", appname);

    return;
}

void show_version(const char * appname, const char * version)
{
    fprintf(stderr, "%s %s\n", appname, version); 
}

bool load_config(const std::string & file, uimg_conf_t & config)
{
    uconfig * pconfig = uconfig::create();
    if (pconfig == NULL) {
        fprintf(stderr, "create config obj failed!\n");
        return false;
    }

    bool suc = pconfig->load(file.c_str());
    if (!suc) {
        delete pconfig;
        return suc;
    }
    
    // server
    do {
        const char * value = NULL;
        
        config.server_ip = "0.0.0.0";
        if ((value = pconfig->get_string("uimg", "hostip")) != NULL) {
            config.server_ip = value;
        } else {
            fprintf(stderr, "don't config host ip, use default(%s)!\n", 
                config.server_ip.c_str());
        }

        config.server_port  = pconfig->get_int("uimg", "hostport");
        if (config.server_port == 0) {
            fprintf(stderr, "don't config listen port!\n");
            suc = false;
            break;
        }
        
        config.server_datapath  = "./data/";
        if ((value = pconfig->get_string("uimg", "datapath")) != NULL) {
            config.server_datapath = value;
        } else {
            fprintf(stderr, "don't config data path, use default(%s)!\n", 
                config.server_datapath.c_str());
        }

        config.threads  = pconfig->get_int("uimg", "workers");
        if (config.threads == 0) {
            config.threads = 2;
        }

        if ((value = pconfig->get_string("uimg", "rootpath")) != NULL) {
            config.root_path = value;
        } else {
            config.root_path.append("./www/");
            fprintf(stderr, "don't config root path, use default(%s)!\n", 
                config.root_path.c_str());
        }

        if ((value = pconfig->get_string("uimg", "limitsize")) != NULL) {
            config.limitsize = atoi(value);
        } else {
            config.limitsize = -1;
        }

        config.deamon = 0;
        if ((value = pconfig->get_string("uimg", "deamon")) != NULL) {
            config.deamon = atoi(value) == 0 ? 0 : 1;
        }

        config.allowall     = true;
        config.allowtypes.clear();
        if ((value = pconfig->get_string("uimg", "allowtypes")) != NULL) {
            std::vector<std::string> out;
            split(value, ',', out);

            size_t allown = out.size();
            if (allown > 0) {
                config.allowall = false;
                for (size_t i = 0; i < allown; i++) {
                    //printf("allowtype:%s\n", out[i].c_str());
                    config.allowtypes.insert(out[i]);
                }
            }
        }
    } while (0);

    // log
    do {
        const char * value = NULL;
        
        config.log_level = ulog_error;
        if ((value = pconfig->get_string("ulog", "level")) != NULL) {
            config.log_level = getlevelbystring(value);
            //fprintf(stdout, "loglevel:%d %s\n", config.log_level, value);
        } else {
            fprintf(stderr, "don't config log level, use default(%d)!\n", 
                config.log_level);
        }

        config.log_path = "./log/";
        if ((value = pconfig->get_string("ulog", "path")) != NULL) {
            config.log_path = value;
        } else {
            fprintf(stderr, "don't config log path, use default(%s)!\n", 
                config.log_path.c_str());
        }

        config.log_filename = "uimg.log";
        if ((value = pconfig->get_string("ulog", "filename")) != NULL) {
            config.log_filename = value;
        } else {
            fprintf(stderr, "don't config log filename, use default(%s)!\n", 
                config.log_filename.c_str());
        }
    } while (0);


    // http
    do {
        const char * value = NULL;
        
        config.etag = 0;
        if ((value = pconfig->get_string("uhttp", "etag")) != NULL) {
            config.etag = atoi(value);
        }

        config.add_header.clear();
        if ((value = pconfig->get_string("uhttp", "headers")) != NULL) {
            std::string headers(value);
            std::string::size_type begin = 0, npos;

            headers.append(";");

            while ((npos = headers.find(';', begin)) != std::string::npos) {
                std::string substr = headers.substr(begin, npos - begin);

                std::string::size_type sep = substr.find(':');
                if (sep != std::string::npos) {
                    config.add_header[substr.substr(0, sep)] = substr.substr(sep + 1);
                    //printf("http header, %s = %s\n", substr.substr(0, sep).c_str(),
                    //    substr.substr(sep + 1).c_str());
                }

                begin = npos + 1;
            }
        } 
    } while (0);
    delete pconfig;

    return suc;
}

int main(int argc, char * argv[])
{
    std::string appname     = get_appname(argv[0]);
    std::string cwd         = get_cwd();
    std::string version     = get_version();
    std::string config_file;
    uimg_conf_t uconfig;
    
    int ch;
    while((ch = getopt(argc, argv, "hvf:")) != -1) {
        switch(ch) {
            case 'h': 
                show_usage(appname.c_str());
                exit(0);
            case 'v':
                show_version(appname.c_str(), version.c_str());
                exit(0);
            case 'f':
                std::string(optarg).swap(config_file);
                break;
            default:
                fprintf(stderr, "unknown arguments. (%c)", ch);
                break;
        }
    }

    //printf("config file:%s\n", config_file.c_str());
    if (config_file.empty()) {
        //fprintf(stderr, "empty config file!\n\n");
        show_usage(appname.c_str());
        exit(0);
    }
    
    if (!load_config(config_file, uconfig)) {
        fprintf(stderr, "load configure file(%s) failed!\n\n",
            config_file.c_str());
        //show_usage(appname.c_str());
        exit(0);
    }
 
    if(uconfig.deamon == 1) {
        if(daemon(1, 1) < 0) {
            fprintf(stderr, "Create daemon failed!\n");
            exit(-1);
        } else {
            fprintf(stdout, "%s %s\n", appname.c_str(), version.c_str());
            fprintf(stdout, "Copyright (c) 2016-2016 sudoku.huang@gmail.com\n");
            fprintf(stderr, "\n");
        }
    }

    uconfig.cwd     = cwd;

    uconfig.root_path = combine_path(cwd, uconfig.root_path);
    uconfig.server_datapath = combine_path(cwd, uconfig.server_datapath);
    uconfig.log_path = combine_path(cwd, uconfig.log_path);

    ulog(ulog_alert, "www  dir:%s\n", uconfig.root_path.c_str());
    ulog(ulog_alert, "data dir:%s\n", uconfig.server_datapath.c_str());
    
	uimg_server server(uconfig);
	server.run();
    
    return 0;
}
