#pragma once
#include "ZRlibrary.h"
#include "ZCacheClient.h"

class ZCacheClientManager
{
public:
	ZCacheClientManager();
	~ZCacheClientManager();

    void addClient(ZCacheClient * client);
    void removeClient(TCPConnectionId id);

protected:
    list<ZCacheClient *> m_clients;
    ZRCRITICALSECTION m_lock;
private:
};