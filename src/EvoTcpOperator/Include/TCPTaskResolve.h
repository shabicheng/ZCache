#ifndef TCPTASKRESOLVE_H
#define TCPTASKRESOLVE_H

#define TCP_BUFFERSIZE 128*1024			//�˿ڻ����ģ
#define TCP_MAX_RECV_COUNT 16			//���һ���Զ�ȡ����
#define TCP_MAX_RECV_SIZE 1024*1024	//���һ���Զ�ȡ������
#define TCP_MAX_SEND_COUNT 16			//���һ���Զ�ȡ����
#define TCP_MAX_SEND_SIZE 1024*1024	//���һ���Զ�ȡ������

SOCKET InitTCPListener(const char* addr , unsigned short port);

SOCKET InitActiveConnection(const char* addr , unsigned short port);

int SetConnectionSockOPT(SOCKET socket);

//��һ���˿ڽ���������ȡ�������ȡ�Ĺ����з����жϣ������ȡ�������ƻ�������
//�򽫵�ǰ���ڶ�ȡ�ĳ����洢��TCPConnectionNode�е�recvContext��
TCPNetworkTask* EvoTCPRecv(TCPConnectionNode* connectionNode);

//��һ���˿ڽ����������ݷ��ͣ�������͹����з����жϣ����緢���������ƻ�������
//�򽫵�ǽ���ڷ��͵ĳ����洢��TCPConnectionNode�е�sendContext��
u_int32_t EvoTCPSend(TCPConnectionNode* connectionNode);

int EvoTCPRecvInterupt(SOCKET socket);

#endif