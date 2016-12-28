/*************************************************************************
    > File Name: uconfig.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Nov 28 14:50:43 CST 2016
 ************************************************************************/

#ifndef __U_CONFIG_H__
#define __U_CONFIG_H__

#include <string>
#include <map>

class uconfig
{
public:
    enum en_uconfig_type {
        en_default = 0,
        en_lua,
        en_unkown,
    };

    static uconfig * create(en_uconfig_type type = en_default);

    typedef std::map<std::string, std::string> group_map_t;
    typedef std::map<std::string, group_map_t> config_map_t;
    
public:
    uconfig();

    virtual ~uconfig();

    int get_int(const char * group, const char * name) const;

    const char * get_string(const char * group, const char * name) const;

    group_map_t  get_group(const char * group) const;
    
public:
    virtual bool load(const char * filename) = 0;

    config_map_t config_;
};

#endif  //__U_CONFIG_H__