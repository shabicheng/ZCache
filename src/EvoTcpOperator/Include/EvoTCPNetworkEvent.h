#ifndef EVONETWORKEVENT_H
#define EVONETWORKEVENT_H

typedef enum EvoTCPNetworkEventType
{
	EVO_TCP_EVENT_ACCEPT , 
	EVO_TCP_EVENT_RECV , 
	EVO_TCP_EVENT_LISTENER_ERR , 
	EVO_TCP_EVENT_CONNECTION_ERR , 
}EvoTCPNetworkEventType;

typedef struct EvoTCPNetworkEvent
{
	TCPConnectionId id;
	TCPConnectionId parentId;
	char* buf;
    u_int32_t bufLength;
	unsigned short port;
	EvoTCPNetworkEventType type;
	unsigned short para1;
} EvoTCPNetworkEvent;


#endif