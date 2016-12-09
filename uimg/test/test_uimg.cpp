#include "uhttp/uhttp.h"
#include "unetwork/uschedule.h"
#include "unetwork/utcpsocket.h"
#include "utools/ufs.h"
#include "utools/umd5sum.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static bool uimg = true;

class uhttpclient 
    : public utask
    , public uhttphandler
{
public:
    uhttpclient(uschedule * schedule, int fd, bool up, std::string & param)
        : utask()
        , schedule_(schedule)
        , socket_fd_(fd)
        , data(param)
        , upload(up)
    {
    }
    
    virtual ~uhttpclient()
    {
        printf("client %3d close!\n", socket_fd_);
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }
    }

    void do_upload(uhttp & http)
    {
        printf("==========do_upload begin-------------\n");
        uhttprequest request;
        request.set_version(uhttp_version_1_1);
        request.set_method(uhttp_method_post);
        request.set_uri("/upload");
        request.set_header(uhttpmessage::HEADER_CONTENT_TYPE, "image/jpeg");

        request.set_content(data);
        http.send_request(request);

        request.output_header(std::cout);
        printf("==========do_upload end-------------\n");
    }

    void do_download(uhttp & http)
    {
        printf("==========do_download begin-------------\n");
        uuri uri;
        if (uimg) {
            uri.set_path("/download");
            uri.set_query("md5", data.c_str());
        } else {
            uri.set_path(data.c_str());
            uri.set_query("p", "0");
        }
        
        uhttprequest request;
        request.set_version(uhttp_version_1_1);
        request.set_method(uhttp_method_get);
        request.set_uri(uri);

        http.send_request(request);
        printf("==========do_download done-------------\n");
    }

    void recv_response(uhttp & http) 
    {
        printf("\n==========recv_response begin-------------\n");
        uhttpresponse response;

        http.recv_response(response);

        response.output_header(std::cout);
        printf("\nres:%d\n", response.statuscode());
        if (upload) {
            printf("content:%s\n", response.content().c_str());
        } else {
            printf("content:%ld\n", response.content().size());
            int size = response.content().size();
            printf("content:%s\n", response.content().substr(0, size > 100 ? 100 : size).c_str());
        }
        printf("\n==========recv_response end-------------\n");
    }
    
    virtual void run()
    {
        utcpsocket * socket = new utcpsocket(1024, socket_fd_, schedule_);
        uhttp http(*socket, *this, false);
    
        printf("client %d running!\n", socket_fd_);
        socket->set_timeout(5 * 1000);
    
        //http->run();

        struct timeval t_start, t_end;
        long cost_time = 0;

        gettimeofday(&t_start, NULL);

        if (upload) {
            do_upload(http);
        } else {
            do_download(http);
        }
        recv_response(http);
        
        gettimeofday(&t_end, NULL);

        cost_time = t_end.tv_sec - t_start.tv_sec;
        cost_time = cost_time * 1000000;

        cost_time += t_end.tv_usec - t_start.tv_usec;
        printf("%s cost %ld usec!\n", 
            upload ? "upload" : "download",
            cost_time);
    
        delete socket;
        delete this;
    }
    
    int onhttp(const uhttprequest & request, uhttpresponse & response)
    {
        return -1;
    }

    int onerror(int errcode, uhttpresponse & response)
    {
        return -1;
    }

private:
    uschedule * schedule_;
    int socket_fd_;
    const std::string & data;
    bool upload;
};

bool checkfile(const std::string & fname)
{
    return access(fname.c_str(), F_OK) == 0;
}

void readdata(const std::string & file, std::string & content)
{
    off_t size = isreg(file.c_str());
    if (size < 0) {
        return;
    } else {
        content.resize(size);
    }

    int fd = open(file.c_str(), O_RDONLY);
    
    if (fd < 0) {
        return;
    }
    
    if (::read(fd, &content[0], content.size()) != (ssize_t)content.size()) {
        fprintf(stderr, "file[%s] read part, failed!!\n", file.c_str());
    }

    close(fd);
}

int main(int argc, char * argv[])
{
    std::string host, file, method, port;
    
    int ch;
    while((ch = getopt(argc, argv, "h:p:f:m:s:")) != -1) {
        switch(ch) {
            case 'h': 
                host = optarg;
                break;
            case 'f': 
                file = optarg;
                break;
            case 'm': 
                method = optarg;
                break;
            case 'p': 
                port = optarg;
                break;
            case 's':
                if (strcmp(optarg, "uimg")) {
                    uimg = false;
                }
                break;
            default:
                fprintf(stderr, "unknown arguments. (%c)", ch);
                break;
        }
    }

    int port_ = 8783;
    if (port.size() > 0) {
        port_ = atoi(port.c_str());
    }

    if (host.size() == 0) {
        host = "127.0.0.1";
    }

    printf("will connect server %s:%d\n", host.c_str(), port_);
    std::string data;
    
    if (method == "upload") {
        if (!checkfile(file)) {
            fprintf(stderr, "upload, but file(%s) not exists!\n", file.c_str());
            exit(0);
        }
        readdata(file, data);
        printf("upload, file:%s, filesize:%ld\n", file.c_str(), data.size());
    } else if (method == "download") {
        if (!umd5sum::ismd5(file)) {
            fprintf(stderr, "download, but param(%s) not md5!\n", file.c_str());
            exit(0);            
        }
        data = file;
        printf("download md5:%s\n", data.c_str());
    }

    if (data.empty()) {
        fprintf(stderr, "error operation! method:%s, data:%s\n", method.c_str(), data.c_str());
        exit(0);
    }
    
    uschedule schedule(8 * 1024, 100000, false, NULL);
    int sock = utcpsocket::connect(host.c_str(), port_);
    if (sock < 0) {
        fprintf(stderr, "connect [%s:%d] failed!\n", host.c_str(), port_);
        exit(0);
    }

    uhttpclient * client = new uhttpclient(&schedule, sock, method == "upload", data);
    schedule.add_task(client);
    schedule.run();
    
    return 0;
}
