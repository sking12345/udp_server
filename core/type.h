#ifndef _TYEP_
#define _TYEP_

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


typedef signed char             int8;
typedef short int               int16;
typedef int                     int32;

typedef unsigned char           uint8;
typedef unsigned short int      uint16;
typedef unsigned int            uint32;
typedef long int            uint64;

#define _OVER_SEND_TIME_ 500  //发送超时，

#define _VERSION_ 0x01
#define _MAX_TASK_LIST_ 500	//最大发送任务队列
#define _THREAD_EXT_ 0x00
#define _THREAD_RUN_ 0x01

#define _UDP_PACK_PSP_ 0x00	//psp sendid //server
#define _UDP_PACK_P2P_ 0x01	//p2p sendid
#define _UDP_PACK_PGP_ 0x02	//发送给sendId 组		//server


#define _TASK_LOGIN_ 1 //登录任务
#define _TASK_LOGIN_SUCCESS_ 2
#define _TASK_LOGIN_FAIL_ 3 //登录失败

#define _TASK_QUITE_ 4 //退出登录
#define _TASK_BACK_QUITE_ 5 //退出登录
#define _TASK_GET_ADDR_ 6 //获取用户addr
#define _TASK_BACK_ADDR_ 7 //返回用户addr
#define _TASK_GET_PACK_ 8	//获取数据包,用于返回缺少那些包
#define _TASK_BACK_PACK_ 9  //返回缺少的包
#define _TASK_EXEIT_PACK_ 10  //包不存在




#define _UDP_PACK_SIZE_ 1470
#define _UDP_DATA_SIZE_ (_UDP_PACK_SIZE_ - sizeof(uint8)*2 - sizeof(uint32)*3 - sizeof(time_t))


typedef struct udp_pack {
	uint8 version;		//版本号
	uint8 type;
	uint8 task;		//任务编号
	uint32 send_id;		//发送给谁的id
	uint32 from_id;		//来自谁的id
	uint16 max_size;	//数据大小
	uint16 sequence;	//包序
	uint16 unique;
	time_t time;
	uint8 data[_UDP_DATA_SIZE_];
} UDP_PACK, *PUDP_PACK;

typedef struct udp_addr {
	uint64 last_time;
	struct sockaddr_in addr;
} UDP_ADDR, *PUDP_ADDR;

#endif







