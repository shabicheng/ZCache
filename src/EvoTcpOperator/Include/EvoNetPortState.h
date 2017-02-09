#ifndef EVONETPORTSTATE_H
#define EVONETPORTSTATE_H


typedef enum TCPListenerState
{
	LISTENER_STATE_EMPTY = 0X00 ,
	LISTENER_STATE_SELECT = 0X01 , 
	LISTENER_STATE_ACCEPT = 0X02 , 
	LISTENER_STATE_INVALID = 0X04 , 

} TCPListenerState;

typedef enum TCPConnectionState
{
	CONNECTION_STATE_EMPTY = 0x00 , // Ŀǰû���̶߳Ը����Ӳ���
	CONNECTION_STATE_SELECT = 0x01 , //Ŀǰ����һ�������߳�����Ը�����ִ��select
	CONNECTION_STATE_RECV = 0x02 , // Ŀǰ����һ�������߳�����Ը�����ִ��recv()
	CONNECTION_STATE_RECVERR = 0x04 ,// �����̷߳������ӹ���
	CONNECTION_STATE_SEND = 0x08 ,//Ŀǰ����һ�������߳�����Ը�����ִ��send()
	CONNECTION_STATE_SENDERR = 0x10 ,//�����̷߳������ӹ���
	CONNECTION_STATE_INVALID = 0x20 ,
	CONNECTION_STATE_POP = 0X40 , //�������Ͻ��յ������ݣ������ύ���ϲ�Ӧ��
}TCPConnectionState;


/*
��ʾ�̵߳Ĺ���״̬
*/
typedef enum EvoTCPThreadState
{
	TCP_THREAD_STATE_INVALID = 0x00,
	TCP_THREAD_STATE_INIT = 0x01, 
	TCP_THREAD_STATE_IDLE = 0x02,

	TCP_THREAD_STATE_INVOKE = 0x03,		
	TCP_THREAD_STATE_SENDING = 0x04,
	TCP_THREAD_STATE_SCANING = 0x05,

	TCP_THREAD_STATE_SELECT = 0x06,
	TCP_THREAD_STATE_RECVING = 0x07,

}EvoTCPThreadState;

typedef  unsigned short EvoThreadId;
#define INVALID_THREAD_ID 255

#endif