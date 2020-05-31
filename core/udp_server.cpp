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

void UdpServer::readable_data(struct udp_pack recv_upd_pack, struct sockaddr_in clent_addr) {
	if (recv_upd_pack.task == _TASK_LOGIN_) {	//登录保存from_id
		Exception::save_info("loign sucess");
		recv_upd_pack.task = _TASK_LOGIN_SUCCESS_;
		this->save_addr(clent_addr, recv_upd_pack.from_id);
		this->send_data(&recv_upd_pack, clent_addr);	//登录成功
		return;
	} else if (recv_upd_pack.task == _TASK_LOGIN_SUCCESS_ || recv_upd_pack.task == _TASK_LOGIN_FAIL_) {	//登录保存from_id
		this->send_feedback(recv_upd_pack, clent_addr);
		Exception::save_info("loign status");
		return;
	} else if (recv_upd_pack.task == _TASK_QUITE_) {	//退出登录
		this->remove_socket_addr(recv_upd_pack.from_id);
		recv_upd_pack.task = _TASK_BACK_QUITE_;
		this->send_data(&recv_upd_pack, clent_addr);	//登录成功
		this->send_feedback(recv_upd_pack, clent_addr);
		return;
	} else if (recv_upd_pack.task == _TASK_BACK_QUITE_) {	//退出登录
		this->send_feedback(recv_upd_pack, clent_addr);
		this->remove_socket_addr(recv_upd_pack.from_id);
		Exception::save_info("quit loign status");
		return;
	} else if (recv_upd_pack.task == _TASK_GET_ADDR_) {
		Exception::save_info("get user addr");
		struct udp_addr *addr_info = this->get_client_addr(recv_upd_pack.send_id);
		if (addr_info != NULL) {
			recv_upd_pack.task = _TASK_BACK_ADDR_;
			memset(recv_upd_pack.data, 0x00, _UDP_DATA_SIZE_);
			memcpy(recv_upd_pack.data, &(addr_info->addr), sizeof(struct sockaddr_in));
			this->send_data(&recv_upd_pack, clent_addr);
		} else {
			recv_upd_pack.task = _TASK_USER_EXITE_;
			memset(recv_upd_pack.data, 0x00, _UDP_DATA_SIZE_);
			this->send_data(&recv_upd_pack, clent_addr);
		}
		return;
	}
	struct udp_addr *client_addr = this->get_client_addr(recv_upd_pack.send_id);
	if (client_addr != NULL) {
		printf("udp_pack->send_id:%d\n", recv_upd_pack.send_id);
		this->send_data(&recv_upd_pack, client_addr->addr);
	}
}
void UdpServer::recved_data(uint8 * recv_data, uint32 data_size, uint16 task, uint32 userid, uint64 unique) { //接受到完整数据回调,正对client 端

}
void UdpServer::excute_command() {	//执行指令

}
void UdpServer::set_command(std::string command) {	//设置指令

}



