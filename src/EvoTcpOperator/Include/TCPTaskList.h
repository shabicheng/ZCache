/********************************************************************
TaskList.h
�����γ������д�����б�
����C������ɡ�
********************************************************************/

#ifndef TCP_TASKLIST_H
#define TCP_TASKLIST_H

#define MAX_TCP_WRITE_TASKLIST_LENGTH 1024


#define ERR_TCP_WRITE_TASKLIST_FULL -1
#define ERR_TCP_WRITE_TASKLIST_NOMEM -2
#define ALERT_TCP_WRITE_TASKLIST_FULL -3

/*
��Ҫ��ʾ��Ҫִ�е����緢������
*/
typedef struct TCPNetworkTask
{
	char* buf;
    u_int32_t buflength;
	EvoTCPMessageKind type;
	unsigned short eleNum; //���ںϰ�������£������ݰ��ڰ������ϲ����ݰ�����
	BYTE flags;
	struct TCPNetworkTask* next;
}TCPNetworkTask;

typedef struct TCPWriteTaskList
{
	//���г���
	unsigned short count;

	unsigned  int size;

	TCPNetworkTask* task_head;

	TCPNetworkTask* task_tail;

}TCPWriteTaskList;

TCPWriteTaskList* ConstructTCPWriteTaskList();

void ReleaseTCPTaskList(TCPWriteTaskList* list);

/********************************************************************
���˼·��飺
�ú����������Ҫ����������������أ�1���������ĵ�ȡ�᣻
2��С�����ݵĺϰ����ͣ�
���ڵ�һ�㣬���ԭ���Ǿ����������������ģ���ˣ�ֻҪ��
���д������ݱ�������Խ����������е���������ɾ��������
�����������档1������������Ľ�������е�ʱ���ǵ�һ��Ԫ
�أ����������ɾ�����������뵽�����У�2��������ݱ��Ĳ���
�������е�ʱ����֮ǰ�����������ģ���ɾ���������ģ�����ζ
�ţ���������д����������ģ���һ����Ψһ�ı��ġ�
���ڵڶ��㣬���ԭ���Ǿ����ٽ����ڴ��������ˣ�ֻ
Ҫ�ǳ�������Ƭ�γߴ�����ݣ�������Ҫ�ϰ�������Ҫ�ϰ�������
��ֱ�ӷ��䷢��Ƭ�γߴ�ġ���ռ䡱��������С���ݰ�ֱ�Ӽ��뵽
�����ռ��С�
********************************************************************/
int InsertTCPWriteTaskToList(TCPWriteTaskList* list , const char* buf , u_int32_t buflength);

int InsertTCPHeartbeatToList(TCPWriteTaskList* list );

void  SwitchTaskList(TCPWriteTaskList* tgtList , TCPWriteTaskList* srcList);


#endif