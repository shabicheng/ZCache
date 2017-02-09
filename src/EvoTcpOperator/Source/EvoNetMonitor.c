#ifdef _WIN32
#define EvoTCPOperatorLibAPI __declspec(dllexport)
#else
#define EvoTCPOperatorLibAPI
#endif

#include "../Include/EvoTCPCommon.h"

EvoNetMonitor g_netmonitor;

//来自TCPConnectionList的全局变量
extern EvoTCPDustbin g_tcp_dustbin;

int InitializeNetMonitor()
{
	//启动监控线程，该线程需要尽量以非阻塞或者
	//较短阻塞完成所有工作，保证循环的周期准确定
	//g_tcp_dustbin已经在TCPConnectionList中完成了初始化

#ifdef _WIN32
    g_netmonitor.threadHandle =(HANDLE)_beginthreadex(NULL ,
                                                        0 ,
                                                        TCPNetMonitorRoutine ,
                                                        0 ,
                                                        0 ,
                                                        &(g_netmonitor.threadId));
#else
    pthread_create(&g_netmonitor.threadHandle,NULL,TCPNetMonitorRoutine,NULL);	
#endif
	
	return 1;
}

int FinalizeNetMonitor()
{
	g_netmonitor.isRunning = 0;

#ifdef _WIN32
	if (WAIT_OBJECT_0 != WaitForSingleObject(g_netmonitor.threadHandle, 2000))
	{
		TerminateThread(g_netmonitor.threadHandle, 0);
	}
	CloseHandle(g_netmonitor.threadHandle);
#else
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
	{
		/* Handle error */
	}

	ts.tv_sec += 2;
	if (pthread_timedjoin_np(g_netmonitor.threadHandle, NULL, &ts) != 0)
	{
		pthread_cancel(g_netmonitor.threadHandle);
	}

#endif // _WIN32
	return 1;
}

#ifdef _WIN32
DWORD WINAPI TCPNetMonitorRoutine(PVOID para)
#else
void* TCPNetMonitorRoutine(PVOID para)
#endif
{
#ifdef _WIN32
#else
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif // _WIN32


	g_netmonitor.isRunning = 1;
	while (g_netmonitor.isRunning)
	{
		TestResourceState();
		CleanDustbin();	
        Sleep(1000);
	}
    return NULL;
}