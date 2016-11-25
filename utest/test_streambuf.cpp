#include "test_tcpstreambuf.h"
#include "test_tcpstream.h"

#include <string>

int main()
{
#if 0
	test_tcp_streambuf * outs = new test_tcp_streambuf(10);
	test_tcp_streambuf * ins = new test_tcp_streambuf(10);
	
	std::cout.rdbuf(outs);
	std::cout << "abcdefghijklmnopqrstuvwxyz" << std::endl;

	std::string in;
	std::cin.rdbuf(ins);

	std::cin >> in;
	std::cout << in;
#endif

	printf("----------------\n");
	std::string in;

	test_tcp_stream stream(10);
	stream.new_rdbuf(new test_tcp_streambuf(10));

	while (stream >> in) {
		stream << in << std::endl;
	}
}
