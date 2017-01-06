#include "ustream/block_tcp_stream.h"
#include "test_tcpstreambuf.h"
#include "test_tcpstream.h"

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>


int main(int argc, char * argv[])
{
    std::string ip("127.0.0.1");
    int port = 8783;
    
	int sock = -1;
	struct sockaddr_in echoserver;
	
	/* Create the TCP socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		return -1;
    }

    /* Construct the server sockaddr_in structure */
    memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
    echoserver.sin_family = AF_INET;                  /* Internet/IP */
    echoserver.sin_addr.s_addr = inet_addr(ip.c_str());  /* IP address */
    echoserver.sin_port = htons(port);       /* server port */
	
    /* Establish connection */
    if (connect(sock,
                (struct sockaddr *) &echoserver,
                sizeof(echoserver)) < 0) {
    }

	block_tcp_stream sockstream(100);
	sockstream.attach(sock);

	std::string in;
	char sin[1024];

	test_tcp_stream input(10);
	input.new_rdbuf(new test_tcp_streambuf(100));

    //start
    //sockstream << "GET";
    //sockstream << "GET /abc/efg";
    sockstream << " GET   /abc/efg    HTTP/1.1   ";
    //sockstream << "get / HTTP/1.1";
    sockstream << "\r\n";
    
    //header
    sockstream << "a:b\r\n";
    sockstream << " c : d \r\n";
    sockstream << "aa" << "\r\n";
    sockstream << "\r\n";
    
    sockstream.flush();
    
    sleep(7);

    close(sock);
}
