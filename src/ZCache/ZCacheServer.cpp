#include "ZCacheServer.h"
#include "ZCacheConfig.h"
#include "ZCommon.h"
#include <EvoTCPCommon.h>
#include "ZCacheClient.h"
#include "ZCacheClientManager.h"
#include "ZCacheProcessor.h"

ZCacheServer * g_cacheServer = NULL;
ZCacheClientManager * g_clientManager;
ZCacheProcessor * g_cacheProcessors;
ZRCRITICALSECTION g_serverCS = ZRCreateCriticalSection();

ZCacheServer * ZCacheServer::getInstance()
{
	if (g_cacheServer == NULL)
	{
		ZRCS_ENTER(g_serverCS);
		if (g_cacheServer != NULL)
		{
			ZRCS_LEAVE(g_serverCS);
			return g_cacheServer;
		}
		g_cacheServer = new ZCacheServer;
		ZRCS_LEAVE(g_serverCS);
	}
	return g_cacheServer;
}

void ZCacheServer::finalizeInstance()
{
	if (g_cacheServer != NULL)
	{
		ZRCS_ENTER(g_serverCS);
		if (g_cacheServer == NULL)
		{

			ZRCS_LEAVE(g_serverCS);
			return;
		}
		delete g_cacheServer;
		g_cacheServer = NULL;
		ZRCS_LEAVE(g_serverCS);
	}
}

int ZCacheTCPNetworkEventHook(EvoTCPNetworkEvent netEvent)
{
	switch (netEvent.type)
	{
	case EVO_TCP_EVENT_ACCEPT:
	{

		ZCacheClient * pClient = new ZCacheClient(netEvent.id, netEvent.buf, netEvent.port);
		INNER_LOG(CINFO, "接收到客户端连接 %s %d", netEvent.buf, netEvent.port);
		netEvent.id.idParam = pClient;
		SetConnectionParam(netEvent.id);
        pClient->nowDataBase = g_cacheServer->getDB(0);
        pClient->nowDataBaseIndex = 0;
		g_clientManager->addClient(pClient);
	}
		break;
	case EVO_TCP_EVENT_RECV:
	{
		g_cacheProcessors[netEvent.id.id % g_config.processorNum].onDataArrive(
			netEvent.id, netEvent.buf, netEvent.bufLength);
	}
		break;
	case EVO_TCP_EVENT_LISTENER_ERR:
		break;

	case EVO_TCP_EVENT_CONNECTION_ERR:
	{
		INNER_LOG(CINFO, "客户端失去连接 %s %d", netEvent.buf, netEvent.port);
		g_clientManager->removeClient(netEvent.id);
	}
		break;
	default:
		break;
	}
	return 0;
}

ZCacheServer::ZCacheServer()
{
    ZCacheConfig::readConfig(g_config);
	ZInitSharedCommand();
	databases = (ZCacheDataBase **)malloc(sizeof(ZCacheDataBase*) * g_config.databaseCount);
	ZCheckPtr(databases);
	memset(databases, 0, sizeof(ZCacheDataBase*) * g_config.databaseCount);
	g_clientManager = new ZCacheClientManager;
	g_cacheProcessors = new ZCacheProcessor[g_config.processorNum];

	EvoTCPInitialize();
	RegisterTCPEventHook(ZCacheTCPNetworkEventHook);
	RegisterListener(g_config.listenIp, g_config.listenPort);
}

ZCacheServer::~ZCacheServer()
{
	EvoTCPFinalize();
	delete[] g_cacheProcessors;
	delete g_clientManager;
	g_clientManager = NULL;
	for (int i = 0; i < g_config.databaseCount; ++i)
	{
		if (databases[i] != NULL)
		{
			delete databases[i];
		}
	}
	free(databases);
}

void ZCacheServer::start()
{
    for (int i = 0; i < g_config.processorNum; ++i)
    {
        g_cacheProcessors[i].Start();
    }
}

void ZCacheServer::stop()
{
    for (int i = 0; i < g_config.processorNum; ++i)
    {
        g_cacheProcessors[i].Stop(true);
    }
}

extern dictType hashDictType;

ZCacheDataBase * ZCacheServer::getDB(int index)
{
	if (index < 0 || index >= g_config.databaseCount)
	{
		INNER_LOG(CWARNING, "无效的数据库ID %d", index);
		return NULL;
	}
	if (databases[index] == NULL)
	{
		ZRCS_ENTER(g_serverCS);
		if (databases[index] != NULL)
		{

			ZRCS_LEAVE(g_serverCS);
			return databases[index];
		}
		databases[index] = new ZCacheDataBase();
		databases[index]->cacheHash = dictCreate(&hashDictType, this);
		ZRCS_LEAVE(g_serverCS);
	}
	return databases[index];
}

void ZCacheServer::clearDB(ZCacheDataBase * pDB)
{
    pDB->lock();

    pDB->clear();
    pDB->cacheHash = dictCreate(&hashDictType, this);

    pDB->unlock();
}

