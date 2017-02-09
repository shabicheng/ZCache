#include "ZCacheClient.h"

ZCacheClient::ZCacheClient(TCPConnectionId id, char * ip, unsigned short port)
    :clientID(id), nowDataBase(NULL), nowDataBaseIndex(-1), clientPort(port)
{
	clientIp = sdsnew(ip);
}

ZCacheClient::~ZCacheClient()
{

}

