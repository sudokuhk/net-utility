#include "utcpsocket.h"

#include "utcpstreambuf.h"
#include "uschedule.h"
#include "utools/ustring.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>

utcpsocket::utcpsocket(size_t buf_size, int sockfd,
    uschedule * sched, bool nonblock, 
    int socket_timeo, int connect_timeo, bool no_delay)
    : utimer()
    , basic_tcp_stream(buf_size)
    , schedule_(sched)
    , socket_fd_(sockfd)
    , connect_timeo_(connect_timeo)
    , socket_timeo_(socket_timeo)
    , errno_(0)
    , close_when_destroy_(true)
    , noblock_(nonblock)
    , nodelay_(no_delay)
    
{
    setnonblock(nonblock);
    setnodelay(no_delay);

    new_rdbuf(new utcpstreambuf(this, sched, buf_size_));

    event_.events   = EPOLLIN | EPOLLERR | EPOLLHUP;
    event_.data.ptr = this;
}

utcpsocket::utcpsocket(size_t buf_size, uschedule * sched, bool nonblock, 
    int socket_timeo, int connect_timeo, bool no_delay)
    : utimer()
    , basic_tcp_stream(buf_size)
    , schedule_(sched)
    , socket_fd_(-1)
    , connect_timeo_(connect_timeo)
    , socket_timeo_(socket_timeo)
    , errno_(0)
    , close_when_destroy_(true)
    , noblock_(nonblock)
    , nodelay_(no_delay)
{
    event_.events   = EPOLLIN | EPOLLERR | EPOLLHUP;
    event_.data.ptr = this;
}


utcpsocket::~utcpsocket()
{
    if (close_when_destroy_ && socket_fd_ > 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool utcpsocket::set_timeout(int ms)
{
    socket_timeo_ = ms;
    return true;
}

int utcpsocket::last_error()
{
    if (errno_ == ETIMEDOUT) {
        return en_socket_timeout;
    } else if (errno_ == EINVAL || errno_ == ECONNREFUSED) {
        return en_socket_refused;
    } else if (errno_ == 0) {
        return en_socket_normal_closed;
    }

    return -1;
}

int utcpsocket::socket()
{
    return socket_fd_;
}

bool utcpsocket::setnonblock(bool nonblock)
{
    int ret = 0;

    int tmp = fcntl(socket_fd_, F_GETFL, 0);

    if (nonblock) {
        ret = fcntl(socket_fd_, F_SETFL, tmp | O_NONBLOCK);
    } else {
        ret = fcntl(socket_fd_, F_SETFL, tmp & (~O_NONBLOCK));
    }

    return 0 == ret;
}

bool utcpsocket::setnodelay(bool nodelay)
{
    int tmp = nodelay ? 1 : 0;
    
    int ret = setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY,
        (char*) &tmp, sizeof(tmp));

    return 0 == ret;

}

int utcpsocket::connect(const char * ip, int port)
{
    if (ip  == NULL || port == 0) {
        return -1;
    }

    uint32_t iip = str2ip(ip);
    
    int sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        return -2;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(iip);
    addr.sin_port   = htons(port);

    new_rdbuf(new utcpstreambuf(this, schedule_, buf_size_));

    if (schedule_->connect(this , (struct sockaddr*) &addr, sizeof(addr)) != 0) {
        close(sock);
        return -3;
    }

    socket_fd_ = sock;
    setnodelay(noblock_);
    setnonblock(nodelay_);

    return 0;
}


int utcpsocket::listen(const char * ip, int port, int backlog)
{
    if (port == 0) {
        return -1;
    }

    uint32_t iip = str2ip(ip);
    
    int sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        return -2;
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(iip);
    addr.sin_port   = htons(port);

    if (::bind(sock , (struct sockaddr*) &addr, sizeof(addr)) != 0) {
        close(sock);
        return -3;
    }

    if (::listen(sock, backlog) != 0) {
        close(sock);
        return -4;
    }

    socket_fd_ = sock;
    setnodelay(noblock_);
    setnonblock(nodelay_);

    return 0;
}

int utcpsocket::accept(struct sockaddr* addr, socklen_t* addrlen, int timeo)
{
    return schedule_->accept(this, addr, addrlen, timeo);
}
