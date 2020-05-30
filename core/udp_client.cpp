#include "udp_client.h"



UdpClient::UdpClient(const char* server_ip, int port) {
	if ((this->sockt_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		Exception::save_error("socket error");
	}
	memset(&this->server_addr, 0, sizeof( struct sockaddr_in));
	this->server_addr.sin_family = AF_INET;
	this->server_addr.sin_port = htons(port);
	this->server_addr.sin_addr.s_addr = inet_addr(server_ip);
	this->from_user_id = 123;

}
UdpClient::~UdpClient() {
	printf("%s\n", "~UdpClient");
}

void UdpClient::login(uint32 userid) {
	this->from_user_id = userid;
	this->send_data(_TASK_LOGIN_, _UDP_PACK_PSP_); //向服务器发送登录指令

}
void UdpClient::get_user_addr(uint32 userid) {

}