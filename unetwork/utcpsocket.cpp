#include "utcpsocket.h"

#include "utcpstreambuf.h"
#include "uschedule.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>

int utcpsocket::connect(const char * host, int port)
{
    if (host == NULL || port == 0) {
        return -1;
    }
    //printf("connect to, host:%s, port:%d\n", host, port);

    struct addrinfo *result, hint;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family   = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, NULL, &hint, &result) < 0) {
        return -1;
    }

    struct sockaddr_in in_addr = *(struct sockaddr_in *)result->ai_addr;
    freeaddrinfo(result);
    
    int sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock  > 0) {
        //struct sockaddr_in in_addr;
        //memset(&in_addr, 0, sizeof(in_addr));

        in_addr.sin_family = AF_INET;
        //in_addr.sin_addr.s_addr = inet_addr(host);
        in_addr.sin_port = htons(port);

        if (::connect(sock , (struct sockaddr*) &in_addr, 
            sizeof(in_addr)) == 0) {
            return sock;
        }
    }

    if (sock > 0) {
        close(sock);
    }
    return -1;
}

utcpsocket::utcpsocket(size_t buf_size, int sockfd,
    uschedule * sched, bool nonblock, 
    int socket_timeo, int connect_timeo, bool no_delay)
    : basic_tcp_stream(buf_size)
    , schedule_(sched)
    , socket_fd_(sockfd)
    , connect_timeo_(connect_timeo)
    , socket_timeo_(socket_timeo)
    , errno_(0)
{
    setnonblock(nonblock);
    setnodelay(no_delay);

    new_rdbuf(new utcpstreambuf(this, sched, buf_size_));

    event_.events   = EPOLLIN | EPOLLERR | EPOLLHUP;
    event_.data.ptr = this;
}

utcpsocket::~utcpsocket()
{
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

