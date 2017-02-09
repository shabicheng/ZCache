#ifndef EVOTCPOPERATOR_H
#define EVOTCPOPERATOR_H

/********************************************************************
EvoTCPOperator.h
���ڴ��������뵥��TCP��صĲ�����
����C������ɡ�
********************************************************************/

//����TCPģ�����幤��״̬
typedef enum TCPNetLayerWorkState
{
	STATE_NOT_INITIALIZED , 

	STATE_HAVE_INITIALIZED ,

}TCPNetLayerWorkState;

typedef enum CallbackFunState
{
	STATE_CALLBACK_INUSE ,
	STATE_CALLBACK_IDLE , 
}CallbackFunState;

typedef enum EvoTCPDataPriority
{
	TCP_DATA_PRIORITY_NORMAL , 
	TCP_DATA_PRIORITY_HIGH , 
	TCP_DATA_PRIORITY_URGENT , 
}EvoTCPDataPriority;

extern TCPNetLayerWorkState g_TCPNetLayerState;

//��ģ����ܴ��ڵ���ȷ����ֵ
#define TCP_RECVTHREADINIT_OK 1
#define TCP_SENDTHREADINIT_OK 1
//��ģ����ܴ��ڵĴ����б�
#define ERR_TCP_UNSUPPORTED -1
#define ERR_NET_CANNOT_INIT -1
#define ERR_BUILTIN_LISTENER_EXISTED -2
#define ERR_BUILTIN_LISTENER_CANNOT_CONNECT -3
#define ERR_BUILTIN_LISTENER_CANNOT_ACCEPT -4
#define ERR_SENDDATA_CONNECTION_NOT_EXIST -5
#define ERR_SENDDATA_CONNECTION_INVALID -6
#define ERR_SENDDATA_SYSTEM_STATE_CONFLICT -7 //ϵͳ�ڲ����ִ��������ȱ�����
#define ERR_LISTENER_NOT_EXIST -8
#define ERR_CONNECTION_NOT_EXIST -9
#define ERR_CALLBACKFUN_INUSE -10


#ifdef _WIN32
#ifdef EvoTCPOperatorLibAPI
#else
#define EvoTCPOperatorLibAPI extern "C" __declspec(dllimport)
#endif
#else
#ifdef EvoTCPOperatorLibAPI
#else
#define EvoTCPOperatorLibAPI extern "C"
#endif
#endif

//�ϲ����ݽ��պ���ԭ��
//���ݽ����̴߳�����˿ڶ�ȡ����������֮��ͨ������
//�ú�����������ύ���˹����н��������ָ��Ĵ��ݣ�
//����ɻص�֮��ϵͳ����������Դ������ϲ�Ӧ�����
//��Ҫ�ڻص������������Դ�ĸ���
typedef int (* EvoTCPNetworkEventHook)(EvoTCPNetworkEvent netEvent);

EvoTCPOperatorLibAPI BOOL IdIsEqual(TCPConnectionId leftId , TCPConnectionId rightId);

EvoTCPOperatorLibAPI BOOL IdIsValid(TCPConnectionId leftId);

EvoTCPOperatorLibAPI int RegisterTCPEventHook(EvoTCPNetworkEventHook hookFunc);

EvoTCPOperatorLibAPI int EvoTCPInitialize();

EvoTCPOperatorLibAPI int SetConnectionParam(TCPConnectionId id);

EvoTCPOperatorLibAPI TCPConnectionId RegisterListener(const char* addr , unsigned short port);

EvoTCPOperatorLibAPI TCPConnectionId RegisterConnection(const char* addr , unsigned short port);

EvoTCPOperatorLibAPI int UnRegisterListener(TCPConnectionId listenerID);

EvoTCPOperatorLibAPI int UnRegisterConnection(TCPConnectionId connID);

EvoTCPOperatorLibAPI int SendData(TCPConnectionId connectionId , const char* data , u_int32_t length);

EvoTCPOperatorLibAPI int SendDataWithPriority(TCPConnectionId connectionId , 
																								  char* data ,
                                                                                                  u_int32_t length ,
																								  EvoTCPDataPriority priority);

EvoTCPOperatorLibAPI int EvoTCPFinalize();


#endif