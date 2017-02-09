#ifndef EVONETMONITOR_H
#define EVONETMONITOR_H

typedef struct EvoNetMonitor
{
#ifdef _WIN32
    HANDLE threadHandle;
    DWORD threadId;
#else
    pthread_t threadHandle;
#endif
	int isRunning;
}EvoNetMonitor;

int InitializeNetMonitor();

int FinalizeNetMonitor();

/*******************************************************************
���̵߳�����
1����������������ӷ����������ģ�
2�������Լ��������ӵ������������������ϵ����κ����ݣ�
������������գ������������ﵽ��ֵ֮�󣬽�����������Ϊ
INVALID�������ýڵ���뵽����վ�����ͺͶ�ȡ�̹߳۲쵽
ʧЧ��־֮�󣬷ֱ�λSEND��RECV״̬��Ȼ���˳������ӣ�
������Ӵ���SELECT״̬������λINVALID����Ҫ�����жϣ�
3�������Լ�����վ�и������ӵ�״̬�����SEND��RECV
���Ѹ�λ�����ͷ���Դ��
*******************************************************************/
#ifdef _WIN32
    DWORD WINAPI TCPNetMonitorRoutine(PVOID para);
#else
    void* TCPNetMonitorRoutine(void* para);
#endif

#endif