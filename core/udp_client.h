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
	void get_user_addr(uint32 userid);
};
#endif