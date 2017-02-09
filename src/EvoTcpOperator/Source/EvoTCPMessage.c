#ifdef _WIN32
#define EvoTCPOperatorLibAPI __declspec(dllexport)
#else
#define EvoTCPOperatorLibAPI
#endif

#include "../Include/EvoTCPCommon.h"

unsigned short MapMsgHeader(char* msgBuf , EvoTCPMessageHeader header)
{
    header.extFlags = htons(header.extFlags);
    header.octetsToNextHeader = htonl(header.octetsToNextHeader);
	memcpy((void*)msgBuf , (void*)&header , SIZEOF_MSGHEADER);
	return SIZEOF_MSGHEADER;
}

EvoTCPMessageHeader UnMapMsgHeader(char* msgBuf)
{
	EvoTCPMessageHeader tmpheader;
	memcpy((void*)&tmpheader , (void*)msgBuf , SIZEOF_MSGHEADER);
    tmpheader.extFlags = ntohs(tmpheader.extFlags);
    tmpheader.octetsToNextHeader = ntohl(tmpheader.octetsToNextHeader);
	return tmpheader;
}