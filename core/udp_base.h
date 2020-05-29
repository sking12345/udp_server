#ifndef _UDP_BASE_
#define _UDP_BASE_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/time.h>      //添加头文件
#include <string.h>
#include <pthread.h>
#include <iostream>

#include <list>
#include <map>
#include "type.h"
#include "exception.h"


class UdpBase {
  public:
	uint32 sockt_fd;
	uint32 task_queue;
	uint32 wait_send_queue;		//未发送成功的任务队列
	uint32 from_user_id;		//服务端 0x00
	pthread_t read_thread_id;	//读数据线程
	pthread_t send_thread_id;	//写数据线程
	pthread_mutex_t send_mutex; //互斥信号量
	pthread_cond_t send_cond;	//条件变量
	uint8 read_thread_status;	//读线程状态
	uint8 send_thread_status;	//写线程状态
	struct sockaddr_in server_addr;	//服务器的addr

	std::list<struct udp_pack*> send_list;	//线程发送数据队列

	std::map<uint32, std::map<uint64, struct udp_pack*> > send_map_list;	//保存发送数据的队列,用于验证数据完整性

	std::map<uint32, std::map<uint64, uint8*> > recv_map_list;	//保存接受队列
	std::map<uint32, struct udp_addr*> users_addr_map;

	std::list<struct udp_pack*> available_pack_list;	//可用包队列，用于包内存管理

  public:
	UdpBase();
	virtual ~UdpBase();
	int create_available_pack(int len);	//创建可用包队列
	struct udp_pack *get_available_pack();	//获取可用包
	int save_available_pack(struct udp_pack*);	//存入一个可用包,用于当数据确认接受完毕后,将数据清0x00，存入可用队列中
	void create_read_thread();	//开始创建读线程
	void create_send_thread();	//开始创建写线程
	void save_addr(struct sockaddr_in, uint32 );
	int send_data(void*, int size, uint32 userid, uint16 task, uint8 type);
	uint64 get_mstime();
	int sendTo(struct udp_pack*, struct sockaddr_in addr);	//发送数据,
	struct udp_addr *get_socket_addr(uint32);
	void send_time_out(uint16 unique, uint8 task, uint8 type);	//发送超时回调
	void recv_start();

};
#endif









