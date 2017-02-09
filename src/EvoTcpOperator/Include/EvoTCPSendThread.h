/********************************************************************
EvoTCOSendThread.h
扫描ConnectionList中的发送任务，并完成发送
采用C语言完成。

基本设计原则概述：
1）网络发送采用双线程；发送线程轮询ConnectionList，在获取一个Connection之后，获得其发送
任务列表（可以根据流量控制原则选择发送列表的部分），然后执行发送。发送过程中不需要对
ConnectionList进行互斥管理；
2）采用信号量完成发送线程间的同步；
2.1、对于发送线程，设置标志位用于表示当前的工作状态，共有三种工作状态，发送、扫描、休眠；
2.2、发送任务到达时，首先将任务存入对应Connection，增加ConnectionList中的发送任务数。如果
存在休眠线程，并且存在任务的connection当前不处于发送状态，则激活任意一个休眠线程，这里
添加任务以及增加任务数采用临界区；
2.3、线程激活之后查看connection列表中的发送任务数，找到存在任务的connection，获取其发送任务，
并修改任务数之后，执行发送任务。这里扫描的过程以及修改任务数的过程采用临界区；
2.4、执行完发送任务之后，线程查看ConnectionList中的发送任务数，如果为0，则进入休眠；否则扫
描列表，找到存在任务的connection，获取其发送任务，并修改任务数之后，执行发送任务。这里扫描
的过程以及修改任务数的过程采用临界区；
3）尽量减少数据发送对其它任务造成的影响。减少临界区的控制范围，send操作尽量不在临界区中
4）选取发送任务要照顾到均衡原则；
********************************************************************/

#ifndef	EVOTCPSENDTHREAD_H
#define EVOTCPSENDTHREAD_H

#define TCP_SEND_THREAD_COUNT 2

#define MAX_SEND_INTERVAL 500

typedef struct TCPSendThreadInfo
{
#ifdef _WIN32
    HANDLE threadHandle;
    DWORD threadId;
#else
    pthread_t threadHandle;
#endif
	EvoTCPThreadState threadState;
	int isRunning;
}TCPSendThreadInfo;

typedef struct TCPSendThreadPool
{
	TCPSendThreadInfo sendThreadList[TCP_SEND_THREAD_COUNT];
	unsigned short sendThreadIdBase;	
#ifdef _WIN32
    HANDLE sendSema;
#else
    sem_t sendSema;
#endif

}TCPSendThreadPool;

int InitializeTCPSendThread();
int FinalizeTCPSendThread();

#ifdef _WIN32
DWORD WINAPI TCPSendThreadRoutine(PVOID para);
#else
void* TCPSendThreadRoutine(void* param);
#endif

int TrytoInvokeTCPSendThread();

#endif