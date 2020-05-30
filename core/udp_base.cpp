#include "udp_base.h"

void* send_thread_function(void* arg) {
	UdpBase *udp_base = (UdpBase*)arg;
	printf("%s\n", "xx");
	while (udp_base->send_thread_status == _THREAD_RUN_) {
		pthread_mutex_lock(&(udp_base->send_mutex));
		if (udp_base->wait_send_queue <= 0) {
			pthread_cond_wait(&(udp_base->send_cond), &(udp_base->send_mutex));
		}
		if (udp_base->send_thread_status != _THREAD_RUN_) {
			pthread_mutex_unlock(&(udp_base->send_mutex));
			pthread_exit(0);
			return NULL;
		}
		struct udp_pack* send_pack = udp_base->send_list.front();
		if (send_pack->type == _UDP_PACK_P2P_) {	//	//p2p
			struct udp_addr *user_addr = udp_base->get_socket_addr(send_pack->send_id);
			if (user_addr != NULL) {
				udp_base->sendTo(send_pack, user_addr->addr);
				udp_base->send_list.erase(udp_base->send_list.begin());
				udp_base->wait_send_queue--;
			} else if (udp_base->get_mstime() - send_pack->time >= _OVER_SEND_TIME_) {	//数据超时为发送出去
				udp_base->send_time_out(send_pack->unique, send_pack->task, 0x00);	//超时为收到p2p 的addr
				udp_base->wait_send_queue--;
				udp_base->send_list.erase(udp_base->send_list.begin());
			}
		} else {
			udp_base->sendTo(send_pack, udp_base->server_addr);
			udp_base->send_list.erase(udp_base->send_list.begin());
			udp_base->wait_send_queue--;

		}
		pthread_mutex_unlock(&(udp_base->send_mutex));
	}

	return NULL;
}

void* read_thread_function(void* arg) {
	UdpBase *udp_base = (UdpBase*)arg;
	udp_base->recv_start();
	return NULL;
}



UdpBase::UdpBase() {
	this->task_queue = 0;
	this->from_user_id = 0x00;

}

UdpBase::~UdpBase() {
	printf("%s\n", "～UdpBase");
}
void UdpBase::recv_start() {
	struct sockaddr_in clent_addr = {0x00};
	struct udp_pack recv_upd_pack = {0x00};
	int recv_num = 0;
	socklen_t len = sizeof(struct sockaddr_in);
	int recv_size = sizeof(struct udp_pack);
	while (this->read_thread_status == _THREAD_RUN_) {
		struct sockaddr_in clent_addr = {0x00};
		struct udp_pack recv_upd_pack = {0x00};
		int recv_num = 0;
		socklen_t len = sizeof(struct sockaddr_in);
		int recv_size = sizeof(struct udp_pack);
		while (this->read_thread_status == _THREAD_RUN_) {
			recv_num = recvfrom(this->sockt_fd, &recv_upd_pack, recv_size, 0, (struct sockaddr *)&clent_addr, (socklen_t *)&len);
			if (recv_num <= 0) {
				Exception::save_error("recvfrom error: recv_num == 0");
				continue;
			}
			printf("recv str:%s\n", (char*)recv_upd_pack.data );
			this->save_addr(clent_addr, recv_upd_pack.from_id);
			if (recv_upd_pack->task == _TASK_LOGIN_) {	//登录保存from_id
				recv_upd_pack->task = _TASK_BACK_LOGIN_;
				this->send_data(recv_upd_pack, clent_addr);	//登录成功
				continue;
			}
			if (recv_upd_pack->task == _TASK_LOGIN_SUCCESS_ || recv_upd_pack->task == _TASK_LOGIN_FAIL_) {	//登录保存from_id
				this->recvfrom(NULL, 0, recv_upd_pack->task, 0x00, recv_upd_pack->task, recv_upd_pack->type);
				Exception::save_info(loign status);
				continue;
			}
			if (recv_upd_pack->task == _TASK_QUITE_) {	//退出登录
				this->remove_socket_addr(recv_upd_pack->from_id);
				recv_upd_pack->task = _TASK_BACK_QUITE_;
				this->send_data(recv_upd_pack, clent_addr);	//登录成功
				continue;
			}
			if (recv_upd_pack->task == _TASK_BACK_QUITE_) {	//退出登录
				this->recvfrom(NULL, 0, recv_upd_pack->task, 0x00, recv_upd_pack->task, recv_upd_pack->type);
				Exception::save_info("loign status");
				continue;
			}
			if (recv_upd_pack->task == _TASK_GET_ADDR_) {
				struct udp_addr *addr_info = this->get_socket_addr(recv_upd_pack->send_id);
				recv_upd_pack->task = _TASK_BACK_ADDR_;
				memset(recv_upd_pack->data, 0x00, _UDP_DATA_SIZE_);
				memcpy(recv_upd_pack->data, addr_info->addr, sizeof(struct sockaddr_in));
				this->send_data(recv_upd_pack, clent_addr);	//登录成功
				// save_addr
			}


			if (this->read_type ==  0x00) {	//立即回调
				this->recvfrom(&recv_upd_pack, clent_addr);
			} else if (this->read_type ==  0x01) {

			}
		}

	}
}

void UdpBase::recvfrom(struct udp_pack*, struct sockaddr_in addr) { //立即回调函数,针对服务端

}
void UdpBase::recvfrom(uint8 *data, uint32 data_size, uint16 task, uint32 userid) { //接受到完整数据回调,正对client 端

}
int UdpBase::send_data(struct udp_pack* pack_data, struct sockaddr_in addr) {
	pthread_mutex_lock(&(this->send_mutex));
	this->sendTo(pack_data, addr);
	pthread_mutex_unlock(&(this->send_mutex));
	pthread_cond_signal(&(this->send_cond));
	return 1;
}
int UdpBase::send_data(void *send_data, int data_size, uint32 send_id, uint16 task, uint8 type) {

	int pack_num = 0;
	if (data_size > _UDP_DATA_SIZE_) {
		if (data_size % _UDP_DATA_SIZE_ > 0) {
			pack_num = (data_size - (data_size % _UDP_DATA_SIZE_)) / _UDP_DATA_SIZE_ + 1;
		} else {
			pack_num = data_size / _UDP_DATA_SIZE_;
		}
	} else {
		pack_num = 1;
	}
	if (this->wait_send_queue > _MAX_TASK_LIST_) {
		Exception::save_error("wait_send_queue > max task list");
		return -1;
	}
	if (this->task_queue >= 65500) {
		this->task_queue = 0;
	}
	this->task_queue++;
	std::map<uint32, std::map<uint64, struct udp_pack*> >::iterator send_map_list_iter;
	std::map<uint64, struct udp_pack*> send_user_map;
	send_map_list_iter = this->send_map_list.find(send_id);
	if (send_map_list_iter != this->send_map_list.end()) {
		send_user_map = send_map_list_iter->second;

	}
	time_t time = this->get_mstime();
	for (int i = 0; i < pack_num; ++i) {
		struct udp_pack *pack_data = this->get_available_pack();
		pack_data->version = _VERSION_;
		pack_data->max_size = data_size;
		pack_data->type = type;
		pack_data->sequence = i;
		pack_data->from_id = this->from_user_id;
		pack_data->send_id = send_id;
		pack_data->task = task;
		pack_data->unique = this->task_queue;
		pack_data->time = time;
		if (data_size > _UDP_DATA_SIZE_) {
			if (i < pack_num - 1) {
				memcpy(pack_data->data, (uint8*)send_data + (i * _UDP_DATA_SIZE_), _UDP_DATA_SIZE_);
			} else {
				memcpy(pack_data->data, (uint8*)send_data + (i * _UDP_DATA_SIZE_), data_size % _UDP_DATA_SIZE_);
			}
		} else {
			memcpy(pack_data->data, send_data, data_size % _UDP_DATA_SIZE_);
		}
		pthread_mutex_lock(&(this->send_mutex));
		this->send_list.push_back(pack_data);
		this->wait_send_queue++;
		pthread_mutex_unlock(&(this->send_mutex));
		pthread_cond_signal(&(this->send_cond));
		uint64 unique = time * 100000 + this->task_queue;
		printf("unique:%ld\n", unique);
		send_user_map.insert(std::pair<uint64, struct udp_pack*>(unique, pack_data));
	}

	send_map_list_iter = this->send_map_list.find(send_id);
	if (send_map_list_iter == this->send_map_list.end()) {
		this->send_map_list.insert(std::pair<uint32, std::map<uint64, struct udp_pack*> >(send_id, send_user_map));
	} else {
		this->send_map_list[send_id] = send_user_map;
	}
	return this->task_queue;
}


int UdpBase::create_available_pack(int len) {
	for (int i = 0; i < len; ++i) {
		struct udp_pack *pack = (struct udp_pack*)malloc(sizeof(struct udp_pack));
		memset(pack, 0x00, sizeof(struct udp_pack));
		this->available_pack_list.push_back(pack);

	}
	return 1;
}
struct udp_pack* UdpBase::get_available_pack() {
	struct udp_pack* pack = NULL;
	if ( this->available_pack_list.size() > 0) {
		pack = this->available_pack_list.front();
		this->available_pack_list.pop_front();
	} else {
		pack = (struct udp_pack*)malloc(sizeof(struct udp_pack));
		memset(pack, 0x00, sizeof(struct udp_pack));
		this->available_pack_list.push_back(pack);
	}
	return pack;
}

int UdpBase::save_available_pack(struct udp_pack* pack) {
	this->available_pack_list.push_back(pack);
	return 1;
}



void UdpBase::create_read_thread(uint8 type) {
	this->read_type = type;
	this->read_thread_status = _THREAD_RUN_ ;	//读线程状态
	pthread_create(&(this->read_thread_id), NULL, read_thread_function, (void *)this);
}
void UdpBase::create_send_thread() {
	pthread_cond_init(&(this->send_cond), NULL);
	pthread_mutex_init(&(this->send_mutex), NULL);
	this->send_thread_status = _THREAD_RUN_ ;	//写线程状态
	pthread_create(&(this->send_thread_id), NULL, send_thread_function, (void *)this);
}

void UdpBase::save_addr(struct sockaddr_in from_addr, uint32 userId) {
	std::map<uint32, struct udp_addr*>::iterator addr_map_iter;
	addr_map_iter = this->users_addr_map.find(userId);
	if (addr_map_iter !=  this->users_addr_map.end()) {
		struct udp_addr *addr = addr_map_iter->second;
		addr->last_time = this->get_mstime();
		memcpy(&(addr->addr), &from_addr, sizeof(struct sockaddr_in));
	} else {
		struct udp_addr *addr = (struct udp_addr*)malloc(sizeof(struct udp_addr));
		memset(addr, 0x00, sizeof(struct udp_addr));
		addr->last_time = this->get_mstime();
		memcpy(&(addr->addr), &from_addr, sizeof(struct sockaddr_in));
		this->users_addr_map.insert(std::pair<uint32, struct udp_addr*>(userId, addr));
	}
}

uint64 UdpBase::get_mstime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);   //该函数在sys/time.h头文件中
	return  tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/*发送数据*/
int UdpBase::sendTo(struct udp_pack* pack_data, struct sockaddr_in addr) {
	int len = sizeof(struct sockaddr_in);
	return sendto(this->sockt_fd, pack_data, sizeof(struct udp_pack), 0, (struct sockaddr *)&addr, len);
}

/**
 * [UdpBase::get_addr 某个用户的addr]
 * @param  userId [description]
 * @return        [description]
 */
struct udp_addr *UdpBase::get_socket_addr(uint32 userId) {
	std::map<uint32, struct udp_addr*>::iterator users_addr_map_iter;
	users_addr_map_iter = this->users_addr_map.find(userId);
	if (users_addr_map_iter != this->users_addr_map.end()) {
		return users_addr_map_iter->second;
	}
	return NULL;
}
//删除用户的addr
void UdpBase::remove_socket_addr(uint32 userId) {

}

/**
 * [UdpBase::send_time_out 超时未发送出去,p2p 未获取到用户的]
 * @param unique [description]
 * @param task   [description]
 * type :0x00: 超时未获取到addr
 * 	 	0x01: 超时未获取到数据确认
 */
void UdpBase::send_time_out(uint16 unique, uint8 task, uint8 type) {

}































