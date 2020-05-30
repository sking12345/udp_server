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
};

class test1 :  public test {
  public:
	test1() {
		std::cout << "tes1t" << std::endl;
	}
	~test1() {
		std::cout << "~test1" << std::endl;
	}
};

int main() {

	// test1 *T = new test1();
	// delete T;
	//
	std::cout << _UDP_DATA_SIZE_ << std::endl;
	UdpServer *base = new UdpServer(9002);
	base->create_available_pack(100);
	base->create_send_thread();
	base->create_read_thread();
	base->recv_start();

	delete base;


	// exception::save_info("zxxx", __FILE__, __LINE__);
	//
	// UdpServer *udp_server = new UdpServer(9002);
	// std::count << udp_server->GetMsTime() << std::endl;
	// udp_server->Start();
	return 0;
}