#include "uschedule.h"
#include "utcpsocket.h"
#include "utimer.h"

#include "uthread/uruntime.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void utask::thread(void * args)
{
    if (args == NULL) {
        return;
    }

    utask * task = (utask *)args;
	
    task->run();

	return;
}

uschedule::utunnel::utunnel(uschedule & schedule)
    : schedule_(schedule)
    , running_(true)
{
    tunnel_[0] = tunnel_[1] = -1;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, tunnel_) == -1) { 
        assert(tunnel_[0] >= 0);
        //printf("error tunnel, %d, %d\n", tunnel_[0], tunnel_[1]);
    } 
    //printf("tunnel, %d, %d\n", tunnel_[0], tunnel_[1]);

    //set send timeout, avoid write to tunnel_[0] block, and thread hangup.
    struct timeval timeout = {0, 1};//3s
    setsockopt(tunnel_[0], SOL_SOCKET, SO_SNDTIMEO, 
        (const char*)&timeout, sizeof(timeout));
}

uschedule::utunnel::~utunnel()
{
    close(tunnel_[0]);
    close(tunnel_[1]);
}

void uschedule::utunnel::run()
{
    utcpsocket * socket = new utcpsocket(10, tunnel_[1], &schedule_, true, -1);
    char buf[2];
    
    while (running_) {
        if (schedule_.read(socket, buf, 1, 0) < 0) {
            break;
        }
        //printf("tunnel running!\n");
    }
    //printf("tunnel exit!\n");
    delete socket;
}

void uschedule::utunnel::emit()
{
    ssize_t nwrite = ::write(tunnel_[0], (void *)"w", 1);
    if (nwrite <= 0) {
        //printf("tunnel emit failed, errno:%d\n", errno);
    }
}

void uschedule::utunnel::shutdown()
{
    running_ = false;
    emit();
}

uschedule::uschedule(size_t stacksize, int maxtask, 
    bool protect_stack, ulock * lock)
    : runtime_(new uruntime(stacksize, protect_stack))
    , timermgr_(new utimermgr())
    , tunnel_(new utunnel(*this))
    , lock_(lock)
    , epoll_fd_(-1)
    , max_task_(maxtask)
    , running_(false)
    , stop_(true)
    , def_lock_()
{
    epoll_fd_ = epoll_create(max_task_);
    if (epoll_fd_ < 0) {
        assert(epoll_fd_ >= 0);
    }

    if (lock_ == NULL) {
        lock_ = &def_lock_;
    }
}

uschedule::~uschedule()
{
    close(epoll_fd_);
    epoll_fd_ = -1;

    delete runtime_;
    delete timermgr_;
    delete tunnel_;
}

void uschedule::add_task(utask * task)
{
    if (task == NULL) {
        return;
    }

    lock_->lock();
    task_queue_.push_back(task);
    lock_->unlock();

    wake();
}

bool uschedule::run()
{
    add_task(tunnel_);
    consume_task();

    struct epoll_event * events 
        = (struct epoll_event *)malloc(max_task_ * sizeof(struct epoll_event));

    running_ = true;
    stop_    = false;

    //printf("schedule running.\n");

    while (running_ && !stop_) {
        
        if (runtime_->finished()) {
            break;
        }

        //printf("epoll timeout:%d\n", timermgr_->next_timeo());
        int nfds = epoll_wait(epoll_fd_, 
            events, max_task_, timermgr_->next_timeo());

        //printf("epoll fds:%d\n", nfds);
        if (nfds >= 0) {
            for (int i = 0; i < nfds; i++) {
                utcpsocket * socket = (utcpsocket *)events[i].data.ptr;
                socket->waited_events() = events[i].events;
                //printf("epoll event, fd:%d, event:%d!\n", 
                //    socket->socket(), events[i].events);

                runtime_->resume(socket->thread());
            }

            if (stop_) {
                resume_all(en_epoll_close);
                break;
            }

            consume_task();
            dealwith_timeout();
        } else {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            resume_all(en_epoll_error);
            break;
        }
    }

    free(events);

    return true;
}

void uschedule::stop()
{
    running_ = false;
    stop_   = true;
    
    tunnel_->shutdown();
}

void uschedule::wake()
{
    tunnel_->emit();
}

void uschedule::consume_task()
{
    lock_->lock();
    while (!task_queue_.empty()) {
        utask * task = task_queue_.front();

        int id = runtime_->new_thread(utask::thread, (void *)task);
        runtime_->resume(id);

        task_queue_.pop_front();
    }
    lock_->unlock();
}

void uschedule::dealwith_timeout()
{
    while (timermgr_->next_timeo() == 0) {
        utimer * timer = timermgr_->pop();

        timer->waited_events() = en_epoll_timeout;
        
        runtime_->resume(timer->thread());
    }
}

void uschedule::resume_all(int reason)
{
    utimer * timer = NULL;
    
    while ((timer = timermgr_->pop()) != NULL) {
        timer->waited_events() = reason;
        runtime_->resume(timer->thread());
    }
}

///////////////////////////////////////////
int uschedule::poll(utcpsocket * socket, int events, int timeo)
{
    int ret = -1;
    int errno_ = 0;
    
    socket->thread() = runtime_->current();
    socket->event().events = events;
    
    timermgr_->add(timeo, socket);
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket->socket(), &socket->event());

    runtime_->yield();

    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, socket->socket(), &socket->event());
    timermgr_->remove(socket);

    int waited_event = socket->waited_events();
    //socket->waited_events() = 0;
    
    if (waited_event > 0) {
        if (waited_event & events) {
            ret = 1;
        } else {
            errno_ = EINVAL;
            ret = 0;
        }
    } else if (waited_event == en_epoll_timeout) {
        errno_ = ETIMEDOUT;
        ret = 0;
    } else if (waited_event == en_epoll_error) {
        errno_ = ECONNREFUSED;
        ret = -1;
    } else {
        errno_ = 0;
        ret = -1;
    }

    socket->set_errno(errno_);

    return ret;
}

int uschedule::accept(utcpsocket * socket, struct sockaddr * addr, 
    socklen_t * addrlen)
{
    int ret = -1;

    ret = ::accept(socket->socket(), addr, addrlen);
    if (ret < 0) {
        if (EAGAIN == errno || EWOULDBLOCK == errno) {
            if (poll(socket, EPOLLIN, -1) > 0) {
                ret = ::accept(socket->socket(), addr, addrlen);
            }
        }
    }

    return ret;
}

int uschedule::connect(utcpsocket * socket, struct sockaddr * addr, 
    socklen_t addrlen)
{
    int ret = -1;

    ret = ::connect(socket->socket(), addr, addrlen);
    if (ret < 0) {
        if (EAGAIN == errno || EINPROGRESS == errno) {
            if (poll(socket, EPOLLIN, -1) > 0) {
                ret = 0;
            }
        }
    }

    return ret;
}


ssize_t uschedule::read(utcpsocket * socket, void * buf, size_t len, int flags)
{
    int ret = ::read(socket->socket(), buf, len);

    if (ret < 0 && EAGAIN == errno) {
        if (poll(socket, EPOLLIN, socket->socket_timeo()) > 0) {
            ret = ::read(socket->socket(), buf, len);
        } else {
            ret = -1;
        }
    }

    return ret;
}

ssize_t uschedule::recv(utcpsocket * socket, void * buf, size_t len, int flags)
{   
    int ret = ::recv(socket->socket(), buf, len, flags);

    //printf("socket:%d, timeo:%d\n", socket->socket(), socket->socket_timeo());
    if (ret < 0 && EAGAIN == errno) {
        if (poll(socket, EPOLLIN, socket->socket_timeo()) > 0) {
            ret = ::recv(socket->socket(), buf, len, flags);
        } else {
            ret = -1;
        }
    }

    return ret;
}

ssize_t uschedule::send(utcpsocket * socket, 
    const void * buf, size_t len, int flags)
{   
    int ret = ::send(socket->socket(), buf, len, flags);

    if (ret < 0 && EAGAIN == errno) {
        if (poll(socket, EPOLLOUT, socket->socket_timeo()) > 0) {
            ret = ::send(socket->socket(), buf, len, flags);
        } else {
            ret = -1;
        }
    }

    return ret;
}

void uschedule::wait(utimer * timer, int waitms)
{
    timer->thread() = runtime_->current();
    timermgr_->add(waitms, timer);
    runtime_->yield();
    timermgr_->remove(timer);
}
