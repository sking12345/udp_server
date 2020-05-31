#include "udp_server.h"


UdpServer::UdpServer(int port) {

	if ((this->sockt_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		Exception::save_error("socket error");
	}
	memset(&(this->server_addr), 0, sizeof(struct sockaddr_in));
	this->server_addr.sin_family = AF_INET;
	this->server_addr.sin_port = htons(port);
	this->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(this->sockt_fd, (struct sockaddr *)&this->server_addr, sizeof(struct sockaddr_in)) < 0) {
		Exception::save_error("bind error");
		exit(0);
	}
	this->from_user_id = 0x00;

}

UdpServer::~UdpServer() {
	printf("%s\n", "~UdpServer");
}



void UdpServer::excute_command() {	//执行指令

}
void UdpServer::set_command(std::string command) {	//设置指令

}


// void start() {
// 	this->recv_start();
// }



