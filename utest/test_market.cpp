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

#include "zlib.h"  


#define REQUEST_CMD "http://db2015.wstock.cn/wsDB_API/kline.php?market=%s&desc=0&q_type=0&r_type=0&stime=%s&etime=%s&u=test&p=8e6a&num=3000"

class uexit 
    : public utask
{
public:
    uexit(uschedule & schedule)
        : schedule_(schedule)
    {
    }
    
    virtual ~uexit() {}
    
    void run()
    {
        schedule_.stop();
        delete this;
    }
private:
    uschedule& schedule_;
};

class uhttpclient 
    : public utask
    , public uhttphandler
{
public:
    uhttpclient(uschedule & schedule, uurl & url, 
        const std::string & market,
        const std::string & stime, 
        const std::string & etime)
        : utask()
        , schedule_(schedule)
        , url_(url)
        , decompress_buf_size_(4 * 1024 * 1024) // 4M
        , decompress_buf_(new unsigned char[decompress_buf_size_])
        , market_(market)
        , stime_(stime)
        , etime_(etime)
    {
    }
    
    virtual ~uhttpclient()
    {
        delete [] decompress_buf_;
    }

    int  gzdecompress(Byte  *zdata, uLong nzdata, 
                 Byte  *data, uLong *ndata) 
    { 
        int  err = 0; 
        z_stream d_stream = {0};  /* decompression stream */
        
        static   char  dummy_head[2] = { 
            0x8 + 0x7 * 0x10, 
            (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF, 
        }; 
        
        d_stream.zalloc     = NULL; 
        d_stream.zfree      = NULL; 
        d_stream.opaque     = NULL; 
        d_stream.next_in    = zdata; 
        d_stream.avail_in   = 0; 
        d_stream.next_out   = data; 
        
        if (inflateInit2(&d_stream, MAX_WBITS + 16) != Z_OK)  return  -1; 
        //if(inflateInit2(&d_stream, 47) != Z_OK) return -1;  
        while (d_stream.total_out < *ndata && d_stream.total_in < nzdata) { 
            
            d_stream.avail_in = d_stream.avail_out = 1;  /* force small buffers */  
            
            if ((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END)  
                break ; 
            if (err != Z_OK) { 
                if (err == Z_DATA_ERROR) { 
                    d_stream.next_in = (Bytef*) dummy_head; 
                    d_stream.avail_in =  sizeof (dummy_head); 
                    
                    if ((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK) { 
                        return  -1; 
                    } 
                    
                }  else  
                    return  -1; 
            } 
        } 
        if (inflateEnd(&d_stream) != Z_OK)  return  -1; 
        *ndata = d_stream.total_out; 
        return  0; 
    } 

    void writefile(const char * stock, const char * daykline)
    {
        const char * filename = "stock.txt";
        FILE * fp = NULL;
        
        if (access(filename, F_OK) != 0) {
            fp = fopen(filename, "w+");
        } else {
            fp = fopen(filename, "a+");
        }
        
        if (fp != NULL) {
            fprintf(fp, "%s\t\t%s\n", stock, daykline);
            fclose(fp);
        }
    }

    uhttpresponse get(uhttp & http, const char * market, 
        const char * sdate, const char *edate)
    {
        char buf[1024];
        snprintf(buf, 1024, REQUEST_CMD, market, sdate, edate);
        
        uurl url(buf);
        
        uhttprequest request;
        request.set_version(uhttp_version_1_1);
        request.set_method(uhttp_method_get);
        request.set_uri(url.uri());

        request.set_header("HOST", url.host().c_str());
        //request.set_header("User-Agent", "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36");
        request.set_header("User-Agent", "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785");
        request.set_header("Content-Type", "application/x-www-form-urlencoded");
        request.set_header("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
        request.set_header("Connection", "keep-alive");
        request.set_header("Upgrade-Insecure-Requests", "1");
        request.set_header("Cache-Control", "max-age=0");
        request.set_header("Accept-Encoding", "gzip, deflate");
        request.set_header("Accept-Language", "zh-CN,zh;q=0.8,en;q=0.6");
        request.set_header("DNT", "1");

        http.send_request(request);

        uhttpresponse response;
        http.recv_response(response);
#if 1
        {
            std::fstream output("unzip", 
                std::ios::out | std::ios::trunc | std::ios::binary);
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
#endif

        
        //decomporess
        const char * encoding = response.get_header("Content-Encoding");
        if (encoding != NULL && strcmp(encoding, "gzip") == 0) {
            printf("zipfile!\n");
            std::string & content = response.content();
            uLong ndata = decompress_buf_size_;
            if (gzdecompress((Byte *)&content[0], content.size(), 
                (Byte *)decompress_buf_, &ndata) == 0) {
                response.set_content((const char *)decompress_buf_, ndata);
            }
        }

        const std::string & content = response.content();
        if (response.statuscode() == uhttp_status_ok) {
            //writefile(stock, content.c_str());
            printf("get market(%s) succeed!\n", market);
        } else {
            printf("get market(%s) error!, info:%s\n", market, content.c_str());
        }
        
#if 1
        {
            std::fstream output(market, 
                std::ios::out | std::ios::trunc | std::ios::binary);
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
#endif
        return response;
    }

    
    void run()
    {
        int fd = utcpsocket::connect("114.55.95.221", url_.port());
        utcpsocket * socket = new utcpsocket(1024 * 1024, fd, &schedule_);
        uhttp * http = new uhttp(*socket, *this);
    
        //printf("client %d running!\n", fd);
        socket->set_timeout(60 * 1000);

        get(*http, market_.c_str(), stime_.c_str(), etime_.c_str());

        delete http;
        delete socket;
        close(fd);
        sleep(10);

        schedule_.add_task(new uexit(schedule_));
        
        delete this;
    }
    
    int onhttp(const uhttprequest & request, uhttpresponse & response)
    {
        return 0;
    }

private:
    uschedule & schedule_;
    uurl url_;
    
    const int       decompress_buf_size_;
    unsigned char * decompress_buf_;

    const std::string market_;
    const std::string stime_;
    const std::string etime_;
};

bool read_stock_ids(const std::string filename, const std::string & market,
    std::vector<std::string> & stockids)
{
    if (filename.empty()) {
        printf("please input stock_ids filename.\n");
        return false;
    }

    if (access(filename.c_str(), F_OK) != 0) {
        printf("please stock_ids filename(%s) not exists.\n", filename.c_str());
        return false;
    }

    FILE * fp = fopen(filename.c_str(), "r");
    char stockid[1024];
    
    while (!feof(fp)) {
        if (fgets(stockid, 1024, fp) != NULL) {
            std::string id(market);
            id.append(stockid);
            id[id.size() - 1] = '\0';
            stockids.push_back(id);
        }
    }
    fclose(fp);

    return true;
}

int main(int argc, char * argv[])
{
    const char * addr_url = REQUEST_CMD;
    std::string stime, etime, market;
    std::vector<std::string> stock_ids;
    
    int ch;
    while((ch = getopt(argc, argv, "s:e:f:m:")) != -1) {
        switch(ch) {
            case 's':
                std::string(optarg).swap(stime);
                break;
            case 'e':
                std::string(optarg).swap(etime);
                break;
            case 'm':
                std::string(optarg).swap(market);
                break;
            default:
                printf("unknown arguments. (%c)", ch);
                break;
        }
    }

    if (market.empty()) {
        printf("invalid market, please check it!\n");
        exit(0);
    }
    
    time_t now;
    struct tm *tm_now;
    time(&now);
    tm_now = localtime(&now);
 
    printf("now datetime: %d-%d-%d %d:%d:%d\n", tm_now->tm_year, tm_now->tm_mon, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
    if (stime.empty()) {
        char buf[100];
        snprintf(buf, 100, "%4d-%02d-%02d", 
            tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday);
        std::string(buf).swap(stime);
    }

    if (etime.empty()) {
        char buf[100];
        snprintf(buf, 100, "%4d-%02d-%02d", 
            tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday);
        std::string(buf).swap(etime);
    }

    printf("stime:%s, etime:%s, market:%s\n", 
        stime.c_str(), etime.c_str(), market.c_str());
    
    uurl url(addr_url);
    if (url.protocol()== uprotocol_unknown) {
        printf("invalid protocol:<%s>!\n", addr_url);
        return 0;
    }

    if (url.protocol() == uprotocol_http) {
        printf("begin run!\n");
        uschedule schedule(8 * 1024, 100);
        uhttpclient * client = new uhttpclient(schedule, url, 
            market, stime, etime);

        schedule.add_task(client);
        schedule.run();       
    } else {
        printf("unsupport protocol!\n");
    }
    
    return 0;
}
