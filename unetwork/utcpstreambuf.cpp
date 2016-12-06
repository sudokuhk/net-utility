#include "utcpsocket.h"
#include "uschedule.h"
#include "utcpstreambuf.h"

utcpstreambuf::utcpstreambuf(utcpsocket * socket, 
    uschedule * sched, size_t buf_size)
    : basic_tcp_streambuf(buf_size)
    , socket_(socket)
    , schedule_(sched)
{
}

utcpstreambuf::~utcpstreambuf()
{   
    socket_   = NULL;
    schedule_ = NULL;
}

ssize_t utcpstreambuf::precv(void * buf, size_t len, int flags)
{
    return schedule_->recv(socket_, buf, len, flags);
}

ssize_t utcpstreambuf::psend(const void * buf, size_t len, int flags)
{
    return schedule_->send(socket_, buf, len, flags);
}

