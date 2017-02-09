#ifndef EVONETMONITOR_H
#define EVONETMONITOR_H

typedef struct EvoNetMonitor
{
#ifdef _WIN32
    HANDLE threadHandle;
    DWORD threadId;
#else
    pthread_t threadHandle;
#endif
	int isRunning;
}EvoNetMonitor;

int InitializeNetMonitor();

int FinalizeNetMonitor();

/*******************************************************************
该线程的任务：
1）周期性向各个连接发送心跳报文；
2）周期性检查各个连接的心跳数量；当连接上到达任何数据，
将心跳数量清空；当心跳数量达到阈值之后，将该连接设置为
INVALID，并将该节点加入到回收站；发送和读取线程观察到
失效标志之后，分别复位SEND和RECV状态，然后退出该连接；
如果链接处于SELECT状态，则置位INVALID后需要触发中断；
3）周期性检查回收站中各个连接的状态，如果SEND和RECV
均已复位，则释放资源；
*******************************************************************/
#ifdef _WIN32
    DWORD WINAPI TCPNetMonitorRoutine(PVOID para);
#else
    void* TCPNetMonitorRoutine(void* para);
#endif

#endif