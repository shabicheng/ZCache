#pragma once
#include "ZCacheObject.h"
#include "sds.h"
#include "dict.h"


#define ZITEM_INVALID 0
#define ZITEM_BUFFER 1
#define ZITEM_LONG 2
#define ZITEM_LIST 3
#define ZITEM_SET 4
#define ZITEM_HASH 5

#define ZITEM_TYPE_NUM 5

class ZCacheItem
{
	friend class ZCacheItemHelper;
public:

	unsigned char itemType;

	static ZCacheItem * fromBuffer(const char * buffer, size_t bufLen, size_t & itemBufLen);

	virtual int initFromBuffer(const char * buffer, size_t bufLen) = 0;

	virtual size_t toBuffer(char * buf, size_t maxBufLen) const = 0;

	virtual ZCacheItem * copySelf()  const = 0;

    virtual void print(FILE * pFile) const
    {
        fprintf(pFile, "Base Item.");
    }

    virtual void clear() {}

protected:
	

	ZCacheItem()
		: itemType(ZITEM_INVALID)
	{

	}
	virtual ~ZCacheItem()
	{
        clear();
	}

private:


};



class ZCacheItemDestroyer
{
public:
	void operator()(ZCacheItem * pItem);
};


typedef ZCacheObject<ZCacheItem, ZCacheItemDestroyer> ZCacheObj;


class ZCacheBuffer : public ZCacheItem
{
	friend class ZCacheItemHelper;
public:

    bool operator == (const ZCacheBuffer & o) const;

    void setStringNoCopy(sds);
    void setStringCopy(sds);
    void removeStringNoFree();
    void removeString();
    sds getString();
    size_t getSize();

	virtual size_t toBuffer(char * buf, size_t maxBufLen) const override;

	virtual ZCacheItem * copySelf() const override;

	virtual int initFromBuffer(const char * buffer, size_t bufLen) override;

    virtual void print(FILE * pFile) const override
    {
        if (cacheBuffer == NULL)
        {
            fputs("null str", pFile);
        }
        else
        {
            fputs(cacheBuffer, pFile);
        }
    }

    virtual void clear()
    {
        if (cacheBuffer != NULL)
        {
            sdsfree(cacheBuffer);
            cacheBuffer = NULL;
        }
    }

	sds cacheBuffer;


protected:
	ZCacheBuffer()
		:cacheBuffer(NULL)
	{
		itemType = ZITEM_BUFFER;
	}


	virtual ~ZCacheBuffer()
	{
	}
};

class ZCacheLong : public ZCacheItem
{
	friend class ZCacheItemHelper;
public:


    void setValue(long val);
    long getValue();
    int incBy(long incVal);


	virtual size_t toBuffer(char * buf, size_t maxBufLen) const override;

	virtual ZCacheItem * copySelf() const override;

	virtual int initFromBuffer(const char * buffer, size_t bufLen) override;

    virtual void print(FILE * pFile) const override
    {
        fprintf(pFile, "%ld", cacheLong);
    }

    virtual void clear()
    {
        cacheLong = 0;
    }

	long cacheLong;

protected:
	ZCacheLong()
		:cacheLong(0)
	{
		itemType = ZITEM_LONG;
	}
	virtual ~ZCacheLong()
	{

	}

};


#include <list>
using std::list;

class ZCacheList : public ZCacheItem
{
	friend class ZCacheItemHelper;
public:

    void clearNoDelete();
    size_t pushBack(ZCacheObj * obj);
    size_t pushFont(ZCacheObj * obj);
    bool popBack(ZCacheObj * & obj);
    bool popFont(ZCacheObj * & obj);
    int getRange(int start, int end, ZCacheList * rangeList);
    int removeRange(int start, int end);
    size_t getSize();

	virtual size_t toBuffer(char * buf, size_t maxBufLen) const override;

	virtual ZCacheItem * copySelf() const override;

	virtual int initFromBuffer(const char * buffer, size_t bufLen) override;


    virtual void print(FILE * pFile) const override
    {
        for (auto & pObj : cacheList)
        {
            pObj->pObj->print(pFile);
            fprintf(pFile, ", ");
        }
    }

    virtual void clear()
    {
        for (auto & pObj : cacheList)
        {
            delete pObj;
        }
        cacheList.clear();
    }

	list<ZCacheObj *> cacheList;

protected:

	ZCacheList()
	{
		itemType = ZITEM_LIST;
	}

	virtual ~ZCacheList()
	{
	}

};


class ZCacheSet : public ZCacheItem
{
	friend class ZCacheItemHelper;
public:


    int add(sds key);
    int remove(sds key);
    bool exist(sds key);
    ZCacheSet * intersection(ZCacheSet * pOther);
    size_t getSize();

	virtual size_t toBuffer(char * buf, size_t maxBufLen) const override;

	virtual ZCacheItem * copySelf() const override;

	virtual int initFromBuffer(const char * buffer, size_t bufLen) override;


    virtual void print(FILE * pFile) const override
    {
        if (cacheSet != NULL)
        {
            dictIterator * pIter = dictGetIterator(cacheSet);
            // key sds value obj
            dictEntry * pEntry = NULL;
            while ((pEntry = dictNext(pIter)) != NULL)
            {
                fputs((sds)pEntry->key, pFile);
                fprintf(pFile, ", ");
            }
            dictReleaseIterator(pIter);
        }
        else
        {
            fputs("null set", pFile);
        }
    }

    virtual void clear()
    {
        if (cacheSet != NULL)
        {
            dictRelease(cacheSet);
            cacheSet = NULL;
        }
    }

	dict * cacheSet;

protected:

	ZCacheSet()
		:cacheSet(NULL)
	{
		itemType = ZITEM_SET;
	}

	virtual ~ZCacheSet()
	{
	}
};

 class ZCacheHash : public ZCacheItem
 {
	 friend class ZCacheItemHelper;
public:

    int add(sds key, ZCacheObj * pObj);
    int replace(sds key, ZCacheObj * pObj);

    size_t getAllKeys(ZCacheList * keyList);

    int rename(sds oldKey, sds newKey);
    ZCacheObj * getValue(sds key);
    int remove(sds key);
    bool exist(sds key);
    size_t getSize();

	virtual int initFromBuffer(const char * buffer, size_t bufLen) override;

	virtual size_t toBuffer(char * buf, size_t maxBufLen) const override;

	virtual ZCacheItem * copySelf() const override;

    virtual void print(FILE * pFile) const override
    {
        if (cacheHash != NULL)
        {
            dictIterator * pIter = dictGetIterator(cacheHash);
            // key sds value obj
            dictEntry * pEntry = NULL;
            while ((pEntry = dictNext(pIter)) != NULL)
            {
                fputs((sds)pEntry->key, pFile);
                fprintf(pFile, " : ");
                ((ZCacheObj *)pEntry->val)->pObj->print(pFile);
                fprintf(pFile, ", ");
            }
            dictReleaseIterator(pIter);
        }
        else
        {
            fputs("null hash", pFile);
        }
    }

    virtual void clear()
    {
        if (cacheHash != NULL)
        {
            dictRelease(cacheHash);
            cacheHash = NULL;
        }
    }

	dict * cacheHash;

protected:

	ZCacheHash()
		:cacheHash(NULL)
	{
		itemType = ZITEM_HASH;
	}

	virtual ~ZCacheHash()
	{
	}
}; 