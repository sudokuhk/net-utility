#ifndef __U_BOUNDARY_PARSER_H__
#define __U_BOUNDARY_PARSER_H__

#include <string>
#include <vector>

class uboundaryparser
{
public:
    static const char CONTENT_DISPOSITION[];
    static const char CONTENT_TYPE[];
    static const char NAME[];
    static const char FILENAME[];

    struct uboundaryinfo 
    {   
        std::string disposition;
        std::string name;
        std::string filename;
        std::string type;

        const char * data;
        int len;
    };
    
public:
    uboundaryparser();

    ~uboundaryparser();

    bool parse(const std::string & content, const char * boundary, 
        std::vector<uboundaryinfo> & output);
    
private:
};

#endif
