#ifdef _WIN32
#define EvoTCPOperatorLibAPI __declspec(dllexport)
#else
#define EvoTCPOperatorLibAPI
#endif

#include "../Include/EvoTCPCommon.h"

/*****************************************************
ConnectionList模块的全局变量
接收线程从两个列表中装载端口，包括了自身的中断端口，然后
进行监听。
*****************************************************/
extern TCPConnectionId INVALID_CONNECTION;

extern TCPConnectionId INVALID_LISTENER;

extern TCPConnectionList g_tcp_connection_list;

extern TCPListenerList g_tcp_listener_list;

extern u_int32_t g_id_base;

u_int32_t testRecvCounter = 0;

/*****************************************************
TCPOperator模块的全局变量
*****************************************************/
//接收线程初始化中完成对每个线程中断端口的初始化，均使用该监听端口
//TCPOperator中模块初始化函数保证进行中断端口初始化之前，已经在该
//端口进行监听
extern u_int32_t g_tcp_self_interupt_port;

extern TCPNetLayerWorkState g_TCPNetLayerState;

extern DWORD g_cpu_number;

//上层应用注册的回调函数
extern EvoTCPNetworkEventHook g_tcpEventCallbackFun;
//extern CRITICAL_SECTION g_tcpEventCallback_cs;
//extern CallbackFunState g_tcpEventCallback_state;


/*****************************************************
本模块的全局变量
*****************************************************/
//需要进行初始化
TCPRecvThreadPool g_tcp_recvthreadpool;


/*****************************************************
本模块的函数
*****************************************************/

int InitializeTCPRecvThread()
{
	int index;
	int waittimes = 0;
	int initTest = 0;
	SOCKET tmpsocket;
	g_tcp_recvthreadpool.recvThreadIdBase = 0;
	for(index=0;index<TCP_RECV_THREAD_COUNT;index++)
	{
		g_tcp_recvthreadpool.recvThreadList[index].threadState = TCP_THREAD_STATE_INVALID;
		g_tcp_recvthreadpool.recvThreadList[index].selfinterupt_recv = INVALID_SOCKET;
		g_tcp_recvthreadpool.recvThreadList[index].selfinterupt_send = INVALID_SOCKET;
		g_tcp_recvthreadpool.recvThreadList[index].recvRound = 0;
		g_tcp_recvthreadpool.recvThreadList[index].selectNum = 0;
	}
	//生成并启动线程
	for(index=0;index<TCP_RECV_THREAD_COUNT;index++)
	{
#ifdef _WIN32
        g_tcp_recvthreadpool.recvThreadList[index].threadHandle = (HANDLE)_beginthreadex(NULL ,
																								0 , 
																								TCPRecvThreadRoutine , 
																								0 ,
																								0 , 
                                                                                                &(g_tcp_recvthreadpool.recvThreadList[index].threadId));



        SetThreadPriority(g_tcp_recvthreadpool.recvThreadList[index].threadHandle , THREAD_PRIORITY_TIME_CRITICAL);

		//如果获取了CPU信息，则尝试指派内核
        if(g_cpu_number == 8) //往往是i7处理器，四核八线程
		{
			//接收线程通常状况下是两个，部署在前两个CPU
			SetThreadIdealProcessor(g_tcp_recvthreadpool.recvThreadList[index].threadHandle , index%2);
		}
		else if(g_cpu_number == 4) //i5双核四线程
		{
			SetThreadIdealProcessor(g_tcp_recvthreadpool.recvThreadList[index].threadHandle , index%2);
        }
#else
        pthread_create(&g_tcp_recvthreadpool.recvThreadList[index].threadHandle,NULL,TCPRecvThreadRoutine,NULL);	
#endif
        //设置线程当前的状态为初始化状态，同一时间内只有一个线程处于该状态，线程在初始化过程中
		//利用该标志找到自身对应的数据结构，并设置中断端口
		g_tcp_recvthreadpool.recvThreadList[index].threadState = TCP_THREAD_STATE_INIT;

		//分别向中断监听端口发起连接，该过程是阻塞过程，意味着函数返回的时候，accept过程也已经结束了
		while(waittimes++<30)
		{
			tmpsocket = InitActiveConnection(NULL , (unsigned short)g_tcp_self_interupt_port);

			if(tmpsocket == INVALID_SOCKET)
			{
				Sleep(100);
			}
			else
			{
				break;
			}
		}
		waittimes = 0;
		if(tmpsocket == INVALID_SOCKET)
		{
			TerminateThread(g_tcp_recvthreadpool.recvThreadList[index].threadHandle , 0);
			return ERR_BUILTIN_LISTENER_CANNOT_CONNECT;
		}
		//保存该端口，以后通过该端口发送中断
		g_tcp_recvthreadpool.recvThreadList[index].selfinterupt_send = tmpsocket;

		//循环等待中断端口接收端初始化
		initTest = 0;
        while(waittimes++<30)
		{
			if(g_tcp_recvthreadpool.recvThreadList[index].selfinterupt_recv == INVALID_SOCKET)
			{
				Sleep(100);
			}
			else
			{	
				initTest = 1;
				break;
			}
		}
		if(!initTest) //中断端口初始化没成功
		{
			TerminateThread(g_tcp_recvthreadpool.recvThreadList[index].threadHandle , 0);
			return ERR_BUILTIN_LISTENER_CANNOT_ACCEPT;
        }
		//
		g_tcp_recvthreadpool.recvThreadList[index].threadState = TCP_THREAD_STATE_IDLE;
	}
	
	return TCP_RECVTHREADINIT_OK;
}
int FinalizeTCPRecvThread()
{
	int index;
	for (index = 0; index < TCP_RECV_THREAD_COUNT; index++)
	{
		g_tcp_recvthreadpool.recvThreadList[index].isRunning = 0;
	}
	for(index=0;index<TCP_RECV_THREAD_COUNT;index++)
	{
#ifdef _WIN32
		if (WAIT_OBJECT_0 != WaitForSingleObject(g_tcp_recvthreadpool.recvThreadList[index].threadHandle, 1000))
		{
			TerminateThread(g_tcp_recvthreadpool.recvThreadList[index].threadHandle, 0);
		}		
		CloseHandle(g_tcp_recvthreadpool.recvThreadList[index].threadHandle);
#else
		struct timespec ts;
		if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
		{
			/* Handle error */
		}

		ts.tv_sec += 1;
		if (pthread_timedjoin_np(g_tcp_recvthreadpool.recvThreadList[index].threadHandle, NULL, &ts) != 0)
		{
			pthread_cancel(g_tcp_recvthreadpool.recvThreadList[index].threadHandle);
		}
#endif // _WIN32

		closesocket(g_tcp_recvthreadpool.recvThreadList[index].selfinterupt_send);
		closesocket(g_tcp_recvthreadpool.recvThreadList[index].selfinterupt_recv);
	}
	return 1;
}


/*******************************************************************************
基本设计原则概述：
网络接收采用两线程；分别负责Listener以及Connection列表中的端口的监听，
利用select检测，并直接完成网络端口读取，进而将获取的数据传递给上层；
*******************************************************************************/
#ifdef _WIN32
DWORD WINAPI TCPRecvThreadRoutine(PVOID para)
#else
void* TCPRecvThreadRoutine(void* para)
#endif
{
#ifdef _WIN32
#else
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif // _WIN32

	EvoThreadId myid = g_tcp_recvthreadpool.recvThreadIdBase++;
	
	fd_set readset;
	fd_set exeptionset;
	unsigned short fdNum = 0;

#ifdef _WIN32
#else
    SOCKET fdMax = 0;
#endif

	SOCKET acceptSocket;
	int threadIndex;
	struct sockaddr_in clientaddr;
    //int addrsize = sizeof(clientaddr);
    socklen_t addrsize = sizeof(clientaddr);
	TCPConnectionId acceptConnId;
	
	int index;
	int selectresult;

	TCPNetworkTask* recvrst = NULL;
	TCPNetworkTask* tmptask = NULL;

	g_tcp_recvthreadpool.recvThreadList[myid].isRunning = 1;

	while (g_tcp_recvthreadpool.recvThreadList[myid].isRunning)
	{
		fdNum = 0;
		FD_ZERO(&readset);
		FD_ZERO(&exeptionset);

#ifdef _WIN32
#else
		fdMax = 0;
#endif
		g_tcp_recvthreadpool.recvThreadList[myid].selectNum = 0;
		//装载ListenList中的端口
		EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
		if(g_tcp_listener_list.listenerNum>0)
		{
			for(index=myid;index<MAX_TCPLISTENER_LIST_LENGTH;index+=TCP_RECV_THREAD_COUNT)
			{
				if(g_tcp_listener_list.tcpListenerList[index]!=NULL)
				{
					if (g_tcp_listener_list.tcpListenerList[index]->state == LISTENER_STATE_EMPTY)
					{
						//printf("线程 %d  添加网络监听 %d\n" , myid ,  g_tcp_listener_list.tcpListenerList[index]->port);
						FD_SET(g_tcp_listener_list.tcpListenerList[index]->socket, &readset);
						FD_SET(g_tcp_listener_list.tcpListenerList[index]->socket, &exeptionset);
#ifdef _WIN32
#else
						if(g_tcp_listener_list.tcpListenerList[index]->socket > fdMax)
						{
							fdMax = g_tcp_listener_list.tcpListenerList[index]->socket;
						}
#endif
						fdNum++;
					}
				}
			}
		}
		LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);

		//装载ConnectionList中的端口
		EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
		if(g_tcp_connection_list.connNum>0)
		{
			for(index=myid;index<MAX_TCPCONNECTION_LIST_LENGTH;index+=TCP_RECV_THREAD_COUNT)
			{
				if(g_tcp_connection_list.tcpConnList[index]!=NULL)
				{
					//只有正常的非读取状态才能进行监听
					if ((g_tcp_connection_list.tcpConnList[index]->curState == CONNECTION_STATE_EMPTY)
						|| (g_tcp_connection_list.tcpConnList[index]->curState == CONNECTION_STATE_SEND)
						|| (g_tcp_connection_list.tcpConnList[index]->recvingThread == INVALID_THREAD_ID)
						&& !(g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_RECVERR
						|| g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_SENDERR
						|| g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_INVALID))
					{
						//printf("线程%d添加网络连接%d : %d\n" , myid , index , g_tcp_connection_list.tcpConnList[index]->port);
						FD_SET(g_tcp_connection_list.tcpConnList[index]->socket, &readset);
						FD_SET(g_tcp_connection_list.tcpConnList[index]->socket, &exeptionset);
#ifdef _WIN32
#else
						if(g_tcp_connection_list.tcpConnList[index]->socket > fdMax)
						{
							fdMax = g_tcp_connection_list.tcpConnList[index]->socket;
						}
#endif
						g_tcp_recvthreadpool.recvThreadList[myid].selectNum++;
						fdNum++;
					}
				}
			}
		}
		LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);

		//装载自身的中断端口
		if (g_tcp_recvthreadpool.recvThreadList[myid].selfinterupt_recv != INVALID_SOCKET)
		{
			//printf("线程 %d 装载中断端口 %d-%d\n" , myid , 
			//									g_tcp_recvthreadpool.recvThreadList[myid].selfinterupt_recv , 
			//									g_tcp_recvthreadpool.recvThreadList[myid].selfinterupt_send);
			FD_SET(g_tcp_recvthreadpool.recvThreadList[myid].selfinterupt_recv, &readset);
			FD_SET(g_tcp_recvthreadpool.recvThreadList[myid].selfinterupt_recv, &exeptionset);
#ifdef _WIN32
#else
			if(g_tcp_recvthreadpool.recvThreadList[myid].selfinterupt_recv > fdMax)
			{
				fdMax = g_tcp_recvthreadpool.recvThreadList[myid].selfinterupt_recv;
			}
#endif
			fdNum++;
		}

		//还没有完成初始化，循环等待
		if(fdNum == 0)
		{
			Sleep(10);
			continue;
		}
		
		if(g_TCPNetLayerState == STATE_HAVE_INITIALIZED)
		{
			//如果系统已经完成了初始化，则应当开始对线程的工作状态进行管理
			//但是在系统初始化阶段中，第一个接收线程会走到这个位置，用来接收所有其它接收线程的
			//自中断连接请求，此时系统的状态是STATE_NOT_INITIALIZED，这个阶段可以不设置工作状态
			g_tcp_recvthreadpool.recvThreadList[myid].threadState = TCP_THREAD_STATE_SELECT;
		}

		//printf("装载完毕\n");

		//开始监听端口，阻塞过程，可以通过中断端口唤醒
#ifdef _WIN32
        selectresult = select(0, &readset,NULL,NULL,NULL);
#else
        selectresult = select(fdMax+1, &readset,NULL,NULL,NULL);
#endif

        //printf("线程%d侦测到数据%d\n",myid,selectresult);

		g_tcp_recvthreadpool.recvThreadList[myid].recvRound++;
		if (selectresult == 0)
		{
			//printf("侦测到错误\n");
			continue;
		}
		else if (selectresult == SOCKET_ERROR)
		{
            //int err = WSAGetLastError();
			
			continue;
		}
		else //注意，监听过程中可能存在端口被上层取消的情况
		{
			if(g_TCPNetLayerState == STATE_HAVE_INITIALIZED)
			{
				g_tcp_recvthreadpool.recvThreadList[myid].threadState = TCP_THREAD_STATE_RECVING;
			}
			
			//查看是否存在Connection
			EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
			if(g_tcp_connection_list.connNum>0)
			{
				for(index=myid;index<MAX_TCPCONNECTION_LIST_LENGTH;index+=TCP_RECV_THREAD_COUNT)
				{
					if(g_tcp_connection_list.tcpConnList[index]!=NULL)
					{
						if(FD_ISSET(g_tcp_connection_list.tcpConnList[index]->socket, &readset))
						{
							//有数据到达，开始接收数据，这里需要避免对g_tcp_connection_list的长期占用
							//数据读取过程脱离临界区保护。但是需要注意，在此过程中的异常处理。
							//数据读取过程中可能存在三种异常，1、recv过程异常；2、send过程异常；3、心跳检测异常；
							//因此，无论是发生哪一种情况的异常，都只是将连接的状态设置为失效即可，而不要对其中的
							//资源进行任何删除操作。
							
							//开始进行数据读取的时候，将CONNECTION_STATE_RECV置位；如果读取过程中出现错误，则
							//将CONNECTION_STATE_RECV复位，将CONNECTION_STATE_RECVERR置位。然后离开读取
							//过程即可。当数据顺利读取完毕之后，设置为CONNECTION_STATE_EMPTY状态。
							g_tcp_connection_list.tcpConnList[index]->recvingThread = myid;
							g_tcp_connection_list.tcpConnList[index]->curState |= CONNECTION_STATE_RECV;
							LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs); //暂时离开临界区进行数据读取
                            //printf("[%d] 开始接收\n" , myid);
							recvrst = EvoTCPRecv(g_tcp_connection_list.tcpConnList[index]);
                            //printf("[%d] 接收完毕\n" , myid);
							EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
							//printf("[%d] 进入处理环节\n" , testRecvCounter++);
							g_tcp_connection_list.tcpConnList[index]->recvingThread = INVALID_THREAD_ID;
							g_tcp_connection_list.tcpConnList[index]->curState &= ~CONNECTION_STATE_RECV;
							//根据返回内容以及连接状态调用数据响应函数
							//由于数据响应函数可能占用时间较长，由此导致g_tcp_connection_list.tcpConnList[index]
							//被锁住，无法向其发送队列插入内容，因此这里采用解锁方式调用响应函数。
							if(recvrst!=NULL)
							{
								//有可能此时连接已经失效
								if(!(g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_INVALID))
								{
									TCPConnectionId tmpId = g_tcp_connection_list.tcpConnList[index]->id;
									//tmpId.id = g_tcp_connection_list.tcpConnList[index]->id.id;
									//tmpId.index = g_tcp_connection_list.tcpConnList[index]->id.index;
									g_tcp_connection_list.tcpConnList[index]->curState |= CONNECTION_STATE_POP;
									LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
                                    //printf("[%d] 接收完毕\n" , testRecvCounter);
									//将数据传递给上层，需要删除所有的数据缓存
									RecvData(tmpId, g_tcp_connection_list.tcpConnList[index]->port, recvrst);
                                    //printf("[%d] 处理完毕\n" , testRecvCounter++);
									
									EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
									g_tcp_connection_list.tcpConnList[index]->curState &= ~CONNECTION_STATE_POP;
								}
									
								//删除数据
								while(recvrst)
								{
									
									if(recvrst->type == EVO_TCP_MSG_HEARTBEAT)
									{
										tmptask = recvrst->next;
										free(recvrst);
										if(tmptask == NULL)
											break;
										recvrst = tmptask;
									}
									else
									{
										//g_tcp_connection_list.tcpConnList[index]->incomingPacketNum+=recvrst->eleNum;
										//printf("<<<<<<<<<<<<<<<<<<<获取数据 : %d\n" , recvrst->buflength);
										tmptask = recvrst->next;
										free(recvrst->buf);
										free(recvrst);
										if(tmptask == NULL)
											break;
										recvrst = tmptask;
									}
								}
							}//向上层传递数据，并删除申请的内存
							else
							{
								//printf("[%d]没有完整数据\n" , testRecvCounter);
							}

							//到此为止，可能存在两种情况，一包数据没收完，出错，都不需要理会	
						}//处理一个网络连接上的数据读取
					}//如果连接有效
				}//遍历所有的网络连接
			}//处理网络连接
			LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);

			//查看是否存在Listener
			EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
			if(g_tcp_listener_list.listenerNum>0)
			{
				for(index=myid;index<MAX_TCPLISTENER_LIST_LENGTH;index+=TCP_RECV_THREAD_COUNT)
				{
					if(g_tcp_listener_list.tcpListenerList[index]!=NULL)
					{
						if(FD_ISSET(g_tcp_listener_list.tcpListenerList[index]->socket, &readset))
						{
							//有新的连接到达
							//首先查看是否是中断端口，通过当前系统状态判定
							if(g_TCPNetLayerState == STATE_NOT_INITIALIZED) //处于初始化状态
							{
								//通过Accept得到本线程的中断端口，由于接收线程是串行初始化,
								//因此只有当前正在初始化的线程的状态为TCP_THREAD_STATE_INIT
								acceptSocket = accept(g_tcp_listener_list.tcpListenerList[index]->socket , (struct sockaddr*)&clientaddr , &addrsize);
								//出错处理
								if(acceptSocket == INVALID_SOCKET)
								{
									//如果无法建立自中断连接，则后续工作无法进行
									//但是这里无法直接以返回值的形式通知上层应用
									//这里的处理方式是再一次尝试，上层应用在经过
									//时延之后，会发现无法初始化成功
									printf("线程自中断连接初始化出现异常，可能为自监听端口失效，建议退出\n");
									continue;
								}


								//由于只有第一个线程负责buildin端口的监听，因此初始化过程中也只有该线程可以得到所有
								//其他线程的初始化连接请求，这里需要利用线程的状态判定是哪一个线程正在进行初始化。
								//线程数据结构刚生成的时候状态为TCP_THREAD_STATE_INVALID，在初始化开始后设置为
								//TCP_THREAD_STATE_INIT，并在初始化结束后转为TCP_THREAD_STATE_IDLE
								for(threadIndex=0;threadIndex<TCP_RECV_THREAD_COUNT;threadIndex++)
								{
									if(g_tcp_recvthreadpool.recvThreadList[threadIndex].threadState == TCP_THREAD_STATE_INIT)
									{
										//只有当前进行初始化的线程才是这个状态，设置自中断连接端口
										g_tcp_recvthreadpool.recvThreadList[threadIndex].selfinterupt_recv = acceptSocket;
                                        //printf("accepted interrupt port\n");
										break;
									}
								}//循环考察各个线程
								break;
							}//当前为系统初始化阶段
							//系统已经完成了初始化，来自外部的连接请求以ConnectionNode的形式插入ConnectionList
							else if(g_TCPNetLayerState == STATE_HAVE_INITIALIZED)
							{
								acceptSocket = accept(g_tcp_listener_list.tcpListenerList[index]->socket , (struct sockaddr*)&clientaddr , &addrsize);

								//出错处理，需要补充
								if(SetConnectionSockOPT(acceptSocket) == -1)
								{
									closesocket(acceptSocket);
								}
								else
								{
                                    printf("线程%d接收到连接请求\n",index);
									//插入ConnectionList，需要避免死锁
                                    acceptConnId = InsertPassiveTCPConnection(inet_ntoa(clientaddr.sin_addr),
										g_tcp_listener_list.tcpListenerList[index]->port, //Listener的port
										g_tcp_listener_list.tcpListenerList[index]->id,  //Listener的Id
										acceptSocket);
									//出错处理，需要补充
									//将网络连接建立事件通知上层
									AcceptConnection(g_tcp_listener_list.tcpListenerList[index]->id , 
										acceptConnId ,
										inet_ntoa(clientaddr.sin_addr), g_tcp_listener_list.tcpListenerList[index]->port);

									//新建立的连接很可能与该Listener不是一个处理线程，因此在插入新的连接之后
									//需要根据处理线程进行唤醒
									if(acceptConnId.index%TCP_RECV_THREAD_COUNT != myid) //需要尝试唤醒另一个线程
									{
										if(g_tcp_recvthreadpool.recvThreadList[(myid+1)%TCP_RECV_THREAD_COUNT].threadState!=TCP_THREAD_STATE_RECVING)
										{
											TrytoInteruptTCPRecvThread((myid+1)%TCP_RECV_THREAD_COUNT);
										}
									}
								}
							}//当前系统为工作状态
						}//分两种情况执行accept
					} //针对一个特定的listener
				}//考察各个listener
			}//如果存在Listener
			LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);

			//printf("[%d] 读取轮数\n" , testRecvCounter++ );
			//最后查看是否为本线程的中断端口，往往意味着需要重新装载
			if(FD_ISSET(g_tcp_recvthreadpool.recvThreadList[myid].selfinterupt_recv , &readset))
			{
				//接收报文头即可
				int rst = EvoTCPRecvInterupt(g_tcp_recvthreadpool.recvThreadList[myid].selfinterupt_recv);
                //printf("线程 %d  被中断\n" , myid);
			}
		}

	}

    return NULL;
}
int TrytoInteruptTCPRecvThread(int threadIndex)
{
	char tmpbuf[SIZEOF_MSGHEADER];
	int sendlength = 0;
	int tmplen = 0;
	char* pos = tmpbuf;
	SOCKET tmpsocket = g_tcp_recvthreadpool.recvThreadList[threadIndex].selfinterupt_send;

	EvoTCPMessageHeader tmpheader;
	tmpheader.type = EVO_TCP_MSG_INTERUPT;
	tmpheader.flags = 0;
	tmpheader.octetsToNextHeader = 0;

    //printf("发送中断端口,线程%d\n",threadIndex);
    MapMsgHeader(tmpbuf , tmpheader);
	while(sendlength<SIZEOF_MSGHEADER)
	{
		if((tmplen = send(tmpsocket , pos , SIZEOF_MSGHEADER - sendlength , 0)) == SOCKET_ERROR)
		{
			int err = GetLastError();
			if(err == WSAEWOULDBLOCK)
			{
				//报文头采用比较简化的实现机制，循环读取
				Sleep(0);
				continue;
			}
			else
			{
				//出错处理，所有的资源由监控线程负责释放
				return -1;
			}
		}
		sendlength+=tmplen;
		pos+=tmplen;
	}
	return 1;

}


//通过以下函数将网络事件传递给上层
//有连接请求到达，接收并调用InsertPassiveTCPConnection后，
//调用该函数，通知上层应用。上层应用可以通过此函数中传递的
//ConnectionId判定后续到达的数据，并利用该id发送数据
int AcceptConnection(TCPConnectionId listenerId , TCPConnectionId connId , char* addr, unsigned short port)
{
	EvoTCPNetworkEvent tmpEvent;
	tmpEvent.type = EVO_TCP_EVENT_ACCEPT;
	tmpEvent.parentId = listenerId;
	tmpEvent.id = connId;
	tmpEvent.buf = _strdup(addr);
	tmpEvent.bufLength = strlen(addr);
	tmpEvent.port = port;
	//EnterCriticalSection(&g_tcpEventCallback_cs);
	if(g_tcpEventCallbackFun!=NULL)
	{
		g_tcpEventCallbackFun(tmpEvent);
	}
	//LeaveCriticalSection(&g_tcpEventCallback_cs);

	return 1;
}

//读取完整数据之后，调用该函数将数据传递给上层
int RecvData(TCPConnectionId connId , unsigned short port, TCPNetworkTask* dataList)
{
	while(dataList)
	{
		if(dataList->type == EVO_TCP_MSG_HEARTBEAT)
		{
			//printf("得到心跳报文\n");
			dataList = dataList->next;
			continue;
		}
		else if(dataList->type == EVO_TCP_MSG_DATA)
		{
			//如果是大包数据则直接发送，不存在内存的再次分配
			if(dataList->eleNum == 1 && dataList->flags == 0)
			{
				EvoTCPNetworkEvent tmpEvent;

				tmpEvent.type = EVO_TCP_EVENT_RECV;
				tmpEvent.parentId = INVALID_CONNECTION;
				tmpEvent.id = connId;
				tmpEvent.buf = dataList->buf;
				tmpEvent.bufLength = dataList->buflength;
				tmpEvent.port = port;
				tmpEvent.para1 = dataList->eleNum;
				//EnterCriticalSection(&g_tcpEventCallback_cs);
				if(g_tcpEventCallbackFun!=NULL)
				{
					//g_tcpEventCallback_state = STATE_CALLBACK_INUSE;
					g_tcpEventCallbackFun(tmpEvent);
					//g_tcpEventCallback_state = STATE_CALLBACK_IDLE;
				}
				//LeaveCriticalSection(&g_tcpEventCallback_cs);
			}
			else //如果是合包数据则需要拆包
			{
                u_int32_t tmplength = 0;
				unsigned short tmpEleNum = dataList->eleNum;
				//printf("准备分发合包数据,数据尺寸%d , 包含数据包 %d\n" , dataList->buflength , dataList->eleNum);
				while(tmpEleNum>0)
				{
					EvoTCPNetworkEvent tmpEvent;
					unsigned short segSize = 0;
					tmpEvent.type = EVO_TCP_EVENT_RECV;
					tmpEvent.parentId = INVALID_CONNECTION;
					tmpEvent.id = connId;
					//解析出子数据包的大小
					memcpy((void*)&segSize , (void*)(dataList->buf+tmplength) , SIZEOF_DATASEGFLAG);
					tmplength+=SIZEOF_DATASEGFLAG;
					//分配内存
					//printf("本包数据 :%d\n" , segSize);
					tmpEvent.buf = (char*)malloc(segSize);
					if(tmpEvent.buf == NULL)
					{
						return 1;
					}
					//复制内容
					memcpy((void*)tmpEvent.buf , (void*)(dataList->buf+tmplength) , segSize);
					tmpEvent.bufLength = segSize;
					tmpEvent.port = port;
					tmpEvent.para1 = 1;
					tmplength+=segSize;
					//EnterCriticalSection(&g_tcpEventCallback_cs);
					if(g_tcpEventCallbackFun!=NULL)
					{
						g_tcpEventCallbackFun(tmpEvent);
					}
					//LeaveCriticalSection(&g_tcpEventCallback_cs);
					free(tmpEvent.buf);
					tmpEleNum--;
				}
			} //合包发送
		}//数据报文

		dataList = dataList->next;
	}

	return 1;
}

