/*************************************************************************
    > File Name: uconfig_default.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Nov 28 15:09:30 CST 2016
 ************************************************************************/

#include "uconfig_default.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream> 
#include <fstream>

uconfig_default::uconfig_default()
    : uconfig()
{
}

uconfig_default::~uconfig_default()
{
}

bool uconfig_default::istrim(char c)
{
    if (c == ' ' || c == '\0' || c == '\n' || c == '\r') {
        return true;
    }
    return false;
}

void uconfig_default::skip_blank(char *& begin, char *& end)
{
    while (end != begin && istrim(*end)) {
        *end -- = '\0';
    }
    while (begin < end && istrim(*begin)) begin ++;
}

bool uconfig_default::load(const char * filename)
{
    if (access(filename, F_OK) != 0) {
        return false;
    }

#ifndef CONF_LINESIZE
#define CONF_LINESIZE   4096
#endif

    bool bok    = false;
    char * rbuf = (char *)malloc(CONF_LINESIZE * sizeof(char));
    int linenum = 0;
    group_map_t * pgroup;
    
    std::ifstream in(filename);
    if (in.is_open()) {
        while (true) {
            in.getline(rbuf, CONF_LINESIZE);
            linenum ++;
            
            if (in.eof()) {
                //printf("end of file!\n");
                break;
            }

            if (!in.good()) {
                fprintf(stderr, "read failed! (%s:%d)\n", filename, linenum);
                goto out;
            }
            
            std::streamsize rdn = in.gcount();

            rbuf[rdn] = '\0';
            char * begin = rbuf;
            char * end   = rbuf + rdn;

            char * anno = strchr(begin, '#');

            if (anno == begin) {
                continue;
            }
            
            if (anno != NULL) {
                *anno -- = '\0';
                end = anno;
            }

            skip_blank(begin, end);

            if (end == begin) {
                //printf("empty line, rdn:%d, line:%d\n", rdn, linenum);
                continue;
            }

            if (*begin == '[' && *end == ']') {
                //printf("group, line:%d, %s\n", linenum, begin);
                begin ++;
                *end -- = '\0';

                skip_blank(begin, end);
                if (begin > end) {
                    fprintf(stderr, "empty group name. (%s:%d:%ld)\n", 
                        filename, linenum, begin - rbuf);
                    goto out;
                }

                //std::string group(begin);
                pgroup = &config_[begin];
                //printf("group:%s\n", begin);
                
            } else if (pgroup != NULL) {
                char * sep = strchr(begin, '=');
                if (sep == NULL) {
                    fprintf(stderr, "no seperate(=)! (%s:%d:%ld)\n",
                        filename, linenum, begin - rbuf);
                    goto out;
                }

                *sep = '\0';
                char * keye     = sep - 1;
                char * valueb   = sep + 1;

                skip_blank(begin, keye);
                skip_blank(valueb, end);

                if (begin > keye) {
                    fprintf(stderr, "empty key! (%s:%d:%ld)(=%s)\n",
                        filename, linenum, begin - rbuf, valueb);
                    goto out;
                }

                (*pgroup)[begin] = valueb;
                //printf("group:%s=%s\n", begin, valueb);
            } else {
                fprintf(stderr, "invalid group item! (%s:%d:%d)\n",
                    filename, linenum, 0);
                goto out;
            }            
            
            //printf("line:[%4ld]%s\n", rdn, rbuf);
        }
        bok = true;
        
    } else {
        fprintf(stderr, "open configure file(%s) failed!", filename);
        goto out;
    }

out:
    if (!bok) {
        config_.clear();
    }
    
    if (in.is_open()) {
        in.close();
    }

    if (rbuf != NULL) {
        free(rbuf);
    }
    
    return bok;
}