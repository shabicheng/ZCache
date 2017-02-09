/********************************************************************
TCPConnectionList.h

��UDPOperator��ͬ��TCPOperator��ConnectionΪ��λ���й���һ�����ӿ���
����˫��ͨ�ţ����Խ��з��ͺͽ������ֶ���������Connection����ͨ�����ַ�
ʽ��������ʽ��������ʽ��Ҫ����������ʽ���ӣ���Ҫ������������socket��Ȼ
���������Զ�˵����ӣ�Ҫ����������ʽ���ӣ�����Ҫ��Զ��socket����������
��

ÿһ����������ͨ������socket����ַ+�˿ڣ����ϲ�Ӧ�ý���������
ÿһ����������ͨ��Զ�˼���socket����ַ+�˿ڣ����ϲ�Ӧ�ý���������

����C������ɡ�
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
ÿһ������˿ڻ������Ӷ���һ����ά��ű�ʶ��
���þ�̬����ά�����еĶ˿ڣ�index�������±ꣻ
���ڴ��ڶ���̶߳Զ˿ڽ��л�����ʣ����ܴ���ʵ�ʷ��ʶ���
���ٵ������Ҳ����˵��ԭ�е�index�Ϻܿ�������µ�socket��
��ʱ�����Ҫ������id�ıȶ�ȷ���Ƿ���֮ǰ�Ķ˿ڡ����id
��ͬ����˵��ԭ�ж˿���Դ�Ѿ��ͷš�
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

	//�����id���ᴫ�ݸ������ɴ����ɵ�����
	TCPConnectionId id;

	//�����ĵ�ַ��Ϣ
	unsigned short int port;
	char* addr;

	//���ɵ��������ӵ�����
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
	unsigned short eleNum; //���ںϰ�������£������ݰ��ڰ������ϲ����ݰ�����
	BYTE flags; 
	char* dataBuf;
}TCPContext;


typedef struct TCPConnectionNode
{
	//�����ڱ��ض�Ӧ�Ĳ����˿�
	SOCKET socket;
	TCPConnectionType type;

	//�����id
	TCPConnectionId id;
	//����Ǳ������ӣ���Ϊ�����˿ڵ�id��
	//������������ӣ���Ϊ��
	TCPConnectionId parentId;

	//���������������Ϊ�Զ˼����˿ڵĵ�ַ��Ϣ
	unsigned short int port;
	char* addr;

	//�����TCP�������Ӷˣ���Ϊ����������У�����Ϊ��
	//���͹���Ϊ���׶Σ���û�б������߳�ѡ�е�״̬�£�
	//���еķ������񶼱��洢�ڸ��б��У�һ���������߳�
	//ѡ�У�����curTasks�б�Ϊ�գ��򽫸��б��е�����ת
	//��curTasks�б��У������ķ�������������洢�ڸ���
	//����
	TCPWriteTaskList* taskList;
	//�����������ͣ�ÿ�ν�taskList�е�����ת��curTask����
	//Ȼ��ѭ�����ͣ��������ͷ��͹������ϲ������ύ����֮��
	//����ϳ̶�
	TCPWriteTaskList* curTasks;

	TCPConnectionState curState;

	unsigned char priority; 

	//���۷��ͻ��ǽ��գ������ÿ��ж�ģʽ
	//ÿ�ο�ʼһ���������ݱ��ĵĽ���ʱ����ʱ�����Ĵ����
	//�����Ļ����У�ֻ���ڽ�����ȫ������²Ž����ݸ��Ƴ�
	//�����ɴ�һ�������һ������û�н�����ϣ����Ѿ����
	//���յĲ��ִ���������Ľṹ�У���һ�ν��չ��̿��Լ�
	//������ʣ�������
	TCPContext recvContext;

	//ÿ�ο�ʼһ���������ݱ��ĵķ���ʱ�����Ƚ����Ĵ渴�Ƶ�
	//�����Ļ����У�ֻ���ڷ�����ȫ������²Ž����ݽ���ɾ��
	//�ɴ�һ�������һ������û�з�����ϣ���ʣ����Ҫ���з�
	//�͵Ĳ������ݴ���������Ľṹ�У���һ�η��͹��̿��Լ�
	//������ʣ�������
	TCPContext sendContext;

	//��netmonitor���������ӣ����ڽ��յ��������籨�ĵ�ʱ��
	//��λ���������DUST_DELETE_DEADLINE�������ø�����
	//״̬ΪINVALID
	unsigned char heartbeat;

	//��ǰ���ڸö˿ڽ��з��͵��̱߳�ţ����û���߳����ڲ�������ΪINVALID_THREAD_ID
	EvoThreadId sendingThread; 
	//��ǰ���ڸö˿ڽ��н��յ��̱߳�ţ����û���߳����ڲ�������ΪINVALID_THREAD_ID
	EvoThreadId recvingThread;

	clock_t lastSendTime; 

	//��ʾ�������Ƿ�����˷�������
	unsigned short sendBlock;

    u_int32_t incomingPacketNum;
    u_int32_t outcomingPacketNum;

}TCPConnectionNode;

/*****************************************************
�������е����ӣ��Լ������ϵķ�������
���ԭ��
1���������ٻ�����Ƶ�ʹ�ã�����ϵͳ��Դ������Ͳ�����
2�������߳������������ݵĶ�ȡ��select�����̺ϲ�������
���������£����Ӷ�ȡ�߳�����Ч�����ޡ�
3�����ͺͽ����߳̾��̶�Ϊ�����������߳������Ӷ�Ӧ��ϵ��
���������߳�������������

�����ӽ�������
�����������������Ӷ��н����˻���������Ҫ�����κ�
һ���½������ӻ�������κ�һ�����Ӳ������ݣ�����Ҫ����
�ٽ������⡣
�����ݷ��ʹ���
�����߳�Ϊ�˻�ȡ���������б�ͬ����Ҫ�����ٽ������⣬Ȼ
���ȡ�κ�һ�������ϵ������б���뿪�ٽ������������ݷ�
�͡�
�����ݽ��մ���
�����߳�ͨ���ٽ�����ȡ��ǰ��������Ч���Ӷ˿ڣ���������
�ݡ���������ݵ����Ҫ�ٴν����ٽ������޸����ӵ�������
��Ҫע����ǣ����ڶ�ȡ�˿ڵ��̶̹߳�����˲����ڶ�ȡ�߳�
֮��Ļ������⣬���ӹ���״̬���ṩ�������̲߳ο���
������ʧЧ����
1�����ݷ��͹����г����쳣������Ҫ������Ӧ������ʧЧ������
�߳�һ������������δ���͵����ݣ���һ������ͨ��TCPConnectionId
������Ӧ����ʧЧ���ڴ˹�������Ҫ�����ٽ������⡣��Ȼ��
�������ӵ�״̬��CONNECTION_STATE_OCCUPY�����ҶԸ�״̬����
��ͬ����Ҫ�ٽ������⣬��˲���Ҫ�������������̶߳Ը����ӵ�
������
2�����ݽ��չ����г����쳣������Ҫ������Ӧ������ʧЧ��
�����߳�ͨ��TCPConnectionId������Ӧ����ʧЧ���ڴ˹�������
Ҫ�����ٽ������⡣���ܴ��ڵ�һ�ֿ����Ƿ����߳��Ѿ�����ʧ
Ч�����������������Ӱ�졣
3���������߳�ͨ������������������ʧЧ�������ڿ���������
�������Ѿ������ٽ�������˿���ֱ����������״̬��
������ɾ������
���������߳�ͳһ�������������̷߳�������ʧЧ�󣬽���
TCPConnectionNode�ṹ���뵽���������У�Ȼ���뿪�����ڿ��н�
��ͳһɾ�����ͷ����������е���Դ����Ҫע����ǣ�����߳���
Ҫ��������ʵ�ʵ�״̬��ɶ�����
1��������������ڶ�ȡ���߷���״̬����û�йرգ������ȹر�
���Ӳ��˳���������߳�Ӧ�ÿ��Ի�ȡ���󣬲��˳�������
2��ֻ�е�SEND��RECV����״̬����λ֮�󣬲��ܽ��ṹ��������
���С�
*****************************************************/
typedef struct TCPConnectionList
{
	CRITICAL_SECTION tcpConnList_cs;
	TCPConnectionNode* tcpConnList[MAX_TCPCONNECTION_LIST_LENGTH];
    u_int32_t connNum;
    u_int32_t send_task_count;
}TCPConnectionList;

/*****************************************************
�������ջ�����ؽṹ
��������������»ὫconnectionNode���뵽����վ
1�����ͳ���2�����ճ���3�������쳣��
����������£�connectionNode�ֱ��CONNECTION_STATE_SENDERR
CONNECTION_STATE_RECVERR�Լ�CONNECTION_STATE_INVALID��λ
NetMonitor�����Բ鿴connectionNode��������
����Ҫ��Ľڵ�������վ������һ������֮������
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
��ģ���Ӧ�ĺ���
*****************************************************/
//������������г�ʼ��
void InitializeTCPList();
//�ͷ����е���Դ������������Դ���ڴ���Դ
void FinalizeTCPList();

//ģ���ⲿ�ӿڣ���Ӧ�ýӿ�ģ���RegisterListener���á���ָ���Ķ�
//�ں��Ͻ���һ�����Ӽ����˿ڣ������ڱ�ģ��Ĺ���������������Ӧ��
//�ڵ㡣�����߳̿��Խ��ö˿�װ�ز�����select��
//�ú����н���ʵ�ʵĶ˿ڳ�ʼ�������ݽṹ�����ɡ����������ά��
//�ȹ���������ɹ�������䲢������Ч�������ʶ�����򷵻�INVALID_LISTENER��
TCPConnectionId InsertListener(const char* addr , unsigned short port);

//ģ���ⲿ�ӿڣ���Ӧ�ýӿ�ģ���UnRegisterListener���á�ע��һ��
//֮ǰע������Ӽ����˿ڣ��������в��Ҷ�Ӧ�Ĺ���ڵ㣬��������
//ΪʧЧ����Դ���ͷ�����Դ����ģ���������ɹ��򷵻�1�����򷵻�-1
int RemoveListener(TCPConnectionId listenerID);

//ģ���ⲿ�ӿڣ���ʼ���׶α�Ӧ�ýӿ�ģ��EvoTCPInitialize���á�ע��
//ϵͳĬ�ϵ����Ӽ����˿ڡ������Ӽ����˿ڸ���Ϊÿһ�������߳̽���
//���жϹ������ӡ����⣬�ϲ�Ӧ��Ҳ�������ø�Ĭ�϶˿ڽ���TCP���ӡ�
//�ú����н���ʵ�ʵĶ˿ڳ�ʼ�������ݽṹ�����ɡ����������ά����
//����������ɹ�������䲢������Ч�������ʶ�����򷵻�INVALID_LISTENER��
//��Ҫע����ǣ��ö˿ڵ���������ΪLISTENER_BUILTIN�����ұ�֤����Ψһ�ԡ�
TCPConnectionId InsertInteruptListener(const char* addr , unsigned short port);


//ģ���ⲿ�ӿڣ���Ӧ�ýӿ�ģ��ĵ�RegisterConnection���ã���һ��ָ
//���Ķ˿ڷ���TCP���ӣ���������Ӧ�Ĺ���ڵ㡣
//�ú����н���ʵ�ʵĶ˿ڳ�ʼ����ͨ������connect�������ӡ����ݽṹ��
//���ɡ�TCP���ӹ��������ά���ȹ���������ɹ�������䲢������Ч����
//���ʶ�����򷵻�INVALID_CONNECTION��
TCPConnectionId InsertActiveTCPConnection(const char* addr , unsigned short port);

//ģ���ⲿ�ӿڣ������ݽ����߳��ڻ�ȡ�ⲿ���ӽ������󣨶Զ˵���connect��
//�����µ�TCP����֮����ã�������Ӧ�Ĺ���ڵ㡣
//���ӵĽ������������ݽ����̵߳�ִ�к�����ʵ�֣��ú����и������ݽṹ��
//���ɡ�TCP���ӹ��������ά���ȹ���������ɹ�������䲢������Ч�������ʶ��
//���򷵻�INVALID_CONNECTION��
TCPConnectionId InsertPassiveTCPConnection(const char* addr , unsigned short port, TCPConnectionId parentId , SOCKET socket);

//ģ���ⲿ�ӿڣ���Ӧ�ýӿ�ģ���UnRegisterConnection���ã��������в��Ҷ�Ӧ��
//����ڵ㣬��������ΪʧЧ����Դ���ͷ�����Դ����ģ����ɡ�
int RemoveConnection(TCPConnectionId connId);

//��������ɾ����Ӧ�����ӣ������ͷ����е���Դ������������ڴ���Դ
//������Ҫע����ǣ����ӿ��ܵ�״̬�Ƿ��ͻ��߽��ա����ڹ���״̬�Ķ˿�
//��Ҫ�ȴ���Щ�˿ڽ���ǹ���״̬��Ȼ���ͷ�����Դ��
//Ϊ��֧������Ҫ�����ж˿ھ�����Ϊ�������Ĺ���ģʽ�������ڹ�������
//�в��ϲ�ѯ����Ĺ���״̬�����CONNECTION_STATE_INVALID�����ã�
//���˳����ͻ��߽���״̬����SEND��RECV״̬������λ�󣬿�ʼ�ͷ������Դ��
//���⻹��Ҫ���ǽ����̵߳Ĺ���״̬����������̴߳���TCP_THREAD_STATE_SELECT
//״̬������Ҫͨ���ж϶˿ڻ��ѡ�
int ReleaseTCPConnectionNode(TCPConnectionNode* node);
int ReleaseTCPListenerNode(TCPListenerNode* node);

void CleanupTCPConnectionList();
void CleanupTCPListenerList();




/*****************************************************
NetMonitorģ����õĺ���
*****************************************************/
//�����е����ӷ�����������
//�������ݱ��Ķ����ж�������״̬���������ĵ�ͬ
//�����Ҫ�����ж�ConnectionNode������������Ƿ�
//��������ֻ�����������Ϊ�յ�����£�����Ҫ����
//��������
//���������ӵ����������������ֵ������ΪINVALID
//�������������վ
int TestResourceState();

int InsertConnIntoDustbin(TCPConnectionNode* node);

int InsertListenerIntoDustbin(TCPListenerNode* node);

//�鿴����վ�е����ӣ����DeadLine�ﵽ��ֵ�����ͷ���Դ
int CleanDustbin();

//�ͷŻ���վ�����е���Դ
int FinalizeDustbin();



#endif