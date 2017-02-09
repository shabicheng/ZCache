#ifdef _WIN32
#define EvoTCPOperatorLibAPI __declspec(dllexport)
#else
#define EvoTCPOperatorLibAPI
#endif

#include "../Include/EvoTCPCommon.h"

//内部资源需要进行初始化
TCPConnectionList g_tcp_connection_list;
//内部资源需要进行初始化
TCPListenerList g_tcp_listener_list;

//系统资源回收相关的结构
EvoTCPDustbin g_tcp_dustbin;

u_int32_t g_id_base = 1;

TCPConnectionId INVALID_CONNECTION ={0 , 0};
TCPConnectionId INVALID_LISTENER ={0 , 0};

extern TCPSendThreadPool g_tcp_sendthreadpool;
extern TCPRecvThreadPool g_tcp_recvthreadpool;

//上层应用注册的回调函数
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
	//清除Listener
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

	//清除Connection
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

	//系统完成初始化之前不接受上层应用的调用
	if(g_TCPNetLayerState!=STATE_HAVE_INITIALIZED)
	{
		return tmpId;
	}
	//首先进入临界区
	EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
	if(g_tcp_listener_list.listenerNum == MAX_TCPLISTENER_LIST_LENGTH)
	{
		LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
		return tmpId;
	}
	//初始化端口
	tmpsocket = InitTCPListener(addr , port);
	if(tmpsocket == INVALID_SOCKET)
	{
		LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
		return tmpId;
	}
	//插入listenerList
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
			//需要唤醒对应的线程
			printf("Listener: %s-%d添加完毕，尝试中断线程 %d\n" , addr , port , index%TCP_RECV_THREAD_COUNT);
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
	//首先进入临界区
	EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
	
	//查找对应的Listener
	for(index=0;index<MAX_TCPLISTENER_LIST_LENGTH;index++)
	{
		if(g_tcp_listener_list.tcpListenerList[index]!=NULL)
		{
			if (IdIsEqual(listenerID, g_tcp_listener_list.tcpListenerList[index]->id)) //找到了对应的Listener
			{
				//设置工作状态
				g_tcp_listener_list.tcpListenerList[index]->state |= LISTENER_STATE_INVALID;
				g_tcp_listener_list.listenerNum -= 1;
				LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
				//需要唤醒对应的线程
				printf("Listener: %s-%d移除完毕，尝试中断线程 %d\n", g_tcp_listener_list.tcpListenerList[index]->addr, g_tcp_listener_list.tcpListenerList[index]->port, index%TCP_RECV_THREAD_COUNT);
				TrytoInteruptTCPRecvThread(index%TCP_RECV_THREAD_COUNT);
				return 1;
			}
		}
	}
	LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
	return -1;
}

//需要保证系统中只有一个中断端口
TCPConnectionId InsertInteruptListener(const char* addr , unsigned short port)
{
	TCPConnectionId tmpId = INVALID_LISTENER;
	SOCKET tmpsocket;
	int index;
	struct sockaddr_in localaddr;
    socklen_t sockaddrlen = sizeof(struct sockaddr_in);

	//首先进入临界区
	EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);

	//首先检查避免重复
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
	//初始化端口
	tmpsocket = InitTCPListener(addr , port);

	if(tmpsocket == INVALID_SOCKET)
	{
		LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
		return tmpId;
	}

    getsockname(tmpsocket, (struct sockaddr*)&localaddr,&sockaddrlen);
    g_tcp_self_interupt_port = ntohs(localaddr.sin_port);

	//插入listenerList
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

//向指定端口发起连接，如果成功则返回对应的id
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
            //printf("建立被动连接完成 : %d\n" , tmpnode->id.id);
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
需要删除节点中的所有网络和内存资源
*****************************************************/
int ReleaseTCPConnectionNode(TCPConnectionNode* node)
{
	shutdown(node->socket , 0);
	//首先关闭socket
	closesocket(node->socket);
	//删除内存资源
	free(node->addr);

	if(node->recvContext.dataBuf!=NULL)
		free(node->recvContext.dataBuf);

	if(node->sendContext.dataBuf!=NULL)
	{
		//可能当前正在执行的发送任务是curtask中的head
		//发送数据空间存在重叠，因此不需要在这里释放
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
	//首先关闭socket
	closesocket(node->port);

	//删除内存资源
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
	//首先查看Listener列表中是否存在撤销的节点
	//首先进入临界区
	EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);

	//查找对应的Listener
	for(index=0;index<MAX_TCPLISTENER_LIST_LENGTH;index++)
	{
		if(g_tcp_listener_list.tcpListenerList[index]!=NULL)
		{
			if(g_tcp_listener_list.tcpListenerList[index]->state & LISTENER_STATE_INVALID)
			{
				//加入垃圾回收队列，如果不成功，则下次继续尝试
				if(InsertListenerIntoDustbin(g_tcp_listener_list.tcpListenerList[index]) == 1)
				{
					//通知上层
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

	//其次查看TCP连接列表中是否存在心跳超时或者失效的节点
	EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
	for(index=0;index<MAX_TCPCONNECTION_LIST_LENGTH;index++)
	{
		if(g_tcp_connection_list.tcpConnList[index] != NULL)
		{
			/*
			printf("连接 %d 收发数据包 : %ld / %ld\n" , index , 
				g_tcp_connection_list.tcpConnList[index]->incomingPacketNum , 
				g_tcp_connection_list.tcpConnList[index]->outcomingPacketNum);
			printf("发送线程1状态 ：%d\n" , g_tcp_sendthreadpool.sendThreadList[0].threadState);
			printf("发送线程2状态 ：%d\n" , g_tcp_sendthreadpool.sendThreadList[1].threadState);
			printf("接收线程1状态 ：%d - %d\n" , g_tcp_recvthreadpool.recvThreadList[0].selectNum , 
				g_tcp_recvthreadpool.recvThreadList[0].recvRound );
			printf("接收线程2状态 ：%d - %d\n" , g_tcp_recvthreadpool.recvThreadList[1].selectNum , 
				g_tcp_recvthreadpool.recvThreadList[1].recvRound );
				*/
			//检查心跳情况
			if(! (g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_POP))
			{
				//如果目前正在把数据弹出到应用层，则因为应用层的不确定性，暂时不予考虑
				//该机制并不完善，后续需要考虑两个措施，一方面增加面向应用层的
				if(g_tcp_connection_list.tcpConnList[index]->heartbeat++ >CONNECTION_HEARTBEAT_DEADLINE)
				{
					printf("心跳超时，判定链接失效 : %s\n" , g_tcp_connection_list.tcpConnList[index]->addr);
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
				//将有问题的连接转移到回收站
				if(InsertConnIntoDustbin(g_tcp_connection_list.tcpConnList[index]) == 1)
				{
					//通知上层
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
				//回收站里也没有空间了，无奈，下次再释放吧

				//如果有必要则唤醒接收线程，重新装载
				TrytoInteruptTCPRecvThread(index%TCP_RECV_THREAD_COUNT);
			}
			else
			{
				//如果目前处于发送状态，或者已经有发送任务，则不需要心跳报文
				//也就是说，如果要将心跳报文插入到任务队列中，则一定是第一个
				//任务。后续如果继续插入其它数据，则可以将心跳报文任务删除
				if(!(g_tcp_connection_list.tcpConnList[index]->curTasks->count>0 ||
					g_tcp_connection_list.tcpConnList[index]->taskList->count>0 ||
					g_tcp_connection_list.tcpConnList[index]->sendContext.dataLength>0 ||
					g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_SEND))
				{
					InsertTCPHeartbeatToList(g_tcp_connection_list.tcpConnList[index]->taskList );
					//如果有必要则唤醒发送线程，发送心跳报文
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
			if(g_tcp_dustbin.dustList[index].deadLine++ > DUST_DELETE_DEADLINE) //需要释放资源
			{
				if(g_tcp_dustbin.dustList[index].type == DUST_CONNECTION)
				{
					printf("删除失效连接\n");
					ReleaseTCPConnectionNode((TCPConnectionNode*)(g_tcp_dustbin.dustList[index].node));
				}
				else if(g_tcp_dustbin.dustList[index].type == DUST_LISTENER)
				{
					ReleaseTCPListenerNode((TCPListenerNode*)(g_tcp_dustbin.dustList[index].node));
				}
				else
				{
					//出错
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
				//出错
			}
		}
	}
	LeaveCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);

	DeleteCriticalSection(&g_tcp_dustbin.tcpDustbin_cs);
	return 1;
}
