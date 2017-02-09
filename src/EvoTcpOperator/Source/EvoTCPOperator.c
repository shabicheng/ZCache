#ifdef _WIN32
#define EvoTCPOperatorLibAPI __declspec(dllexport)
#pragma comment(lib,"Ws2_32.lib")
#else
#define EvoTCPOperatorLibAPI
#include <signal.h>
#endif

#include "../Include/EvoTCPCommon.h"


//#pragma data_seg("Shared")
//unsigned long GLOBAL_TCP_SELF_INTERUPT_PORT = 17905;
//unsigned long GLOBAL_TCP_SELF_INTERUPT_PORT = 0;
//#pragma data_seg()

//#pragma comment(linker , "/Section:Shared,RWS")

TCPNetLayerWorkState g_TCPNetLayerState = STATE_NOT_INITIALIZED;
u_int32_t g_tcp_self_interupt_port = 0;

DWORD g_cpu_number = 0;

//上层应用注册的回调函数
EvoTCPNetworkEventHook g_tcpEventCallbackFun = NULL;
//CRITICAL_SECTION g_tcpEventCallback_cs;
//CallbackFunState g_tcpEventCallback_state = STATE_CALLBACK_IDLE;

/********************************************************************
来自ConnectionList的全局变量
********************************************************************/
extern TCPConnectionList g_tcp_connection_list;
extern TCPListenerList g_tcp_listener_list;
extern EvoTCPDustbin g_tcp_dustbin;
extern u_int32_t g_id_base;
extern TCPConnectionId INVALID_CONNECTION;
extern TCPConnectionId INVALID_LISTENER;



int EvoTCPInitialize()
{
	TCPConnectionId selfInteruptListenerId;
	int rst = 0;

#ifdef _WIN32
    WSADATA  Ws;
    SYSTEM_INFO siSysInfo;

	//开始进行初始化
	//首先获取当前运行系统的环境特征
    GetSystemInfo(&siSysInfo);
	
    g_cpu_number = siSysInfo.dwNumberOfProcessors;



    if ( WSAStartup(MAKEWORD(2,2), &Ws) != 0 )
	{
		return ERR_NET_CANNOT_INIT;
    }
#else
    g_cpu_number = get_nprocs();
	signal(SIGPIPE, SIG_IGN);
#endif
	//初始化网络监听队列
	InitializeTCPList();

	//初始化自身中断端口
      //GLOBAL_TCP_SELF_INTERUPT_PORT+=1;
	//g_tcp_self_interupt_port = GLOBAL_TCP_SELF_INTERUPT_PORT;
	selfInteruptListenerId = InsertInteruptListener(NULL , (unsigned short)g_tcp_self_interupt_port);
	if(IdIsEqual(selfInteruptListenerId ,INVALID_LISTENER))
	{
		//注册失败
		FinalizeTCPList();
        //();
		return ERR_BUILTIN_LISTENER_EXISTED;
	}

	//启动监听线程，目前只有一个默认监听端口
	rst = InitializeTCPRecvThread();
	if(rst!=TCP_RECVTHREADINIT_OK)
	{
		FinalizeTCPList();
        //();
		return rst;
	}

	//启动发送线程
	rst = InitializeTCPSendThread();
	if(rst!=TCP_SENDTHREADINIT_OK)
	{
		FinalizeTCPRecvThread();
		FinalizeTCPSendThread();
		FinalizeTCPList();
        //();
		return rst;
	}
	//初始化网络状态监控线程
    InitializeNetMonitor();

	//InitializeCriticalSection(&g_tcpEventCallback_cs);
	g_TCPNetLayerState = STATE_HAVE_INITIALIZED;
	return 1;
}

TCPConnectionId RegisterListener(const char* addr , unsigned short port)
{
	if (g_TCPNetLayerState != STATE_HAVE_INITIALIZED)
	{
		return INVALID_LISTENER;
	}
	return InsertListener(addr , port);
}

TCPConnectionId RegisterConnection(const char* addr , unsigned short port)
{
	if (g_TCPNetLayerState != STATE_HAVE_INITIALIZED)
	{
		return INVALID_CONNECTION;
	}
	return InsertActiveTCPConnection(addr , port);
}

EvoTCPOperatorLibAPI int SetConnectionParam(TCPConnectionId id)
{
    int rst = -1;
    EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
    if (id.index < MAX_TCPCONNECTION_LIST_LENGTH && id.index >= 0)
    {
        rst = 1;
        g_tcp_connection_list.tcpConnList[id.index]->id = id;
    }
    LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
    return rst;
}



int UnRegisterListener(TCPConnectionId listenerID)
{
	if (g_TCPNetLayerState != STATE_HAVE_INITIALIZED)
	{
		return ERR_LISTENER_NOT_EXIST;
	}
	if(RemoveListener(listenerID) == -1)
		return ERR_LISTENER_NOT_EXIST;
	else
		return 1;
}

int UnRegisterConnection(TCPConnectionId connID)
{
	if (g_TCPNetLayerState != STATE_HAVE_INITIALIZED)
	{
		return ERR_CONNECTION_NOT_EXIST;
	}
	if(RemoveConnection(connID) == -1)
		return ERR_CONNECTION_NOT_EXIST;
	else
		return 1;
}

int SendData(TCPConnectionId connectionId , const char* data , u_int32_t length)
{
	//找到对应的连接
    //TCPConnectionNode* tmpnode = NULL;
	int rst = 0;
	//首先检查connectionId的有效性
	if (g_TCPNetLayerState != STATE_HAVE_INITIALIZED || connectionId.index > MAX_TCPCONNECTION_LIST_LENGTH)
		return ERR_SENDDATA_CONNECTION_INVALID;

	EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);

	if(g_tcp_connection_list.tcpConnList[connectionId.index] != NULL)
	{
		if(g_tcp_connection_list.tcpConnList[connectionId.index]->id.id == connectionId.id)
		{
			//判断有效性
			if((g_tcp_connection_list.tcpConnList[connectionId.index]->curState & CONNECTION_STATE_RECVERR) ||
				(g_tcp_connection_list.tcpConnList[connectionId.index]->curState & CONNECTION_STATE_SENDERR) ||
				(g_tcp_connection_list.tcpConnList[connectionId.index]->curState & CONNECTION_STATE_INVALID))
			{
				LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
				return ERR_SENDDATA_CONNECTION_INVALID;
			}
			//插入队列中
			rst = InsertTCPWriteTaskToList(g_tcp_connection_list.tcpConnList[connectionId.index]->taskList ,
																			data , length);
	
			//如果从来没有被发送过，则设置第一个包进入的时间为上一次时间
			if(g_tcp_connection_list.tcpConnList[connectionId.index]->lastSendTime == 0 && rst > 0)
				g_tcp_connection_list.tcpConnList[connectionId.index]->lastSendTime = clock();
			//如果不处于发送状态，则尝试唤醒沉睡线程
			if(!(g_tcp_connection_list.tcpConnList[connectionId.index]->curState & CONNECTION_STATE_SEND &&
				g_tcp_connection_list.tcpConnList[connectionId.index]->sendingThread!=INVALID_THREAD_ID))
			{
				TrytoInvokeTCPSendThread();
			}
			LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
			if(rst == ERR_TCP_WRITE_TASKLIST_FULL)
			{
				Sleep(1);
			}
			else if(rst > MAX_TCP_WRITE_TASKLIST_LENGTH/2)
			{
				Sleep(1);
			}
			return rst;
		}
	}
	LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);

	return ERR_SENDDATA_CONNECTION_NOT_EXIST;

}

int SendDataWithPriority(TCPConnectionId connectionId , 
						 char* data ,
                         u_int32_t length ,
						 EvoTCPDataPriority priority)
{
	return ERR_TCP_UNSUPPORTED;
}

int EvoTCPFinalize()
{
#ifdef _WIN32
    if(WSACleanup() == SOCKET_ERROR)
        return WSAGetLastError();
#endif

	FinalizeNetMonitor();	//add by yxg
	FinalizeDustbin();
	FinalizeTCPRecvThread();
	FinalizeTCPSendThread();
	FinalizeTCPList();
	//DeleteCriticalSection(&g_tcpEventCallback_cs);
	g_TCPNetLayerState = STATE_NOT_INITIALIZED;
	return 1;
}

int RegisterTCPEventHook(EvoTCPNetworkEventHook hookFunc)
{
	//EnterCriticalSection(&g_tcpEventCallback_cs);
	//if(g_tcpEventCallback_state!=STATE_CALLBACK_IDLE)
	//{
	//	LeaveCriticalSection(&g_tcpEventCallback_cs);
	//	return ERR_CALLBACKFUN_INUSE;
	//}
	g_tcpEventCallbackFun = hookFunc;
	//LeaveCriticalSection(&g_tcpEventCallback_cs);
	return 1;
}