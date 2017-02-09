#include "ZCacheItem.h"
#include "ZCommon.h"
#include "ZCacheItemHelper.h"

ZCacheItem * ZCacheItem::fromBuffer(const char * buffer, size_t bufLen, size_t & itemBufLen)
{
	if (bufLen < 1)
	{
		return NULL;
	}
	unsigned char itemType = buffer[0];
	itemBufLen = 1;
	
	ZCacheItem * pItem = g_itemHelper.createItem(itemType);
    ZCheckPtr(pItem);
	itemBufLen += pItem->initFromBuffer(buffer + 1, bufLen - 1);
	
	return pItem;
}


static int sdsDictKeyCompare(void *privdata, const void *key1,
	const void *key2)
{
	unsigned int sdskey1 = sdskey((sds)key1);
	unsigned int sdskey2 = sdskey((sds)key2);
	if (sdskey1 != INVALID_SDS_KEY &&
		sdskey2 != INVALID_SDS_KEY)
	{
		return sdskey1 == sdskey2;
	}

	int l1, l2;



	l1 = sdslen((sds)key1);
	l2 = sdslen((sds)key2);
	if (l1 != l2) return 0;
	return memcmp(key1, key2, l1) == 0;
}

static void dictKeyDestructor(void *privdata, void *val)
{
	sdsfree((sds)val);
}

static void dictValueDestructor(void *privdata, void *val)
{
	delete (ZCacheObj*)val;
}

static unsigned int dictSdsHash(const void *key) {
	unsigned int sKey = sdskey((sds)key);
	if (sKey != INVALID_SDS_KEY)
	{
		return sKey;
	}
	return dictGenHashFunction((const unsigned char *)key, sdslen((sds)key));
}

dictType setDictType = {
	dictSdsHash,               /* hash function */
	NULL,                      /* key dup */
	NULL,                      /* val dup */
	sdsDictKeyCompare,         /* key compare */
	dictKeyDestructor, /* key destructor */
	NULL                       /* val destructor */
};

dictType hashDictType = {
	dictSdsHash,                /* hash function */
	NULL,                       /* key dup */
	NULL,                       /* val dup */
	sdsDictKeyCompare,          /* key compare */
	dictKeyDestructor,  /* key destructor */
	dictValueDestructor   /* val destructor */
};



bool ZCacheBuffer::operator==(const ZCacheBuffer & o) const
{
    if (cacheBuffer == NULL)
    {
        if (o.cacheBuffer == NULL)
        {
            return true;
        }
        return false;
    }
    return sdscmp(cacheBuffer, o.cacheBuffer) == 0;
}

void ZCacheBuffer::setStringNoCopy(sds s)
{
    if (cacheBuffer != NULL)
    {
        sdsfree(cacheBuffer);
    }
    cacheBuffer = s;
}

void ZCacheBuffer::setStringCopy(sds s)
{
    if (cacheBuffer != NULL)
    {
        sdsfree(cacheBuffer);
    }
    cacheBuffer = sdsdup(s);
}

void ZCacheBuffer::removeStringNoFree()
{
    cacheBuffer = NULL;
}

void ZCacheBuffer::removeString()
{
    clear();
}

sds ZCacheBuffer::getString()
{
    return cacheBuffer;
}

size_t ZCacheBuffer::getSize()
{
    return sdslen(cacheBuffer);
}

size_t ZCacheBuffer::toBuffer(char * buf, size_t maxBufLen) const
{
#ifndef NDEBUG
    assert(sdskey(cacheBuffer) != 0);
    assert(sdskey(cacheBuffer) != INVALID_SDS_KEY);
#endif
	size_t bufSize = sdslen(cacheBuffer) + 8;
	if (bufSize + 1 > maxBufLen)
	{
		return bufSize + 1;
	}

	buf[0] = itemType;
	buf += 1;
	*(long *)buf = bufSize - 8;
	buf += sizeof(long);
	*(unsigned int *)buf = sdskey(cacheBuffer);
	buf += sizeof(unsigned int);
	memcpy(buf, cacheBuffer, bufSize - 8);
	return bufSize + 1;
}

ZCacheItem * ZCacheBuffer::copySelf() const
{
	ZCacheBuffer * pBuf = new ZCacheBuffer;
	ZCheckPtr(pBuf);
	pBuf->cacheBuffer = sdsdup(cacheBuffer);
	return pBuf;
}

int ZCacheBuffer::initFromBuffer(const char * buffer, size_t bufLen)
{
	if (bufLen < 8)
	{
		return -1;
	}
	long sLen = *(long *)buffer;
	if (sLen + 8 > (long)bufLen)
	{
		return -2;
	}
	buffer += sizeof(long);
	unsigned int sKey = *(unsigned int *)buffer;
	buffer += sizeof(unsigned int);
	cacheBuffer = sdsnewlen(buffer, sLen);
	sdssetkey(cacheBuffer, sKey);
	return sLen + 8;
}

void ZCacheLong::setValue(long val)
{
    cacheLong = val;
}

long ZCacheLong::getValue()
{
    return cacheLong;
}

int ZCacheLong::incBy(long incVal)
{
    long long newValue = (long long)cacheLong + incVal;
    cacheLong = (long)newValue;
    if (newValue > LONG_MAX)
    {
        return -1;
    }
    else if (newValue < LONG_MIN)
    {
        return -2;
    }
    return 0;
}

size_t ZCacheLong::toBuffer(char * buf, size_t maxBufLen) const
{
	if (maxBufLen < 5)
	{
		return 5;
	}
	buf[0] = itemType;
	buf += 1;
	*(long *)buf = cacheLong;
	return 5;
}


ZCacheItem * ZCacheLong::copySelf() const
{
	ZCacheLong * pLong = new ZCacheLong;
	ZCheckPtr(pLong);
	pLong->cacheLong = cacheLong;
	return pLong;
}

int ZCacheLong::initFromBuffer(const char * buffer, size_t bufLen)
{
	if (bufLen < 4)
	{
		return -1;
	}
	
	cacheLong = *(long *)buffer;

	return 4;
}

void ZCacheList::clearNoDelete()
{
//     for (auto & pObj : cacheList)
//     {
//         // remove的时候再delete，这样里面的内存不会被delete掉
//         // 但是如果这个不是最后一个sharedptr，则其他的还会delete掉
//         pObj->removeObj();
//         delete pObj;
//     }
    cacheList.clear();
}

size_t ZCacheList::pushBack(ZCacheObj * obj)
{
    cacheList.push_back(obj);
    return cacheList.size();
}

size_t ZCacheList::pushFont(ZCacheObj * obj)
{
    cacheList.push_front(obj);
    return cacheList.size();
}

bool ZCacheList::popBack(ZCacheObj * & obj)
{
    if (cacheList.empty())
    {
        return false;
    }
    obj = cacheList.back();
    cacheList.pop_back();
    return true;
}

bool ZCacheList::popFont(ZCacheObj * & obj)
{
    if (cacheList.empty())
    {
        return false;
    }
    obj = cacheList.front();
    cacheList.pop_front();
    return true;
}

int ZCacheList::getRange(int start, int end, ZCacheList * rangeList)
{
    if (start < 0)
    {
        start = cacheList.size() + 1 + start;
    }
    if (end < 0)
    {
        end = cacheList.size() + 1 + end;
    }
    if (end < start || start < 0 || end > (int)cacheList.size())
    {
        return -1;
    }
    int i = 0;
    list<ZCacheObj *>::iterator sIter, eIter;
    for (auto iter = cacheList.begin(); (size_t)i < cacheList.size(); ++i, ++iter)
    {
        if (i == start)
        {
            sIter = iter;
        }
        if (i == end)
        {
            eIter = iter;
            break;
        }
    }
    if (i == cacheList.size())
    {
        eIter = cacheList.end();
    }
    rangeList->cacheList.assign(sIter, eIter);
    return end - start;
}

int ZCacheList::removeRange(int start, int end)
{
    if (start < 0)
    {
        start = cacheList.size() + 1 + start;
    }
    if (end < 0)
    {
        end = cacheList.size() + 1 + end;
    }
    if (end < start || start < 0 || end > (int)cacheList.size())
    {
        return -1;
    }
    int i = 0;
    list<ZCacheObj *>::iterator sIter, eIter;
    for (auto iter = cacheList.begin(); (size_t)i < cacheList.size(); ++i)
    {
        if (i == start)
        {
            sIter = iter;
        }
        if (i == end)
        {
            eIter = iter;
            break;
        }
    }
    if (i == cacheList.size())
    {
        eIter = cacheList.end();
    }
    cacheList.erase(sIter, eIter);
    return end - start;
}

size_t ZCacheList::getSize()
{
    return cacheList.size();
}

size_t ZCacheList::toBuffer(char * buf, size_t maxBufLen) const
{
	size_t listLen = cacheList.size();
	size_t totalLen = sizeof(size_t) + 1;
	// 第一次遍历，拿到最大的bufLen
	for (const auto & listItem : cacheList)
	{
		totalLen += listItem->pObj->toBuffer(NULL, 0);
	}
	if (totalLen > maxBufLen)
	{
		return totalLen;
	}
	buf[0] = itemType;
	buf += 1;
	*(size_t*)buf = listLen;
	buf += sizeof(size_t);
	size_t nowLen = 0;
	maxBufLen -= sizeof(size_t) + 1;
	for (const auto & listItem : cacheList)
	{
		nowLen += listItem->pObj->toBuffer(buf + nowLen, maxBufLen - nowLen);
	}
	return totalLen;
}

ZCacheItem * ZCacheList::copySelf() const
{
	ZCacheList * pList = new ZCacheList;
	ZCheckPtr(pList);
	for (const auto & listItem : cacheList)
	{
		pList->cacheList.push_back(new ZCacheObj(listItem->pObj->copySelf()));
	}
	return pList;
}

int ZCacheList::initFromBuffer(const char * buffer, size_t bufLen)
{
	if (bufLen < sizeof(size_t))
	{
		return -1;
	}
	size_t listLen = *(size_t *)buffer;
	buffer += sizeof(size_t);
	size_t readSize = 0;
	bufLen -= sizeof(size_t);
	size_t itemBufLen = 0;
	for (size_t i = 0; i < listLen; ++i)
	{
		cacheList.push_back(new ZCacheObj(ZCacheItem::fromBuffer(buffer + readSize, bufLen - readSize, itemBufLen)));
		readSize += itemBufLen;
	}
	return readSize + sizeof(size_t);
}

extern dictType setDictType;
extern dictType hashDictType;

int ZCacheSet::add(sds key)
{
    int addRst = dictAdd(cacheSet, key, NULL);
    if (DICT_ERR == addRst)
    {
        return 0;
    }
    return 1;
}

int ZCacheSet::remove(sds key)
{
    int addRst = dictDelete(cacheSet, key);
    if (DICT_ERR == addRst)
    {
        return 0;
    }
    return 1;
}

bool ZCacheSet::exist(sds key)
{
    if (NULL == dictFind(cacheSet, key))
    {
        return false;
    }
    return true;
}

ZCacheSet * ZCacheSet::intersection(ZCacheSet * pOther)
{
    ZCacheSet * pDest = (ZCacheSet *)g_itemHelper.createItem(ZITEM_SET);
    dictIterator * pIter = dictGetIterator(cacheSet);
    // key sds value NULL
    dictEntry * pEntry = NULL;
    while ((pEntry = dictNext(pIter)) != NULL)
    {
        if (pOther->exist((sds)pEntry->key))
        {
            pDest->add(sdsdup((sds)pEntry->key));
        }
    }
    dictReleaseIterator(pIter);
    return pDest;
}

size_t ZCacheSet::getSize()
{
    return dictGetHashTableUsed(cacheSet);
}

size_t ZCacheSet::toBuffer(char * buf, size_t maxBufLen) const
{
	if (cacheSet == NULL)
	{
		return 0;
	}
	size_t setLen = dictGetHashTableUsed(cacheSet);
	size_t totalLen = sizeof(size_t) + 1;
	// 第一次遍历，拿到最大的bufLen
	dictIterator * pIter = dictGetIterator(cacheSet);
	// key sds value NULL
	dictEntry * pEntry = NULL;
	while ((pEntry = dictNext(pIter)) != NULL)
	{
		totalLen += sdslen((sds)pEntry->key) + 8;
	}
	dictReleaseIterator(pIter);
	if (totalLen > maxBufLen)
	{
		return totalLen;
	}
	buf[0] = itemType;
	buf += 1;
	*(size_t*)buf = setLen;
	buf += sizeof(size_t);
	size_t nowLen = 0;
	maxBufLen -= sizeof(size_t) + 1;
	
	pIter = dictGetIterator(cacheSet);
	// key sds value NULL
	pEntry = NULL;
	while ((pEntry = dictNext(pIter)) != NULL)
	{
		char * nowBuf = buf + nowLen;
		long sLen = sdslen((sds)pEntry->key);
		*(long *)nowBuf = sLen;
		nowBuf += sizeof(long);
		*(unsigned int *)nowBuf = sdskey((sds)pEntry->key);
		nowBuf += sizeof(unsigned int);
		memcpy(nowBuf, pEntry->key, sLen);
		nowLen += sLen + 8;
	}
	dictReleaseIterator(pIter);
	return totalLen;
}

ZCacheItem * ZCacheSet::copySelf() const
{
	ZCacheSet * pSet = new ZCacheSet;
	ZCheckPtr(pSet);
	if (cacheSet == NULL)
	{
		return pSet;
	}
	dictIterator * pIter = dictGetIterator(cacheSet);
	// key sds value ZCacheObj
	dictEntry * pEntry = NULL;
	while ((pEntry = dictNext(pIter)) != NULL)
	{
		// 深拷贝
		dictAdd(pSet->cacheSet, sdsdup((sds)pEntry->key), NULL);
	}
	dictReleaseIterator(pIter);
	return pSet;
}

int ZCacheSet::initFromBuffer(const char * buffer, size_t bufLen)
{
	if (bufLen < sizeof(size_t))
	{
		return -1;
	}
	if (cacheSet == NULL)
	{
		cacheSet = dictCreate(&setDictType, NULL);
	}
	else
	{
		dictRelease(cacheSet);
		cacheSet = dictCreate(&setDictType, NULL);
	}
	size_t dictLen = *(size_t*)buffer;
	buffer += sizeof(size_t);

	size_t readSize = sizeof(size_t);
	size_t itemBufLen = 0;

	for (size_t i = 0; i < dictLen; ++i)
	{
		// 读sds
		long sLen = *(long *)buffer;
		buffer += sizeof(long);
		unsigned int sKey = *(unsigned int *)buffer;
		buffer += sizeof(unsigned int);
		sds s = sdsnewlen(buffer, sLen);
		buffer += sLen;

		readSize += 8 + sLen;

		sdssetkey(s, sKey);
		dictAdd(cacheSet, s, NULL);
	}
	return readSize;
}

size_t ZCacheHash::toBuffer(char * buf, size_t maxBufLen) const
{
	if (cacheHash == NULL)
	{
		return 0;
	}
	size_t hashLen = dictGetHashTableUsed(cacheHash);
	size_t totalLen = sizeof(size_t) + 1;
	// 第一次遍历，拿到最大的bufLen
	dictIterator * pIter = dictGetIterator(cacheHash);
	// key sds value obj
	dictEntry * pEntry = NULL;
	while ((pEntry = dictNext(pIter)) != NULL)
	{
		totalLen += sdslen((sds)pEntry->key) + 8;
		totalLen += ((ZCacheObj *)pEntry->val)->pObj->toBuffer(NULL, 0);
	}
	dictReleaseIterator(pIter);
	if (totalLen > maxBufLen)
	{
		return totalLen;
	}
	buf[0] = itemType;
	buf += 1;
	*(size_t*)buf = hashLen;
	buf += sizeof(size_t);
	size_t nowLen = 0;
	maxBufLen -= sizeof(size_t) + 1;
	
	pIter = dictGetIterator(cacheHash);
	// key sds value NULL
	pEntry = NULL;
	while ((pEntry = dictNext(pIter)) != NULL)
	{
		char * nowBuf = buf + nowLen;
		long sLen = sdslen((sds)pEntry->key);
		*(long *)nowBuf = sLen;
		nowBuf += sizeof(long);
		*(unsigned int *)nowBuf = sdskey((sds)pEntry->key);
		nowBuf += sizeof(unsigned int);
		memcpy(nowBuf, pEntry->key, sLen);
		nowLen += sLen + 8;

		nowLen += ((ZCacheObj *)pEntry->val)->pObj->toBuffer(buf + nowLen, maxBufLen - nowLen);
	}
	dictReleaseIterator(pIter);
	return nowLen;
}

ZCacheItem * ZCacheHash::copySelf() const
{
	ZCacheHash * pHash = new ZCacheHash;
	ZCheckPtr(pHash);
	dictIterator * pIter = dictGetIterator(cacheHash);
	// key sds value ZCacheObj
	dictEntry * pEntry = NULL;
	while ((pEntry = dictNext(pIter)) != NULL)
	{
		// 深拷贝
		dictAdd(pHash->cacheHash, sdsdup((sds)pEntry->key), new ZCacheObj(((ZCacheObj *)pEntry->val)->pObj->copySelf()));
	}
	dictReleaseIterator(pIter);
	return pHash;
}

int ZCacheHash::add(sds key, ZCacheObj * pObj)
{
    int addRst = dictAdd(cacheHash, key, pObj);
    if (DICT_ERR == addRst)
    {
        return 0;
    }
    return 1;
}

int ZCacheHash::replace(sds key, ZCacheObj * pObj)
{
    int addRst = dictReplace(cacheHash, key, pObj);
    if (DICT_ERR == addRst)
    {
        return 0;
    }
    return 1;
}

size_t ZCacheHash::getAllKeys(ZCacheList * keyList)
{
    dictIterator * pIter = dictGetIterator(cacheHash);
    // key sds value obj
    dictEntry * pEntry = NULL;
    while ((pEntry = dictNext(pIter)) != NULL)
    {
        ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
        pBuf->cacheBuffer = (sds)pEntry->key;
        keyList->cacheList.push_back(new ZCacheObj(pBuf));
    }
    dictReleaseIterator(pIter);
    return dictGetHashTableUsed(cacheHash);
}

int ZCacheHash::rename(sds oldKey, sds newKey)
{
    dictEntry * oldEntry = dictFind(cacheHash, oldKey);
    if (oldEntry != NULL)
    {
        dictEntry * newEntry = dictFind(cacheHash, newKey);
        if (newEntry != NULL)
        {
            return -2;
        }
        void * val = oldEntry->val;
        // 保障老的val不被delete，但是老的key需要被delete
        oldEntry->val = NULL;
        dictDelete(cacheHash, oldKey);
        dictAdd(cacheHash, newKey, val);
        return 1;
    }
    return -1;
}

ZCacheObj * ZCacheHash::getValue(sds key)
{
    dictEntry * pRst = dictFind(cacheHash, key);
    if (pRst == NULL)
    {
        return NULL;
    }
    return (ZCacheObj *)pRst->val;
}

int ZCacheHash::remove(sds key)
{
    if (DICT_ERR == dictDelete(cacheHash, key))
    {
        return 0;
    }
    return 1;
}

bool ZCacheHash::exist(sds key)
{
    if (NULL == dictFind(cacheHash, key))
    {
        return false;
    }
    return true;
}

size_t ZCacheHash::getSize()
{
    return dictGetHashTableUsed(cacheHash);
}

int ZCacheHash::initFromBuffer(const char * buffer, size_t bufLen)
{
	if (bufLen < sizeof(size_t))
	{
		return -1;
	}
	if (cacheHash == NULL)
	{
		cacheHash = dictCreate(&hashDictType, NULL);
	}
	else
	{
		dictRelease(cacheHash);
		cacheHash = dictCreate(&hashDictType, NULL);
	}

	size_t dictLen = *(size_t*)buffer;
	buffer += sizeof(size_t);

	size_t readSize = sizeof(size_t);
	size_t itemBufLen = 0;

	for (size_t i = 0; i < dictLen; ++i)
	{
		// 读sds
		long sLen = *(long *)buffer;
		buffer += sizeof(long);
		unsigned int sKey = *(unsigned int *)buffer;
		buffer += sizeof(unsigned int);
		sds s = sdsnewlen(buffer, sLen);
		buffer += sLen;

		readSize += 8 + sLen;

		sdssetkey(s, sKey);

		// 获取obj

		ZCacheObj * pObj = new ZCacheObj(ZCacheItem::fromBuffer(buffer + readSize, bufLen - readSize, itemBufLen));
		readSize += itemBufLen;
		dictAdd(cacheHash, s, pObj);
	}
	return readSize;
}

void ZCacheItemDestroyer::operator()(ZCacheItem * pItem)
{
	g_itemHelper.deleteItem(pItem);
}
