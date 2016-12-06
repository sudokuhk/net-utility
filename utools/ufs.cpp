/*************************************************************************
    > File Name: ufs_tools.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Nov 28 21:37:27 CST 2016
 ************************************************************************/

#include "ufs.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

int makedir(const char * newdir)
{
    std::string path(newdir);
    std::string dir;

    if (path[path.size() - 1] != '/') {
        path.append("/");
    }

    std::string::size_type nbegin = 0;
    std::string::size_type npos   = 0;

    struct stat st;
    
    while ((npos = path.find('/', nbegin)) != std::string::npos) {
        
        dir.append(path.substr(nbegin, npos - nbegin));
        nbegin = npos + 1;
        dir.append("/");
        
        if(stat(dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            continue;
        } else if (errno != ENOENT) {
            return -1;
        }
        
        if ((mkdir(dir.c_str(), 0755) == -1) && (errno == ENOENT)) {
            //fprintf(stderr, "create director:%s failed!\n", dir.c_str());
            return -1;
        }
    }

    return 0;
}

static mode_t get_mode(const char * name)
{
    struct stat st;
    if(stat(name, &st) < 0) {
        return 0;
    }
    return st.st_mode;
}

int isdir(const char * dirname)
{
    mode_t mode = get_mode(dirname);
    if (mode == 0) {
        return -1;
    }
    
    if(S_ISDIR(mode)) {
        return 1;
    } 

    return -2;
}

off_t isreg(const char * fname)
{
    struct stat st;
    if(stat(fname, &st) < 0) {
        return -1;
    }
    
    if(S_ISREG(st.st_mode)) {
        return st.st_size;
    } 

    return -2;
}

std::string combine_path(const std::string & first, const std::string & second)
{
    if (second.empty()) {
        return first;
    } else if (second[0] != '.') {
        return std::string(first).append("/").append(second);
    } else if (first.empty()) {
        return second;
    }
    
    std::string::size_type foff = first.size() - 1;
    std::string::size_type i    = 0;
    std::string::size_type size = second.size();
    int cnt = 0;

    std::string tmp(second);
    if (second[size - 1] != '/') {
        tmp.append("/");
        size += 1;
    }

    if (first[foff] == '/') {
        foff --;
    }
    
    for (i = 0; i < size; ) {
        
        if (tmp[i] == '.') {
            cnt ++;
            i ++;
            continue;
        }
        
        if (cnt == 0) {
            if (i > 0) {
                i --;
            }
            break;
        } 

        if (cnt == 1) {
            i += 2; //skip ./
            cnt = 0;
            continue;
        } 

        if (cnt == 2) {
            std::string::size_type npos = first.rfind('/', foff);
            if (npos == std::string::npos) {
                break;
            }

            i += 3; //skip ../
            foff = npos - 1;
            cnt = 0;
        }
    }

    return first.substr(0, foff + 1).append("/").append(tmp.substr(i));
}
