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
	int sock = -1;
	struct sockaddr_in echoserver;
	
	/* Create the TCP socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		return -1;
    }


    /* Construct the server sockaddr_in structure */
    memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
    echoserver.sin_family = AF_INET;                  /* Internet/IP */
    echoserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
    echoserver.sin_port = htons(atoi(argv[2]));       /* server port */
	
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

	while (input >> in) {
		sockstream << in;
		sockstream.flush();

		//memset(sin, 0, 1024);
		if (sockstream.getline(sin, 1024).good()) {
			input << sin << std::endl;
		} else {
			input << "error! " << sockstream.last_error() << std::endl;
			break;
		}
	}
	
}
