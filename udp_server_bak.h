#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/time.h>      //添加头文件

#include <iostream>
#include <list>
#include <map>

#include "exception.h"
#include "type.h"


class UdpServer {

  public:
	int socket_fd;
	int port;
	// std::map<uint32 userid, std::map<struct sockaddr_in, uint64 time> > login_map;
	std::map<uint32, struct addr_map* > login_time_map;

  public:
	UdpServer(int port);
	~UdpServer();
	void Start();
	void Recvfrom(struct sockaddr_in addr, struct udp_buf *recv_buf);
	int Abnormal(int err);
	int SendTo(struct udp_buf *send_buf, struct sockaddr_in addr);

	uint64 GetMsTime();
	int ClientNotLoginCall(struct udp_buf *send_buf, struct sockaddr_in addr);	//转发给某个userid 时，某个useid 未登录时的回调函数

};

#endif


