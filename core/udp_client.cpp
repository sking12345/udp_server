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

void UdpClient::login(uint32 userid) {
	this->from_user_id = userid;
	this->send_data(_TASK_LOGIN_, _UDP_PACK_PSP_); //向服务器发送登录指令
}
void UdpClient::readable_data(struct udp_pack recv_upd_pack, struct sockaddr_in clent_addr) {
	uint64 unique = recv_upd_pack.time * 10000 + recv_upd_pack.unique;
	if (recv_upd_pack.task == _TASK_GET_PACK_) {
		struct udp_pack *pack_data = this->get_send_pack(recv_upd_pack.send_id, unique, recv_upd_pack.sequence);
		this->sendTo(pack_data, clent_addr);
		Exception::save_info("get pack");
		return;
	} else if (recv_upd_pack.task == _TASK_USER_EXITE_) {
		Exception::save_info("_TASK_USER_EXITE_");	//
		this->send_feedback(recv_upd_pack, clent_addr);
		return;
	} else if (recv_upd_pack.task == _TASK_END_) {	//结束了某个任务的数据包发送,则需要将包,存入可用队列中
		printf("_TASK_END_:%d\n", recv_upd_pack.unique);
		pthread_mutex_lock(&(this->read_mutex));
		uint64 unique = recv_upd_pack.time * 10000 + recv_upd_pack.unique;
		this->free_send_data(unique);
		pthread_mutex_unlock(&(this->read_mutex));
		return;
	} else if (recv_upd_pack.task == _TASK_BACK_ADDR_) {	//保存返回的用户地址
		struct sockaddr_in back_user_addr = {0x00};
		memcpy(&back_user_addr, recv_upd_pack.data, sizeof(struct sockaddr_in));
		this->save_addr(back_user_addr, recv_upd_pack.send_id);
		Exception::save_info("back user addr");
		return;
	}
	pthread_mutex_lock(&(this->read_mutex));
	int data_pack_num = this->get_pack_num(recv_upd_pack.max_size);
	if (data_pack_num <= 1) {
		printf("sennd task end:%d\n", recv_upd_pack.from_id);
		this->recved_data(recv_upd_pack.data, recv_upd_pack.max_size, recv_upd_pack.task, recv_upd_pack.from_id, 0x00);
		this->confirm_end(recv_upd_pack, clent_addr);
		pthread_mutex_unlock(&(this->read_mutex));
		return;
	}

	this->recv_map_log_iter = this->recv_map_log.find(recv_upd_pack.from_id);
	if (this->recv_map_log_iter != this->recv_map_log.end()) {
		this->user_map_log = this->recv_map_log_iter->second;
		this->user_map_log_iter = this->user_map_log.find(unique);
		if (this->user_map_log_iter != this->user_map_log.end()) {
			this->pack_map_log = this->user_map_log_iter->second;
			if (this->pack_map_log.find(recv_upd_pack.sequence) != this->pack_map_log.end()) {
				pthread_mutex_unlock(&(this->read_mutex));
				return;
			}
		}
		this->pack_map_log.insert(std::pair<uint16, uint16>(recv_upd_pack.sequence, recv_upd_pack.sequence));
		this->recv_map_log[recv_upd_pack.from_id][unique] = this->pack_map_log;
	} else {
		this->pack_map_log.insert(std::pair<uint16, uint16>(recv_upd_pack.sequence, recv_upd_pack.sequence));
		this->user_map_log.insert(std::pair<uint64, std::map<uint16, uint16> >(unique, this->pack_map_log));
		this->recv_map_log.insert(std::pair<uint32, std::map<uint64, std::map<uint16, uint16> > >(recv_upd_pack.from_id, this->user_map_log));
	}

	uint8 *recv_data = NULL;
	this->recv_map_list_iter = this->recv_map_list.find(recv_upd_pack.from_id);
	if (this->recv_map_list_iter == this->recv_map_list.end()) {
		recv_data =  (uint8*)malloc(data_pack_num * _UDP_DATA_SIZE_);

		this->user_recv_map_list.insert(std::pair<uint64, uint8*>(unique, recv_data));
		this->recv_map_list.insert(std::pair<uint32, std::map<uint64, uint8*> >(recv_upd_pack.from_id, this->user_recv_map_list));
	} else {
		this->user_recv_map_list = this->recv_map_list_iter->second;
		this->user_recv_map_list_iter = this->user_recv_map_list.find(unique);
		if (this->user_recv_map_list_iter == this->user_recv_map_list.end()) {
			recv_data =  (uint8*)malloc(data_pack_num * _UDP_DATA_SIZE_);
			this->user_recv_map_list.insert(std::pair<uint64, uint8*>(unique, recv_data));
			this->recv_map_list[recv_upd_pack.from_id] = this->user_recv_map_list;
		} else {
			recv_data = this->user_recv_map_list_iter->second;
		}
	}
	memcpy(recv_data + _UDP_DATA_SIZE_ * recv_upd_pack.sequence, recv_upd_pack.data, _UDP_DATA_SIZE_);
	pthread_mutex_unlock(&(this->read_mutex));
	if (this->recv_map_log[recv_upd_pack.from_id][unique].size() == data_pack_num) {

		this->recved_data(recv_data, recv_upd_pack.max_size, recv_upd_pack.task, recv_upd_pack.from_id, unique);
		this->confirm_end(recv_upd_pack, clent_addr);
		this->recv_map_log_iter = this->recv_map_log.find(recv_upd_pack.from_id);
		if (this->recv_map_log_iter != this->recv_map_log.end()) {
			this->user_map_log = this->recv_map_log_iter->second;
			this->user_map_log_iter = this->user_map_log.find(unique);
			if (this->user_map_log_iter != this->user_map_log.end()) {
				this->pack_map_log = this->user_map_log_iter->second;
				this->pack_map_log_iter = this->pack_map_log.find(recv_upd_pack.sequence);
				if (this->pack_map_log_iter != this->pack_map_log.end()) {
					this->pack_map_log.erase(this->pack_map_log_iter);
				}
				this->user_map_log.erase(this->user_map_log_iter);
			}
			this->recv_map_log.erase(this->recv_map_log_iter);
		}
	}
}

void UdpClient::recved_data(uint8 * recv_data, uint32 data_size, uint16 task, uint32 userid, uint64 unique) { //接受到完整数据回调,正对client 端
	printf("recved_data:%s\n", recv_data);
	if (unique != 0x00) {
		this->free_recved_data(userid, unique);
	}
}


















