/********************************************************************
TaskList.h
用于形成网络读写任务列表
采用C语言完成。
********************************************************************/

#ifndef TCP_TASKLIST_H
#define TCP_TASKLIST_H

#define MAX_TCP_WRITE_TASKLIST_LENGTH 1024


#define ERR_TCP_WRITE_TASKLIST_FULL -1
#define ERR_TCP_WRITE_TASKLIST_NOMEM -2
#define ALERT_TCP_WRITE_TASKLIST_FULL -3

/*
主要表示需要执行的网络发送任务。
*/
typedef struct TCPNetworkTask
{
	char* buf;
    u_int32_t buflength;
	EvoTCPMessageKind type;
	unsigned short eleNum; //存在合包的情况下，该数据包内包含的上层数据包数量
	BYTE flags;
	struct TCPNetworkTask* next;
}TCPNetworkTask;

typedef struct TCPWriteTaskList
{
	//队列长度
	unsigned short count;

	unsigned  int size;

	TCPNetworkTask* task_head;

	TCPNetworkTask* task_tail;

}TCPWriteTaskList;

TCPWriteTaskList* ConstructTCPWriteTaskList();

void ReleaseTCPTaskList(TCPWriteTaskList* list);

/********************************************************************
设计思路简介：
该函数的设计需要考虑两个方面的因素，1、心跳报文的取舍；
2、小包数据的合包发送；
对于第一点，设计原则是尽量不发送心跳报文，因此，只要队
列中存在数据报文则可以将队列中所有的心跳报文删除。这体
现在两个方面。1）如果心跳报文进入队列中的时候不是第一个元
素，则可以立刻删除，而不插入到队列中；2）如果数据报文插入
到队列中的时候发现之前存在心跳报文，则删除心跳报文；这意味
着，如果队列中存在心跳报文，则一定是唯一的报文。
对于第二点，设计原则是尽量少进行内存分配活动，因此，只
要是超过发送片段尺寸的数据，都不需要合包；而需要合包的数据
则直接分配发送片段尺寸的“大空间”，后续的小数据包直接加入到
这个大空间中。
********************************************************************/
int InsertTCPWriteTaskToList(TCPWriteTaskList* list , const char* buf , u_int32_t buflength);

int InsertTCPHeartbeatToList(TCPWriteTaskList* list );

void  SwitchTaskList(TCPWriteTaskList* tgtList , TCPWriteTaskList* srcList);


#endif