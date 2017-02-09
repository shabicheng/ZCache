/********************************************************************
TCPConnectionList.h

与UDPOperator不同，TCPOperator以Connection为单位进行管理。一条连接可以
进行双向通信，可以进行发送和接收两种动作。建立Connection可以通过两种方
式：被动方式和主动方式。要建立被动方式连接，需要首先设立监听socket，然
后接受来自远端的连接；要建立主动方式连接，则需要向远端socket发起连接请
求。

每一条被动连接通过监听socket（地址+端口）与上层应用建立关联；
每一条主动连接通过远端监听socket（地址+端口）与上层应用建立关联；

采用C语言完成。
********************************************************************/
#ifndef CONNECTIONLIST_H
#define CONNECTIONLIST_H

#define ERR_CONNECTION_LIST_FULL -1
#define ERR_CONNECTION_LIST_FAIL -2
#define ERR_CONNECTION_LIST_NOTFOUND -3
#define ERR_CONNECTION_NODE_CANNOT_RELEASE -4
#define ERR_LISTENER_NODE_CANNOT_RELEASE -5

#define MAX_TCPCONNECTION_LIST_LENGTH 256
#define MAX_TCPLISTENER_LIST_LENGTH 16
//#define CONNECTION_HEARTBEAT_DEADLINE 5
#define CONNECTION_HEARTBEAT_DEADLINE 10
#define MAX_TCP_DUSTBIN_SIZE 32
#define DUST_DELETE_DEADLINE 5


typedef enum TCPListenerType
{
	LISTENER_BUILTIN , 
	LISTENER_USER , 
}TCPListenerType;

typedef enum TCPConnectionType
{
	CONNECTION_ACTIVE ,
	CONNECTION_PASSIVE ,
}TCPConnectionType;


/*****************************************************
每一个网络端口或者连接都用一个二维编号标识。
采用静态数组维护所有的端口，index是数组下标；
由于存在多个线程对端口进行互斥访问，可能存在实际访问对象
销毁的情况，也就是说，原有的index上很快分配了新的socket，
这时候就需要用利用id的比对确定是否是之前的端口。如果id
不同，则说明原有端口资源已经释放。
*****************************************************/
typedef struct TCPConnectionId
{
    u_int32_t index;
    u_int32_t id;
    void * idParam;
}TCPConnectionId;


#define INIT_CONNECTION_ID {0 , 0}
#define INIT_LISTENER_ID {0 , 0}


typedef struct TCPListenerNode
{
	SOCKET socket;
	TCPConnectionType type;

	//自身的id，会传递给所有由此生成的连接
	TCPConnectionId id;

	//监听的地址信息
	unsigned short int port;
	char* addr;

	//生成的所有连接的数量
    u_int32_t count;

	TCPListenerState state;

}TCPListenerNode;

typedef struct TCPListenerList
{
	CRITICAL_SECTION tcpListenerList_cs;
	TCPListenerNode* tcpListenerList[MAX_TCPLISTENER_LIST_LENGTH];
    u_int32_t listenerNum;
}TCPListenerList;

typedef struct TCPContext
{
    u_int32_t dataLength;
    u_int32_t curLength;
	unsigned short eleNum; //存在合包的情况下，该数据包内包含的上层数据包数量
	BYTE flags; 
	char* dataBuf;
}TCPContext;


typedef struct TCPConnectionNode
{
	//连接在本地对应的操作端口
	SOCKET socket;
	TCPConnectionType type;

	//自身的id
	TCPConnectionId id;
	//如果是被动连接，则为监听端口的id，
	//如果是主动连接，则为空
	TCPConnectionId parentId;

	//如果是主动连接则为对端监听端口的地址信息
	unsigned short int port;
	char* addr;

	//如果是TCP主动连接端，则为发送任务队列，否则为空
	//发送过成为两阶段：在没有被发送线程选中的状态下，
	//所有的发送任务都被存储在该列表中；一旦被发送线程
	//选中，并且curTasks列表为空，则将该列表中的任务转
	//入curTasks列表中；后续的发送任务则继续存储在该列
	//表中
	TCPWriteTaskList* taskList;
	//采用批量发送，每次将taskList中的任务转入curTask链表
	//然后循环发送，尽量降低发送过程与上层任务提交过程之间
	//的耦合程度
	TCPWriteTaskList* curTasks;

	TCPConnectionState curState;

	unsigned char priority; 

	//无论发送还是接收，均采用可中断模式
	//每次开始一个独立数据报文的接收时都暂时将报文存放在
	//上下文环境中，只有在接收完全的情况下才将数据复制出
	//来。由此一来，如果一包数据没有接收完毕，则已经完成
	//接收的部分存放在上下文结构中，下一次接收过程可以继
	//续接收剩余的数据
	TCPContext recvContext;

	//每次开始一个独立数据报文的发送时都首先将报文存复制到
	//上下文环境中，只有在发送完全的情况下才将数据进行删除
	//由此一来，如果一包数据没有发送完毕，则剩余需要进行发
	//送的部分数据存放在上下文结构中，下一次发送过程可以继
	//续发送剩余的数据
	TCPContext sendContext;

	//由netmonitor周期性增加，并在接收到任意网络报文的时候
	//复位。如果超过DUST_DELETE_DEADLINE，则设置该连接
	//状态为INVALID
	unsigned char heartbeat;

	//当前正在该端口进行发送的线程编号，如果没有线程正在操作，则为INVALID_THREAD_ID
	EvoThreadId sendingThread; 
	//当前正在该端口进行接收的线程编号，如果没有线程正在操作，则为INVALID_THREAD_ID
	EvoThreadId recvingThread;

	clock_t lastSendTime; 

	//表示该连接是否出现了发送阻塞
	unsigned short sendBlock;

    u_int32_t incomingPacketNum;
    u_int32_t outcomingPacketNum;

}TCPConnectionNode;

/*****************************************************
管理所有的连接，以及连接上的发送任务。
设计原则：
1）尽量减少互斥机制的使用，减少系统资源的申请和操作。
2）减少线程数量，将数据的读取与select侦测过程合并，数据
量大的情况下，增加读取线程数的效果有限。
3）发送和接收线程均固定为两个，接收线程与连接对应关系固
定，发送线程争抢发送任务。

【连接建立处理】
这里仅仅针对整个连接队列进行了互斥管理，如果要插入任何
一个新建的连接或者针对任何一个连接插入数据，都需要进行
临界区互斥。
【数据发送处理】
发送线程为了获取发送任务列表同样需要进行临界区互斥，然
后获取任何一个连接上的任务列表后离开临界区，进行数据发
送。
【数据接收处理】
接收线程通过临界区获取当前的所有有效连接端口，并监听数
据。如果有数据到达，需要再次进入临界区，修改连接的心跳数
需要注意的是，由于读取端口的线程固定，因此不存在读取线程
之间的互斥问题，连接工作状态仅提供给发送线程参考。
【连接失效处理】
1）数据发送过程中出现异常，则需要设置相应的连接失效。发送
线程一方面抛弃所有未发送的数据，另一方面则通过TCPConnectionId
设置相应连接失效。在此过程中需要进行临界区互斥。当然，
由于连接的状态是CONNECTION_STATE_OCCUPY，并且对该状态的设
置同样需要临界区互斥，因此不需要担心其他发送线程对该连接的
操作。
2）数据接收过程中出现异常，则需要设置相应的连接失效。
接收线程通过TCPConnectionId设置相应连接失效。在此过程中需
要进行临界区互斥。可能存在的一种可能是发送线程已经设置失
效，不过不会产生不良影响。
3）网络监控线程通过心跳计数发现连接失效，由于在考察心跳的
过程中已经进入临界区，因此可以直接设置连接状态。
【连接删除处理】
由网络监控线程统一处理。当网络监控线程发现连接失效后，将其
TCPConnectionNode结构插入到垃圾队列中，然后离开。并在空闲阶
段统一删除和释放垃圾队列中的资源。需要注意的是，监控线程需
要根据连接实际的状态完成动作。
1）如果连接正处于读取或者发送状态，且没有关闭，则首先关闭
连接并退出，则操作线程应该可以获取错误，并退出操作；
2）只有当SEND和RECV两个状态都复位之后，才能将结构插入垃圾
队列。
*****************************************************/
typedef struct TCPConnectionList
{
	CRITICAL_SECTION tcpConnList_cs;
	TCPConnectionNode* tcpConnList[MAX_TCPCONNECTION_LIST_LENGTH];
    u_int32_t connNum;
    u_int32_t send_task_count;
}TCPConnectionList;

/*****************************************************
垃圾回收机制相关结构
在以下三种情况下会将connectionNode加入到回收站
1）发送出错；2）接收出错；3）心跳异常；
在以上情况下，connectionNode分别对CONNECTION_STATE_SENDERR
CONNECTION_STATE_RECVERR以及CONNECTION_STATE_INVALID置位
NetMonitor周期性查看connectionNode，将其中
满足要求的节点放入回收站。并在一定周期之后销毁
*****************************************************/
typedef enum EvoDustType
{
	DUST_CONNECTION , 
	DUST_LISTENER , 
}EvoDustType;

typedef struct EvoDustNode
{
	EvoDustType type;
	void* node;
	unsigned short deadLine;
}EvoDustNode;

typedef struct EvoTCPDustbin
{
	CRITICAL_SECTION tcpDustbin_cs;
	EvoDustNode dustList[MAX_TCP_DUSTBIN_SIZE];
}EvoTCPDustbin;




/*****************************************************
本模块对应的函数
*****************************************************/
//对两个链表进行初始化
void InitializeTCPList();
//释放所有的资源，包括网络资源和内存资源
void FinalizeTCPList();

//模块外部接口，被应用接口模块的RegisterListener调用。在指定的端
//口号上建立一个连接监听端口，并且在本模块的管理链表中增加相应的
//节点。接收线程可以将该端口装载并进行select。
//该函数中进行实际的端口初始化、数据结构的生成、管理链表的维护
//等工作。如果成功，则分配并返回有效的网络标识；否则返回INVALID_LISTENER。
TCPConnectionId InsertListener(const char* addr , unsigned short port);

//模块外部接口，被应用接口模块的UnRegisterListener调用。注销一个
//之前注册的连接监听端口，在链表中查找对应的管理节点，将其设置
//为失效，资源的释放由资源管理模块完成如果成功则返回1，否则返回-1
int RemoveListener(TCPConnectionId listenerID);

//模块外部接口，初始化阶段被应用接口模块EvoTCPInitialize调用。注册
//系统默认的连接监听端口。该连接监听端口负责为每一个接收线程建立
//自中断管理连接。此外，上层应用也可以利用该默认端口建立TCP连接。
//该函数中进行实际的端口初始化、数据结构的生成、管理链表的维护等
//工作。如果成功，则分配并返回有效的网络标识；否则返回INVALID_LISTENER。
//需要注意的是，该端口的类型设置为LISTENER_BUILTIN，并且保证具有唯一性。
TCPConnectionId InsertInteruptListener(const char* addr , unsigned short port);


//模块外部接口，由应用接口模块的的RegisterConnection调用，向一个指
//定的端口发起TCP连接，并建立相应的管理节点。
//该函数中进行实际的端口初始化、通过调用connect建立连接、数据结构的
//生成、TCP连接管理链表的维护等工作。如果成功，则分配并返回有效的网
//络标识；否则返回INVALID_CONNECTION。
TCPConnectionId InsertActiveTCPConnection(const char* addr , unsigned short port);

//模块外部接口，由数据接收线程在获取外部连接建立请求（对端调用connect）
//建立新的TCP连接之后调用，建立相应的管理节点。
//连接的建立工作在数据接收线程的执行函数中实现，该函数中负责数据结构的
//生成、TCP连接管理链表的维护等工作。如果成功，则分配并返回有效的网络标识；
//否则返回INVALID_CONNECTION。
TCPConnectionId InsertPassiveTCPConnection(const char* addr , unsigned short port, TCPConnectionId parentId , SOCKET socket);

//模块外部接口，由应用接口模块的UnRegisterConnection调用，在链表中查找对应的
//管理节点，将其设置为失效，资源的释放由资源管理模块完成。
int RemoveConnection(TCPConnectionId connId);

//根据输入删除对应的连接，并且释放所有的资源，包括网络和内存资源
//这里需要注意的是，链接可能的状态是发送或者接收。对于工作状态的端口
//需要等待这些端口进入非工作状态，然后释放其资源。
//为了支撑以上要求，所有端口均设置为非阻塞的工作模式，并且在工作过程
//中不断查询自身的工作状态，如果CONNECTION_STATE_INVALID被设置，
//则退出发送或者接收状态。当SEND和RECV状态均被复位后，开始释放相关资源。
//此外还需要考虑接收线程的工作状态，如果接收线程处于TCP_THREAD_STATE_SELECT
//状态，则需要通过中断端口唤醒。
int ReleaseTCPConnectionNode(TCPConnectionNode* node);
int ReleaseTCPListenerNode(TCPListenerNode* node);

void CleanupTCPConnectionList();
void CleanupTCPListenerList();




/*****************************************************
NetMonitor模块调用的函数
*****************************************************/
//向所有的连接发送心跳报文
//由于数据报文对于判定网络存活状态与心跳报文等同
//因此需要首先判断ConnectionNode的任务队列中是否
//存在任务，只有在任务队列为空的情况下，才需要发送
//心跳报文
//检查各条连接的心跳，如果超出阈值则设置为INVALID
//并将其移入回收站
int TestResourceState();

int InsertConnIntoDustbin(TCPConnectionNode* node);

int InsertListenerIntoDustbin(TCPListenerNode* node);

//查看回收站中的连接，如果DeadLine达到阈值，则释放资源
int CleanDustbin();

//释放回收站里所有的资源
int FinalizeDustbin();



#endif