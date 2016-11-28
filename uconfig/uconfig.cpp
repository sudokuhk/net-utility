/*************************************************************************
    > File Name: uconfig.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Nov 28 14:30:48 CST 2016
 ************************************************************************/

#include "uconfig.h"
#include "uconfig_default.h"
#include <stdlib.h>

uconfig * uconfig::create(en_uconfig_type type)
{
    uconfig * pconfig = NULL;

    switch (type) {
    case en_default:
        pconfig = new uconfig_default();
        break;
    case en_lua:
        break;
    default:
        break;
    }

    return pconfig;
}
    
uconfig::uconfig()
    : config_()
{
}

uconfig::~uconfig()
{
    config_.clear();
}

int uconfig::get_int(const char * group, const char * name) const
{
    const char * value = get_string(group, name);

    return value == NULL ? 0 : atoi(value);
}

const char * uconfig::get_string(const char * group, const char * name) const
{
    const char * value = NULL;

    if (group != NULL && name != NULL) {
        config_map_t::const_iterator config_it = config_.find(group);
        if (config_it != config_.end()) {
            const group_map_t & group_map  = config_it->second;
            group_map_t::const_iterator group_it = group_map.find(name);
            if (group_it != group_map.end()) {
                value = group_it->second.c_str();
            }
        }
    }
    
    return value;
}
