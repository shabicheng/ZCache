/********************************************************************
EvoTCPMessage.h

TCP������֮�ϵĻ������ݴ��䱨�ĸ�ʽ���ø�ʽ���κ�Ӧ��
�޹ء���ģ��ʵ�����б��ĵı���롣

���ķ�Ϊ����ͷ�ͱ������������֣��ϲ�Ӧ����Э�飬����RTPS
��Э����ڱ�������ʵ�֡�

�ײ�����ݷ��ͺͽ��ܾ����ù̶����Ȼ��棬�趨ΪMSG_ENCODE_BUF_LENGTH��
�����Ϣ�ı��������þ�̬�ڴ���ɣ�ÿһ�������̺߳ͽ���
�߳��о�����MSG_ENCODE_BUF_LENGTH���ȵ��շ������ڴ档

����C������ɡ�
********************************************************************/

#ifndef EVOTCPMESSAGE_H
#define EVOTCPMESSAGE_H

#define SIZEOF_SHORT 2
#define SIZEOF_USHORT 2
#define SIZEOF_LONG 4
#define SIZEOF_ULONG 4

#define SIZEOF_MSGHEADER sizeof(EvoTCPMessageHeader)
#define SIZEOF_DATASEGFLAG 2

#define MSG_ENCODE_BUF_LENGTH 1024*4  //���ε���send��recv�����ߴ�

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
	unsigned short totalNum; //ȫ��������
	unsigned short curPos; //��ǰ���ı��
}EvoTCPSequence;

typedef struct EvoTCPMessageHeader
{
	BYTE type;
	BYTE flags;
	unsigned short extFlags; //��������ݱ��ģ���Ϊ�ϰ�����
    u_int32_t octetsToNextHeader;
}EvoTCPMessageHeader;

//msgBufΪ�Ѿ�������ڴ棬��������в������ڴ����
unsigned short MapMsgHeader(char* msgBuf , EvoTCPMessageHeader header);

EvoTCPMessageHeader UnMapMsgHeader(char* msgBuf);


#endif