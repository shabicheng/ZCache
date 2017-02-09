/********************************************************************
EvoTCOSendThread.h
ɨ��ConnectionList�еķ������񣬲���ɷ���
����C������ɡ�

�������ԭ�������
1�����緢�Ͳ���˫�̣߳������߳���ѯConnectionList���ڻ�ȡһ��Connection֮�󣬻���䷢��
�����б����Ը�����������ԭ��ѡ�����б�Ĳ��֣���Ȼ��ִ�з��͡����͹����в���Ҫ��
ConnectionList���л������
2�������ź�����ɷ����̼߳��ͬ����
2.1�����ڷ����̣߳����ñ�־λ���ڱ�ʾ��ǰ�Ĺ���״̬���������ֹ���״̬�����͡�ɨ�衢���ߣ�
2.2���������񵽴�ʱ�����Ƚ���������ӦConnection������ConnectionList�еķ��������������
���������̣߳����Ҵ��������connection��ǰ�����ڷ���״̬���򼤻�����һ�������̣߳�����
��������Լ����������������ٽ�����
2.3���̼߳���֮��鿴connection�б��еķ������������ҵ����������connection����ȡ�䷢������
���޸�������֮��ִ�з�����������ɨ��Ĺ����Լ��޸��������Ĺ��̲����ٽ�����
2.4��ִ���귢������֮���̲߳鿴ConnectionList�еķ��������������Ϊ0����������ߣ�����ɨ
���б��ҵ����������connection����ȡ�䷢�����񣬲��޸�������֮��ִ�з�����������ɨ��
�Ĺ����Լ��޸��������Ĺ��̲����ٽ�����
3�������������ݷ��Ͷ�����������ɵ�Ӱ�졣�����ٽ����Ŀ��Ʒ�Χ��send�������������ٽ�����
4��ѡȡ��������Ҫ�չ˵�����ԭ��
********************************************************************/

#ifndef	EVOTCPSENDTHREAD_H
#define EVOTCPSENDTHREAD_H

#define TCP_SEND_THREAD_COUNT 2

#define MAX_SEND_INTERVAL 500

typedef struct TCPSendThreadInfo
{
#ifdef _WIN32
    HANDLE threadHandle;
    DWORD threadId;
#else
    pthread_t threadHandle;
#endif
	EvoTCPThreadState threadState;
	int isRunning;
}TCPSendThreadInfo;

typedef struct TCPSendThreadPool
{
	TCPSendThreadInfo sendThreadList[TCP_SEND_THREAD_COUNT];
	unsigned short sendThreadIdBase;	
#ifdef _WIN32
    HANDLE sendSema;
#else
    sem_t sendSema;
#endif

}TCPSendThreadPool;

int InitializeTCPSendThread();
int FinalizeTCPSendThread();

#ifdef _WIN32
DWORD WINAPI TCPSendThreadRoutine(PVOID para);
#else
void* TCPSendThreadRoutine(void* param);
#endif

int TrytoInvokeTCPSendThread();

#endif