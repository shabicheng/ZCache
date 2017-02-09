/********************************************************************
EvoTCPMessage.h

TCP数据流之上的基本数据传输报文格式。该格式与任何应用
无关。本模块实现所有报文的编解码。

报文分为报文头和报文体两个部分，上层应用类协议，例如RTPS
等协议均在报文体内实现。

底层的数据发送和接受均采用固定长度缓存，设定为MSG_ENCODE_BUF_LENGTH，
因此消息的编解码均采用静态内存完成，每一个发送线程和接收
线程中均设置MSG_ENCODE_BUF_LENGTH长度的收发操作内存。

采用C语言完成。
********************************************************************/

#ifndef EVOTCPMESSAGE_H
#define EVOTCPMESSAGE_H

#define SIZEOF_SHORT 2
#define SIZEOF_USHORT 2
#define SIZEOF_LONG 4
#define SIZEOF_ULONG 4

#define SIZEOF_MSGHEADER sizeof(EvoTCPMessageHeader)
#define SIZEOF_DATASEGFLAG 2

#define MSG_ENCODE_BUF_LENGTH 1024*4  //单次调用send和recv的最大尺寸

typedef enum EvoTCPMessageKind 
{
	EVO_TCP_MSG_HANDSHAKE = 0x01 , 
	EVO_TCP_MSG_DATA = 0x02 , 
	EVO_TCP_MSG_DATAFRAG = 0x03, 
	EVO_TCP_MSG_HEARTBEAT = 0x04 , 
	EVO_TCP_MSG_INTERUPT = 0X05 , 
}EvoTCPMessageKind;

typedef struct EvoTCPSequence
{
	unsigned short totalNum; //全部报文数
	unsigned short curPos; //当前报文编号
}EvoTCPSequence;

typedef struct EvoTCPMessageHeader
{
	BYTE type;
	BYTE flags;
	unsigned short extFlags; //如果是数据报文，则为合包数量
    u_int32_t octetsToNextHeader;
}EvoTCPMessageHeader;

//msgBuf为已经分配的内存，编码过程中不存在内存分配
unsigned short MapMsgHeader(char* msgBuf , EvoTCPMessageHeader header);

EvoTCPMessageHeader UnMapMsgHeader(char* msgBuf);


#endif