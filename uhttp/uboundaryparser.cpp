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

/*
First boundary: ------WebKitFormBoundaryzpnYCFS44g1YQgyR\r\n
    Encapsulated multipart part:  (image/jpeg)
        Content-Disposition: form-data; name="userfile"; filename="\345\257\270\347\205\247_\351\273\204\345\235\244.jpg"\r\n
        Content-Type: image/jpeg\r\n\r\n
        JPEG File Interchange Format
            Marker: Start of Image (0xffd8)
*/
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

    uboundaryinfo info;
        
    while ((pos = content.find(boundaryin, begin)) != std::string::npos) {

        begin = pos + boundaryin.size();
        if (content.size() > begin + 2) {
            if (content[begin] == '-' && content[begin + 1] == '-') {
                suc =  true;
                break;
            }
        }

        // 1. find CONTENT_DISPOSITION
        begin = begin;
        pos = content.find(CONTENT_DISPOSITION, begin);
        if (pos == std::string::npos) {
            break;
        }

        begin = pos + sizeof(CONTENT_DISPOSITION) - 1;
        pos = content.find(';', begin);
        if (pos == std::string::npos) {
            break;
        }

        data    = content.data() + begin;
        len     = pos - begin;
        while (*data == ' ') {
            data ++;
            len --;
        }
        std::string(data, len).swap(info.disposition);

        // 2. find NAME
        begin   = pos + 1;
        pos = content.find(NAME, begin);
        if (pos == std::string::npos) {
            break;
        }
        begin = pos + sizeof(NAME);
        pos = content.find('"', begin);
        if (pos == std::string::npos) {
            break;
        }
        
        data    = content.data() + begin;
        len     = pos - begin;
        while (*data == ' ' || *data == '"') {
            data ++;
            len --;
        }
        std::string(data, len).swap(info.name);

        // 3. find FILENAME
        begin   = pos + 1;
        pos = content.find(FILENAME, begin);
        if (pos == std::string::npos) {
            break;
        }
        begin = pos + sizeof(FILENAME);
        pos = content.find('"', begin);
        if (pos == std::string::npos) {
            break;
        }
        
        data    = content.data() + begin;
        len     = pos - begin;
        while (*data == ' ' || *data == '"') {
            data ++;
            len --;
        }
        std::string(data, len).swap(info.filename);

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
        while (*data == ' ') {
            data ++;
            len --;
        }
        std::string(data, len).swap(info.type);

        begin = pos + sizeof("\r\n");
        if (content.size() > begin + 2 && content[begin] == '\r' 
            && content[begin + 1] == '\n') {
            begin += 2;

            pos = content.find("\r\n", begin);
            if (pos == std::string::npos) {
                break;
            }
            info.data   = content.data() + begin;
            info.len    = pos - begin;

            output.push_back(info);
            
            begin = pos + 2;
        } else {
            break;
        }
    }

    if (!suc) {
        output.clear();
    }
    
    return suc;
}
