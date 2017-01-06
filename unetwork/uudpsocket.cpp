#include "uudpsocket.h"
#include "uschedule.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef UUDP_STREAM
uudpstreambuf::uudpstreambuf(uudpsocket & socket, 
    uschedule & sched, size_t buf_size)
    : basic_udp_streambuf(buf_size)
    , socket_(socket)
    , schedule_(sched)
{
}

uudpstreambuf::~uudpstreambuf()
{   
}

ssize_t uudpstreambuf::precv(void * buf, size_t len, int flags)
{
    return schedule_.recv(&socket_, buf, len, flags);
}

ssize_t uudpstreambuf::psend(const void * buf, size_t len, int flags)
{
    // udp, send directly.
    return ::send(socket_.socket(), buf, len, flags);
}

uudpsocket::uudpsocket(size_t buf_size, int sockfd,
    uschedule & sched, bool nonblock, int socket_timeo)
    : basic_udp_stream(buf_size)
    , schedule_(sched)
    , socket_fd_(sockfd)
    , socket_timeo_(socket_timeo)
    , errno_(0)
    , close_when_destroy_(true)
{
    setnonblock(nonblock);

    new_rdbuf(new uudpstreambuf(*this, sched, buf_size_));

    event_.events   = EPOLLIN | EPOLLERR | EPOLLHUP;
    event_.data.ptr = this;
}

uudpsocket::uudpsocket(size_t buf_size, uschedule & sched, 
        bool nonblock, int socket_timeo)
    : basic_udp_stream(buf_size)
    , schedule_(sched)
    , socket_fd_(-1)
    , socket_timeo_(socket_timeo)
    , errno_(0)
    , close_when_destroy_(true)
{
    new_rdbuf(new uudpstreambuf(*this, sched, buf_size_));
    event_.events   = EPOLLIN | EPOLLERR | EPOLLHUP;
    event_.data.ptr = this;
}

uudpsocket::~uudpsocket()
{
    if (close_when_destroy_ && socket_fd_ > 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool uudpsocket::set_timeout(int ms)
{
    socket_timeo_ = ms;
    return true;
}

bool uudpsocket::connect(const char * ip, int port)
{
    if (ip == NULL || port == 0) {
        return false;
    }

    if (socket_fd_ < 0) {
        socket_fd_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    if (socket_fd_ < 0) {
        return false;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family       = AF_INET;
    addr.sin_addr.s_addr  = inet_addr(ip);
    addr.sin_port         = htons(port);

    if (::connect(socket_fd_, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        return false;
    }

    return setnonblock(true);    
}

bool uudpsocket::bind(const char * ip, int port)
{
    if (port == 0) {
        return false;
    }

    if (socket_fd_ < 0) {
        socket_fd_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    if (socket_fd_ < 0) {
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family       = AF_INET;
    addr.sin_addr.s_addr  = ip == NULL ? htonl(INADDR_ANY) : inet_addr(ip);
    addr.sin_port         = htons(port);

    if (::bind(socket_fd_, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        return false;
    }

    return setnonblock(true);  
}

int uudpsocket::last_error()
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

void uudpsocket::remote_info(char * ipb, size_t ipl, int * port)
{
}

int uudpsocket::socket()
{
    return socket_fd_;
}

bool uudpsocket::setnonblock(bool nonblock)
{
    int ret = 0;

    int tmp = fcntl(socket_fd_, F_GETFL, 0);

    if (nonblock) {
        if (tmp | O_NONBLOCK) {
            ret = fcntl(socket_fd_, F_SETFL, tmp | O_NONBLOCK);
        } else {
            ret = 0;
        }
    } else {
        ret = fcntl(socket_fd_, F_SETFL, tmp & (~O_NONBLOCK));
    }

    return 0 == ret;
}

#else
uudpsocket::uudpsocket(uschedule & sched, int timeo)
    : sched_(sched)
    , sock_(-1)
    , timeo_(timeo)
    , errno_(0)
{
    event_.events   = EPOLLIN | EPOLLERR | EPOLLHUP;
    event_.data.ptr = this;
}

uudpsocket::~uudpsocket()
{
    if (sock_ > 0) {
        ::close(sock_);
        sock_ = -1;
    }
}

int uudpsocket::bind(const char * ip, int port)
{
    if (port == 0) {
        return false;
    }

    if (sock_ < 0) {
        sock_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    if (sock_ < 0) {
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family       = AF_INET;
    addr.sin_addr.s_addr  = ip == NULL ? htonl(INADDR_ANY) : inet_addr(ip);
    addr.sin_port         = htons(port);

    if (::bind(sock_, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        return false;
    }

    return setnonblock(true);  
}

int uudpsocket::connect(const char * ip, int port)
{
    if (ip == NULL || port == 0) {
        return false;
    }

    if (sock_ < 0) {
        sock_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    if (sock_ < 0) {
        return false;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family       = AF_INET;
    addr.sin_addr.s_addr  = inet_addr(ip);
    addr.sin_port         = htons(port);

    // udp, return almost immediately.
    if (::connect(sock_, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        return false;
    }

    return setnonblock(true);    
}

ssize_t uudpsocket::write(const void *buf, size_t count)
{
    if (sock_ > 0) {
        return ::write(sock_, buf, count);
    }
    return -1;
}

ssize_t uudpsocket::send(const void *buf, size_t len, int flags)
{
    if (sock_ > 0) {
        return ::send(sock_, buf, len, flags);
    }
    return -1;
}

ssize_t uudpsocket::sendto(const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen)
{
    if (sock_ > 0) {
        return ::sendto(sock_, buf, len, flags, dest_addr, addrlen);
    }
    return -1;
}

ssize_t uudpsocket::read(void *buf, size_t count)
{
    if (sock_ < 0) {
        return -1;
    }
    
    ssize_t ret = ::read(sock_, buf, count);
    if (ret < 0 && EAGAIN == errno) {
        if (sched_.poll(this, EPOLLIN, timeo_) > 0) {
            ret = ::read(sock_, buf, count);
        } else {
            ret = -1;
        }
    }

    return ret;
}

ssize_t uudpsocket::recv(void *buf, size_t len, int flags)
{
    if (sock_ < 0) {
        return -1;
    }
    
    ssize_t ret = ::recv(sock_, buf, len, flags);
    if (ret < 0 && EAGAIN == errno) {
        if (sched_.poll(this, EPOLLIN, timeo_) > 0) {
            ret = ::recv(sock_, buf, len, flags);
        } else {
            ret = -1;
        }
    }

    return ret;
}

ssize_t uudpsocket::recvfrom(void *buf, size_t len, int flags,
                    struct sockaddr *src_addr, socklen_t *addrlen)
{
    if (sock_ < 0) {
        return -1;
    }
    
    ssize_t ret = ::recvfrom(sock_, buf, len, flags, src_addr, addrlen);
    if (ret < 0 && EAGAIN == errno) {
        if (sched_.poll(this, EPOLLIN, timeo_) > 0) {
            ret = ::recvfrom(sock_, buf, len, flags, src_addr, addrlen);
        } else {
            ret = -1;
        }
    }

    return ret;
}

bool uudpsocket::setnonblock(bool nonblock)
{
    int ret = 0;

    int tmp = fcntl(sock_, F_GETFL, 0);

    if (nonblock && (tmp & O_NONBLOCK) == 0) {
        ret = fcntl(sock_, F_SETFL, tmp | O_NONBLOCK);
    } else if (!nonblock && (tmp & O_NONBLOCK) != 0) {
        ret = fcntl(sock_, F_SETFL, tmp & (~O_NONBLOCK));
    }

    return 0 == ret;
}


#endif
