#include <iostream>
#include "core/exception.h"
#include "core/udp_base.h"
#include "core/udp_server.h"


class test {
  public:
	test() {
		std::cout << "test" << std::endl;
	}
	virtual ~test() {
		std::cout << "~test" << std::endl;
	}
	virtual void test3() {}
};

class test1 :  public test {
  public:
	test1() {
		std::cout << "tes1t" << std::endl;
	}
	~test1() {
		std::cout << "~test1" << std::endl;
	}
	void test3() {
		std::cout << "~test31" << std::endl;
	}
};

int main() {

	// test *T = new test1();
	// T->test3();
	// delete T;

	std::cout << _UDP_DATA_SIZE_ << std::endl;
	UdpServer *base = new UdpServer(9002);
	base->create_send_thread();
	base->create_read_thread();
	base->recv_start();

	delete base;

	return 0;
}