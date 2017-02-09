#ifndef TCPTASKRESOLVE_H
#define TCPTASKRESOLVE_H

#define TCP_BUFFERSIZE 128*1024			//端口缓存规模
#define TCP_MAX_RECV_COUNT 16			//最多一次性读取批次
#define TCP_MAX_RECV_SIZE 1024*1024	//最多一次性读取数据量
#define TCP_MAX_SEND_COUNT 16			//最多一次性读取批次
#define TCP_MAX_SEND_SIZE 1024*1024	//最多一次性读取数据量

SOCKET InitTCPListener(const char* addr , unsigned short port);

SOCKET InitActiveConnection(const char* addr , unsigned short port);

int SetConnectionSockOPT(SOCKET socket);

//对一个端口进行连续读取，如果读取的过程中发生中断，例如读取容量限制或者阻塞
//则将当前正在读取的场景存储在TCPConnectionNode中的recvContext中
TCPNetworkTask* EvoTCPRecv(TCPConnectionNode* connectionNode);

//对一个端口进行连续数据发送，如果发送过程中发生中断，例如发送容量限制或者阻塞
//则将挡墙正在发送的场景存储在TCPConnectionNode中的sendContext中
u_int32_t EvoTCPSend(TCPConnectionNode* connectionNode);

int EvoTCPRecvInterupt(SOCKET socket);

#endif