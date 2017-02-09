#ifdef _WIN32
#define EvoTCPOperatorLibAPI __declspec(dllexport)
#else
#define EvoTCPOperatorLibAPI
#endif

#include "../Include/EvoTCPCommon.h"

//�ڲ���Դ��Ҫ���г�ʼ��
TCPConnectionList g_tcp_connection_list;
//�ڲ���Դ��Ҫ���г�ʼ��
TCPListenerList g_tcp_listener_list;

//ϵͳ��Դ������صĽṹ
EvoTCPDustbin g_tcp_dustbin;

u_int32_t g_id_base = 1;

TCPConnectionId INVALID_CONNECTION ={0 , 0};
TCPConnectionId INVALID_LISTENER ={0 , 0};

extern TCPSendThreadPool g_tcp_sendthreadpool;
extern TCPRecvThreadPool g_tcp_recvthreadpool;

//�ϲ�Ӧ��ע��Ļص�����
extern EvoTCPNetworkEventHook g_tcpEventCallbackFun;
//extern CRITICAL_SECTION g_tcpEventCallback_cs;

extern u_int32_t g_tcp_self_interupt_port;



BOOL IdIsEqual(TCPConnectionId leftId , TCPConnectionId rightId)
{
	if(leftId.id == rightId.id && leftId.index == rightId.index)
		return TRUE;
	else
		return FALSE;
}

BOOL IdIsValid(TCPConnectionId leftId)
{
	if(leftId.id == 0 && leftId.index == 0)
		return FALSE;
	else
		return TRUE;
}

void InitializeTCPList()
{
	int index;
	InitializeCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	g_tcp_connection_list.connNum = 0;
	g_tcp_connection_list.send_task_count = 0;
	InitializeCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
	g_tcp_listener_list.listenerNum = 0;
	InitializeCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);

	for(index=0;index<MAX_TCPCONNECTION_LIST_LENGTH;index++)
	{
		g_tcp_connection_list.tcpConnList[index] = NULL;
	}
	for(index = 0;index<MAX_TCPLISTENER_LIST_LENGTH;index++)
	{
		g_tcp_listener_list.tcpListenerList[index] = NULL;
	}
	for(index = 0;index<MAX_TCP_DUSTBIN_SIZE;index++)
	{
		g_tcp_dustbin.dustList[index].node = NULL;
	}
	
}

void FinalizeTCPList()
{
	int index;
	//���Listener
	EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
	for(index=0;index<MAX_TCPLISTENER_LIST_LENGTH;index++)
	{
		if(g_tcp_listener_list.tcpListenerList[index]!=NULL)
		{
			ReleaseTCPListenerNode(g_tcp_listener_list.tcpListenerList[index]) ;
			g_tcp_listener_list.tcpListenerList[index] = NULL;
		}
	}
	LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);

	//���Connection
	EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	for(index=0;index<MAX_TCPCONNECTION_LIST_LENGTH;index++)
	{
		if(g_tcp_connection_list.tcpConnList[index]!=NULL)
		{
			ReleaseTCPConnectionNode(g_tcp_connection_list.tcpConnList[index]) ;
			g_tcp_connection_list.tcpConnList[index] = NULL;
		}
	}
	LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);

	DeleteCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	DeleteCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
}


TCPConnectionId InsertListener(const char* addr , unsigned short port)
{
	TCPConnectionId tmpId = INVALID_LISTENER;
	SOCKET tmpsocket;
	int index;

	//ϵͳ��ɳ�ʼ��֮ǰ�������ϲ�Ӧ�õĵ���
	if(g_TCPNetLayerState!=STATE_HAVE_INITIALIZED)
	{
		return tmpId;
	}
	//���Ƚ����ٽ���
	EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
	if(g_tcp_listener_list.listenerNum == MAX_TCPLISTENER_LIST_LENGTH)
	{
		LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
		return tmpId;
	}
	//��ʼ���˿�
	tmpsocket = InitTCPListener(addr , port);
	if(tmpsocket == INVALID_SOCKET)
	{
		LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
		return tmpId;
	}
	//����listenerList
	for(index=0;index<MAX_TCPLISTENER_LIST_LENGTH;index++)
	{
		if(g_tcp_listener_list.tcpListenerList[index]==NULL)
		{
			g_tcp_listener_list.tcpListenerList[index] = (TCPListenerNode*)malloc(sizeof(TCPListenerNode));
			g_tcp_listener_list.tcpListenerList[index]->type = LISTENER_USER;
            g_tcp_listener_list.tcpListenerList[index]->addr = _strdup(addr);
			g_tcp_listener_list.tcpListenerList[index]->port = port;
			g_tcp_listener_list.tcpListenerList[index]->count = 0;
			g_tcp_listener_list.tcpListenerList[index]->socket = tmpsocket;
			g_tcp_listener_list.tcpListenerList[index]->state = LISTENER_STATE_EMPTY;
			g_tcp_listener_list.tcpListenerList[index]->id.index = index;
			g_tcp_listener_list.tcpListenerList[index]->id.id = g_id_base++;

			tmpId = g_tcp_listener_list.tcpListenerList[index]->id;
			g_tcp_listener_list.listenerNum+=1;
			LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
			//��Ҫ���Ѷ�Ӧ���߳�
			printf("Listener: %s-%d�����ϣ������ж��߳� %d\n" , addr , port , index%TCP_RECV_THREAD_COUNT);
			TrytoInteruptTCPRecvThread(index%TCP_RECV_THREAD_COUNT);
			return tmpId;
		}
	}
	LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
	return tmpId;
}

int RemoveListener(TCPConnectionId listenerID)
{
	int index = 0;
	//���Ƚ����ٽ���
	EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
	
	//���Ҷ�Ӧ��Listener
	for(index=0;index<MAX_TCPLISTENER_LIST_LENGTH;index++)
	{
		if(g_tcp_listener_list.tcpListenerList[index]!=NULL)
		{
			if (IdIsEqual(listenerID, g_tcp_listener_list.tcpListenerList[index]->id)) //�ҵ��˶�Ӧ��Listener
			{
				//���ù���״̬
				g_tcp_listener_list.tcpListenerList[index]->state |= LISTENER_STATE_INVALID;
				g_tcp_listener_list.listenerNum -= 1;
				LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
				//��Ҫ���Ѷ�Ӧ���߳�
				printf("Listener: %s-%d�Ƴ���ϣ������ж��߳� %d\n", g_tcp_listener_list.tcpListenerList[index]->addr, g_tcp_listener_list.tcpListenerList[index]->port, index%TCP_RECV_THREAD_COUNT);
				TrytoInteruptTCPRecvThread(index%TCP_RECV_THREAD_COUNT);
				return 1;
			}
		}
	}
	LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
	return -1;
}

//��Ҫ��֤ϵͳ��ֻ��һ���ж϶˿�
TCPConnectionId InsertInteruptListener(const char* addr , unsigned short port)
{
	TCPConnectionId tmpId = INVALID_LISTENER;
	SOCKET tmpsocket;
	int index;
	struct sockaddr_in localaddr;
    socklen_t sockaddrlen = sizeof(struct sockaddr_in);

	//���Ƚ����ٽ���
	EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);

	//���ȼ������ظ�
	for(index=0;index<MAX_TCPLISTENER_LIST_LENGTH;index++)
	{
		if(g_tcp_listener_list.tcpListenerList[index] !=NULL)
		{
			if(g_tcp_listener_list.tcpListenerList[index]->type == LISTENER_BUILTIN)
			{
				LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
				return INVALID_LISTENER;
			}
		}
	}

	if(g_tcp_listener_list.listenerNum == MAX_TCPLISTENER_LIST_LENGTH)
	{
		LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
		return tmpId;
	}
	//��ʼ���˿�
	tmpsocket = InitTCPListener(addr , port);

	if(tmpsocket == INVALID_SOCKET)
	{
		LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
		return tmpId;
	}

    getsockname(tmpsocket, (struct sockaddr*)&localaddr,&sockaddrlen);
    g_tcp_self_interupt_port = ntohs(localaddr.sin_port);

	//����listenerList
	for(index=0;index<MAX_TCPLISTENER_LIST_LENGTH;index++)
	{
		if(g_tcp_listener_list.tcpListenerList[index]==NULL)
		{
			g_tcp_listener_list.tcpListenerList[index] = (TCPListenerNode*)malloc(sizeof(TCPListenerNode));
			g_tcp_listener_list.tcpListenerList[index]->type = LISTENER_BUILTIN;
            g_tcp_listener_list.tcpListenerList[index]->addr = addr==NULL ? NULL : _strdup(addr);
			g_tcp_listener_list.tcpListenerList[index]->port = port;
			g_tcp_listener_list.tcpListenerList[index]->count = 0;
			g_tcp_listener_list.tcpListenerList[index]->socket = tmpsocket;
			g_tcp_listener_list.tcpListenerList[index]->state = LISTENER_STATE_EMPTY;
			g_tcp_listener_list.tcpListenerList[index]->id.index = index;
			g_tcp_listener_list.tcpListenerList[index]->id.id = g_id_base++;

			tmpId = g_tcp_listener_list.tcpListenerList[index]->id;
			g_tcp_listener_list.listenerNum+=1;
			LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
			return tmpId;
		}
	}
	LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
	return tmpId;
}

//��ָ���˿ڷ������ӣ�����ɹ��򷵻ض�Ӧ��id
TCPConnectionId InsertActiveTCPConnection(const char* addr , unsigned short port)
{
	TCPConnectionId tmpid = INVALID_CONNECTION;
	TCPConnectionNode* tmpnode = NULL;
	int index;
	SOCKET socket;

	EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	if(g_tcp_connection_list.connNum == MAX_TCPCONNECTION_LIST_LENGTH)
	{
		LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
		return tmpid;
	}
	tmpnode = (TCPConnectionNode*)malloc(sizeof(TCPConnectionNode));
	if(tmpnode == NULL)
	{
		LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
		return tmpid;
	}
	socket = InitActiveConnection(addr , port);
	if(socket == INVALID_SOCKET)
	{
		LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
		return tmpid;
	}
	for(index=0;index<MAX_TCPCONNECTION_LIST_LENGTH;index++)
	{
		if(g_tcp_connection_list.tcpConnList[index] == NULL)
		{
            tmpnode->addr = _strdup(addr);
			tmpnode->curState = CONNECTION_STATE_EMPTY;
			tmpnode ->parentId = INVALID_CONNECTION;
			tmpnode->port = port;
			tmpnode ->priority = 0;
			tmpnode->heartbeat = 0;
			tmpnode->socket = socket;
			tmpnode ->taskList = ConstructTCPWriteTaskList();
			tmpnode->curTasks = ConstructTCPWriteTaskList();
			tmpnode->type = CONNECTION_ACTIVE;
			tmpnode->id.id = g_id_base++;
			tmpnode->id.index = index;
			tmpnode->recvContext.dataLength = 0;
			tmpnode->recvContext.curLength = 0;
			tmpnode->recvContext.dataBuf = NULL;
			tmpnode->sendContext.dataLength = 0;
			tmpnode->sendContext.curLength = 0;
			tmpnode->sendContext.dataBuf = NULL;
			tmpnode->sendContext.eleNum = 0;
			tmpnode->sendingThread = INVALID_THREAD_ID;
			tmpnode->recvingThread = INVALID_THREAD_ID;
			tmpnode->lastSendTime = 0;
			tmpnode->sendBlock = 0;
			tmpnode->incomingPacketNum = 0;
			tmpnode->outcomingPacketNum = 0;

			g_tcp_connection_list.tcpConnList[index] = tmpnode;
			g_tcp_connection_list.connNum++;
			break;
		}
	}
	TrytoInteruptTCPRecvThread(index%TCP_RECV_THREAD_COUNT);
	LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	return tmpnode->id;
}


TCPConnectionId InsertPassiveTCPConnection(const char* addr , unsigned short port, TCPConnectionId parentId , SOCKET socket)
{
	TCPConnectionId tmpid = INVALID_CONNECTION;
	TCPConnectionNode* tmpnode = NULL;
	int index;

	EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	if(g_tcp_connection_list.connNum == MAX_TCPCONNECTION_LIST_LENGTH)
	{
		LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
		return tmpid;
	}
	tmpnode = (TCPConnectionNode*)malloc(sizeof(TCPConnectionNode));
	if(tmpnode == NULL)
	{
		LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
		return tmpid;
	}
	for(index=0;index<MAX_TCPCONNECTION_LIST_LENGTH;index++)
	{
		if(g_tcp_connection_list.tcpConnList[index] == NULL)
		{
            tmpnode->addr = _strdup(addr);
			tmpnode->curState = CONNECTION_STATE_EMPTY;
			tmpnode ->parentId = parentId;
			tmpnode->port = port;
			tmpnode ->priority = 0;
			tmpnode->heartbeat = 0;
			tmpnode->socket = socket;
			tmpnode ->taskList = ConstructTCPWriteTaskList();
			tmpnode->curTasks = ConstructTCPWriteTaskList();
			tmpnode->type = CONNECTION_PASSIVE;
			tmpnode->id.id = g_id_base++;
			tmpnode->id.index = index;
			tmpnode->recvContext.dataLength = 0;
			tmpnode->recvContext.curLength = 0;
			tmpnode->recvContext.dataBuf = NULL;
			tmpnode->recvContext.eleNum = 0;
			tmpnode->sendContext.dataLength = 0;
			tmpnode->sendContext.curLength = 0;
			tmpnode->sendContext.dataBuf = NULL;
			tmpnode->sendContext.eleNum = 0;
			tmpnode->sendingThread = INVALID_THREAD_ID;
			tmpnode->recvingThread = INVALID_THREAD_ID;
			tmpnode->lastSendTime = 0;
			tmpnode->sendBlock = 0;
			tmpnode->incomingPacketNum = 0;
			tmpnode->outcomingPacketNum = 0;

			g_tcp_connection_list.tcpConnList[index] = tmpnode;
			g_tcp_connection_list.connNum++;
            //printf("��������������� : %d\n" , tmpnode->id.id);
			break;
		}
	}
	LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	return tmpnode->id;
}

int RemoveConnection(TCPConnectionId connId)
{
	int index;
	EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	
	for(index=0;index<MAX_TCPCONNECTION_LIST_LENGTH;index++)
	{
		if(g_tcp_connection_list.tcpConnList[index] != NULL)
		{
			if(IdIsEqual(g_tcp_connection_list.tcpConnList[index]->id , connId))
			{
				g_tcp_connection_list.tcpConnList[index]->curState |=CONNECTION_STATE_INVALID;
				g_tcp_connection_list.connNum-=1;
				
				LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
				TrytoInteruptTCPRecvThread(index%TCP_RECV_THREAD_COUNT);
				return 1;
			}
		}
	}
	LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	return -1;
}

/*****************************************************
��Ҫɾ���ڵ��е�����������ڴ���Դ
*****************************************************/
int ReleaseTCPConnectionNode(TCPConnectionNode* node)
{
	shutdown(node->socket , 0);
	//���ȹر�socket
	closesocket(node->socket);
	//ɾ���ڴ���Դ
	free(node->addr);

	if(node->recvContext.dataBuf!=NULL)
		free(node->recvContext.dataBuf);

	if(node->sendContext.dataBuf!=NULL)
	{
		//���ܵ�ǰ����ִ�еķ���������curtask�е�head
		//�������ݿռ�����ص�����˲���Ҫ�������ͷ�
		if(node->curTasks!=NULL)
		{
			if(node->curTasks->task_head!=NULL)
			{
				if(node->curTasks->task_head->buf!=node->sendContext.dataBuf)
				{
					free(node->sendContext.dataBuf);
				}
			}
			else
			{
				free(node->sendContext.dataBuf);
			}
		}
		else
		{
			free(node->sendContext.dataBuf);
		}
	}
	
	if(node->taskList!=NULL)
	{
		while(node->taskList->task_head!=NULL)
		{
			node->taskList->task_tail = node->taskList->task_head->next;
			free(node->taskList->task_head->buf);
			free(node->taskList->task_head);
			node->taskList->task_head = node->taskList->task_tail;
		}
	}
	if(node->curTasks!=NULL)
	{
		while(node->curTasks->task_head!=NULL)
		{
			node->curTasks->task_tail = node->curTasks->task_head->next;
			free(node->curTasks->task_head->buf);
			free(node->curTasks->task_head);
			node->curTasks->task_head = node->curTasks->task_tail;
		}
		
	}
	
	free(node);

	return 1;
}

int ReleaseTCPListenerNode(TCPListenerNode* node)
{
	//���ȹر�socket
	closesocket(node->port);

	//ɾ���ڴ���Դ
	free(node);

	return 1;
}

void CleanupTCPConnectionList()
{
	int index;
	EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	for(index=0;index<MAX_TCPCONNECTION_LIST_LENGTH;index++)
	{
		if(g_tcp_connection_list.tcpConnList[index] != NULL)
		{
			ReleaseTCPConnectionNode(g_tcp_connection_list.tcpConnList[index]);
			g_tcp_connection_list.tcpConnList[index] = NULL;
		}
	}
	LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	return;
}

void CleanupTCPListenerList()
{
	int index;
	EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
	for(index=0;index<MAX_TCPLISTENER_LIST_LENGTH;index++)
	{
		if(g_tcp_listener_list.tcpListenerList[index] != NULL)
		{
			ReleaseTCPListenerNode(g_tcp_listener_list.tcpListenerList[index]);
			g_tcp_listener_list.tcpListenerList[index] = NULL;
		}
	}
	LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
	return;
}

int TestResourceState()
{
	int index;
	//���Ȳ鿴Listener�б����Ƿ���ڳ����Ľڵ�
	//���Ƚ����ٽ���
	EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);

	//���Ҷ�Ӧ��Listener
	for(index=0;index<MAX_TCPLISTENER_LIST_LENGTH;index++)
	{
		if(g_tcp_listener_list.tcpListenerList[index]!=NULL)
		{
			if(g_tcp_listener_list.tcpListenerList[index]->state & LISTENER_STATE_INVALID)
			{
				//�����������ն��У�������ɹ������´μ�������
				if(InsertListenerIntoDustbin(g_tcp_listener_list.tcpListenerList[index]) == 1)
				{
					//֪ͨ�ϲ�
					LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);	//add by yxg 20161104
					//EnterCriticalSection(&g_tcpEventCallback_cs);
					if(g_tcpEventCallbackFun!=NULL)
					{
						EvoTCPNetworkEvent tmpEvent;
						tmpEvent.type = EVO_TCP_EVENT_LISTENER_ERR;
						tmpEvent.parentId = INVALID_LISTENER;
						tmpEvent.id = g_tcp_listener_list.tcpListenerList[index]->id;
						tmpEvent.buf = g_tcp_listener_list.tcpListenerList[index]->addr;
						tmpEvent.bufLength = strlen(tmpEvent.buf);
						tmpEvent.port = g_tcp_listener_list.tcpListenerList[index]->port;
						g_tcpEventCallbackFun(tmpEvent);
					}
					//LeaveCriticalSection(&g_tcpEventCallback_cs);
					EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);	//add by yxg 20161104
					g_tcp_listener_list.tcpListenerList[index] = NULL;
				}
			}
		}
	}
	LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);

	//��β鿴TCP�����б����Ƿ����������ʱ����ʧЧ�Ľڵ�
	EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	for(index=0;index<MAX_TCPCONNECTION_LIST_LENGTH;index++)
	{
		if(g_tcp_connection_list.tcpConnList[index] != NULL)
		{
			/*
			printf("���� %d �շ����ݰ� : %ld / %ld\n" , index , 
				g_tcp_connection_list.tcpConnList[index]->incomingPacketNum , 
				g_tcp_connection_list.tcpConnList[index]->outcomingPacketNum);
			printf("�����߳�1״̬ ��%d\n" , g_tcp_sendthreadpool.sendThreadList[0].threadState);
			printf("�����߳�2״̬ ��%d\n" , g_tcp_sendthreadpool.sendThreadList[1].threadState);
			printf("�����߳�1״̬ ��%d - %d\n" , g_tcp_recvthreadpool.recvThreadList[0].selectNum , 
				g_tcp_recvthreadpool.recvThreadList[0].recvRound );
			printf("�����߳�2״̬ ��%d - %d\n" , g_tcp_recvthreadpool.recvThreadList[1].selectNum , 
				g_tcp_recvthreadpool.recvThreadList[1].recvRound );
				*/
			//����������
			if(! (g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_POP))
			{
				//���Ŀǰ���ڰ����ݵ�����Ӧ�ò㣬����ΪӦ�ò�Ĳ�ȷ���ԣ���ʱ���迼��
				//�û��Ʋ������ƣ�������Ҫ����������ʩ��һ������������Ӧ�ò��
				if(g_tcp_connection_list.tcpConnList[index]->heartbeat++ >CONNECTION_HEARTBEAT_DEADLINE)
				{
					printf("������ʱ���ж�����ʧЧ : %s\n" , g_tcp_connection_list.tcpConnList[index]->addr);
					g_tcp_connection_list.tcpConnList[index]->curState |= CONNECTION_STATE_INVALID;
				}
			}
			
			if(((g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_RECVERR) ||
				(g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_SENDERR) ||
				(g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_INVALID)) &&
				(g_tcp_connection_list.tcpConnList[index]->sendingThread == INVALID_THREAD_ID) &&
				(g_tcp_connection_list.tcpConnList[index]->recvingThread == INVALID_THREAD_ID) &&
				!(g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_POP))
			{
				//�������������ת�Ƶ�����վ
				if(InsertConnIntoDustbin(g_tcp_connection_list.tcpConnList[index]) == 1)
				{
					//֪ͨ�ϲ�
					LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);	//add by yxg 20161104
					//EnterCriticalSection(&g_tcpEventCallback_cs);
					if(g_tcpEventCallbackFun!=NULL)
					{
						EvoTCPNetworkEvent tmpEvent;
						tmpEvent.type = EVO_TCP_EVENT_CONNECTION_ERR;
						tmpEvent.parentId = g_tcp_connection_list.tcpConnList[index]->parentId;
						tmpEvent.id = g_tcp_connection_list.tcpConnList[index]->id;
						tmpEvent.buf = g_tcp_connection_list.tcpConnList[index]->addr;
						tmpEvent.bufLength = strlen(tmpEvent.buf);
						tmpEvent.port = g_tcp_connection_list.tcpConnList[index]->port;
						g_tcpEventCallbackFun(tmpEvent);
					}
					//LeaveCriticalSection(&g_tcpEventCallback_cs);
					EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);	//add by yxg 20161104
					g_tcp_connection_list.tcpConnList[index] = NULL;
				}
				//����վ��Ҳû�пռ��ˣ����Σ��´����ͷŰ�

				//����б�Ҫ���ѽ����̣߳�����װ��
				TrytoInteruptTCPRecvThread(index%TCP_RECV_THREAD_COUNT);
			}
			else
			{
				//���Ŀǰ���ڷ���״̬�������Ѿ��з�����������Ҫ��������
				//Ҳ����˵�����Ҫ���������Ĳ��뵽��������У���һ���ǵ�һ��
				//���񡣺���������������������ݣ�����Խ�������������ɾ��
				if(!(g_tcp_connection_list.tcpConnList[index]->curTasks->count>0 ||
					g_tcp_connection_list.tcpConnList[index]->taskList->count>0 ||
					g_tcp_connection_list.tcpConnList[index]->sendContext.dataLength>0 ||
					g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_SEND))
				{
					InsertTCPHeartbeatToList(g_tcp_connection_list.tcpConnList[index]->taskList );
					//����б�Ҫ���ѷ����̣߳�������������
					if(!(g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_SEND))
					{
						TrytoInvokeTCPSendThread();
					}
				}
			}
		}
	}
	LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	return 1;	
}

int InsertConnIntoDustbin(TCPConnectionNode* node)
{
	int index;
	EnterCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);
	for(index=0;index<MAX_TCP_DUSTBIN_SIZE;index++)
	{
		if(g_tcp_dustbin.dustList[index].node == NULL)
		{
			g_tcp_dustbin.dustList[index].type = DUST_CONNECTION;
			g_tcp_dustbin.dustList[index].node = (void*)node;
			g_tcp_dustbin.dustList[index].deadLine = 0;
			LeaveCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);
			return 1;
		}
	}
	LeaveCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);
	return -1;
}

int InsertListenerIntoDustbin(TCPListenerNode* node)
{
	int index;
	EnterCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);
	for(index=0;index<MAX_TCP_DUSTBIN_SIZE;index++)
	{
		if(g_tcp_dustbin.dustList[index].node == NULL)
		{
			g_tcp_dustbin.dustList[index].type = DUST_LISTENER;
			g_tcp_dustbin.dustList[index].node = (void*)node;
			g_tcp_dustbin.dustList[index].deadLine = 0;
			LeaveCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);
			return 1;
		}
	}
	LeaveCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);
	return -1;
}

int CleanDustbin()
{
	int index;
	EnterCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);
	for(index=0;index<MAX_TCP_DUSTBIN_SIZE;index++)
	{
		if(g_tcp_dustbin.dustList[index].node != NULL)
		{
			if(g_tcp_dustbin.dustList[index].deadLine++ > DUST_DELETE_DEADLINE) //��Ҫ�ͷ���Դ
			{
				if(g_tcp_dustbin.dustList[index].type == DUST_CONNECTION)
				{
					printf("ɾ��ʧЧ����\n");
					ReleaseTCPConnectionNode((TCPConnectionNode*)(g_tcp_dustbin.dustList[index].node));
				}
				else if(g_tcp_dustbin.dustList[index].type == DUST_LISTENER)
				{
					ReleaseTCPListenerNode((TCPListenerNode*)(g_tcp_dustbin.dustList[index].node));
				}
				else
				{
					//����
				}
				g_tcp_dustbin.dustList[index].node = NULL;
				g_tcp_dustbin.dustList[index].deadLine = 0;
			}
		}
	}
	LeaveCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);
	return 1;
}


int FinalizeDustbin()
{
	int index;
	EnterCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);
	for(index=0;index<MAX_TCP_DUSTBIN_SIZE;index++)
	{
		if(g_tcp_dustbin.dustList[index].node != NULL)
		{
			if(g_tcp_dustbin.dustList[index].type == DUST_CONNECTION)
			{
				ReleaseTCPConnectionNode((TCPConnectionNode*)(g_tcp_dustbin.dustList[index].node));
			}
			else if(g_tcp_dustbin.dustList[index].type == DUST_LISTENER)
			{
				ReleaseTCPListenerNode((TCPListenerNode*)(g_tcp_dustbin.dustList[index].node));
			}
			else
			{
				//����
			}
		}
	}
	LeaveCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);

	DeleteCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);
	return 1;
}
