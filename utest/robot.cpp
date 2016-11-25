#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>

static bool stop = false;

typedef struct tag_thread_args
{
    int num;
    char ip[32];
    int port;
} thread_args_t;

void * thread_f(void * args)
{
    thread_args_t * params = (thread_args_t *)args;

    int num = params->num;
    int * sockets = (int *)malloc(sizeof(int) * params->num);

    struct sockaddr_in in_addr;
    memset(&in_addr, 0, sizeof(in_addr));

    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = inet_addr(params->ip);
    in_addr.sin_port = htons(params->port);
    
    for (int i = 0; i < num; i++) {
        sockets[i] = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        connect(sockets[i], (struct sockaddr*) &in_addr, sizeof(in_addr));
    }

    std::string sendb;
    
    const char data[] = "abcdefghijk";

    char recvb[100];
    int len = htonl(strlen(data));
    //printf("data len:%zd, %s\n", strlen(data), data);

    sendb.append((char *)&len, sizeof(len));
    sendb.append(data);

    //printf("data len:%zd\n", sendb.size());
    
    while (!stop) {
        for (int i = 0; i < num; i++) {
            write(sockets[i], sendb.data(), sendb.size());
            //int nread = read(sockets[i], recvb, sizeof(data));
            //recvb[nread] = 0;
            //printf("buf:%s\n", recvb);
        }

        timespec t;
        int ms = 100;
    
        t.tv_sec    = ms / 1000; 
        t.tv_nsec   = (ms % 1000) * 1000000;
        int ret     = 0;
        
        do {
            ret = nanosleep(&t, &t);
        } while (ret == -1 && errno == EINTR); 
    }

    for (int i = 0; i < num; i++) {
        close(sockets[i]);
    }
    free(sockets);
}

int main(int argc, char * argv[])
{
    tag_thread_args args;
    int threadn = 1;
    
    memset(&args, 0, sizeof(args));
    args.num = 100;
    args.port = 8888;
    strcpy(args.ip, "127.0.0.1");
    
    char ch;
    while ((ch = getopt(argc,argv,"t:p:h:n:")) != -1) {
        switch(ch) {
        case 't':  
            threadn = atoi(optarg);
            break;  
        case 'p': 
            args.port = atoi(optarg);
            break;  
        case 'n': 
            args.num = atoi(optarg);
            break; 
        case 'h': 
            strcpy(args.ip, optarg);
            break; 
        default:  
            printf("other option :%c\n",ch);  
            break;
        }  
    }

    pthread_t * thread = (pthread_t *)malloc(sizeof(pthread_t) * threadn);
    for (int i = 0 ; i < threadn; i++) {
        pthread_create(&thread[i], NULL, thread_f, &args);
    }

    for (int i = 0 ; i < threadn; i++) {
        pthread_join(thread[i], NULL);
    }

    free(thread);
    return 0;
}
