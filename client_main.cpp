#include <iostream>
#include "core/exception.h"
#include "core/udp_client.h"

int main() {


	std::cout << _UDP_DATA_SIZE_ << std::endl;
	std::cout << sizeof(struct udp_pack) << std::endl;
	const char *IP = "127.0.0.1";
	UdpClient *base = new UdpClient(IP, 9002);
	base->create_send_thread();
	base->create_read_thread(0x01);
	base->login(1235);
	base->get_socket_addr(1234);
	sleep(1);
	time_t  start_time = base->get_mstime();
	const char *str = "Google Play uses your app bundle to generate and serve optimized APKs for each device configuration, so only the code and resources that are needed for a specific device are downloaded to run your app. You no longer have to build, sign, and manage multiple APKs to optimize support for different devices, and users get smaller, more-optimized downloads.Google Play uses your app bundle to generate and serve optimized APKs for each device configuration, so only the code and resources that are needed for a specific device are downloaded to run your app. You no longer have to build, sign, and manage multiple APKs to optimize support for different devices, and users get smaller, more-optimized downloads.Google Play uses your app bundle to generate and serve optimized APKs for each device configuration, so only the code and resources that are needed for a specific device are downloaded to run your app. You no longer have to build, sign, and manage multiple APKs to optimize support for different devices, and users get smaller, more-optimized downloads.Google Play uses your app bundle to generate and serve optimized APKs for each device configuration, so only the code and resources that are needed for a specific device are downloaded to run your app. You no longer have to build, sign, and manage multiple APKs to optimize support for different devices, and users get smaller, more-optimized downloads.Google Play uses your app bundle to generate and serve optimized APKs for each device configuration, so only the code and resources that are needed for a specific device are downloaded to run your app. You no longer have to build, sign, and manage multiple APKs to optimize support for different devices, and users get smaller, more-optimized downloads.Google Play uses your app bundle to generate and serve optimized APKs for each device configuration, so only the code and resources that are needed for a specific device are downloaded to run your app. You no longer have to build, sign, and manage multiple APKs to optimize support for different devices, and users get smaller, more-optimized downloads.";
	//const char *str = "ddddd";
	std::cout << strlen(str) << std::endl;
	for (int j = 0; j < 10000; ++j) {
		sleep(1);
		for (int i = 0; i < 100; ++i) {
			printf("index::%d\n", j * 1000 + i);
			base->send_data((void*)str, strlen(str), 1234, _TASK_TEXT_, _UDP_PACK_P2P_);
		}
	}

	std::cout << "time::" << base->get_mstime() - start_time << std::endl;
	printf("%s\n", "end");
	// base->send_data((void*)str, strlen(str), 1234, _TASK_TEXT_, _UDP_PACK_PSP_);
	// base->send_data((void*)str, strlen(str), 1234, _TASK_TEXT_, _UDP_PACK_P2P_);
	// base->send_data((void*)str, strlen(str), 1234, _TASK_TEXT_, _UDP_PACK_PSP_);
	// base->send_data((void*)str, strlen(str), 1234, _TASK_TEXT_, _UDP_PACK_PSP_);

	sleep(100000);
	delete base;


	return 0;
}