#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//#define UUDP_STREAM

#include <unetwork/uudpsocket.h>
#include <unetwork/uschedule.h>

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

class mytask : public utask
{
public:
    mytask(uschedule & sche, uudpsocket & socket, bool server) 
        : schedule_(sche)
        , socket_(socket)
        , server_(server)
    {
    }

    virtual ~mytask() 
    {
    }
    
    virtual void run()
    {
        char * buf = (char *)malloc(4096);
        
        if (server_) {
            printf("server mode!\n");
            int rdn, wtn;
            char buf[1025];
            struct sockaddr_in src_addr;
            socklen_t addrlen = sizeof(src_addr);
            
            while (1) {
                rdn = socket_.recvfrom(buf, 1024, 0, (struct sockaddr *)&src_addr, &addrlen);
                if (rdn > 0) {
                    buf[rdn] = 0;
                    printf("recv from[%s:%d]:(%d)%s\n", 
                        inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port), 
                        rdn, buf);

                    wtn = socket_.sendto(buf, rdn, 0, (struct sockaddr *)&src_addr, addrlen);
                    printf("write back:%d\n", wtn);
                } else {
                    printf("errno:%d, rdn:%d\n", socket_.get_errno(), rdn);
                }
            }
        } else {
            #ifdef UUDP_STREAM
            std::string in;
            std::streamsize rdn;
            
            while (std::cin >> in) {
                rdn = in.size();
                socket_.write((char *)&rdn, 4);
                if (!socket_.good()) {
                    fprintf(stderr, "bad socket!\n");
                    break;
                }
                socket_  << in;
                socket_.flush();

                socket_.read((char *)&rdn, 4);
                std::cout << "rdn:" << rdn << std::endl;
                if (socket_.good()) {
                    socket_.read(buf, rdn);
                    buf[rdn] = 0;
                    std::cout << "read:[" << buf << "]\n";
                }
            }
            #else
            char buf[1024];
            std::string in;
            int wrn, rdn;
            while (std::cin >> in) {
                if ((wrn = socket_.send(in.c_str(), in.size(), 0)) != in.size()) {
                    printf("send failed!, want:%zd, acc:%d\n", in.size(), wrn);
                }

                rdn = socket_.recv(buf, 1024, 0);
                if (rdn == 0) {
                    printf("errno:%d, rdn:%d\n", socket_.get_errno(), rdn);
                }
                if (rdn > 0) {
                    buf[rdn] = 0;
                    printf("echo back:%s\n", buf);
                }
            }
            #endif
        }
    }

private:
    uschedule & schedule_;
    uudpsocket & socket_;
    bool server_;
};

int main(int argc, char * argv[])
{
    bool client = false;
    bool server = false;
    int ch;

    std::string host;
    int port1, port2;
    
    while((ch = getopt(argc, argv, "h:p:b:sc")) != -1) {
        switch(ch) {
            case 'h': 
                host = optarg;
                break;
            case 'p':
                port1 = atoi(optarg);
                break;
            case 'b':
                port2 = atoi(optarg);
                break;
            case 'c':
                client  = true;
                break;
            case 's':
                server  = true;
                break;
            default:
                fprintf(stderr, "unknown arguments. (%c)", ch);
                break;
        }
    }

    if (server && client) {
        server = false;
    }
    if (!server && !client) {
        client = true;
    }

    if (server && port2 == 0) {
        fprintf(stderr, "server mode, bind port 0\n");
        exit(0);
    }

    if (client && (host.empty() || port1 == 0)) {
        fprintf(stderr, "host or port null!\n");
        exit(0);
    }

    uschedule schedule(1024, 100, true, new mylock());
    #ifdef UUDP_STREAM
    uudpsocket socket(1024, schedule);
    socket.bind(NULL, port2);
    if (client) {
        socket.connect(host.c_str(), port1);
    }
    #else
    int timeo = 10 * 1000;
    if (server) {
        timeo = -1;
    }
    uudpsocket socket(schedule, timeo); //timeout 10s
    if (port2 != 0) {
        socket.bind(NULL, port2);
    }
    if (client) {
        socket.connect(host.c_str(), port1);
    }
    #endif
    mytask task(schedule, socket, server);
    schedule.add_task(&task);
    schedule.run();

    return 0;
}
