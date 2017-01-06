#ifndef __U_UDP_SOCKET_H__
#define __U_UDP_SOCKET_H__

#include "utimer.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

class uschedule;

#ifdef UUDP_STREAM

#include "ustream/basic_udp_stream.h"
#include "ustream/basic_udp_streambuf.h"

class uudpsocket;

class uudpstreambuf 
    : public basic_udp_streambuf
{
public:
    uudpstreambuf(uudpsocket & socket, uschedule & sched, size_t buf_size);

    virtual ~uudpstreambuf();

    virtual ssize_t precv(void * buf, size_t len, int flags);
	
	virtual ssize_t psend(const void * buf, size_t len, int flags);

private:
    uudpsocket & socket_;
    uschedule & schedule_;
};

class uudpsocket
    : public basic_udp_stream
    , public utimer
{
public:
    uudpsocket(size_t buf_size, int sockfd, uschedule & sched, 
        bool nonblock = true, int socket_timeo = 5000);

    uudpsocket(size_t buf_size, uschedule & sched, 
        bool nonblock = true, int socket_timeo = 5000);

	virtual ~uudpsocket();

	virtual bool set_timeout(int ms);

	virtual int last_error();

  	virtual int socket();

    virtual void remote_info(char * ipb, size_t ipl, int * port);

    bool connect(const char * ip, int port);

    bool bind(const char * ip, int port);

    struct epoll_event & event()    { return event_;    }

    void set_errno(int no)  { errno_ = no; }

    int socket_timeo() { return socket_timeo_; }

private:
    bool setnonblock(bool nonblock);

private:
	uschedule & schedule_;
    struct epoll_event event_;
    
	int socket_fd_;
	int socket_timeo_;
    int errno_;
    bool close_when_destroy_;
};

#else

class uudpsocket
    : public utimer
{
public:
    uudpsocket(uschedule & sched, int timeo);

    virtual ~uudpsocket();
    
    int bind(const char * ip, int port);
    
    int connect(const char * ip, int port);
    
    ssize_t read(void *buf, size_t count);
    
    ssize_t write(const void *buf, size_t count);
    
    ssize_t send(const void *buf, size_t len, int flags);
    
    ssize_t sendto(const void *buf, size_t len, int flags,
                   const struct sockaddr *dest_addr, socklen_t addrlen);

    ssize_t recv(void *buf, size_t len, int flags);

    ssize_t recvfrom(void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);

    struct epoll_event & event()    { return event_;    }
    
    void set_errno(int no)  { errno_ = no; }

    int get_errno() { return errno_; }

    int socket_timeo() { return timeo_; }

    int socket() { return sock_; }
private:
    bool setnonblock(bool nonblock);
    
private:
    uschedule & sched_;
    int sock_;
    struct epoll_event event_;
    
	int timeo_;
    int errno_;
};

#endif

#endif
