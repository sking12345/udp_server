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
	pthread_cond_init(&(this->read_cond), NULL);
	pthread_mutex_init(&(this->read_mutex), NULL);

}

UdpBase::~UdpBase() {
	printf("%s\n", "～UdpBase");
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
		printf("tasK:%d, recv_upd_pack->data::%s\n", recv_upd_pack.task, recv_upd_pack.data);

		uint64 unique = recv_upd_pack.time * 10000 + recv_upd_pack.unique;
		this->save_addr(clent_addr, recv_upd_pack.from_id);
		if (recv_upd_pack.task == _TASK_LOGIN_) {	//登录保存from_id
			Exception::save_info("loign sucess");
			recv_upd_pack.task = _TASK_LOGIN_SUCCESS_;
			this->save_addr(clent_addr, recv_upd_pack.from_id);
			this->send_data(&recv_upd_pack, clent_addr);	//登录成功
			continue;
		}
		if (recv_upd_pack.task == _TASK_LOGIN_SUCCESS_ || recv_upd_pack.task == _TASK_LOGIN_FAIL_) {	//登录保存from_id
			this->send_feedback(recv_upd_pack, clent_addr);
			Exception::save_info("loign status");
			continue;
		}
		if (recv_upd_pack.task == _TASK_QUITE_) {	//退出登录
			this->remove_socket_addr(recv_upd_pack.from_id);
			recv_upd_pack.task = _TASK_BACK_QUITE_;
			this->send_data(&recv_upd_pack, clent_addr);	//登录成功
			this->send_feedback(recv_upd_pack, clent_addr);
			continue;
		}
		if (recv_upd_pack.task == _TASK_BACK_QUITE_) {	//退出登录
			this->send_feedback(recv_upd_pack, clent_addr);
			this->remove_socket_addr(recv_upd_pack.from_id);
			Exception::save_info("quit loign status");
			continue;
		}
		if (recv_upd_pack.task == _TASK_GET_ADDR_) {
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


			continue;
		}
		if (recv_upd_pack.task == _TASK_BACK_ADDR_) {	//保存返回的用户地址
			struct sockaddr_in back_user_addr = {0x00};
			memcpy(&back_user_addr, recv_upd_pack.data, sizeof(struct sockaddr_in));
			this->save_addr(back_user_addr, recv_upd_pack.send_id);
			Exception::save_info("back user addr");
			continue;
		}
		if (recv_upd_pack.task == _TASK_GET_PACK_) {
			std::map<uint32, std::map<uint64, struct udp_pack*> >::iterator send_map_list_iter;
			std::map<uint64, struct udp_pack*> send_user_map;
			std::map<uint64, struct udp_pack*>::iterator send_user_map_iter;
			send_map_list_iter = this->send_map_list.find(recv_upd_pack.from_id);
			if (send_map_list_iter != this->send_map_list.end()) {
				send_user_map = send_map_list_iter->second;
				uint64 unique =  recv_upd_pack.unique * 10000 + recv_upd_pack.sequence;
				send_user_map_iter = send_user_map.find(unique);
				if (send_user_map_iter != send_user_map.end()) {
					struct udp_pack *get_pack = send_user_map_iter->second;
					this->send_data(get_pack, clent_addr);
				} else {
					recv_upd_pack.task = _TASK_EXEIT_PACK_;
					this->send_data(&recv_upd_pack, clent_addr);
				}
			}
			Exception::save_info("get pack");
			continue;
		}
		if (recv_upd_pack.task == _TASK_USER_EXITE_) {
			Exception::save_info("_TASK_USER_EXITE_");	//
			std::map<uint32, std::map<uint64, struct udp_pack*> >::iterator send_map_list_iter;
			std::map<uint64, struct udp_pack*> user_send_map_list;
			std::map<uint64, struct udp_pack*>::iterator user_send_map_list_iter;
			send_map_list_iter = this->send_map_list.find(recv_upd_pack.send_id);
			if (send_map_list_iter != this->send_map_list.end()) {
				user_send_map_list = send_map_list_iter->second;
				uint64 user_unique =  recv_upd_pack.unique * 10000 + recv_upd_pack.sequence;
				user_send_map_list_iter =  user_send_map_list.find(user_unique);
				if (user_send_map_list_iter != user_send_map_list.end()) {
					struct udp_pack* udp_pack = user_send_map_list_iter->second;
					pthread_mutex_lock(&(this->send_mutex));		//进行写锁: 重复内存的使用，防止异常
					this->save_available_pack(udp_pack);
					pthread_mutex_unlock(&(this->send_mutex));
				}
			}
			this->send_feedback(recv_upd_pack, clent_addr);
			continue;
		}

		if (recv_upd_pack.task == _TASK_END_) {	//结束了某个任务的数据包发送,则需要将包,存入可用队列中
			Exception::save_info("_TASK_END_");
			std::map<uint32, std::map<uint64, struct udp_pack*> >::iterator send_map_list_iter;
			std::map<uint64, struct udp_pack*> user_send_map_list;
			std::map<uint64, struct udp_pack*>::iterator user_send_map_list_iter;
			send_map_list_iter = this->send_map_list.find(recv_upd_pack.send_id);
			if (send_map_list_iter != this->send_map_list.end()) {
				user_send_map_list = send_map_list_iter->second;
				int pack_num = this->get_pack_num(recv_upd_pack.max_size);
				for (int i = 0; i < pack_num; i++) {
					printf("ii===:%d\n", i);
					uint64 user_unique =  recv_upd_pack.unique * 10000 + i;
					user_send_map_list_iter =  user_send_map_list.find(user_unique);
					if (user_send_map_list_iter != user_send_map_list.end()) {
						struct udp_pack* udp_pack = user_send_map_list_iter->second;
						pthread_mutex_lock(&(this->send_mutex));		//进行写锁: 重复内存的使用，防止异常
						this->save_available_pack(udp_pack);
						pthread_mutex_unlock(&(this->send_mutex));
					}
				}
			}
			continue;
		}
		if (this->read_type ==  0x00) {	//立即回调
			this->recved_data(&recv_upd_pack, clent_addr);
			Exception::save_error("recvfrom read_type==0x00");
			continue;
		}

		pthread_mutex_lock(&(this->read_mutex));
		std::map<uint32, std::map<uint64, std::map<uint16, uint16> > >::iterator recv_map_log_iter;
		std::map<uint64, std::map<uint16, uint16> > user_map_log;
		std::map<uint64, std::map<uint16, uint16> >::iterator user_map_log_iter;
		std::map<uint16, uint16> pack_map_log;

		recv_map_log_iter = this->recv_map_log.find(recv_upd_pack.from_id);

		if (recv_map_log_iter != this->recv_map_log.end()) {
			user_map_log = recv_map_log_iter->second;
			user_map_log_iter = user_map_log.find(unique);
			if (user_map_log_iter != user_map_log.end()) {
				pack_map_log = user_map_log_iter->second;
				if (pack_map_log.find(recv_upd_pack.sequence) != pack_map_log.end()) {
					pthread_mutex_unlock(&(this->read_mutex));
					continue;
				}
			}
			pack_map_log.insert(std::pair<uint16, uint16>(recv_upd_pack.sequence, recv_upd_pack.sequence));
			this ->recv_map_log[recv_upd_pack.from_id][unique] = pack_map_log;
		} else {
			pack_map_log.insert(std::pair<uint16, uint16>(recv_upd_pack.sequence, recv_upd_pack.sequence));
			user_map_log.insert(std::pair<uint64, std::map<uint16, uint16> >(unique, pack_map_log));
			this ->recv_map_log.insert(std::pair<uint32, std::map<uint64, std::map<uint16, uint16> > >(recv_upd_pack.from_id, user_map_log));
		}
		int data_pack_num = this->get_pack_num(recv_upd_pack.max_size);
		if (data_pack_num <= 1) {	//数据只被拆分成一个数据包的时候
			this->recved_data(recv_upd_pack.data, recv_upd_pack.max_size, recv_upd_pack.task, recv_upd_pack.from_id);
			this->confirm_end(recv_upd_pack, clent_addr);
			pthread_mutex_unlock(&(this->read_mutex));
			continue;
		}
		std::map<uint32, std::map<uint64, uint8*> >::iterator recv_map_list_iter;
		std::map<uint64, uint8*> user_recv_map_list;
		std::map<uint64, uint8*>::iterator user_recv_map_list_iter;
		recv_map_list_iter = this->recv_map_list.find(recv_upd_pack.from_id);
		uint8 *recv_data = NULL;
		if (recv_map_list_iter ==  this->recv_map_list.end()) {
			recv_data = (uint8*)malloc(data_pack_num * _UDP_DATA_SIZE_);
			memset(recv_data, 0x00, data_pack_num * _UDP_DATA_SIZE_);
			user_recv_map_list.insert(std::pair<uint64, uint8*>(unique, recv_data));
			this->recv_map_list.insert(std::pair<uint32, std::map<uint64, uint8*> >(recv_upd_pack.from_id, user_recv_map_list));
		} else {
			user_recv_map_list = recv_map_list_iter->second;
			user_recv_map_list_iter = user_recv_map_list.find(unique);
			if (user_recv_map_list_iter == user_recv_map_list.end()) {
				recv_data = (uint8*)malloc(data_pack_num * _UDP_DATA_SIZE_);
				memset(recv_data, 0x00, data_pack_num * _UDP_DATA_SIZE_);
				user_recv_map_list.insert(std::pair<uint64, uint8*>(unique, recv_data));
				this->recv_map_list[recv_upd_pack.from_id] = user_recv_map_list;
			} else {
				recv_data = user_recv_map_list_iter->second;
			}
		}
		memcpy(recv_data + _UDP_DATA_SIZE_ * recv_upd_pack.sequence, recv_upd_pack.data, _UDP_DATA_SIZE_);
		pthread_mutex_unlock(&(this->read_mutex));
		if (this ->recv_map_log[recv_upd_pack.from_id][unique].size() == data_pack_num) {
			this->recved_data(recv_data, recv_upd_pack.max_size, recv_upd_pack.task, recv_upd_pack.from_id, unique);
			this->confirm_end(recv_upd_pack, clent_addr);
		}
	}
}

void UdpBase::recved_data(struct udp_pack* udp_pack, struct sockaddr_in addr) { //立即回调函数,针对服务端
	struct udp_addr *client_addr = this->get_client_addr(udp_pack->send_id);
	printf("formid:%d ,send_id:%d\n", udp_pack->from_id, udp_pack->send_id);
	if (client_addr != NULL) {
		this->send_data(udp_pack, client_addr->addr);
	} else {
		udp_pack->task = _TASK_USER_EXITE_;
		memset(udp_pack->data, 0x00, _UDP_DATA_SIZE_);
		this->send_data(udp_pack, addr);
	}
}
void UdpBase::recved_data(uint8 * recv_data, uint32 data_size, uint16 task, uint32 userid, uint64 unique) { //接受到完整数据回调,正对client 端
	printf("rect_data:%s\n,data_size:%d,task:%d,useid:%d\n", (char*) recv_data, data_size, task, userid);
	if (unique != 0x00) {
		this->free_recved_data(userid, unique);
	}
}

void UdpBase::free_recved_data(uint32 userid, uint64 unique) {
	std::map<uint32, std::map<uint64, uint8*> >::iterator recv_map_list_iter;
	std::map<uint64, uint8*> user_recv_map_list;
	std::map<uint64, uint8*>::iterator user_recv_map_list_iter;
	recv_map_list_iter = this->recv_map_list.find(userid);
	if (recv_map_list_iter !=  this->recv_map_list.end()) {
		user_recv_map_list = recv_map_list_iter->second;
		user_recv_map_list_iter = user_recv_map_list.find(unique);
		if (user_recv_map_list_iter != user_recv_map_list.end()) {
			uint8* recved_data = user_recv_map_list_iter->second;
			free(recved_data);
			recved_data = NULL;
			user_recv_map_list.erase(user_recv_map_list_iter);
			this->recv_map_list[userid] = user_recv_map_list;
		}
	}

}

void UdpBase::free_recved_data(uint32 userid) {
	std::map<uint32, std::map<uint64, uint8*> >::iterator recv_map_list_iter;
	std::map<uint64, uint8*> user_recv_map_list;
	std::map<uint64, uint8*>::iterator user_recv_map_list_iter;
	recv_map_list_iter = this->recv_map_list.find(userid);
	if (recv_map_list_iter !=  this->recv_map_list.end()) {
		user_recv_map_list = recv_map_list_iter->second;
		for (user_recv_map_list_iter = user_recv_map_list.begin(); user_recv_map_list_iter != user_recv_map_list.end();) {
			uint8* recved_data = user_recv_map_list_iter->second;
			free(recved_data);
			recved_data = NULL;
			user_recv_map_list.erase(user_recv_map_list_iter++);
		}
		this->recv_map_list.erase(recv_map_list_iter);
	}
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
	int pack_num = this->get_pack_num(data_size);
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
		uint64 unique =  pack_data->unique * 10000 + i;
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

int UdpBase::send_data(uint8 task, uint8 type, uint32 userid) {
	const char *send_str = "...";
	this->send_data((void*)send_str, strlen(send_str), userid, task, type);
	return 1;
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

int UdpBase::save_available_pack(struct udp_pack * pack) {
	memset(pack, 0x00, sizeof(struct udp_pack));
	this->available_pack_list.push_back(pack);
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
	printf("confirm_end:%d\n", pack_data.task);
	this->send_data(&pack_data, addr);
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































