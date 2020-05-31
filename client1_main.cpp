#include <iostream>
#include "core/udp_base.h"
#include "core/exception.h"
#include "core/udp_client.h"

int main() {


	std::cout << _UDP_DATA_SIZE_ << std::endl;
	std::cout << sizeof(struct udp_pack) << std::endl;
	const char *IP = "127.0.0.1";
	UdpClient *base = new UdpClient(IP, 9002);

	base->create_send_thread();
	base->create_read_thread(0x01);
	base->login(1234);

	sleep(10000000);
	delete base;

	return 0;
}