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
	int test_num;
	uint32 sockt_fd;
	uint32 task_queue;
	uint32 wait_send_queue;		//未发送成功的任务队列
	uint32 from_user_id;		//服务端 0x00
	uint8 read_type;
	pthread_t read_thread_id;	//读数据线程
	pthread_t send_thread_id;	//写数据线程
	pthread_mutex_t send_mutex; //互斥信号量
	pthread_cond_t send_cond;	//条件变量

	pthread_mutex_t read_mutex; //互斥信号量
	pthread_cond_t read_cond;	//条件变量

	uint8 read_thread_status;	//读线程状态
	uint8 send_thread_status;	//写线程状态
	struct sockaddr_in server_addr;	//服务器的addr

	std::list<struct udp_pack*> send_list;	//线程发送数据队列

	// std::map<uint32, std::map<uint64, struct udp_pack*> > send_map_list;	//保存发送数据的队列,用于验证数据完整性

	//uint64 = time*10000 +task_queue; uint16:sequeue ,udp_pack = pakc
	std::map < uint32, std::map < uint64, std::map<uint16, struct udp_pack*> > > send_map_list;	//保存发送数据的队列,用于验证数据完整性

	std::map<uint32, std::map<uint64, uint8*> > recv_map_list;	//保存接受队列

	//保存接受包的日志记录 uint32: sequeue
	//uint64: unique
	std::map<uint32, std::map<uint64, std::map<uint16, uint16>  > > recv_map_log;

	std::map<uint32, struct udp_addr*> users_addr_map;
	std::list<struct udp_pack*> available_pack_list;	//可用包队列，用于包内存管理


	std::map<uint32, std::map<uint64, std::map<uint16, uint16> > >::iterator recv_map_log_iter;
	std::map<uint64, std::map<uint16, uint16> > user_map_log;
	std::map<uint64, std::map<uint16, uint16> >::iterator user_map_log_iter;
	std::map<uint16, uint16> pack_map_log;
	std::map<uint16, uint16>::iterator pack_map_log_iter;

	std::map<uint32, std::map<uint64, uint8*> >::iterator recv_map_list_iter;
	std::map<uint64, uint8*> user_recv_map_list;
	std::map<uint64, uint8*>::iterator user_recv_map_list_iter;

	/*用于释放发送缓存数据*/
	std::map < uint32, std::map < uint64, std::map<uint16, struct udp_pack*> > >::iterator send_map_list_iter;
	std::map < uint64, std::map<uint16, struct udp_pack*> > send_user_map;
	std::map < uint64, std::map<uint16, struct udp_pack*> >::iterator send_user_map_iter;
	std::map<uint16, struct udp_pack*> send_user_pack_map;
	std::map<uint16, struct udp_pack*>::iterator send_user_pack_map_iter;


  public:
	UdpBase();
	virtual ~UdpBase();

	int save_available_pack(struct udp_pack*);	//存入一个可用包,用于当数据确认接受完毕后,将数据清0x00，存入可用队列中
	void create_read_thread(uint8 type = 0x00);	//开始创建读线程 type:0x00 包立即回调 0x01 完整包回调 0x02: 立即回调及完整包回调
	void create_send_thread();	//开始创建写线程
	void save_addr(struct sockaddr_in, uint32 );
	int send_data(void*, int size, uint32 userid, uint16 task, uint8 type);
	int send_data(struct udp_pack* pack_data, struct sockaddr_in addr);	//发送数据,

	int send_data(uint8 task, uint8 type, uint32 userid = 0x00);	//发送数据,	发送任务指令
	uint64 get_mstime();
	int sendTo(struct udp_pack*, struct sockaddr_in addr);	//发送数据,
	struct udp_addr *get_socket_addr(uint32);
	struct udp_addr *get_client_addr(uint32);


	void send_time_out(uint16 unique, uint8 task, uint8 type);	//发送超时回调
	void recv_start();
	void recved_data(struct udp_pack*, struct sockaddr_in addr);		//立即回调函数
	void recved_data(uint8 *data, uint32 data_size, uint16 task, uint32 userid, uint64 unique = 0x00);	//接受到完整数据回调
	void free_recved_data(uint32 userid); //释放某个用户的所有接受资源
	void free_recved_data(uint32 userid, uint64 unique); //释放某个任务资源
	void free_send_data(uint32 userid, uint64 unique);
	void free_send_data(uint32 userid);
	struct udp_pack* get_send_pack(uint32 userid, uint64 unique, uint16 sequeue);	//获取缺失的包
	void remove_socket_addr(uint32 userId);
	int get_pack_num(int size);	//根据数据的大小，计算出会给拆分成多少个数据包
	int confirm_end(struct udp_pack, struct sockaddr_in addr);	//确认结束某个数据包的结束
	void send_feedback(struct udp_pack, struct sockaddr_in addr);	//发送数据的反馈

	void test(uint32 userId);


};
#endif









