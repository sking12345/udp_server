#include "udp_client.h"



UdpClient::UdpClient(const char* server_ip, int port) {
	if ((this->sockt_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		Exception::save_error("socket error");
	}
	memset(&this->server_addr, 0, sizeof( struct sockaddr_in));
	this->server_addr.sin_family = AF_INET;
	this->server_addr.sin_port = htons(port);
	this->server_addr.sin_addr.s_addr = inet_addr(server_ip);

}
UdpClient::~UdpClient() {
	printf("%s\n", "~UdpClient");
}

