#include "udp_base.h"

void* send_thread_function(void* arg) {
	UdpBase *udp_base = (UdpBase*)arg;
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
		if (udp_base->wait_send_queue <= 0) {
			continue;
		}
		struct udp_pack* send_pack = udp_base->send_list.front();
		if (send_pack->type == _UDP_PACK_P2P_) {	//	//p2p
			struct udp_addr *user_addr = udp_base->get_client_addr(send_pack->send_id);
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
	this->test_num = 0;
	pthread_cond_init(&(this->read_cond), NULL);
	pthread_mutex_init(&(this->read_mutex), NULL);

}

UdpBase::~UdpBase() {
	printf("%s\n", "～UdpBase");
	this->send_thread_status = _THREAD_EXT_;
	this->read_thread_status = _THREAD_EXT_;
	pthread_mutex_lock(&(this->send_mutex));
	pthread_cancel(this->send_thread_id);
	pthread_mutex_unlock(&(this->send_mutex));
	pthread_cancel(this->read_thread_id);
	std::map < uint64, std::map<uint16, struct udp_pack*> >::iterator send_map_list_iter;
	for (send_map_list_iter = this->send_map_list.begin(); send_map_list_iter != this->send_map_list.end();) {
		this->free_send_data(send_map_list_iter->first);
	}
	std::map<uint32, std::map<uint64, uint8*> >::iterator recv_map_list_iter;
	for (recv_map_list_iter = this->recv_map_list.begin(); recv_map_list_iter != this->recv_map_list.end();) {
		this->free_recved_data(recv_map_list_iter->first);
		this->recv_map_list.erase(recv_map_list_iter++);
	}


}
void UdpBase::recv_start() {

	socklen_t len = sizeof(struct sockaddr_in);
	int recv_size = sizeof(struct udp_pack);
	int recv_num = 0;
	while (this->read_thread_status == _THREAD_RUN_) {
		struct sockaddr_in clent_addr = {0x00};
		struct udp_pack recv_upd_pack = {0x00};
		recv_num = recvfrom(this->sockt_fd, &recv_upd_pack, recv_size, 0, (struct sockaddr *)&clent_addr, (socklen_t *)&len);
		if (recv_num <= 0) {
			continue;
		}
		this->readable_data(recv_upd_pack, clent_addr);
	}
}

void UdpBase::free_recved_data(uint32 userid, uint64 unique) {
	uint8 *recv_data = NULL;
	this->recv_map_list_iter = this->recv_map_list.find(userid);
	if (this->recv_map_list_iter != this->recv_map_list.end()) {
		this->user_recv_map_list = this->recv_map_list_iter->second;
		this->user_recv_map_list_iter = this->user_recv_map_list.find(unique);
		if (this->user_recv_map_list_iter != this->user_recv_map_list.end()) {
			/*----*/
			recv_data = this->user_recv_map_list_iter->second;
			/*----*/
			free(recv_data);
			recv_data = NULL;
			this->user_recv_map_list.erase(this->user_recv_map_list_iter);
		}
		this->recv_map_list.erase(this->recv_map_list_iter);
	}
}
struct udp_pack* UdpBase::get_send_pack(uint32 userid, uint64 unique, uint16 sequeue) {

	return NULL;
}
void UdpBase::free_recved_data(uint32 userid) {
	uint8 *recv_data = NULL;
	this->recv_map_list_iter = this->recv_map_list.find(userid);
	if (this->recv_map_list_iter != this->recv_map_list.end()) {
		this->user_recv_map_list = this->recv_map_list_iter->second;
		for (this->user_recv_map_list_iter = this->user_recv_map_list.begin(); this->user_recv_map_list_iter != this->user_recv_map_list.end(); ) {
			recv_data = this->user_recv_map_list_iter->second;
			/*----*/
			free(recv_data);
			recv_data = NULL;
			this->user_recv_map_list.erase(this->user_recv_map_list_iter++);
		}
		this->recv_map_list.erase(this->recv_map_list_iter);
	}
}
void UdpBase::free_send_data(uint64 unique) {
	pthread_mutex_lock(&(this->send_mutex));
	std::map < uint64, std::map<uint16, struct udp_pack*> >::iterator send_map_list_iter;
	std::map<uint16, struct udp_pack*> send_task_pack_map;
	std::map<uint16, struct udp_pack*>::iterator send_task_pack_map_iter;
	send_map_list_iter = this->send_map_list.find(unique);
	if (send_map_list_iter != this->send_map_list.end()) {
		send_task_pack_map = send_map_list_iter->second;
		for (send_task_pack_map_iter = send_task_pack_map.begin(); send_task_pack_map_iter != send_task_pack_map.end();) {
			struct udp_pack* pack_data = send_task_pack_map_iter->second;
			free(pack_data);
			pack_data = NULL;
			send_task_pack_map.erase(send_task_pack_map_iter++);
		}
		this->send_map_list.erase(send_map_list_iter);
	}
	pthread_mutex_unlock(&(this->send_mutex));
}

int UdpBase::send_data(struct udp_pack * pack_data, struct sockaddr_in addr) {
	this->sendTo(pack_data, addr);
	return 1;
}
int UdpBase::send_data(void *send_data, int data_size, uint32 send_id, uint16 task, uint8 type) {
	if (type == _UDP_PACK_P2P_) {
		if (this->get_client_addr(send_id) == NULL) {
			this->get_socket_addr(send_id);
			return -1;
		}
	}
	if (this->send_thread_status == _THREAD_EXT_) {	//发送线程已关闭
		return -2;
	}
	int pack_num = this->get_pack_num(data_size);
	if (this->wait_send_queue > _MAX_TASK_LIST_) {
		Exception::save_error("wait_send_queue > max task list");
		return -1;
	}
	if (this->task_queue >= 65500) {
		this->task_queue = 0;
	}
	this->task_queue++;
	std::map<uint16, struct udp_pack*> send_user_pack_map_save;
	time_t time = this->get_mstime();
	for (int i = 0; i < pack_num; ++i) {
		struct udp_pack *pack_data = (struct udp_pack *)malloc(sizeof(struct udp_pack));
		memset(pack_data, 0x00, sizeof(struct udp_pack));
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
		send_user_pack_map_save.insert(std::pair<uint16, struct udp_pack*>(i, pack_data));
		pthread_mutex_lock(&(this->send_mutex));
		this->send_list.push_back(pack_data);
		this->wait_send_queue++;
		pthread_mutex_unlock(&(this->send_mutex));
		pthread_cond_signal(&(this->send_cond));
	}
	uint64 unique = time * 10000 + this->task_queue;
	pthread_mutex_lock(&(this->send_mutex));
	this->send_map_list.insert(std::pair < uint64, std::map<uint16, struct udp_pack*> >(unique, send_user_pack_map_save));
	pthread_mutex_unlock(&(this->send_mutex));

	return this->task_queue;
}




int UdpBase::send_data(uint8 task, uint8 type, uint32 userid) {
	const char *send_str = "...";
	this->send_data((void*)send_str, strlen(send_str), userid, task, type);
	return 1;
}

/*
 * type: 0x00： 立即转发
 * type :0x01: 接送完整数据回调
 */

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

int UdpBase::confirm_end(struct udp_pack pack_data, struct sockaddr_in addr) {	//确认结束某个数据包的结束
	pack_data.task = _TASK_END_;
	pthread_mutex_lock(&(this->send_mutex));
	this->sendTo(&pack_data, addr);
	pthread_mutex_unlock(&(this->send_mutex));
	return 1;
}

uint64 UdpBase::get_mstime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);   //该函数在sys/time.h头文件中
	return  tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/*发送数据*/
int UdpBase::sendTo(struct udp_pack * pack_data, struct sockaddr_in addr) {
	int len = sizeof(struct sockaddr_in);
	int send_num = sendto(this->sockt_fd, pack_data, sizeof(struct udp_pack), 0, (struct sockaddr *)&addr, len);
	if (send_num < 0) {
		printf("%s\n", "---------------------arild");
	}
	return send_num;
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
	} else {
		this->send_data(_TASK_GET_ADDR_, 0x00, userId);
	}
	return NULL;
}

struct udp_addr *UdpBase::get_client_addr(uint32 userId) {
	std::map<uint32, struct udp_addr*>::iterator users_addr_map_iter;
	users_addr_map_iter = this->users_addr_map.find(userId);
	if (users_addr_map_iter != this->users_addr_map.end()) {
		return users_addr_map_iter->second;
	}
	return NULL;
}
int UdpBase::get_pack_num(int data_size) {	//根据数据的大小，计算出会给拆分成多少个数据包
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
	return pack_num;
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
void UdpBase::send_feedback(struct udp_pack, struct sockaddr_in addr) {

}































