#ifndef _UDP_CLIENT_
#define _UDP_CLIENT_


#include <iostream>
#include "type.h"
#include "exception.h"
#include "udp_base.h"


class UdpClient : public udp_base {
  public:
	UdpClient(const char* ip, int port);
	~UdpClient();
}
#endif