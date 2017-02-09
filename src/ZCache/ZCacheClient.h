#pragma once
#include "EvoTCPCommon.h"
#include "ZCacheDataBase.h"
#include "sds.h"
class ZCacheClient
{
public:
	ZCacheClient(TCPConnectionId clientID, char * ip, unsigned short port);
	~ZCacheClient();

    TCPConnectionId clientID;
    ZCacheDataBase * nowDataBase;
    int nowDataBaseIndex;
	sds clientIp;
	unsigned short clientPort;

protected:
	    
private:
};