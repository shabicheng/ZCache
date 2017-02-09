#include "ZCacheClientManager.h"

ZCacheClientManager::ZCacheClientManager()
{
    m_lock = ZRCreateCriticalSection();
}

ZCacheClientManager::~ZCacheClientManager()
{
    ZRCS_ENTER(m_lock);
    for (auto client : m_clients)
    {
        delete client;
    }
    ZRCS_LEAVE(m_lock);
    ZRDeleteCriticalSection(m_lock);
}

void ZCacheClientManager::addClient(ZCacheClient * client)
{
    ZRCS_ENTER(m_lock);
    m_clients.push_back(client);
    ZRCS_LEAVE(m_lock);
}

void ZCacheClientManager::removeClient(TCPConnectionId id)
{
    ZRCS_ENTER(m_lock);
    for (auto iter = m_clients.begin(); iter != m_clients.end(); ++iter)
    {
		if (IdIsEqual((*iter)->clientID, id))
		{
			delete *iter;
			m_clients.erase(iter);
			break;
		}
    }
    ZRCS_LEAVE(m_lock);
}

