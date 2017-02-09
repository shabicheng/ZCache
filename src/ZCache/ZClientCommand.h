#pragma once
#include "ZCacheItem.h"

class ZClient;

class ZClientCommand
{
public:
	ZClientCommand();
	~ZClientCommand();

    bool setSynClient(ZClient * pSynClient);
    bool ping();
    int clearDB();
    int set(sds key, ZCacheItem * pItem);
    int setNx(sds key, ZCacheItem * pItem);
    int remove(sds key);
    ZCacheItem * get(sds key, int * errorFlag = NULL);
    bool exist(sds key);
    long inc(sds key, int * errorFlag = NULL);
    long dec(sds key, int * errorFlag = NULL);
    long incBy(sds key, long val, int * errorFlag = NULL);
    long decBy(sds key, long val, int * errorFlag = NULL);
    bool select(long dbIndex);
    long dbSize();
    int rename(sds oldKey, sds newKey);
    long lpush(sds listKey, ZCacheItem * pItem);
    long rpush(sds listKey, ZCacheItem * pItem);
    ZCacheItem * lpop(sds listKey, int * errorFlag = NULL);
    ZCacheItem * rpop(sds listKey, int * errorFlag = NULL);
    long llen(sds listKey);
    int lrange(sds listKey, int start, int end, ZCacheList * & pList);
    unsigned short type(sds key);
    int sadd(sds setKey, sds key);
    int sexist(sds setKey, sds key);
    int srem(sds setKey, sds key);
    int ssize(sds setKey);


protected:

    ZClient * m_client;
	
private:
};
