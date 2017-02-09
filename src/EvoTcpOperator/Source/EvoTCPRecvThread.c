#ifdef _WIN32
#define EvoTCPOperatorLibAPI __declspec(dllexport)
#else
#define EvoTCPOperatorLibAPI
#endif

#include "../Include/EvoTCPCommon.h"

/*****************************************************
ConnectionListģ���ȫ�ֱ���
�����̴߳������б���װ�ض˿ڣ�������������ж϶˿ڣ�Ȼ��
���м�����
*****************************************************/
extern TCPConnectionId INVALID_CONNECTION;

extern TCPConnectionId INVALID_LISTENER;

extern TCPConnectionList g_tcp_connection_list;

extern TCPListenerList g_tcp_listener_list;

extern u_int32_t g_id_base;

u_int32_t testRecvCounter = 0;

/*****************************************************
TCPOperatorģ���ȫ�ֱ���
*****************************************************/
//�����̳߳�ʼ������ɶ�ÿ���߳��ж϶˿ڵĳ�ʼ������ʹ�øü����˿�
//TCPOperator��ģ���ʼ��������֤�����ж϶˿ڳ�ʼ��֮ǰ���Ѿ��ڸ�
//�˿ڽ��м���
extern u_int32_t g_tcp_self_interupt_port;

extern TCPNetLayerWorkState g_TCPNetLayerState;

extern DWORD g_cpu_number;

//�ϲ�Ӧ��ע��Ļص�����
extern EvoTCPNetworkEventHook g_tcpEventCallbackFun;
//extern CRITICAL_SECTION g_tcpEventCallback_cs;
//extern CallbackFunState g_tcpEventCallback_state;


/*****************************************************
��ģ���ȫ�ֱ���
*****************************************************/
//��Ҫ���г�ʼ��
TCPRecvThreadPool g_tcp_recvthreadpool;


/*****************************************************
��ģ��ĺ���
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
	//���ɲ������߳�
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

		//�����ȡ��CPU��Ϣ������ָ���ں�
        if(g_cpu_number == 8) //������i7���������ĺ˰��߳�
		{
			//�����߳�ͨ��״������������������ǰ����CPU
			SetThreadIdealProcessor(g_tcp_recvthreadpool.recvThreadList[index].threadHandle , index%2);
		}
		else if(g_cpu_number == 4) //i5˫�����߳�
		{
			SetThreadIdealProcessor(g_tcp_recvthreadpool.recvThreadList[index].threadHandle , index%2);
        }
#else
        pthread_create(&g_tcp_recvthreadpool.recvThreadList[index].threadHandle,NULL,TCPRecvThreadRoutine,NULL);	
#endif
        //�����̵߳�ǰ��״̬Ϊ��ʼ��״̬��ͬһʱ����ֻ��һ���̴߳��ڸ�״̬���߳��ڳ�ʼ��������
		//���øñ�־�ҵ������Ӧ�����ݽṹ���������ж϶˿�
		g_tcp_recvthreadpool.recvThreadList[index].threadState = TCP_THREAD_STATE_INIT;

		//�ֱ����жϼ����˿ڷ������ӣ��ù������������̣���ζ�ź������ص�ʱ��accept����Ҳ�Ѿ�������
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
		//����ö˿ڣ��Ժ�ͨ���ö˿ڷ����ж�
		g_tcp_recvthreadpool.recvThreadList[index].selfinterupt_send = tmpsocket;

		//ѭ���ȴ��ж϶˿ڽ��ն˳�ʼ��
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
		if(!initTest) //�ж϶˿ڳ�ʼ��û�ɹ�
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
�������ԭ�������
������ղ������̣߳��ֱ���Listener�Լ�Connection�б��еĶ˿ڵļ�����
����select��⣬��ֱ���������˿ڶ�ȡ����������ȡ�����ݴ��ݸ��ϲ㣻
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
		//װ��ListenList�еĶ˿�
		EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
		if(g_tcp_listener_list.listenerNum>0)
		{
			for(index=myid;index<MAX_TCPLISTENER_LIST_LENGTH;index+=TCP_RECV_THREAD_COUNT)
			{
				if(g_tcp_listener_list.tcpListenerList[index]!=NULL)
				{
					if (g_tcp_listener_list.tcpListenerList[index]->state == LISTENER_STATE_EMPTY)
					{
						//printf("�߳� %d  ���������� %d\n" , myid ,  g_tcp_listener_list.tcpListenerList[index]->port);
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

		//װ��ConnectionList�еĶ˿�
		EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
		if(g_tcp_connection_list.connNum>0)
		{
			for(index=myid;index<MAX_TCPCONNECTION_LIST_LENGTH;index+=TCP_RECV_THREAD_COUNT)
			{
				if(g_tcp_connection_list.tcpConnList[index]!=NULL)
				{
					//ֻ�������ķǶ�ȡ״̬���ܽ��м���
					if ((g_tcp_connection_list.tcpConnList[index]->curState == CONNECTION_STATE_EMPTY)
						|| (g_tcp_connection_list.tcpConnList[index]->curState == CONNECTION_STATE_SEND)
						|| (g_tcp_connection_list.tcpConnList[index]->recvingThread == INVALID_THREAD_ID)
						&& !(g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_RECVERR
						|| g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_SENDERR
						|| g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_INVALID))
					{
						//printf("�߳�%d�����������%d : %d\n" , myid , index , g_tcp_connection_list.tcpConnList[index]->port);
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

		//װ��������ж϶˿�
		if (g_tcp_recvthreadpool.recvThreadList[myid].selfinterupt_recv != INVALID_SOCKET)
		{
			//printf("�߳� %d װ���ж϶˿� %d-%d\n" , myid , 
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

		//��û����ɳ�ʼ����ѭ���ȴ�
		if(fdNum == 0)
		{
			Sleep(10);
			continue;
		}
		
		if(g_TCPNetLayerState == STATE_HAVE_INITIALIZED)
		{
			//���ϵͳ�Ѿ�����˳�ʼ������Ӧ����ʼ���̵߳Ĺ���״̬���й���
			//������ϵͳ��ʼ���׶��У���һ�������̻߳��ߵ����λ�ã����������������������̵߳�
			//���ж��������󣬴�ʱϵͳ��״̬��STATE_NOT_INITIALIZED������׶ο��Բ����ù���״̬
			g_tcp_recvthreadpool.recvThreadList[myid].threadState = TCP_THREAD_STATE_SELECT;
		}

		//printf("װ�����\n");

		//��ʼ�����˿ڣ��������̣�����ͨ���ж϶˿ڻ���
#ifdef _WIN32
        selectresult = select(0, &readset,NULL,NULL,NULL);
#else
        selectresult = select(fdMax+1, &readset,NULL,NULL,NULL);
#endif

        //printf("�߳�%d��⵽����%d\n",myid,selectresult);

		g_tcp_recvthreadpool.recvThreadList[myid].recvRound++;
		if (selectresult == 0)
		{
			//printf("��⵽����\n");
			continue;
		}
		else if (selectresult == SOCKET_ERROR)
		{
            //int err = WSAGetLastError();
			
			continue;
		}
		else //ע�⣬���������п��ܴ��ڶ˿ڱ��ϲ�ȡ�������
		{
			if(g_TCPNetLayerState == STATE_HAVE_INITIALIZED)
			{
				g_tcp_recvthreadpool.recvThreadList[myid].threadState = TCP_THREAD_STATE_RECVING;
			}
			
			//�鿴�Ƿ����Connection
			EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
			if(g_tcp_connection_list.connNum>0)
			{
				for(index=myid;index<MAX_TCPCONNECTION_LIST_LENGTH;index+=TCP_RECV_THREAD_COUNT)
				{
					if(g_tcp_connection_list.tcpConnList[index]!=NULL)
					{
						if(FD_ISSET(g_tcp_connection_list.tcpConnList[index]->socket, &readset))
						{
							//�����ݵ����ʼ�������ݣ�������Ҫ�����g_tcp_connection_list�ĳ���ռ��
							//���ݶ�ȡ���������ٽ���������������Ҫע�⣬�ڴ˹����е��쳣����
							//���ݶ�ȡ�����п��ܴ��������쳣��1��recv�����쳣��2��send�����쳣��3����������쳣��
							//��ˣ������Ƿ�����һ��������쳣����ֻ�ǽ����ӵ�״̬����ΪʧЧ���ɣ�����Ҫ�����е�
							//��Դ�����κ�ɾ��������
							
							//��ʼ�������ݶ�ȡ��ʱ�򣬽�CONNECTION_STATE_RECV��λ�������ȡ�����г��ִ�����
							//��CONNECTION_STATE_RECV��λ����CONNECTION_STATE_RECVERR��λ��Ȼ���뿪��ȡ
							//���̼��ɡ�������˳����ȡ���֮������ΪCONNECTION_STATE_EMPTY״̬��
							g_tcp_connection_list.tcpConnList[index]->recvingThread = myid;
							g_tcp_connection_list.tcpConnList[index]->curState |= CONNECTION_STATE_RECV;
							LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs); //��ʱ�뿪�ٽ����������ݶ�ȡ
                            //printf("[%d] ��ʼ����\n" , myid);
							recvrst = EvoTCPRecv(g_tcp_connection_list.tcpConnList[index]);
                            //printf("[%d] �������\n" , myid);
							EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
							//printf("[%d] ���봦����\n" , testRecvCounter++);
							g_tcp_connection_list.tcpConnList[index]->recvingThread = INVALID_THREAD_ID;
							g_tcp_connection_list.tcpConnList[index]->curState &= ~CONNECTION_STATE_RECV;
							//���ݷ��������Լ�����״̬����������Ӧ����
							//����������Ӧ��������ռ��ʱ��ϳ����ɴ˵���g_tcp_connection_list.tcpConnList[index]
							//����ס���޷����䷢�Ͷ��в������ݣ����������ý�����ʽ������Ӧ������
							if(recvrst!=NULL)
							{
								//�п��ܴ�ʱ�����Ѿ�ʧЧ
								if(!(g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_INVALID))
								{
									TCPConnectionId tmpId = g_tcp_connection_list.tcpConnList[index]->id;
									//tmpId.id = g_tcp_connection_list.tcpConnList[index]->id.id;
									//tmpId.index = g_tcp_connection_list.tcpConnList[index]->id.index;
									g_tcp_connection_list.tcpConnList[index]->curState |= CONNECTION_STATE_POP;
									LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
                                    //printf("[%d] �������\n" , testRecvCounter);
									//�����ݴ��ݸ��ϲ㣬��Ҫɾ�����е����ݻ���
									RecvData(tmpId, g_tcp_connection_list.tcpConnList[index]->port, recvrst);
                                    //printf("[%d] �������\n" , testRecvCounter++);
									
									EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
									g_tcp_connection_list.tcpConnList[index]->curState &= ~CONNECTION_STATE_POP;
								}
									
								//ɾ������
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
										//printf("<<<<<<<<<<<<<<<<<<<��ȡ���� : %d\n" , recvrst->buflength);
										tmptask = recvrst->next;
										free(recvrst->buf);
										free(recvrst);
										if(tmptask == NULL)
											break;
										recvrst = tmptask;
									}
								}
							}//���ϲ㴫�����ݣ���ɾ��������ڴ�
							else
							{
								//printf("[%d]û����������\n" , testRecvCounter);
							}

							//����Ϊֹ�����ܴ������������һ������û���꣬����������Ҫ���	
						}//����һ�����������ϵ����ݶ�ȡ
					}//���������Ч
				}//�������е���������
			}//������������
			LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);

			//�鿴�Ƿ����Listener
			EnterCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);
			if(g_tcp_listener_list.listenerNum>0)
			{
				for(index=myid;index<MAX_TCPLISTENER_LIST_LENGTH;index+=TCP_RECV_THREAD_COUNT)
				{
					if(g_tcp_listener_list.tcpListenerList[index]!=NULL)
					{
						if(FD_ISSET(g_tcp_listener_list.tcpListenerList[index]->socket, &readset))
						{
							//���µ����ӵ���
							//���Ȳ鿴�Ƿ����ж϶˿ڣ�ͨ����ǰϵͳ״̬�ж�
							if(g_TCPNetLayerState == STATE_NOT_INITIALIZED) //���ڳ�ʼ��״̬
							{
								//ͨ��Accept�õ����̵߳��ж϶˿ڣ����ڽ����߳��Ǵ��г�ʼ��,
								//���ֻ�е�ǰ���ڳ�ʼ�����̵߳�״̬ΪTCP_THREAD_STATE_INIT
								acceptSocket = accept(g_tcp_listener_list.tcpListenerList[index]->socket , (struct sockaddr*)&clientaddr , &addrsize);
								//������
								if(acceptSocket == INVALID_SOCKET)
								{
									//����޷��������ж����ӣ�����������޷�����
									//���������޷�ֱ���Է���ֵ����ʽ֪ͨ�ϲ�Ӧ��
									//����Ĵ���ʽ����һ�γ��ԣ��ϲ�Ӧ���ھ���
									//ʱ��֮�󣬻ᷢ���޷���ʼ���ɹ�
									printf("�߳����ж����ӳ�ʼ�������쳣������Ϊ�Լ����˿�ʧЧ�������˳�\n");
									continue;
								}


								//����ֻ�е�һ���̸߳���buildin�˿ڵļ�������˳�ʼ��������Ҳֻ�и��߳̿��Եõ�����
								//�����̵߳ĳ�ʼ����������������Ҫ�����̵߳�״̬�ж�����һ���߳����ڽ��г�ʼ����
								//�߳����ݽṹ�����ɵ�ʱ��״̬ΪTCP_THREAD_STATE_INVALID���ڳ�ʼ����ʼ������Ϊ
								//TCP_THREAD_STATE_INIT�����ڳ�ʼ��������תΪTCP_THREAD_STATE_IDLE
								for(threadIndex=0;threadIndex<TCP_RECV_THREAD_COUNT;threadIndex++)
								{
									if(g_tcp_recvthreadpool.recvThreadList[threadIndex].threadState == TCP_THREAD_STATE_INIT)
									{
										//ֻ�е�ǰ���г�ʼ�����̲߳������״̬���������ж����Ӷ˿�
										g_tcp_recvthreadpool.recvThreadList[threadIndex].selfinterupt_recv = acceptSocket;
                                        //printf("accepted interrupt port\n");
										break;
									}
								}//ѭ����������߳�
								break;
							}//��ǰΪϵͳ��ʼ���׶�
							//ϵͳ�Ѿ�����˳�ʼ���������ⲿ������������ConnectionNode����ʽ����ConnectionList
							else if(g_TCPNetLayerState == STATE_HAVE_INITIALIZED)
							{
								acceptSocket = accept(g_tcp_listener_list.tcpListenerList[index]->socket , (struct sockaddr*)&clientaddr , &addrsize);

								//��������Ҫ����
								if(SetConnectionSockOPT(acceptSocket) == -1)
								{
									closesocket(acceptSocket);
								}
								else
								{
                                    printf("�߳�%d���յ���������\n",index);
									//����ConnectionList����Ҫ��������
                                    acceptConnId = InsertPassiveTCPConnection(inet_ntoa(clientaddr.sin_addr),
										g_tcp_listener_list.tcpListenerList[index]->port, //Listener��port
										g_tcp_listener_list.tcpListenerList[index]->id,  //Listener��Id
										acceptSocket);
									//��������Ҫ����
									//���������ӽ����¼�֪ͨ�ϲ�
									AcceptConnection(g_tcp_listener_list.tcpListenerList[index]->id , 
										acceptConnId ,
										inet_ntoa(clientaddr.sin_addr), g_tcp_listener_list.tcpListenerList[index]->port);

									//�½��������Ӻܿ������Listener����һ�������̣߳�����ڲ����µ�����֮��
									//��Ҫ���ݴ����߳̽��л���
									if(acceptConnId.index%TCP_RECV_THREAD_COUNT != myid) //��Ҫ���Ի�����һ���߳�
									{
										if(g_tcp_recvthreadpool.recvThreadList[(myid+1)%TCP_RECV_THREAD_COUNT].threadState!=TCP_THREAD_STATE_RECVING)
										{
											TrytoInteruptTCPRecvThread((myid+1)%TCP_RECV_THREAD_COUNT);
										}
									}
								}
							}//��ǰϵͳΪ����״̬
						}//���������ִ��accept
					} //���һ���ض���listener
				}//�������listener
			}//�������Listener
			LeaveCriticalSection(&g_tcp_listener_list.tcpListenerList_cs);

			//printf("[%d] ��ȡ����\n" , testRecvCounter++ );
			//���鿴�Ƿ�Ϊ���̵߳��ж϶˿ڣ�������ζ����Ҫ����װ��
			if(FD_ISSET(g_tcp_recvthreadpool.recvThreadList[myid].selfinterupt_recv , &readset))
			{
				//���ձ���ͷ����
				int rst = EvoTCPRecvInterupt(g_tcp_recvthreadpool.recvThreadList[myid].selfinterupt_recv);
                //printf("�߳� %d  ���ж�\n" , myid);
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

    //printf("�����ж϶˿�,�߳�%d\n",threadIndex);
    MapMsgHeader(tmpbuf , tmpheader);
	while(sendlength<SIZEOF_MSGHEADER)
	{
		if((tmplen = send(tmpsocket , pos , SIZEOF_MSGHEADER - sendlength , 0)) == SOCKET_ERROR)
		{
			int err = GetLastError();
			if(err == WSAEWOULDBLOCK)
			{
				//����ͷ���ñȽϼ򻯵�ʵ�ֻ��ƣ�ѭ����ȡ
				Sleep(0);
				continue;
			}
			else
			{
				//���������е���Դ�ɼ���̸߳����ͷ�
				return -1;
			}
		}
		sendlength+=tmplen;
		pos+=tmplen;
	}
	return 1;

}


//ͨ�����º����������¼����ݸ��ϲ�
//���������󵽴���ղ�����InsertPassiveTCPConnection��
//���øú�����֪ͨ�ϲ�Ӧ�á��ϲ�Ӧ�ÿ���ͨ���˺����д��ݵ�
//ConnectionId�ж�������������ݣ������ø�id��������
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

//��ȡ��������֮�󣬵��øú��������ݴ��ݸ��ϲ�
int RecvData(TCPConnectionId connId , unsigned short port, TCPNetworkTask* dataList)
{
	while(dataList)
	{
		if(dataList->type == EVO_TCP_MSG_HEARTBEAT)
		{
			//printf("�õ���������\n");
			dataList = dataList->next;
			continue;
		}
		else if(dataList->type == EVO_TCP_MSG_DATA)
		{
			//����Ǵ��������ֱ�ӷ��ͣ��������ڴ���ٴη���
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
			else //����Ǻϰ���������Ҫ���
			{
                u_int32_t tmplength = 0;
				unsigned short tmpEleNum = dataList->eleNum;
				//printf("׼���ַ��ϰ�����,���ݳߴ�%d , �������ݰ� %d\n" , dataList->buflength , dataList->eleNum);
				while(tmpEleNum>0)
				{
					EvoTCPNetworkEvent tmpEvent;
					unsigned short segSize = 0;
					tmpEvent.type = EVO_TCP_EVENT_RECV;
					tmpEvent.parentId = INVALID_CONNECTION;
					tmpEvent.id = connId;
					//�����������ݰ��Ĵ�С
					memcpy((void*)&segSize , (void*)(dataList->buf+tmplength) , SIZEOF_DATASEGFLAG);
					tmplength+=SIZEOF_DATASEGFLAG;
					//�����ڴ�
					//printf("�������� :%d\n" , segSize);
					tmpEvent.buf = (char*)malloc(segSize);
					if(tmpEvent.buf == NULL)
					{
						return 1;
					}
					//��������
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
			} //�ϰ�����
		}//���ݱ���

		dataList = dataList->next;
	}

	return 1;
}

