#include "unetwork/uschedule.h"
#include "unetwork/utimer.h"
#include "unetwork/utcpsocket.h"

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>

#include <string>
#include <vector>

class uclient : public utask
{
public:
    uclient(uschedule * schedule, int fd)
    : utask()
    , schedule_(schedule)
    , socket_fd_(fd)
    , running_(false)
    , buf_()
    , active_(false)
    {
        //assert(fd > 0);
        buf_.resize(1024);
    }

    uclient(uschedule * schedule, const char * host, int port)
        : utask()
        , schedule_(schedule)
        , socket_fd_(-1)
        , running_(false)
        , buf_()
        , active_(true)
        , host_(host)
        , port_(port)
    {
        buf_.resize(1024);
    }


    virtual ~uclient()
    {
        printf("client %3d close!\n", socket_fd_);
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }
    }

    void run()
    {
        utcpsocket * socket = NULL;
        int len = 0;

        if (active_) {
            socket_fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            if (socket_fd_ < 0) {
                goto exit;
            } else {
                struct sockaddr_in in_addr;
                memset(&in_addr, 0, sizeof(in_addr));

                in_addr.sin_family = AF_INET;
                in_addr.sin_addr.s_addr = inet_addr(host_.c_str());
                in_addr.sin_port = htons(port_);
        
                socket = new utcpsocket(1024, socket_fd_, schedule_);
                
                if (schedule_->connect(socket, (struct sockaddr*) &in_addr, 
                    sizeof(in_addr)) < 0) {
                    goto exit;
                }
            }
        } else {
            if (socket_fd_ < 0) {
                goto exit;
            }
            socket = new utcpsocket(1024, socket_fd_, schedule_);
        }

        printf("client %d running!\n", socket_fd_);
        socket->set_timeout(60 * 1000);
        running_ = true;

        while (running_) {
            //ret = schedule_->recv(socket, (void *)&buf_[0], buf_.size(), 0);
            //ret = socket->readsome((char *)&buf_[0], buf_.size());
            if (!socket->read((char *)&len, sizeof(len)).good()) {
                break;
            }
            len = ntohl(len);
            //printf("recv:%d\n", len);
            
            if (!socket->read((char *)&buf_[0], len).good()) {
                //printf("recv negative! break!\n");
                break;
            }
            
            //printf("recv:%d, %s\n", len, (char *)&buf_[0]);
            
            //schedule_->send(socket, (void *)&buf_[0], ret, 0);
            //socket->write((char *)&buf_[0], len);
            //socket->flush();
        }

        printf("client %d exit!\n", socket_fd_);

        running_ = false;

    exit: 
        if (socket != NULL) {
            delete socket;
        }
        delete this;
    }

private:
    uschedule * schedule_;
    int socket_fd_;
    bool running_;
    std::vector<char> buf_;

    bool active_;
    std::string host_;
    int port_;
};


class userver : public utask
{
public:
    userver(uschedule * schedule, const char * host, int port)
        : utask()
        , schedule_(schedule)
        , socket_fd_(-1)
        , host_(host)
        , port_(port)
        , running_(false)
    {
    }
    
    virtual ~userver()
    {
    }
    
    void run()
    {
        int l = listen();
        assert(l >= 0);
    
        utcpsocket * socket = new utcpsocket(100, socket_fd_, schedule_);
        
        running_ = true;
        struct sockaddr_in clientaddr;
        socklen_t len;
        int clientfd = -1;
        
        while (running_) {
            clientfd = schedule_->accept(socket, 
                (struct sockaddr *)&clientaddr, &len);
            
            if (clientfd < 0) {
                printf("accept error, myerrno:%d, errno:%d(%s)\n",
                    socket->last_error(), errno, strerror(errno));
                break;
            }
    
            utask * task = new uclient(schedule_, clientfd);
            printf("client fd:%d, task:%p\n", clientfd, task);
    
            schedule_->add_task(task);
        }
        printf("server exit!\n");
        close(socket->socket());
    
        delete socket;
        delete this;
    }

private:    
    int listen()
    {
        int ret = -1;
        socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    
        int flags = 1;
        setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, 
            (char*) &flags, sizeof(flags));
    
        struct sockaddr_in server_sockaddr;
        server_sockaddr.sin_family  = AF_INET;
        server_sockaddr.sin_port    = htons(port_);
        server_sockaddr.sin_addr.s_addr = inet_addr(host_.c_str());
    
        if (bind(socket_fd_, (struct sockaddr *)&server_sockaddr,
            sizeof(server_sockaddr)) == -1) {
            goto out;
        }
        
        if (::listen(socket_fd_, 1024) == -1) {
            goto out;
        }
    
        ret = 0;
        
        return ret;
    out:
        close(socket_fd_);
        socket_fd_ = -1;
    
        return ret;
    }


private:
    uschedule * schedule_;
    int socket_fd_;
    std::string host_;
    int port_;
    bool running_;
};

class mylock : public ulock
{
public:
    mylock() {
        pthread_mutex_init(&mutex_, NULL);
    }

    virtual ~mylock() {
        pthread_mutex_destroy(&mutex_);
    }

    virtual void lock()
    {
        pthread_mutex_lock(&mutex_);
    }

    virtual void unlock()
    {
        pthread_mutex_unlock(&mutex_);
    }

private:
    pthread_mutex_t mutex_;
};

class task : public utask, public utimer
{
public:
    task(uschedule * schedule, int id) : schedule_(schedule), id_(id) {}

    virtual ~task() {}

    virtual void run() {
        int slpms = rand() % 10000;
        
        printf("task %3d! begin run \n", id_);
        printf("task %3d, sleep %d ms!\n", id_, slpms);
        schedule_->wait(this, slpms);
        printf("task %3d wake up and end!\n", id_);

        delete this;
    }

private:
    uschedule * schedule_;
    int id_;
};

void * thread_func(void * args)
{
    uschedule * schedule = (uschedule *)args;
    schedule->run();

    return NULL;
}

void singlethread()
{
    uschedule schedule(1024 * 8, 100000, false, NULL);
    schedule.add_task(new userver(&schedule, "0.0.0.0", 8888));

    for (int i = 0; i < 10; i++) {
        schedule.add_task(new task(&schedule, i));
    }
    schedule.run();
    schedule.stop();
}

static bool exit_ = false;

void sig(int signo)
{
    if (signo == SIGUSR1) {
        exit_ = true;
    }
}

void multithread()
{
    signal(SIGUSR1, sig);
    
    mylock lock;
    uschedule schedule(8 * 1024, 100000, false, &lock);

    schedule.add_task(new userver(&schedule, "0.0.0.0", 8888));

    pthread_t t1;
    pthread_create(&t1, NULL, thread_func, &schedule);
    //sleep(1);

    /*
    for (int i = 0; i < 10; i++) {
        printf("add task:%3d\n", i);
        schedule.add_task(new task(&schedule, i));
    }
    */

    /*while (!exit_) {
        sleep(1);
    }
    printf("receive exit!\n");
    schedule.stop();
    */
    pthread_join(t1, NULL);
}

int main(int argc, char * argv[])
{
    //singlethread();
    multithread();
    
    return 0;
}
