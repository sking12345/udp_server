#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_


#include <iostream>

#include "type.h"
#include "exception.h"
#include "udp_base.h"


class UdpServer : public UdpBase {
  public:
	UdpServer(int port);
	~UdpServer();
	void excute_command();	//执行指令
	void set_command(std::string command);	//设置指令
	void readable_data(struct udp_pack, struct sockaddr_in addr);
	void recved_data(uint8 * recv_data, uint32 data_size, uint16 task, uint32 userid, uint64 unique);

};



#endif

