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
	CONNECTION_STATE_EMPTY = 0x00 , // 目前没有线程对该连接操作
	CONNECTION_STATE_SELECT = 0x01 , //目前正有一个接收线程在针对该连接执行select
	CONNECTION_STATE_RECV = 0x02 , // 目前正有一个接收线程在针对该连接执行recv()
	CONNECTION_STATE_RECVERR = 0x04 ,// 接收线程发现连接故障
	CONNECTION_STATE_SEND = 0x08 ,//目前正有一个接收线程在针对该连接执行send()
	CONNECTION_STATE_SENDERR = 0x10 ,//发送线程发现连接故障
	CONNECTION_STATE_INVALID = 0x20 ,
	CONNECTION_STATE_POP = 0X40 , //该连接上接收到了数据，正在提交给上层应用
}TCPConnectionState;


/*
表示线程的工作状态
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