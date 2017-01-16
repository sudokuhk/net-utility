#include "uhttp/uhttp.h"
#include "unetwork/uschedule.h"
#include "unetwork/utcpsocket.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

class uhttpclient 
    : public utask
    , public uhttphandler
{
public:
    uhttpclient(uschedule * schedule, int fd)
        : utask()
        , schedule_(schedule)
        , socket_fd_(fd)
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
    
    virtual void run()
    {
        utcpsocket * socket = new utcpsocket(1024, socket_fd_, schedule_);
        uhttp * http = new uhttp(*socket, *this);
    
        printf("client %d running!\n", socket_fd_);
        socket->set_timeout(60 * 1000);
    
        http->run();
    
        printf("client %d exit!\n", socket_fd_);
    
        delete http;
        delete socket;
        delete this;
    }
    
    int onhttp(const uhttprequest & request, uhttpresponse & response, int)
    {
        response.set_version(request.version());
        response.set_statuscode(uhttp_status_not_found);
        
        response.set_header(uhttpresponse::HEADER_CONTENT_TYPE, "text/html");
    
        const char * content = "<html><body><h1>404 Not Found!</h1></body></html>";
        response.set_content(content, strlen(content));

        printf("return:%s\n", content);
        
        return 0;
    }

private:
    uschedule * schedule_;
    int socket_fd_;
};


class uhttpserver : public utask
{
public:
    uhttpserver(uschedule * schedule, const char * host, int port)
        : utask()
        , schedule_(schedule)
        , socket_fd_(-1)
        , host_(host)
        , port_(port)
        , running_(false)
    {
    }
    
    virtual ~uhttpserver()
    {
    }
    
    virtual void run()
    {
        utcpsocket * socket = new utcpsocket(100, socket_fd_, schedule_);
        socket->listen(host_.c_str(), port_, 100);
        
        running_ = true;
        struct sockaddr_in clientaddr;
        socklen_t len;
        int clientfd = -1;
        
        while (running_) {
            clientfd = schedule_->accept(socket, 
                (struct sockaddr *)&clientaddr, &len, -1);
            
            if (clientfd < 0) {
                printf("accept error, myerrno:%d, errno:%d(%s)\n",
                    socket->last_error(), errno, strerror(errno));
                break;
            }
    
            utask * task = new uhttpclient(schedule_, clientfd);
            printf("client fd:%d, task:%p\n", clientfd, task);
    
            schedule_->add_task(task);
        }
        printf("server exit!\n");
        close(socket->socket());
    
        delete socket;
        delete this;
    }
    
private:
    uschedule * schedule_;
    int socket_fd_;
    std::string host_;
    int port_;
    bool running_;
};


static bool exit_ = false;

void sig(int signo)
{
    if (signo == SIGUSR1) {
        exit_ = true;
    }
}

void * thread_func(void * args)
{
    uschedule * schedule = (uschedule *)args;
    schedule->run();

    return NULL;
}

void multithread()
{
    signal(SIGUSR1, sig);
    
    uschedule schedule(8 * 1024, 100000, false, NULL);

    schedule.add_task(new uhttpserver(&schedule, "0.0.0.0", 8783));

    pthread_t t1;
    pthread_create(&t1, NULL, thread_func, &schedule);

    pthread_join(t1, NULL);
    
    return;
}

int main(int argc, char * argv[])
{
    multithread();
    
    return 0;
}
