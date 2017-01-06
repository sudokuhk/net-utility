#ifndef _USCHEDULE_H__
#define _USCHEDULE_H__

#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <assert.h>

#include <deque>

class uruntime;
class utimermgr;
class utcpsocket;
class utimer;
class uudpsocket;

enum 
{
    en_socket_refused = -1,
    en_socket_timeout = -300,
    en_socket_normal_closed = -301,
};

enum 
{
    en_epoll_timeout = 0,
    en_epoll_error   = -1,
    en_epoll_close   = -2,
};

class utask
{
public:
	virtual ~utask(void) {}
	
	virtual void run() = 0;

	static void thread(void * args);
};

class ulock
{
public:
    virtual ~ulock() {}

    virtual void lock() = 0;

    virtual void unlock() = 0;
};

class uschedule 
{
private:
    class utunnel : public utask
    {
    public:
        utunnel(uschedule & schedule);

        virtual ~utunnel();

        virtual void run();

        void emit();

        void shutdown();

    private:
        uschedule & schedule_;
        int tunnel_[2];
        bool running_;
    };

    class default_lock : public ulock
    {
    public:
        virtual ~default_lock() {}

        virtual void lock() {}

        virtual void unlock() {}
    };
    
public:
    uschedule(size_t stacksize, int maxtask, 
        bool protect_stack = true, ulock * lock = NULL);

	~uschedule();

	void add_task(utask * task);

	bool run();

	void stop();

	void wake();

public:
    int poll(utcpsocket * socket, int events, int timeo);
    int accept(utcpsocket* socket, struct sockaddr* addr, socklen_t* addrlen);
    int connect(utcpsocket* socket, struct sockaddr* addr, socklen_t addrlen);
    ssize_t read(utcpsocket * socket, void * buf, size_t len, int flags);
    ssize_t recv(utcpsocket * socket, void * buf, size_t len, int flags);
    ssize_t send(utcpsocket * socket, const void * buf, size_t len, int flags);

    int poll(uudpsocket * socket, int events, int timeo);
    #ifdef UUDP_STREAM
    ssize_t read(uudpsocket * socket, void * buf, size_t len, int flags);
    ssize_t recv(uudpsocket * socket, void * buf, size_t len, int flags);
    #endif
    
    void wait(utimer * timer, int waitms);
    
private:
	void consume_task();
	void dealwith_timeout();
    void resume_all(int reason);
	
private:
	uruntime *  runtime_;
    utimermgr * timermgr_;
    utunnel *   tunnel_;
    ulock *     lock_;
        
	int epoll_fd_;
	int max_task_;
	bool running_;
	bool stop_;
    
    default_lock def_lock_;
	std::deque<utask *> task_queue_;   
};

#endif