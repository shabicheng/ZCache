/********************************************************************
EvoTCPRecvThread.h

接收线程负责监听被动链接的建立，并对所有的连接进行监听，并读取数据。

设计原则：
1）数据的读取过程尽量保证公平性，避免过长时间为某个端口服务；
2）采用可中断方式，单次接收不完的数据存放在上下文环境中；

采用C语言完成。
********************************************************************/
#ifndef EVOTCPRECVTHREAD_H
#define EVOTCPRECVTHREAD_H

#define TCP_RECV_THREAD_COUNT 2


typedef struct TCPRecvThreadInfo
{
#ifdef _WIN32
    HANDLE threadHandle;
    DWORD threadId;
#else
    pthread_t threadHandle;
#endif
	SOCKET selfinterupt_send;
	SOCKET selfinterupt_recv;
	EvoTCPThreadState threadState;
    u_int32_t recvRound;
	unsigned short selectNum;
	int isRunning;
}TCPRecvThreadInfo;

typedef struct TCPRecvThreadPool
{
	TCPRecvThreadInfo recvThreadList[TCP_RECV_THREAD_COUNT];
	unsigned short recvThreadIdBase;	
}TCPRecvThreadPool;


int InitializeTCPRecvThread();
int FinalizeTCPRecvThread();

#ifdef _WIN32
DWORD WINAPI TCPRecvThreadRoutine(PVOID para);
#else
void* TCPRecvThreadRoutine(PVOID para);
#endif

//模块外部接口，由连接管理模块、资源管理模块在添加或者移除
//TCPListener或者TCPConnection时调用，用来中断对应现成的select，
//在重新装载之后重新执行。
//输入：需要中断的线程编号；
//输出：int
//将向自中断端口发送自中断报文，如果成功则返回1，否则返回-1
//需要考虑出错之后的处理。出错意味着自中断端口出现问题
int TrytoInteruptTCPRecvThread(int threadIndex);


//通过以下函数将网络事件传递给上层
//有连接请求到达，接收并调用InsertPassiveTCPConnection后，
//调用该函数，通知上层应用。上层应用可以通过此函数中传递的
//ConnectionId判定后续到达的数据，并利用该id发送数据
int AcceptConnection(TCPConnectionId listenerId , TCPConnectionId connId , char* addr, unsigned short port);

//读取完整数据之后，调用该函数将数据传递给上层
int RecvData(TCPConnectionId connId, unsigned short port, TCPNetworkTask* dataList);

#endif