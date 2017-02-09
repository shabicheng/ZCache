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
extern TCPConnectionList g_tcp_connection_list;

extern TCPListenerList g_tcp_listener_list;

extern u_int32_t g_id_base;

u_int32_t testCounter = 0;

/*****************************************************
TCPOperatorģ���ȫ�ֱ���
*****************************************************/
//�����̳߳�ʼ������ɶ�ÿ���߳��ж϶˿ڵĳ�ʼ������ʹ�øü����˿�
//TCPOperator��ģ���ʼ��������֤�����ж϶˿ڳ�ʼ��֮ǰ���Ѿ��ڸ�
//�˿ڽ��м���
extern u_int32_t g_tcp_self_interupt_port;

extern TCPNetLayerWorkState g_TCPNetLayerState;

extern DWORD g_cpu_number;

/*****************************************************
��ģ���ȫ�ֱ���
*****************************************************/
//��Ҫ���г�ʼ��
TCPSendThreadPool g_tcp_sendthreadpool;


/*****************************************************
��ģ��ĺ���
*****************************************************/

int InitializeTCPSendThread()
{
	int index; //�����������е�connection

    //int waittimes = 0;
    //int initTest = 0;

#ifdef _WIN32
    g_tcp_sendthreadpool.sendSema = CreateSemaphore(NULL, 0, TCP_SEND_THREAD_COUNT, NULL);
#else
    sem_init(&g_tcp_sendthreadpool.sendSema,0,0);
#endif
	g_tcp_sendthreadpool.sendThreadIdBase = 0;
	for(index=0;index<TCP_SEND_THREAD_COUNT;index++)
	{
		g_tcp_sendthreadpool.sendThreadList[index].threadState = TCP_THREAD_STATE_INVALID;

#ifdef _WIN32
        g_tcp_sendthreadpool.sendThreadList[index].threadHandle = (HANDLE)_beginthreadex(NULL ,
                                                                                         0 ,
                                                                                         TCPSendThreadRoutine ,
                                                                                         0 ,
                                                                                         0 ,
                                                                                         &(g_tcp_sendthreadpool.sendThreadList[index].threadId));


        SetThreadPriority(g_tcp_sendthreadpool.sendThreadList[index].threadHandle , THREAD_PRIORITY_BELOW_NORMAL);

		//�����ȡ��CPU��Ϣ������ָ���ں�
        if(g_cpu_number == 8) //������i7���������ĺ˰��߳�
		{
            //�����߳�ͨ��״����������
			SetThreadIdealProcessor(g_tcp_sendthreadpool.sendThreadList[index].threadHandle , index%2+2);
		}
		else if(g_cpu_number == 4) //i5˫�����߳�
		{
			SetThreadIdealProcessor(g_tcp_sendthreadpool.sendThreadList[index].threadHandle , index%2+2);
        }
#else
        pthread_create(&g_tcp_sendthreadpool.sendThreadList[index].threadHandle,NULL,TCPSendThreadRoutine,NULL);
#endif
		g_tcp_sendthreadpool.sendThreadList[index].threadState = TCP_THREAD_STATE_IDLE;
	}
	return TCP_SENDTHREADINIT_OK;
}

int FinalizeTCPSendThread()
{
	int index;
	for (index = 0; index < TCP_SEND_THREAD_COUNT; index++)
	{
		g_tcp_sendthreadpool.sendThreadList[index].isRunning = 0;
	}

#ifdef _WIN32
	CloseHandle(g_tcp_sendthreadpool.sendSema);
#else
	sem_destroy(&g_tcp_sendthreadpool.sendSema);
#endif

	for(index=0;index<TCP_SEND_THREAD_COUNT;index++)
	{
#ifdef _WIN32
		if (WAIT_OBJECT_0 != WaitForSingleObject(g_tcp_sendthreadpool.sendThreadList[index].threadHandle, 1000))
		{
			TerminateThread(g_tcp_sendthreadpool.sendThreadList[index].threadHandle, 0);
		}
		CloseHandle(g_tcp_sendthreadpool.sendThreadList[index].threadHandle);
#else
		struct timespec ts;
		if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
		{
			/* Handle error */
		}

		ts.tv_sec += 1;
		if (pthread_timedjoin_np(g_tcp_sendthreadpool.sendThreadList[index].threadHandle, NULL, &ts) != 0)
		{
			pthread_cancel(g_tcp_sendthreadpool.sendThreadList[index].threadHandle);
		}
#endif // _WIN32	
	}
	return 1;
}

#ifdef _WIN32
DWORD WINAPI TCPSendThreadRoutine(PVOID para)
#else
void* TCPSendThreadRoutine(void * para)
#endif
{
#ifdef _WIN32
#else
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif // _WIN3

	int index;
	TCPConnectionId curSendingId; //��ǰ����ִ�з������������id
	int tgtIndex = 0; //������¼������������connection���±�
    u_int32_t taskCount = 0; //������¼��ǰ����������Ķ˿��ϵ���������
	
	//���ڷ��Ͷ��������ƣ���ʱ�������������ƻ���
	//��¼���߳��������������Ĵ������������һ���޶ȣ�����Ҫ���й���
	//���ﲢû������ض����ӽ����������Ƶ����
	unsigned short sendBlockTimes = 0;
	//��¼�Ƿ��ڱ����з��������߳�
	unsigned char findBlockingConnection = 0;

	//һ�η���ѭ�����Ѿ����͵ĳߴ�����ݰ�����
	//����������޶���Ƴ����η���ѭ�������ⵥһ
	//���ӹ���ʱ��ռ�÷����߳�
    u_int32_t sendSize = 0;
    //u_int32_t lastCount = 0;
    //unsigned short sleepInterval = 100;

	EvoThreadId myid = g_tcp_sendthreadpool.sendThreadIdBase++;

	g_tcp_sendthreadpool.sendThreadList[myid].isRunning = 1;

	while (g_tcp_sendthreadpool.sendThreadList[myid].isRunning)
	{
		//�ȴ������ѣ���������һ�Ĵ��ڣ������߳̿��ܱ�����֮��ò�����������
		//������ᵼ�·����߳̿�תһ��ѭ��������Ӱ����ȷ��
#ifdef _WIN32
        WaitForSingleObject(g_tcp_sendthreadpool.sendSema, INFINITE); //�ȴ�������
#else
        sem_wait(&g_tcp_sendthreadpool.sendSema);
#endif
        //printf("send data...\n");
		while(1) //���뱾�η���ѭ��
		{
			findBlockingConnection = 0;
			//printf("�����߳� %d  -- ���빤��״̬\n" , myid);
			//�̱߳����ѣ�˵��������Ҫ���͵�����
			EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
			g_tcp_sendthreadpool.sendThreadList[myid].threadState = TCP_THREAD_STATE_SCANING;
			//��ʼɨ��connectionList����ѯ��Ҫ���͵�����
			//��������ʱ����Ҫ��֤��ֻ�����������Ķ˿ڲ�����SEND״̬������
			//�����������������£��Ż����߳�
			//��Ҫע����ǣ����ܴ�����������������б���û�����񣬵����������Ļ�
			//������Ȼ����û�з�����ϵ����ݣ���ˣ���Ҫ�������Ļ���ͬ����Ϊһ��
			//��������
			//��һ���棬�ڵȴ����Ͷ���curTask��ͬ������һ�������ķ�������
			/******************************* ���ڵ�����*********************************
			����1��
			��ģ������������⣺����״̬ΪSEND���߳�ΪINVALID��ʼ�ղ鲻����
			����ʲô�׶Σ�����һ���߳̽�״̬�޸�ΪSEND�������������������ʱ
			�򣬸�����ᵼ�����������߳�ͬʱ����˯��״̬�����ڵĽ��������ͬ
			ʱ�ж����ӵ��߳�id�������INVALID����Ҫ���ѳ�˯�̡߳�2014-8-9
			ͬ��������Ҳ���������ݽ����߳��У����ᵼ��ֹͣ��������
			******************************************************************************/
			tgtIndex = 0;
			taskCount = 0;
			for(index=0;index<MAX_TCPCONNECTION_LIST_LENGTH;index++)
			{

				if(g_tcp_connection_list.tcpConnList[index]!=NULL)
				{
					//��������������ڷ���״̬���߳���״̬���򲻿��Ƿ���
					//���ڴ�������1������������������һ�������߳�id�ļ��
					//�����һ�ִ��и����ӷ����������ڱ��ֲ������ʹ���
					if((g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_SENDERR)||
						(g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_INVALID) ||
						(g_tcp_connection_list.tcpConnList[index]->sendBlock == 1)) 
					{
						if(g_tcp_connection_list.tcpConnList[index]->sendBlock == 1
							&&(g_tcp_connection_list.tcpConnList[index]->taskList->count>0
							|| g_tcp_connection_list.tcpConnList[index]->sendContext.dataLength>0
							|| g_tcp_connection_list.tcpConnList[index]->curTasks->count>0))
						{	
							//�����������ӣ����������������ݣ��򱾴β����Ƿ��ͣ�����һ��ѭ��֮��
							//�ٿ��Ƿ���
							findBlockingConnection = 1; 
							//��Ϣһ�֣���һ���и����������Լ������̶ȷ���
							g_tcp_connection_list.tcpConnList[index]->sendBlock  = 0;
						}
						//�����Ӳ����Ϸ���Ҫ�󣬿�����һ����
						continue;
					}

					if(!(g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_SEND)||
						g_tcp_connection_list.tcpConnList[index]->sendingThread == INVALID_THREAD_ID)
					{
						//�����ӿ���ִ�з���������������
						//�������п���ִ�з�����������ӣ��ֱ������������ʱ���������������
						//��ʱ������ķ�Χ�ڣ�ѡ�������������ġ����ʱ�䳬���޶ȣ�����������
						if(g_tcp_connection_list.tcpConnList[index]->taskList->count>0
							|| g_tcp_connection_list.tcpConnList[index]->sendContext.dataLength>0
							|| g_tcp_connection_list.tcpConnList[index]->curTasks->count>0)
						{
							//�鿴�ϴδ���ʱ��
							clock_t tmpclock = clock();
							unsigned short tmpcount = g_tcp_connection_list.tcpConnList[index]->taskList->count;
							if(g_tcp_connection_list.tcpConnList[index]->sendContext.dataLength>0)
								tmpcount += 1;
							if(g_tcp_connection_list.tcpConnList[index]->curTasks->count>0)
								tmpcount += g_tcp_connection_list.tcpConnList[index]->curTasks->count;

							if((tmpclock - g_tcp_connection_list.tcpConnList[index]->lastSendTime) > MAX_SEND_INTERVAL) 
								//���������ʱ�䣬��Ҫ���̴�������ֻ��һ������
							{
								tgtIndex = index;
								taskCount = tmpcount;
								g_tcp_connection_list.tcpConnList[index]->lastSendTime = tmpclock;
								break;
							}

							//�ҵ�������������
							if(tmpcount> taskCount)
							{
								tgtIndex = index;
								taskCount = tmpcount;
							}
						}
					}//�ж�����״̬
				}//����һ������
			}
			//ѡ���������ӣ����궨����״̬
			if(taskCount>0)
			{
				//printf("�߳� %d ��ȡ���� %d �������� %d \n" , myid , tgtIndex , taskCount);
				if(g_tcp_connection_list.tcpConnList[tgtIndex]->sendContext.dataLength == 0 &&
					g_tcp_connection_list.tcpConnList[tgtIndex]->curTasks->count == 0 &&
					g_tcp_connection_list.tcpConnList[tgtIndex]->taskList->count > 0 )
				{
					//�����ǰ��������Ϊ�գ������ת��������������͵�ǰ��������
					//������ڸĽ��ռ䣬����ʹ���͸��Ӿ���
					SwitchTaskList(g_tcp_connection_list.tcpConnList[tgtIndex]->curTasks , 
						g_tcp_connection_list.tcpConnList[tgtIndex]->taskList);
				}
				//�з�����������ָ���½�
				if(sendBlockTimes>0)
					sendBlockTimes--;
			}
			else
			{
				g_tcp_sendthreadpool.sendThreadList[myid].threadState = TCP_THREAD_STATE_IDLE;
				LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
				if(findBlockingConnection == 1) //�������ݣ��������ӷ���������
				{
					//�������Ϊ�������¿�ѭ��������Կ��ǹ�����߳�
					//���𲻻���Ʊ��̵߳ķ��ͣ����ǿ��԰Ѵ�����Դ�������߳�
					sendBlockTimes++;
					//������ܴ���һ������Ƕ�������Ȼ������δ���͵�����
					//������Ȼ�ڷ������в������ݵ�ʱ������ӵ����������
					//����²�Ӧ���뿪�ڲ�ѭ��
					Sleep(0);
					//printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					continue;
				}
				else
				{
					break; //��������㣬���������ź���
				}
			}
			//�뿪�ٽ�����������Ҫ��֤���͹����в����������
			//������Ҫ��ָ���ӵ�ʧЧ��ʧЧ�����Ӳ�Ӧ�õ������ݽṹ���쳣
			//���ӵ�ʧЧ�������Խ����̡߳�����߳��Լ��ϲ��ע������
			//���͵Ĺ�������Ҫ���ϼ�����������״̬����������κ��쳣�����˳�����
			g_tcp_connection_list.tcpConnList[tgtIndex]->curState |= CONNECTION_STATE_SEND;
			g_tcp_connection_list.tcpConnList[tgtIndex]->sendingThread = myid;
			curSendingId = g_tcp_connection_list.tcpConnList[tgtIndex]->id;
			LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
			//���ٽ�������ִ�з����������۷��ͻ��ǽ����߳̾�����ֱ�ӽ�����Դ�ͷ�
			//����������ǰ�ȫ�ģ�������Ҫ�ڷ��͹�������ʱ��ѯ��ǰ����״̬����ʱ�˳�
			sendSize = 0;
			sendSize = EvoTCPSend(g_tcp_connection_list.tcpConnList[tgtIndex]);
			if(sendSize == 0)
			{
				Sleep(1);
			}
			EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
			//ִ����ú���֮����ܴ��ڵ������û�з�����ϣ������������ƣ�����Ҫ��
			//��һ�ε��øú����м������͡�������Ҫע����ǣ��������û�з��͵�����
			//����Ҫ������������
			//�ߵ���һ��������һ������Ƿ��Ͷ˿��Ѿ�ʧЧ���Է��رգ��������ӽ����
			//��վ��ԭ������λ����ΪNULL�����Ѿ�����������ȡ������˺���������Ҫ��
			//����Ч��
			if(g_tcp_connection_list.tcpConnList[tgtIndex]!=NULL)
			{
				//�������֮ǰ���Ǹ����ӣ���˵��û����
				if(IdIsEqual(g_tcp_connection_list.tcpConnList[tgtIndex]->id , curSendingId))
				{
					g_tcp_connection_list.tcpConnList[tgtIndex]->sendingThread = INVALID_THREAD_ID;
					//����Ϊֹ������ȷ���ö˿ڲ����ڷ���״̬����λCONNECTION_STATE_SEND
					g_tcp_connection_list.tcpConnList[tgtIndex]->curState &= ~CONNECTION_STATE_SEND;
					g_tcp_sendthreadpool.sendThreadList[myid].threadState = TCP_THREAD_STATE_SCANING;

				}
			}
			LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
			//printf("�����߳� %d  -- ���������,�鿴��������\n" , myid);
		}
	}
    return NULL;
}

int TrytoInvokeTCPSendThread()
{
	//�鿴�����������������״̬�����������Ҫ���͵����񣬲��Ҵ��ڴ���
	//TCP_THREAD_STATE_IDLE״̬�ķ����̣߳�����
	int hasSleepThread = 0;
	int index = 0;

	for(index=0;index<TCP_SEND_THREAD_COUNT;index++)
	{
		if(g_tcp_sendthreadpool.sendThreadList[index].threadState == TCP_THREAD_STATE_IDLE)
		{
			hasSleepThread = 1;
			break;
		}
	}
	if(hasSleepThread)
#ifdef _WIN32
        ReleaseSemaphore(g_tcp_sendthreadpool.sendSema, 1, NULL);
#else
        sem_post(&g_tcp_sendthreadpool.sendSema);
#endif
	return 1;
}