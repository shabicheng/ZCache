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
extern TCPConnectionList g_tcp_connection_list;

extern TCPListenerList g_tcp_listener_list;

extern u_int32_t g_id_base;

u_int32_t testCounter = 0;

/*****************************************************
TCPOperator模块的全局变量
*****************************************************/
//接收线程初始化中完成对每个线程中断端口的初始化，均使用该监听端口
//TCPOperator中模块初始化函数保证进行中断端口初始化之前，已经在该
//端口进行监听
extern u_int32_t g_tcp_self_interupt_port;

extern TCPNetLayerWorkState g_TCPNetLayerState;

extern DWORD g_cpu_number;

/*****************************************************
本模块的全局变量
*****************************************************/
//需要进行初始化
TCPSendThreadPool g_tcp_sendthreadpool;


/*****************************************************
本模块的函数
*****************************************************/

int InitializeTCPSendThread()
{
	int index; //用来遍历所有的connection

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

		//如果获取了CPU信息，则尝试指派内核
        if(g_cpu_number == 8) //往往是i7处理器，四核八线程
		{
            //发送线程通常状况下是两个
			SetThreadIdealProcessor(g_tcp_sendthreadpool.sendThreadList[index].threadHandle , index%2+2);
		}
		else if(g_cpu_number == 4) //i5双核四线程
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
	TCPConnectionId curSendingId; //当前正在执行发送任务的连接id
	int tgtIndex = 0; //用来记录发送任务最多的connection的下标
    u_int32_t taskCount = 0; //用来记录当前最多任务数的端口上的任务数量
	
	//用于发送端流量控制，暂时不启用流量控制机制
	//记录本线程连续发生阻塞的次数，如果超过一定限度，则需要进行挂起
	//这里并没有针对特定连接进行流量控制的设计
	unsigned short sendBlockTimes = 0;
	//记录是否在本轮中发现阻塞线程
	unsigned char findBlockingConnection = 0;

	//一次发送循环中已经发送的尺寸和数据包个数
	//如果超出了限额，则推出本次发送循环，避免单一
	//连接过长时间占用发送线程
    u_int32_t sendSize = 0;
    //u_int32_t lastCount = 0;
    //unsigned short sleepInterval = 100;

	EvoThreadId myid = g_tcp_sendthreadpool.sendThreadIdBase++;

	g_tcp_sendthreadpool.sendThreadList[myid].isRunning = 1;

	while (g_tcp_sendthreadpool.sendThreadList[myid].isRunning)
	{
		//等待被唤醒，由于问题一的存在，发送线程可能被唤醒之后得不到发送任务
		//该问题会导致发送线程空转一个循环，不会影响正确性
#ifdef _WIN32
        WaitForSingleObject(g_tcp_sendthreadpool.sendSema, INFINITE); //等待被唤醒
#else
        sem_wait(&g_tcp_sendthreadpool.sendSema);
#endif
        //printf("send data...\n");
		while(1) //进入本次发送循环
		{
			findBlockingConnection = 0;
			//printf("发送线程 %d  -- 进入工作状态\n" , myid);
			//线程被唤醒，说明存在需要发送的任务
			EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
			g_tcp_sendthreadpool.sendThreadList[myid].threadState = TCP_THREAD_STATE_SCANING;
			//开始扫描connectionList，查询需要发送的任务
			//添加任务的时候需要保证，只有在添加任务的端口不处于SEND状态，并且
			//存在休眠任务的情况下，才唤醒线程
			//需要注意的是，可能存在如下情况：任务列表中没有任务，但是在上下文环
			//境中仍然存在没有发送完毕的数据，因此，需要将上下文环境同样作为一个
			//发送任务
			//另一方面，在等待发送队列curTask中同样存在一定数量的发送任务
			/******************************* 存在的问题*********************************
			问题1：
			该模块存在如下问题：连接状态为SEND，线程为INVALID，始终查不出来
			是在什么阶段，有哪一个线程将状态修改为SEND。尤其在任务队列满的时
			候，该问题会导致两个发送线程同时进入睡眠状态。现在的解决方法是同
			时判断连接的线程id，如果是INVALID则需要唤醒沉睡线程。2014-8-9
			同样的问题也存在于数据接收线程中，将会导致停止接收数据
			******************************************************************************/
			tgtIndex = 0;
			taskCount = 0;
			for(index=0;index<MAX_TCPCONNECTION_LIST_LENGTH;index++)
			{

				if(g_tcp_connection_list.tcpConnList[index]!=NULL)
				{
					//如果该连接正处于发送状态或者出错状态，则不考虑发送
					//由于存在问题1，因此这里增加了最后一个关于线程id的检查
					//如果上一轮次中该连接发送阻塞，在本轮不做发送处理
					if((g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_SENDERR)||
						(g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_INVALID) ||
						(g_tcp_connection_list.tcpConnList[index]->sendBlock == 1)) 
					{
						if(g_tcp_connection_list.tcpConnList[index]->sendBlock == 1
							&&(g_tcp_connection_list.tcpConnList[index]->taskList->count>0
							|| g_tcp_connection_list.tcpConnList[index]->sendContext.dataLength>0
							|| g_tcp_connection_list.tcpConnList[index]->curTasks->count>0))
						{	
							//存在阻塞连接，并且连接上有数据，则本次不考虑发送，经过一个循环之后
							//再考虑发送
							findBlockingConnection = 1; 
							//休息一轮，下一轮中根据数据量以及紧急程度发送
							g_tcp_connection_list.tcpConnList[index]->sendBlock  = 0;
						}
						//本连接不符合发送要求，考虑下一连接
						continue;
					}

					if(!(g_tcp_connection_list.tcpConnList[index]->curState & CONNECTION_STATE_SEND)||
						g_tcp_connection_list.tcpConnList[index]->sendingThread == INVALID_THREAD_ID)
					{
						//该连接可以执行发送任务并且有任务
						//对于所有可以执行发送任务的连接，分别从任务数量和时间紧迫性两个方面
						//在时间允许的范围内，选择任务数量最多的。如果时间超过限度，则立即发送
						if(g_tcp_connection_list.tcpConnList[index]->taskList->count>0
							|| g_tcp_connection_list.tcpConnList[index]->sendContext.dataLength>0
							|| g_tcp_connection_list.tcpConnList[index]->curTasks->count>0)
						{
							//查看上次处理时间
							clock_t tmpclock = clock();
							unsigned short tmpcount = g_tcp_connection_list.tcpConnList[index]->taskList->count;
							if(g_tcp_connection_list.tcpConnList[index]->sendContext.dataLength>0)
								tmpcount += 1;
							if(g_tcp_connection_list.tcpConnList[index]->curTasks->count>0)
								tmpcount += g_tcp_connection_list.tcpConnList[index]->curTasks->count;

							if((tmpclock - g_tcp_connection_list.tcpConnList[index]->lastSendTime) > MAX_SEND_INTERVAL) 
								//超出最大间隔时间，需要立刻处理，哪怕只有一个报文
							{
								tgtIndex = index;
								taskCount = tmpcount;
								g_tcp_connection_list.tcpConnList[index]->lastSendTime = tmpclock;
								break;
							}

							//找到任务最多的连接
							if(tmpcount> taskCount)
							{
								tgtIndex = index;
								taskCount = tmpcount;
							}
						}
					}//判定连接状态
				}//考察一个连接
			}
			//选定发送连接，并标定工作状态
			if(taskCount>0)
			{
				//printf("线程 %d 获取连接 %d 任务数量 %d \n" , myid , tgtIndex , taskCount);
				if(g_tcp_connection_list.tcpConnList[tgtIndex]->sendContext.dataLength == 0 &&
					g_tcp_connection_list.tcpConnList[tgtIndex]->curTasks->count == 0 &&
					g_tcp_connection_list.tcpConnList[tgtIndex]->taskList->count > 0 )
				{
					//如果当前任务链表为空，则进行转换，否则继续发送当前任务链表
					//这里存在改进空间，可以使发送更加均衡
					SwitchTaskList(g_tcp_connection_list.tcpConnList[tgtIndex]->curTasks , 
						g_tcp_connection_list.tcpConnList[tgtIndex]->taskList);
				}
				//有发送任务，阻塞指数下降
				if(sendBlockTimes>0)
					sendBlockTimes--;
			}
			else
			{
				g_tcp_sendthreadpool.sendThreadList[myid].threadState = TCP_THREAD_STATE_IDLE;
				LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
				if(findBlockingConnection == 1) //存在数据，但是连接发生过阻塞
				{
					//如果是因为阻塞导致空循环，则可以考虑挂起该线程
					//挂起不会改善本线程的发送，但是可以把处理资源给其它线程
					sendBlockTimes++;
					//这里可能存在一个情况是队列中仍然存在尚未发送的数据
					//但是仍然在发送其中部分数据的时候发生了拥塞，在这种
					//情况下不应该离开内层循环
					Sleep(0);
					//printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					continue;
				}
				else
				{
					break; //跳到最外层，重新申请信号量
				}
			}
			//离开临界区，这里需要保证发送过程中不会出现意外
			//意外主要是指连接的失效。失效的连接不应该导致数据结构的异常
			//连接的失效可能来自接收线程、监控线程以及上层的注销操作
			//发送的过程中需要不断检查连接自身的状态，如果存在任何异常，则退出发送
			g_tcp_connection_list.tcpConnList[tgtIndex]->curState |= CONNECTION_STATE_SEND;
			g_tcp_connection_list.tcpConnList[tgtIndex]->sendingThread = myid;
			curSendingId = g_tcp_connection_list.tcpConnList[tgtIndex]->id;
			LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
			//在临界区以外执行发送任务，无论发送还是接收线程均不会直接进行资源释放
			//因此这样做是安全的，但是需要在发送过程中随时查询当前连接状态，及时退出
			sendSize = 0;
			sendSize = EvoTCPSend(g_tcp_connection_list.tcpConnList[tgtIndex]);
			if(sendSize == 0)
			{
				Sleep(1);
			}
			EnterCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
			//执行完该函数之后可能存在的情况是没有发送完毕（到达流量限制）。需要在
			//下一次调用该函数中继续发送。这里需要注意的是，如果存在没有发送的任务
			//则不需要插入心跳报文
			//走到这一步，另外一种情况是发送端口已经失效（对方关闭），该连接进入回
			//收站，原有连接位置上为NULL或者已经被其它连接取代，因此后续操作需要判
			//定有效性
			if(g_tcp_connection_list.tcpConnList[tgtIndex]!=NULL)
			{
				//如果还是之前的那个连接，则说明没问题
				if(IdIsEqual(g_tcp_connection_list.tcpConnList[tgtIndex]->id , curSendingId))
				{
					g_tcp_connection_list.tcpConnList[tgtIndex]->sendingThread = INVALID_THREAD_ID;
					//到此为止，可以确定该端口不处于发送状态，复位CONNECTION_STATE_SEND
					g_tcp_connection_list.tcpConnList[tgtIndex]->curState &= ~CONNECTION_STATE_SEND;
					g_tcp_sendthreadpool.sendThreadList[myid].threadState = TCP_THREAD_STATE_SCANING;

				}
			}
			LeaveCriticalSection(&g_tcp_connection_list.tcpConnList_cs);
			//printf("发送线程 %d  -- 任务发送完毕,查看其它任务\n" , myid);
		}
	}
    return NULL;
}

int TrytoInvokeTCPSendThread()
{
	//查看各个连接任务链表的状态，如果存在需要发送的任务，并且存在处于
	//TCP_THREAD_STATE_IDLE状态的发送线程，则唤醒
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