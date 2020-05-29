#include "udp_server.h"


UdpServer::UdpServer(int port) {
	this->port = port;
	this->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (this->socket_fd < 0) {
		std::cout << "socket err:" << errno << std::endl;
		exit(1);
	}
	struct sockaddr_in addr_serv;
	memset(&addr_serv, 0, sizeof(struct sockaddr_in));
	addr_serv.sin_family = AF_INET;
	addr_serv.sin_port = htons(port);
	addr_serv.sin_addr.s_addr = htonl(INADDR_ANY);  //自动获取IP地址

	if (bind(this->socket_fd, (struct sockaddr *)&addr_serv, sizeof(addr_serv)) < 0) {
		std::cout << "bind err:" << errno << std::endl;
		exit(1);
	}
}
UdpServer::~UdpServer() {


}

void UdpServer::Start() {

	// struct sockaddr_in addr_client;
	// int len = sizeof(addr_client);
	// struct udp_buf udp_buf = {0x00};
	// while (1) {
	// 	int recv_num = recvfrom(this->socket_fd, &udp_buf, sizeof(struct udp_buf), 0, (struct sockaddr *)&addr_client, (socklen_t *)&len);
	// 	if (recv_num < 0) {
	// 		this->Abnormal(recv_num);
	// 		continue;
	// 		exit(1);
	// 	}
	// 	printf("task:%d\n", udp_buf.task);
	// 	this->Recvfrom(addr_client, &udp_buf);
	// }
	// close(this->socket_fd);
}

int  UdpServer::SendTo(struct udp_buf *send_buf, struct sockaddr_in addr) {	//发送数据
	int len = sizeof(struct sockaddr_in);
	return sendto(this->socket_fd, send_buf, sizeof(struct udp_buf), 0, (struct sockaddr *)&addr, len);
}

int UdpServer::Abnormal(int err) {
	printf("%s\n", "erro");
	return err;
}
/*
* 接受数据
 */
void UdpServer::Recvfrom(struct sockaddr_in addr, struct udp_buf *recv_buf) {

	// if (recv_buf->task == _TASK_LOGIN_) {
	// 	uint64 ms_time = this->GetMsTime();
	// 	std::map<uint32,  struct addr_map*>::iterator login_iter;
	// 	login_iter = this->login_time_map.find(recv_buf->from_userid);
	// 	struct addr_map *login_map = NULL;
	// 	if (login_iter != this->login_time_map.end()) {
	// 		login_map  = login_iter->second;
	// 		login_map->mstime = this->GetMsTime();
	// 		memcpy(&login_map->addr, &addr, sizeof(struct sockaddr_in));

	// 	} else {
	// 		login_map = (struct addr_map*)malloc(sizeof(struct addr_map));
	// 		memset(login_map, 0x00, sizeof(struct addr_map));
	// 		login_map->mstime = this->GetMsTime();
	// 		memcpy(&login_map->addr, &addr, sizeof(struct sockaddr_in));
	// 		this->login_time_map.insert(std::pair<uint32, struct addr_map* >(recv_buf->from_userid, login_map));
	// 	}

	// 	login_iter = this->login_time_map.find(recv_buf->from_userid);
	// 	login_map  = login_iter->second;
	// 	printf("mstime::%ld\n", login_map->mstime);
	// 	recv_buf->task = _TASK_BACK_LOGIN_;
	// 	this->SendTo(recv_buf, login_map->addr);
	// 	return;
	// }

	// if (recv_buf->task == _TASK_QUITE_) {
	// 	std::map<uint32, struct addr_map* >::iterator login_addr_iter;
	// 	login_addr_iter = this->login_time_map.find(recv_buf->from_userid);
	// 	if (login_addr_iter != this->login_time_map.end()) {
	// 		struct addr_map* login_addr =  login_addr_iter->second;
	// 		free(login_addr);
	// 		login_addr = NULL;
	// 		this->login_time_map.erase(login_addr_iter);
	// 	}
	// 	return;
	// }

	// if (recv_buf->task == _TASK_GET_ADDR_) {
	// 	std::map<uint32, struct addr_map* >::iterator login_addr_iter;
	// 	login_addr_iter = this->login_time_map.find(recv_buf->send_userid);
	// 	if (login_addr_iter != this->login_time_map.end()) {
	// 		struct addr_map* login_addr =  login_addr_iter->second;
	// 		memcpy(recv_buf->data, login_addr, sizeof(struct addr_map));
	// 		recv_buf->task = _TASK_BACK_ADDR_;
	// 	} else {
	// 		recv_buf->task = _TASK_ADDR_EXITS_;
	// 	}
	// 	this->SendTo(recv_buf, addr);
	// 	return;
	// }
	// if (recv_buf->type == 0x00) {	//转发单个client 消息时
	// 	// uint32 send_userid = recv_buf->send_userid;
	// 	std::map<uint32,  struct addr_map* >::iterator login_iter;
	// 	login_iter = this->login_time_map.find(recv_buf->send_userid);
	// 	if (login_iter != this->login_time_map.end()) {
	// 		struct addr_map *login_map = login_iter->second;;
	// 		int send_num = this->SendTo(recv_buf, login_map->addr);
	// 		printf("send_num::%d\n", send_num);
	// 	} else {
	// 		//发送给某个用户，未登录的情况，
	// 		// recv_buf->task = _TASK_NOT_LOGIN_;
	// 		this->ClientNotLoginCall(recv_buf, addr);
	// 	}
	// 	printf("forward:%s\n", recv_buf->data);
	// } else if (recv_buf->type == 0x02) {	//转发给多个client 消息时

	// }

}


int UdpServer::ClientNotLoginCall(struct udp_buf *send_buf, struct sockaddr_in addr) {	//转发给某个userid 时，某个useid 未登录时的回调函数
	printf("%s\n", "user not login");
	return 0;
}


/*生成数据包唯一值*/
uint64 UdpServer::GetMsTime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);   //该函数在sys/time.h头文件中
	return  tv.tv_sec * 1000 + tv.tv_usec / 1000;
}





















