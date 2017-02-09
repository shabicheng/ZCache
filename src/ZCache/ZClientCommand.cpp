#include "ZClientCommand.h"
#include "ZCommon.h"
#include "ZCacheItemHelper.h"


#include "ZClient.h"

#define ZCHECKCMD_OK_ERR(pCmd) (pCmd)->cmdType == ZCMD_OK ? 1 : (pCmd)->itemLen

ZClientCommand::ZClientCommand()
    :m_client(NULL)
{

}


ZClientCommand::~ZClientCommand()
{
    if (m_client != NULL)
    {
        delete m_client;
    }
}

bool ZClientCommand::setSynClient(ZClient * pSynClient)
{
    if (pSynClient == NULL || !pSynClient->isSyn())
    {
        return false;
    }
    m_client = pSynClient;

    return true;
}

bool ZClientCommand::ping()
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_PING;
    pCmd->itemLen = 0;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);


    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return true;
}

int ZClientCommand::clearDB()
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_CLEARDB;
    pCmd->itemLen = 0;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);


    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return true;
}

int ZClientCommand::set(sds key, ZCacheItem * pItem)
{
    if (pItem == NULL)
    {
        return -1;
    }
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_SET;
    pCmd->itemLen = 2;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(key);
    pBuf->setStringNoCopy(key);
    pCmd->pItemList[0] = pBuf;
    pCmd->pItemList[1] = pItem;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pCmd->itemLen = 1;
    pBuf->removeStringNoFree();

    int rst = -1;

    if (pRespCmd != NULL)
    {
        rst = ZCHECKCMD_OK_ERR(pRespCmd);
    }


    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

int ZClientCommand::setNx(sds key, ZCacheItem * pItem)
{
    if (pItem == NULL)
    {
        return -1;
    }
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_SETNX;
    pCmd->itemLen = 2;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(key);
    pBuf->setStringNoCopy(key);
    pCmd->pItemList[0] = pBuf;
    pCmd->pItemList[1] = pItem;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pCmd->itemLen = 1;
    pBuf->removeStringNoFree();

    int rst = -1;

    if (pRespCmd != NULL)
    {
        rst = ZCHECKCMD_OK_ERR(pRespCmd);
    }


    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

int ZClientCommand::remove(sds key)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_REMOVE;
    pCmd->itemLen = 1;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(key);
    pBuf->setStringNoCopy(key);
    pCmd->pItemList[0] = pBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    int rst = -1;

    if (pRespCmd != NULL)
    {
        rst = ZCHECKCMD_OK_ERR(pRespCmd);
    }

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

ZCacheItem * ZClientCommand::get(sds key, int * errorFlag /*= NULL*/)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_GET;
    pCmd->itemLen = 1;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(key);
    pBuf->setStringNoCopy(key);
    pCmd->pItemList[0] = pBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    ZCacheItem * pRst = NULL;

    if (errorFlag != NULL)
    {
        *errorFlag = 0;
    }

    if (pRespCmd != NULL)
    {
        if (pRespCmd->cmdType != ZCMD_OBJ || pRespCmd->itemLen != 1)
        {
            if (errorFlag != NULL)
            {
                *errorFlag = pRespCmd->itemLen;
            }
        }
        else
        {
            // 返回的item不能被自动销毁
            pRespCmd->itemLen = 0;
            pRst = pRespCmd->pItemList[0];

        }
    }

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return pRst;
}

bool ZClientCommand::exist(sds key)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_EXIST;
    pCmd->itemLen = 1;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(key);
    pBuf->setStringNoCopy(key);
    pCmd->pItemList[0] = pBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    bool rst = false;
    if (pRespCmd != NULL)
    {
        rst = pRespCmd->cmdType == ZCMD_OK;
    }

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

long ZClientCommand::inc(sds key, int * errorFlag /*= NULL*/)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_INC;
    pCmd->itemLen = 1;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(key);
    pBuf->setStringNoCopy(key);
    pCmd->pItemList[0] = pBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    long rst = -1;
    if (pRespCmd != NULL)
    {
        if (pRespCmd->cmdType == ZCMD_ERR)
        {
            if (errorFlag != NULL)
            {
                *errorFlag = pRespCmd->itemLen;
            }
        }
        rst = pRespCmd->itemLen;
    }
    else
    {
        if (errorFlag != NULL)
        {
            *errorFlag = -1;
        }
    }

    

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

long ZClientCommand::dec(sds key, int * errorFlag /*= NULL*/)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_DEC;
    pCmd->itemLen = 1;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(key);
    pBuf->setStringNoCopy(key);
    pCmd->pItemList[0] = pBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    long rst = -1;
    if (pRespCmd != NULL)
    {
        if (pRespCmd->cmdType == ZCMD_ERR)
        {
            if (errorFlag != NULL)
            {
                *errorFlag = pRespCmd->itemLen;
            }
        }
        rst = pRespCmd->itemLen;
    }
    else
    {
        if (errorFlag != NULL)
        {
            *errorFlag = -1;
        }
    }
    


    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}



long ZClientCommand::incBy(sds key, long val, int * errorFlag /*= NULL*/)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_INCBY;
    pCmd->itemLen = 2;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    ZCacheLong * pVal = (ZCacheLong *)g_itemHelper.createItem(ZITEM_LONG);
    sdskeygen(key);
    pBuf->setStringNoCopy(key);
    pVal->setValue(val);
    pCmd->pItemList[0] = pBuf;
    pCmd->pItemList[1] = pVal;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    long rst = -1;
    if (pRespCmd != NULL)
    {
        if (pRespCmd->cmdType == ZCMD_ERR)
        {
            if (errorFlag != NULL)
            {
                *errorFlag = pRespCmd->itemLen;
            }
        }
        rst = pRespCmd->itemLen;
    }
    else
    {
        if (errorFlag != NULL)
        {
            *errorFlag = -1;
        }
    }
    

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

long ZClientCommand::decBy(sds key, long val, int * errorFlag /*= NULL*/)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_DECBY;
    pCmd->itemLen = 2;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    ZCacheLong * pVal = (ZCacheLong *)g_itemHelper.createItem(ZITEM_LONG);
    sdskeygen(key);
    pBuf->setStringNoCopy(key);
    pVal->setValue(val);
    pCmd->pItemList[0] = pBuf;
    pCmd->pItemList[1] = pVal;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    long rst = -1;
    if (pRespCmd != NULL)
    {
        if (pRespCmd->cmdType == ZCMD_ERR)
        {
            if (errorFlag != NULL)
            {
                *errorFlag = pRespCmd->itemLen;
            }
        }
        rst = pRespCmd->itemLen;
    }
    else
    {
        if (errorFlag != NULL)
        {
            *errorFlag = -1;
        }
    }
    

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

bool ZClientCommand::select(long dbIndex)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_SElECT;
    pCmd->itemLen = 1;
    ZCacheLong * pVal = (ZCacheLong *)g_itemHelper.createItem(ZITEM_LONG);
    pVal->setValue(dbIndex);
    pCmd->pItemList[0] = pVal;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);

    bool rst = false;
    if (pRespCmd != NULL)
    {
        rst = pRespCmd->cmdType == ZCMD_OK;
    }


    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

long ZClientCommand::dbSize()
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_DBSIZE;
    pCmd->itemLen = 0;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);

    long rst = -1;
    if (pRespCmd != NULL)
    {
        rst = pRespCmd->itemLen;
    }
    
    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

int ZClientCommand::rename(sds oldKey, sds newKey)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_RENAME;
    pCmd->itemLen = 2;
    ZCacheBuffer * pOldBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    ZCacheBuffer * pNewBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(oldKey);
    sdskeygen(newKey);
    pOldBuf->setStringNoCopy(oldKey);
    pNewBuf->setStringNoCopy(newKey);
    pCmd->pItemList[0] = pOldBuf;
    pCmd->pItemList[1] = pNewBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pOldBuf->removeStringNoFree();
    pNewBuf->removeStringNoFree();

    int rst = -1;
    if (pRespCmd != NULL)
    {
        rst = ZCHECKCMD_OK_ERR(pRespCmd);
    }

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

long ZClientCommand::lpush(sds listKey, ZCacheItem * pItem)
{
    if (pItem == NULL)
    {
        return -1;
    }
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_LPUSH;
    pCmd->itemLen = 2;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(listKey);
    pBuf->setStringNoCopy(listKey);
    pCmd->pItemList[0] = pBuf;
    pCmd->pItemList[1] = pItem;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    // pItem不能被销毁，需要把len设为1
    pCmd->itemLen = 1;

    long rst = -1;
    if (pRespCmd != NULL)
    {
        rst = pRespCmd->itemLen;
    }

    

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}


long ZClientCommand::rpush(sds listKey, ZCacheItem * pItem)
{
    if (pItem == NULL)
    {
        return -1;
    }
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_RPUSH;
    pCmd->itemLen = 2;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(listKey);
    pBuf->setStringNoCopy(listKey);
    pCmd->pItemList[0] = pBuf;
    pCmd->pItemList[1] = pItem;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    // pItem不能被销毁，需要把len设为1
    pCmd->itemLen = 1;


    long rst = -1;
    if (pRespCmd != NULL)
    {
        rst = pRespCmd->itemLen;
    }

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

ZCacheItem * ZClientCommand::lpop(sds listKey, int * errorFlag /*= NULL*/)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_LPOP;
    pCmd->itemLen = 1;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(listKey);
    pBuf->setStringNoCopy(listKey);
    pCmd->pItemList[0] = pBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    if (errorFlag != NULL)
    {
        *errorFlag = 0;
    }
    ZCacheItem * pRst = NULL;
    if (pRespCmd != NULL)
    {
        if (pRespCmd->cmdType != ZCMD_OBJ || pRespCmd->itemLen != 1)
        {
            if (errorFlag != NULL)
            {
                *errorFlag = pRespCmd->itemLen;
            }
        }
        else
        {
            pRst = pRespCmd->pItemList[0];
        }
        pRespCmd->itemLen = 0;
    }

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return pRst;
}

ZCacheItem * ZClientCommand::rpop(sds listKey, int * errorFlag /*= NULL*/)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_RPOP;
    pCmd->itemLen = 1;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(listKey);
    pBuf->setStringNoCopy(listKey);
    pCmd->pItemList[0] = pBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();
    if (errorFlag != NULL)
    {
        *errorFlag = 0;
    }
    ZCacheItem * pRst = NULL;
    if (pRespCmd != NULL)
    {
        if (pRespCmd->cmdType != ZCMD_OBJ || pRespCmd->itemLen != 1)
        {
            if (errorFlag != NULL)
            {
                *errorFlag = pRespCmd->itemLen;
            }
        }
        else
        {
            pRst = pRespCmd->pItemList[0];
        }
        pRespCmd->itemLen = 0;
    }

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return pRst;
}

long ZClientCommand::llen(sds listKey)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_LLEN;
    pCmd->itemLen = 1;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(listKey);
    pBuf->setStringNoCopy(listKey);
    pCmd->pItemList[0] = pBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    long rst = -1;

    if (pRespCmd != NULL)
    {
        rst = pRespCmd->itemLen;
    }
    
    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

int ZClientCommand::lrange(sds listKey, int start, int end, ZCacheList * & pList)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_LRANGE;
    pCmd->itemLen = 3;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    ZCacheLong * pStart = (ZCacheLong *)g_itemHelper.createItem(ZITEM_LONG);
    ZCacheLong * pEnd = (ZCacheLong *)g_itemHelper.createItem(ZITEM_LONG);
    sdskeygen(listKey);
    pBuf->setStringNoCopy(listKey);
    pStart->setValue(start);
    pEnd->setValue(end);
    pCmd->pItemList[0] = pBuf;
    pCmd->pItemList[1] = pStart;
    pCmd->pItemList[2] = pEnd;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    int rst = -1;
    if (pRespCmd != NULL)
    {
        if (pRespCmd->cmdType == ZCMD_ERR)
        {
            rst = pRespCmd->itemLen;
        }
        else if (pRespCmd->cmdType == ZCMD_OBJ &&
            pRespCmd->itemLen == 1 &&
            pRespCmd->pItemList[0]->itemType == ZITEM_LIST)
        {
            pList = (ZCacheList *)(pRespCmd->pItemList[0]);
            // 直接指针赋值给pList，然后把len设为0，防止自动回收
            pRespCmd->itemLen = 0;
            rst = pList->getSize();
        }
    }


    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

unsigned short ZClientCommand::type(sds key)
{

    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_TYPE;
    pCmd->itemLen = 1;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(key);
    pBuf->setStringNoCopy(key);
    pCmd->pItemList[0] = pBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    unsigned short itemType = ZITEM_INVALID;

    if (pRespCmd != NULL)
    {
        if (pRespCmd->cmdType == ZCMD_ERR)
        {
        }
        else
        {
            itemType = (unsigned short)pRespCmd->itemLen;
        }
    }

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return itemType;
}

int ZClientCommand::sadd(sds setKey, sds key)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_SADD;
    pCmd->itemLen = 2;
    ZCacheBuffer * pSetBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    ZCacheBuffer * pKeyBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(setKey);
    sdskeygen(key);
    pSetBuf->setStringNoCopy(setKey);
    pKeyBuf->setStringNoCopy(key);
    pCmd->pItemList[0] = pSetBuf;
    pCmd->pItemList[1] = pKeyBuf;
    
    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pSetBuf->removeStringNoFree();
    pKeyBuf->removeStringNoFree();

    int rst = -1;
    if (pRespCmd != NULL)
    {
        rst = ZCHECKCMD_OK_ERR(pRespCmd);
    }

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

int ZClientCommand::sexist(sds setKey, sds key)
{

    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_SEXIST;
    pCmd->itemLen = 2;
    ZCacheBuffer * pSetBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    ZCacheBuffer * pKeyBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(setKey);
    sdskeygen(key);
    pSetBuf->setStringNoCopy(setKey);
    pKeyBuf->setStringNoCopy(key);
    pCmd->pItemList[0] = pSetBuf;
    pCmd->pItemList[1] = pKeyBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pSetBuf->removeStringNoFree();
    pKeyBuf->removeStringNoFree();

    int rst = -1;
    if (pRespCmd != NULL)
    {
        rst = ZCHECKCMD_OK_ERR(pRespCmd);
        if (rst == ZMinus1Error)
        {
            rst = 0;
        }
    }

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

int ZClientCommand::srem(sds setKey, sds key)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_SREM;
    pCmd->itemLen = 2;
    ZCacheBuffer * pSetBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    ZCacheBuffer * pKeyBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(setKey);
    sdskeygen(key);
    pSetBuf->setStringNoCopy(setKey);
    pKeyBuf->setStringNoCopy(key);
    pCmd->pItemList[0] = pSetBuf;
    pCmd->pItemList[1] = pKeyBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pSetBuf->removeStringNoFree();
    pKeyBuf->removeStringNoFree();

    int rst = -1;
    if (pRespCmd != NULL)
    {
        rst = ZCHECKCMD_OK_ERR(pRespCmd);
    }

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

int ZClientCommand::ssize(sds setKey)
{
    ZCheckPtr(m_client);
    ZCacheCommand * pCmd = m_client->getCmd();

    pCmd->cmdType = ZCMD_SSIZE;
    pCmd->itemLen = 1;
    ZCacheBuffer * pBuf = (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER);
    sdskeygen(setKey);
    pBuf->setStringNoCopy(setKey);
    pCmd->pItemList[0] = pBuf;

    ZCacheCommand * pRespCmd = m_client->requestSyn(pCmd);
    pBuf->removeStringNoFree();

    long rst = -1;
    if (pRespCmd != NULL)
    {
        rst = pRespCmd->itemLen;
    }

    m_client->deleteCmd(pCmd);
    m_client->deleteCmd(pRespCmd);
    return rst;
}

