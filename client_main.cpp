#include <iostream>
#include "core/exception.h"
#include "core/udp_client.h"

int main() {


	std::cout << _UDP_DATA_SIZE_ << std::endl;
	const char *IP = "127.0.0.1";
	UdpClient *base = new UdpClient(IP, 9002);
	base->create_available_pack(100);
	base->create_send_thread();
	base->create_read_thread();
	const char *str = "ddddddx";
	base->send_data((void*)str, strlen(str), 123, 0x00, _UDP_PACK_PSP_);

	sleep(1);
	delete base;

	return 0;
}