#include "uboundaryparser.h"

const char uboundaryparser::CONTENT_DISPOSITION[]   = "Content-Disposition:";
const char uboundaryparser::CONTENT_TYPE[]          = "Content-Type:";
const char uboundaryparser::NAME[]                  = "name=";
const char uboundaryparser::FILENAME[]              = "filename=";


uboundaryparser::uboundaryparser()
{
}

uboundaryparser::~uboundaryparser()
{
}

bool uboundaryparser::parse(const std::string & content, const char * boundary,
    std::vector<uboundaryinfo> & output)
{
    if (content.empty() || boundary == NULL) {
        return false;
    }

    bool suc = false;
    output.clear();

    std::string boundaryin("--");
    boundaryin.append(boundary);

    std::string::size_type begin   = 0;
    std::string::size_type pos     = 0;
    std::string::size_type idx     = 0;
    const char * data = NULL;
    std::string::size_type len     = 0;
    bool first                     = true;

    uboundaryinfo info;
        
    while ((pos = content.find(boundaryin, begin)) != std::string::npos) {

        if (first) {
            first = false;
        } else {
            info.data   = content.data() + begin;
            info.len    = pos - begin - 2;
            
            if (info.len > 0) {
                output.push_back(info);
            } 
        }
        
        begin = pos + boundaryin.size();
        if (content.size() >= begin + 2) {
            if (content[begin] == '-' && content[begin + 1] == '-') {
                suc =  true;
                break;
            }
        }

        // 1. find CONTENT_DISPOSITION
        begin = begin;
        pos = content.find(CONTENT_DISPOSITION, begin);
        if (pos != std::string::npos) {
            begin = pos + sizeof(CONTENT_DISPOSITION) - 1;
            pos = content.find(';', begin);
            if (pos != std::string::npos) {
                data    = content.data() + begin;
                len     = pos - begin;
                while (*data == ' ') {
                    data ++;
                    len --;
                }
                std::string(data, len).swap(info.disposition);
            }
        }

        // 2. find NAME
        begin   = pos + 1;
        pos = content.find(NAME, begin);
        if (pos != std::string::npos) {
            begin = pos + sizeof(NAME);
            pos = content.find('"', begin);
            if (pos != std::string::npos) {
                data    = content.data() + begin;
                len     = pos - begin;
                while (len > 0 && (*data == ' ' || *data == '"')) {
                    data ++;
                    len --;
                }
                std::string(data, len).swap(info.name);
            }
        }

        // 3. find FILENAME
        begin   = pos + 1;
        pos = content.find(FILENAME, begin);
        if (pos != std::string::npos) {
            begin = pos + sizeof(FILENAME);
            pos = content.find('"', begin);
            if (pos != std::string::npos) {
                data    = content.data() + begin;
                len     = pos - begin;
                while (len > 0 && (*data == ' ' || *data == '"')) {
                    data ++;
                    len --;
                }
                std::string(data, len).swap(info.filename);
            }
        }

        // 3. find CONTENT_TYPE
        begin   = pos + 1;
        pos = content.find(CONTENT_TYPE, begin);
        if (pos == std::string::npos) {
            break;
        }
        begin = pos + sizeof(CONTENT_TYPE) - 1;
        pos = content.find("\r\n", begin);
        if (pos == std::string::npos) {
            break;
        }
        
        data    = content.data() + begin;
        len     = pos - begin;
        while (len > 0 && *data == ' ') {
            data ++;
            len --;
        }
        std::string(data, len).swap(info.type);

        begin = pos + 2; //sizeof("\r\n");  // skip \r\n
        if (content.size() >= begin + 2 && content[begin] == '\r' 
            && content[begin + 1] == '\n') {
            begin += 2;                     // skip \r\n
        } else {
            break;
        }
    }

    if (!suc) {
        output.clear();
    }
    
    return suc;
}
