#ifndef _UDP_CLIENT_
#define _UDP_CLIENT_


#include <iostream>
#include "type.h"
#include "exception.h"
#include "udp_base.h"


class UdpClient : public UdpBase {
  public:
	UdpClient(const char* ip, int port);
	~UdpClient();
	void login(uint32 userid);
	void recved_data(uint8 * recv_data, uint32 data_size, uint16 task, uint32 userid, uint64 unique = 0x00);
	void readable_data(struct udp_pack, struct sockaddr_in addr);
};
#endif