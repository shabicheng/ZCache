/********************************************************************
EvoTCPRecvThread.h

�����̸߳�������������ӵĽ������������е����ӽ��м���������ȡ���ݡ�

���ԭ��
1�����ݵĶ�ȡ���̾�����֤��ƽ�ԣ��������ʱ��Ϊĳ���˿ڷ���
2�����ÿ��жϷ�ʽ�����ν��ղ�������ݴ���������Ļ����У�

����C������ɡ�
********************************************************************/
#ifndef EVOTCPRECVTHREAD_H
#define EVOTCPRECVTHREAD_H

#define TCP_RECV_THREAD_COUNT 2


typedef struct TCPRecvThreadInfo
{
#ifdef _WIN32
    HANDLE threadHandle;
    DWORD threadId;
#else
    pthread_t threadHandle;
#endif
	SOCKET selfinterupt_send;
	SOCKET selfinterupt_recv;
	EvoTCPThreadState threadState;
    u_int32_t recvRound;
	unsigned short selectNum;
	int isRunning;
}TCPRecvThreadInfo;

typedef struct TCPRecvThreadPool
{
	TCPRecvThreadInfo recvThreadList[TCP_RECV_THREAD_COUNT];
	unsigned short recvThreadIdBase;	
}TCPRecvThreadPool;


int InitializeTCPRecvThread();
int FinalizeTCPRecvThread();

#ifdef _WIN32
DWORD WINAPI TCPRecvThreadRoutine(PVOID para);
#else
void* TCPRecvThreadRoutine(PVOID para);
#endif

//ģ���ⲿ�ӿڣ������ӹ���ģ�顢��Դ����ģ������ӻ����Ƴ�
//TCPListener����TCPConnectionʱ���ã������ж϶�Ӧ�ֳɵ�select��
//������װ��֮������ִ�С�
//���룺��Ҫ�жϵ��̱߳�ţ�
//�����int
//�������ж϶˿ڷ������жϱ��ģ�����ɹ��򷵻�1�����򷵻�-1
//��Ҫ���ǳ���֮��Ĵ���������ζ�����ж϶˿ڳ�������
int TrytoInteruptTCPRecvThread(int threadIndex);


//ͨ�����º����������¼����ݸ��ϲ�
//���������󵽴���ղ�����InsertPassiveTCPConnection��
//���øú�����֪ͨ�ϲ�Ӧ�á��ϲ�Ӧ�ÿ���ͨ���˺����д��ݵ�
//ConnectionId�ж�������������ݣ������ø�id��������
int AcceptConnection(TCPConnectionId listenerId , TCPConnectionId connId , char* addr, unsigned short port);

//��ȡ��������֮�󣬵��øú��������ݴ��ݸ��ϲ�
int RecvData(TCPConnectionId connId, unsigned short port, TCPNetworkTask* dataList);

#endif