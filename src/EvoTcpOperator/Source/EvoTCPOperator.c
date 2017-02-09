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

//�ϲ�Ӧ��ע��Ļص�����
EvoTCPNetworkEventHook g_tcpEventCallbackFun = NULL;
//CRITICAL_SECTION g_tcpEventCallback_cs;
//CallbackFunState g_tcpEventCallback_state = STATE_CALLBACK_IDLE;

/********************************************************************
����ConnectionList��ȫ�ֱ���
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

	//��ʼ���г�ʼ��
	//���Ȼ�ȡ��ǰ����ϵͳ�Ļ�������
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
	//��ʼ�������������
	InitializeTCPList();

	//��ʼ�������ж϶˿�
      //GLOBAL_TCP_SELF_INTERUPT_PORT+=1;
	//g_tcp_self_interupt_port = GLOBAL_TCP_SELF_INTERUPT_PORT;
	selfInteruptListenerId = InsertInteruptListener(NULL , (unsigned short)g_tcp_self_interupt_port);
	if(IdIsEqual(selfInteruptListenerId ,INVALID_LISTENER))
	{
		//ע��ʧ��
		FinalizeTCPList();
        //();
		return ERR_BUILTIN_LISTENER_EXISTED;
	}

	//���������̣߳�Ŀǰֻ��һ��Ĭ�ϼ����˿�
	rst = InitializeTCPRecvThread();
	if(rst!=TCP_RECVTHREADINIT_OK)
	{
		FinalizeTCPList();
        //();
		return rst;
	}

	//���������߳�
	rst = InitializeTCPSendThread();
	if(rst!=TCP_SENDTHREADINIT_OK)
	{
		FinalizeTCPRecvThread();
		FinalizeTCPSendThread();
		FinalizeTCPList();
        //();
		return rst;
	}
	//��ʼ������״̬����߳�
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
	//�ҵ���Ӧ������
    //TCPConnectionNode* tmpnode = NULL;
	int rst = 0;
	//���ȼ��connectionId����Ч��
	if (g_TCPNetLayerState != STATE_HAVE_INITIALIZED || connectionId.index > MAX_TCPCONNECTION_LIST_LENGTH)
		return ERR_SENDDATA_CONNECTION_INVALID;

	EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);

	if(g_tcp_connection_list.tcpConnList[connectionId.index] != NULL)
	{
		if(g_tcp_connection_list.tcpConnList[connectionId.index]->id.id == connectionId.id)
		{
			//�ж���Ч��
			if((g_tcp_connection_list.tcpConnList[connectionId.index]->curState & CONNECTION_STATE_RECVERR) ||
				(g_tcp_connection_list.tcpConnList[connectionId.index]->curState & CONNECTION_STATE_SENDERR) ||
				(g_tcp_connection_list.tcpConnList[connectionId.index]->curState & CONNECTION_STATE_INVALID))
			{
				LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
				return ERR_SENDDATA_CONNECTION_INVALID;
			}
			//���������
			rst = InsertTCPWriteTaskToList(g_tcp_connection_list.tcpConnList[connectionId.index]->taskList ,
																			data , length);
	
			//�������û�б����͹��������õ�һ���������ʱ��Ϊ��һ��ʱ��
			if(g_tcp_connection_list.tcpConnList[connectionId.index]->lastSendTime == 0 && rst > 0)
				g_tcp_connection_list.tcpConnList[connectionId.index]->lastSendTime = clock();
			//��������ڷ���״̬�����Ի��ѳ�˯�߳�
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