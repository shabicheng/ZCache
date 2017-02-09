#include "ZCacheCommand.h"
#include "ZCacheItemHelper.h"
#include "ZCacheServer.h"

#define ZSENDSHAREDCMD(pClient, sharedCmd) ::SendData((pClient)->clientID, (sharedCmd).buf, (sharedCmd).bufLen)

#define packCMDType(cmdType, buf) *(unsigned short *)(buf) = (unsigned short)(cmdType)
#define getCMDType(buf) *(unsigned short *)(buf)

#define printCMDLenError(cmdType, bufLen, cmdLen) INNER_LOG(CWARNING, "CMD 0x%04x 长度不一致 buf %d != cmd %d.", (long)(cmdType), (long)(bufLen), (long)(cmdLen))

ZCacheCommandShared g_sharedCommand;







void ZInitSharedCommand()
{
	packCMDType(ZCMD_OK, g_sharedCommand.ok.buf);
	g_sharedCommand.ok.bufLen = sizeof(unsigned short);
	packCMDType(ZCMD_FULL, g_sharedCommand.full.buf);
	g_sharedCommand.full.bufLen = sizeof(unsigned short);
	packCMDType(ZCMD_PING, g_sharedCommand.ping.buf);
	g_sharedCommand.ping.bufLen = sizeof(unsigned short);
	packCMDType(ZCMD_PONG, g_sharedCommand.pong.buf);
	g_sharedCommand.pong.bufLen = sizeof(unsigned short);



	packCMDType(ZCMD_LONG, g_sharedCommand.zero.buf);
	g_sharedCommand.zero.bufLen = sizeof(unsigned short) +
		ZPackLong(g_sharedCommand.zero.buf + sizeof(unsigned short), 8, 0);


	packCMDType(ZCMD_LONG, g_sharedCommand.one.buf);
	g_sharedCommand.one.bufLen = sizeof(unsigned short) +
		ZPackLong(g_sharedCommand.one.buf + sizeof(unsigned short), 8, 1);




	packCMDType(ZCMD_ERR, g_sharedCommand.minus1Err.buf);
	g_sharedCommand.minus1Err.bufLen = sizeof(unsigned short) +
		ZPackLong(g_sharedCommand.minus1Err.buf + sizeof(unsigned short), 8, -1);

	packCMDType(ZCMD_ERR, g_sharedCommand.minus2Err.buf);
	g_sharedCommand.minus2Err.bufLen = sizeof(unsigned short) +
		ZPackLong(g_sharedCommand.minus2Err.buf + sizeof(unsigned short), 8, -2);

	packCMDType(ZCMD_ERR, g_sharedCommand.minus3Err.buf);
	g_sharedCommand.minus3Err.bufLen = sizeof(unsigned short) +
		ZPackLong(g_sharedCommand.minus3Err.buf + sizeof(unsigned short), 8, -3);

	packCMDType(ZCMD_ERR, g_sharedCommand.minus4Err.buf);
	g_sharedCommand.minus4Err.bufLen = sizeof(unsigned short) +
		ZPackLong(g_sharedCommand.minus4Err.buf + sizeof(unsigned short), 8, -4);

	packCMDType(ZCMD_ERR, g_sharedCommand.wrongtypeErrM100.buf);
	g_sharedCommand.wrongtypeErrM100.bufLen = sizeof(unsigned short) +
		ZPackLong(g_sharedCommand.wrongtypeErrM100.buf + sizeof(unsigned short), 8, -100);

	packCMDType(ZCMD_ERR, g_sharedCommand.nokeyErrM101.buf);
	g_sharedCommand.nokeyErrM101.bufLen = sizeof(unsigned short) +
		ZPackLong(g_sharedCommand.nokeyErrM101.buf + sizeof(unsigned short), 8, -101);

	packCMDType(ZCMD_ERR, g_sharedCommand.nilErrorM102.buf);
	g_sharedCommand.nilErrorM102.bufLen = sizeof(unsigned short) +
        ZPackLong(g_sharedCommand.nilErrorM102.buf + sizeof(unsigned short), 8, -102);

    packCMDType(ZCMD_ERR, g_sharedCommand.nocmdErrM103.buf);
    g_sharedCommand.nocmdErrM103.bufLen = sizeof(unsigned short) +
        ZPackLong(g_sharedCommand.nocmdErrM103.buf + sizeof(unsigned short), 8, -103);

    packCMDType(ZCMD_ERR, g_sharedCommand.cmdparamErrM104.buf);
    g_sharedCommand.cmdparamErrM104.bufLen = sizeof(unsigned short) +
        ZPackLong(g_sharedCommand.cmdparamErrM104.buf + sizeof(unsigned short), 8, -104);
  
    packCMDType(ZCMD_ERR, g_sharedCommand.existErrM105.buf);
    g_sharedCommand.existErrM105.bufLen = sizeof(unsigned short) +
        ZPackLong(g_sharedCommand.existErrM105.buf + sizeof(unsigned short), 8, -105);
    
    packCMDType(ZCMD_ERR, g_sharedCommand.overflowErr106.buf);
    g_sharedCommand.overflowErr106.bufLen = sizeof(unsigned short) +
        ZPackLong(g_sharedCommand.overflowErr106.buf + sizeof(unsigned short), 8, -106);

}

int ZProcess_PING(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.pong);
}

int ZProcess_SET(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 2 || 
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do
    ZCacheObj * pObj = new ZCacheObj(pCmd->pItemList[1]);
    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    int rst = pDB->add(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer,
        pObj
    );
    pDB->unlock();

    // 由于OBJ已经被ZCacheObj * pObj接管了，所以第二个Item不能由CMD管理
    pCmd->itemLen = 1;


    if (rst == 0)
    {
        // 如果添加失败，需要删除这个obj
        delete pObj;
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.existErrM105);
    }
    else
    {
        // 如果添加成功
        // 这个sds已经被征用了，不能free
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->removeStringNoFree();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.ok);
    }
}

int ZProcess_SETNX(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 2 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do
    ZCacheObj * pObj = new ZCacheObj(pCmd->pItemList[1]);

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    int rst = pDB->replace(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer,
        pObj
    );
    pDB->unlock();

    pCmd->itemLen = 1;
    if (rst == 0)
    {
        // 当前已经存在key
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.existErrM105);
    }
    else
    {
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->removeStringNoFree();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.ok);
    }

}

int ZProcess_GET(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 1 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pObj = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);
    

    if (pObj == NULL)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }
    else
    {
        pSendCmd->cmdType = ZCMD_OBJ;
        pSendCmd->itemLen = 1;
        pSendCmd->pItemList[0] = pObj->pObj;
        size_t bufLen = ZPackCommand(pSendCmd, sendBuf, maxSendBufLen);
        if (bufLen > maxSendBufLen)
        {
            sendBuf = (char *)realloc(pSendCmd, bufLen);
            maxSendBufLen = bufLen;
            ZPackCommand(pSendCmd, sendBuf, maxSendBufLen);
        }
        pDB->unlock();
        int rst = ::SendData(pCmd->client->clientID, sendBuf, bufLen);
        pSendCmd->itemLen = 0;
        return rst;
    }
}

int ZProcess_EXIST(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 1 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    bool rst = pDB->exist(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);
    pDB->unlock();

    if (rst)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.ok);
    }
    else
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }
}

static int ZProcess_INCBY_VALUE(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen, long val, int itemNum)
{
    // check
    if (pCmd->itemLen != itemNum ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pObj = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);
    if (pObj == NULL)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }
    ZCacheItem * pItem = pObj->pObj;
    if (pItem->itemType != ZITEM_LONG)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.wrongtypeErrM100);
    }

    int rst = ((ZCacheLong *)pItem)->incBy(val);
    if (rst < 0)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.overflowErr106);
    }
    long rstVal = ((ZCacheLong *)pItem)->cacheLong;
    pDB->unlock();
    return ZPackLongAndSendCommand(pCmd->client, ZCMD_LONG, rstVal);
}

int ZProcess_INC(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    return ZProcess_INCBY_VALUE(pCmd, pSendCmd, sendBuf, maxSendBufLen, 1, 1);
}

int ZProcess_DEC(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    return ZProcess_INCBY_VALUE(pCmd, pSendCmd, sendBuf, maxSendBufLen, -1, 1);


}

int ZProcess_INCBY(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    if (pCmd->itemLen != 2 ||
        pCmd->pItemList[1]->itemType != ZITEM_LONG)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    long deltaVal = ((ZCacheLong *)(pCmd->pItemList[1]))->cacheLong;
    return ZProcess_INCBY_VALUE(pCmd, pSendCmd, sendBuf, maxSendBufLen, deltaVal, 2);
    
}

int ZProcess_DECBY(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    if (pCmd->itemLen != 2 ||
        pCmd->pItemList[1]->itemType != ZITEM_LONG)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    long deltaVal = -((ZCacheLong *)(pCmd->pItemList[1]))->cacheLong;
    return ZProcess_INCBY_VALUE(pCmd, pSendCmd, sendBuf, maxSendBufLen, deltaVal, 2);

}

int ZProcess_SElECT(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    if (pCmd->itemLen != 1 ||
        pCmd->pItemList[0]->itemType != ZITEM_LONG)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    long dbIndex = ((ZCacheLong *)(pCmd->pItemList[0]))->cacheLong;

    ZCacheDataBase * pDB = ZCacheServer::getInstance()->getDB(dbIndex);

    if (pDB == NULL)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.minus1Err);
    }

    pCmd->client->nowDataBase = pDB;
    pCmd->client->nowDataBaseIndex = dbIndex;

    return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.ok);


}

int ZProcess_KEYS(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    if (pCmd->itemLen != 0)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    ZCacheList * pList = (ZCacheList *)g_itemHelper.createItem(ZITEM_LIST);
    pSendCmd->cmdType = ZCMD_OBJ;
    pSendCmd->itemLen = 1;
    pSendCmd->pItemList[0] = pList;

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    size_t keySize = pDB->getAllKeys(pList);
    pDB->unlock();
    int rst = ZPackAndSendCommand(pCmd->client, pSendCmd, sendBuf, maxSendBufLen);
    // 手动清除list，因为内部所有的都是引用，需要先调用clearNoDelete

    for (auto & item : pList->cacheList)
    {
        ((ZCacheBuffer *)(item->pObj))->cacheBuffer = NULL;
    }

    // pList->clearNoDelete();
    return rst;
}

int ZProcess_DBSIZE(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    if (pCmd->itemLen != 0)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    size_t keySize = pDB->getSize();
    pDB->unlock();


    return ZPackLongAndSendCommand(pCmd->client, ZCMD_LONG, keySize);

}

int ZProcess_RENAME(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 2 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER ||
        pCmd->pItemList[1]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do
    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    int rst = pDB->rename(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer,
        ((ZCacheBuffer *)(pCmd->pItemList[1]))->cacheBuffer);
    pDB->unlock();
    if (rst == -1)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }
    if (rst == -2)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.existErrM105);
    }
    // 如果rename成功，那第二个key就被放到dictionary中了
    pCmd->itemLen = 1;
    return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.ok);


}

int ZProcess_LPUSH(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 2 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pItem = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);


    if (pItem == NULL)
    {
        pItem = new ZCacheObj(g_itemHelper.createItem(ZITEM_LIST));
        ZCheckPtr(pItem);

        // 这还需要记得，把新建的list加入到dict中
        pDB->add(((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer, pItem);
        // 这个sds已经被征用了，不能free
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->removeStringNoFree();
    }

    if (pItem->pObj->itemType != ZITEM_LIST)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.wrongtypeErrM100);
    }


    ZCacheObj * pObj = new ZCacheObj(pCmd->pItemList[1]);
    ZCacheList * pList = (ZCacheList *)pItem->pObj;
    pCmd->itemLen = 1;

    size_t listSize = pList->pushFont(pObj);

    pDB->unlock();
    return ZPackLongAndSendCommand(pCmd->client, ZCMD_LONG, (long)listSize);
    
}

int ZProcess_RPUSH(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 2 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pItem = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);


    if (pItem == NULL)
    {
        pItem = new ZCacheObj(g_itemHelper.createItem(ZITEM_LIST));
        ZCheckPtr(pItem);

        // 这还需要记得，把新建的list加入到dict中
        pDB->add(((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer, pItem);
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->removeStringNoFree();
    }

    if (pItem->pObj->itemType != ZITEM_LIST)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.wrongtypeErrM100);
    }


    ZCacheObj * pObj = new ZCacheObj(pCmd->pItemList[1]);
    ZCacheList * pList = (ZCacheList *)pItem->pObj;
    pCmd->itemLen = 1;

    size_t listSize = pList->pushBack(pObj);

    pDB->unlock();
    return ZPackLongAndSendCommand(pCmd->client, ZCMD_LONG, (long)listSize);


}

int ZProcess_LPOP(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 1 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pItem = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);


    if (pItem == NULL)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }

    if (pItem->pObj->itemType != ZITEM_LIST)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.wrongtypeErrM100);
    }


    ZCacheList * pList = (ZCacheList *)pItem->pObj;

    ZCacheObj * pObj = NULL;
    bool rst = pList->popFont(pObj);

    pDB->unlock();
    if (rst)
    {
        pSendCmd->cmdType = ZCMD_OBJ;
        pSendCmd->itemLen = 1;
        pSendCmd->pItemList[0] = pObj->pObj;
        int sendRst = ZPackAndSendCommand(pCmd->client, pSendCmd, sendBuf, maxSendBufLen);
        pSendCmd->itemLen = 0;
        return sendRst;
    }
    else
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nilErrorM102);
    }

}

int ZProcess_RPOP(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 1 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pItem = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);


    if (pItem == NULL)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }

    if (pItem->pObj->itemType != ZITEM_LIST)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.wrongtypeErrM100);
    }


    ZCacheList * pList = (ZCacheList *)pItem->pObj;

    ZCacheObj * pObj = NULL;
    bool rst = pList->popBack(pObj);
    pDB->unlock();

    if (rst)
    {
        pSendCmd->cmdType = ZCMD_OBJ;
        pSendCmd->itemLen = 1;
        pSendCmd->pItemList[0] = pObj->pObj;
        int sendRst = ZPackAndSendCommand(pCmd->client, pSendCmd, sendBuf, maxSendBufLen);
        pSendCmd->itemLen = 0;
        return sendRst;
    }
    else
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nilErrorM102);
    }
}

int ZProcess_LLEN(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 1 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pItem = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);


    if (pItem == NULL)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }

    if (pItem->pObj->itemType != ZITEM_LIST)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.wrongtypeErrM100);
    }


    ZCacheList * pList = (ZCacheList *)pItem->pObj;

    size_t listSize = pList->getSize();

    pDB->unlock();

    return ZPackLongAndSendCommand(pCmd->client, ZCMD_LONG, (long)listSize);
}

int ZProcess_LRANGE(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 3 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER ||
        pCmd->pItemList[1]->itemType != ZITEM_LONG ||
        pCmd->pItemList[2]->itemType != ZITEM_LONG)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pItem = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);


    if (pItem == NULL)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }

    if (pItem->pObj->itemType != ZITEM_LIST)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.wrongtypeErrM100);
    }


    ZCacheList * pList = (ZCacheList *)pItem->pObj;

    ZCacheList * pRangeList = (ZCacheList *)g_itemHelper.createItem(ZITEM_LIST);

    int rst = pList->getRange(((ZCacheLong *)(pCmd->pItemList[1]))->cacheLong,
        ((ZCacheLong *)(pCmd->pItemList[2]))->cacheLong, pRangeList);

    if (rst < 0)
    {
        pDB->unlock();
        pRangeList->clearNoDelete();
        ZDeleteItem(pRangeList);
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }


    pSendCmd->itemLen = 1;
    pSendCmd->pItemList[0] = pRangeList;

    size_t bufLen = ZPackCommand(pSendCmd, sendBuf, maxSendBufLen);
    if (bufLen > maxSendBufLen)
    {
        sendBuf = (char *)realloc(pSendCmd, bufLen);
        maxSendBufLen = bufLen;
        ZPackCommand(pSendCmd, sendBuf, maxSendBufLen);
    }

    pDB->unlock();
    // 这边手动销毁
    pSendCmd->itemLen = 0;
    pRangeList->clearNoDelete();
    ZDeleteItem(pRangeList);
    return ::SendData(pCmd->client->clientID, sendBuf, bufLen);



}

int ZProcess_TYPE(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 1 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pItem = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);
    long itemType = pItem == NULL ? -1 : (long)pItem->pObj->itemType;
    pDB->unlock();
    if (itemType < 0)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }

    return ZPackLongAndSendCommand(pCmd->client, ZCMD_LONG, itemType);

}

extern dictType setDictType;

int ZProcess_SADD(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{

    // check
    if (pCmd->itemLen != 2 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER ||
        pCmd->pItemList[1]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pItem = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);


    if (pItem == NULL)
    {
        pItem = new ZCacheObj(g_itemHelper.createItem(ZITEM_SET));
        ZCheckPtr(pItem);
        ZCacheSet * pNewSet = (ZCacheSet *)pItem->pObj;
        pNewSet->cacheSet = dictCreate(&setDictType, NULL);
        ZCheckPtr(pNewSet->cacheSet);

        // 这还需要记得，把新建的list加入到dict中
        pDB->add(((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer, pItem);
        // 这个sds已经被征用了，不能free
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->removeStringNoFree();
    }

    if (pItem->pObj->itemType != ZITEM_SET)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.wrongtypeErrM100);
    }


    ZCacheSet * pSet = (ZCacheSet *)pItem->pObj;

    int rst = pSet->add(((ZCacheBuffer *)(pCmd->pItemList[1]))->cacheBuffer);
    pDB->unlock();
    if (rst == 1)
    {
        // 已经加入成功，占用该sds key
        ((ZCacheBuffer *)(pCmd->pItemList[1]))->removeStringNoFree();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.ok);
    }
    else
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.existErrM105);
    }

}

int ZProcess_SREM(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 2 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER ||
        pCmd->pItemList[1]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pItem = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);


    if (pItem == NULL)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }


    if (pItem->pObj->itemType != ZITEM_SET)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.wrongtypeErrM100);
    }


    ZCacheSet * pSet = (ZCacheSet *)pItem->pObj;

    int rst = pSet->remove(((ZCacheBuffer *)(pCmd->pItemList[1]))->cacheBuffer);
    pDB->unlock();
    if (rst == 1)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.ok);
    }
    else
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.minus1Err);
    }
}

int ZProcess_SSIZE(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 1 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pItem = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);

    if (pItem == NULL)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }

    if (pItem->pObj->itemType != ZITEM_SET)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.wrongtypeErrM100);
    }


    ZCacheSet * pSet = (ZCacheSet *)pItem->pObj;

    size_t setSize = pSet->getSize();
    pDB->unlock();
    return ZPackLongAndSendCommand(pCmd->client, ZCMD_LONG, (long)setSize);
}

int ZProcess_REMOVE(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 1 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    int rst = pDB->remove(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);


    pDB->unlock();
    if (rst == 1)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.ok);
    }
    else
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }
}

int ZProcess_SEXIST(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 2 ||
        pCmd->pItemList[0]->itemType != ZITEM_BUFFER ||
        pCmd->pItemList[1]->itemType != ZITEM_BUFFER)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();
    ZCacheObj * pItem = pDB->getValue(
        ((ZCacheBuffer *)(pCmd->pItemList[0]))->cacheBuffer);

    if (pItem == NULL)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nokeyErrM101);
    }

    if (pItem->pObj->itemType != ZITEM_SET)
    {
        pDB->unlock();
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.wrongtypeErrM100);
    }


    ZCacheSet * pSet = (ZCacheSet *)pItem->pObj;

    bool rst = pSet->exist(((ZCacheBuffer *)(pCmd->pItemList[1]))->cacheBuffer);
    pDB->unlock();
    if (rst)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.ok);
    }
    else
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.minus1Err);
    }
}

int ZProcess_CLEARDB(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    // check
    if (pCmd->itemLen != 0)
    {
        return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.cmdparamErrM104);
    }

    // do

    ZCacheDataBase * pDB = pCmd->client->nowDataBase;
    pDB->lock();

    ZCacheServer::getInstance()->clearDB(pDB);
    pDB->unlock();
    return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.ok);
}

int ZPackAndSendCommand(ZCacheClient * pClient, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    size_t bufLen = ZPackCommand(pSendCmd, sendBuf, maxSendBufLen);
    if (bufLen > maxSendBufLen)
    {
        sendBuf = (char *)realloc(sendBuf, bufLen);
        maxSendBufLen = bufLen;
        ZPackCommand(pSendCmd, sendBuf, maxSendBufLen);
    }
    return ::SendData(pClient->clientID, sendBuf, bufLen);
}

int ZPackLongAndSendCommand(ZCacheClient * pClient, unsigned short cmdType, long value)
{
    char buf[sizeof(unsigned short) + sizeof(long)];
    char * tmpBuf = buf;
    *(unsigned short *)tmpBuf = cmdType;
    tmpBuf += sizeof(unsigned short);
    *(long *)tmpBuf = value;
    return ::SendData(pClient->clientID, buf, sizeof(buf));
}

int ZUnPackCommand(ZCacheCommand * pCmd, const char * buf, size_t bufLen)
{
    int rst = -1;
    // ERR LONG 只带6个字节
    unsigned short cmdType = getCMDType(buf);
    pCmd->cmdType = cmdType;
    if (cmdType == ZCMD_ERR || cmdType == ZCMD_LONG)
    {
        if (bufLen != sizeof(unsigned short) + sizeof(long))
        {
            printCMDLenError(cmdType, bufLen, sizeof(unsigned short) + sizeof(long));
            return -1;
        }
        buf += sizeof(unsigned short);
        bufLen -= sizeof(unsigned short);
        pCmd->itemLen = *(long *)buf;
    }
    // 其他的除了ZCMD_OBJ外Base之上只有2个字节
    else if (cmdType != ZCMD_OBJ && cmdType < ZCMD_BASE)
    {
        if (bufLen != sizeof(unsigned short))
        {
            printCMDLenError(cmdType, bufLen, sizeof(unsigned short));
            return -1;
        }
    }
    // BASE之下的直接解析
    else
    {
        if (bufLen < sizeof(unsigned short) + sizeof(long))
        {
            printCMDLenError(cmdType, bufLen, sizeof(unsigned short) + sizeof(long));
            return -1;
        }
        buf += sizeof(unsigned short);
        bufLen -= sizeof(unsigned short);
        pCmd->itemLen = *(long *)buf;
        buf += sizeof(long);
        bufLen -= sizeof(long);
        if (pCmd->itemLen > MAX_COMMAND_ITEM_SIZE)
        {
            INNER_LOG(CWARNING, "CMD 0x%04x item 长度%d越界.", (long)cmdType, pCmd->itemLen);
            pCmd->itemLen = 0;
            return -1;
        }
        size_t nowSize = 0;
        size_t itemSize = 0;
        for (long i = 0; i < pCmd->itemLen; ++i)
        {
            pCmd->pItemList[i] = ZCacheItem::fromBuffer(buf + nowSize, bufLen - nowSize, itemSize);
            if (pCmd->pItemList[i] == NULL)
            {
                INNER_LOG(CWARNING, "CMD 0x%04x 第%d个item解析失败.", (long)cmdType, i);
                return -1;
            }
            nowSize += itemSize;
        }
        if (nowSize != bufLen)
        {
            printCMDLenError(cmdType, nowSize, bufLen);
            return -1;
        }
    }

    return (int)bufLen;
}

size_t ZPackCommand(ZCacheCommand * pCmd, char * buf, size_t maxBufLen)
{
    unsigned short cmdType = pCmd->cmdType;
    if (cmdType == ZCMD_ERR || cmdType == ZCMD_LONG)
    {
        if (maxBufLen < sizeof(unsigned short) + sizeof(long))
        {
            return sizeof(unsigned short) + sizeof(long);
        }
        *(unsigned short *)buf = cmdType;
        buf += sizeof(unsigned short);
        *(long *)buf = pCmd->itemLen;
        return sizeof(unsigned short) + sizeof(long);
    }
    // 其他的除了ZCMD_OBJ外Base之上只有2个字节
    else if (cmdType != ZCMD_OBJ && cmdType < ZCMD_BASE)
    {
        if (maxBufLen < sizeof(unsigned short))
        {
            return sizeof(unsigned short);
        }
        *(unsigned short *)buf = cmdType;
        return sizeof(unsigned short);
    }
    // BASE之下的直接解析
    else
    {

        size_t packLen = sizeof(unsigned short) + sizeof(long);
        for (long i = 0; i < pCmd->itemLen; ++i)
        {
            packLen += pCmd->pItemList[i]->toBuffer(NULL, 0);
        }
        if (packLen > maxBufLen)
        {
            return packLen;
        }
        *(unsigned short *)buf = pCmd->cmdType;
        buf += sizeof(unsigned short);
        *(long *)buf = pCmd->itemLen;
        buf += sizeof(long);

        size_t nowPackLen = 0;
        maxBufLen -= sizeof(unsigned short) + sizeof(long);

        for (long i = 0; i < pCmd->itemLen; ++i)
        {
            nowPackLen += pCmd->pItemList[i]->toBuffer(buf + nowPackLen, maxBufLen - nowPackLen);
        }
        return packLen;
    }

}

int ZClearCommand(ZCacheCommand * pCmd)
{
    // 不涉及OBJ的不需要走下面的delete，这里特别需要考虑的是type = long 时，这个时候itemlen 代表的是long的值，这个时候需要特别注意
    if (pCmd == NULL || (pCmd->cmdType < ZCMD_BASE && pCmd->cmdType != ZCMD_OBJ))
    {
        return 1;
    }
	pCmd->client = NULL;
	for (long i = 0; i < pCmd->itemLen; ++i)
	{
		g_itemHelper.deleteItem(pCmd->pItemList[i]);
		pCmd->pItemList[i] = NULL;
	}
	pCmd->itemLen = 0;
    return 1;
}

int ZProcessCommand(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen)
{
    switch (pCmd->cmdType)
    {
    case ZCMD_PING:
        return ZProcess_PING(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_SET:
        return ZProcess_SET(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_SETNX:
        return ZProcess_SETNX(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_EXIST:
        return ZProcess_EXIST(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_GET:
        return ZProcess_GET(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_INC:
        return ZProcess_INC(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_DEC:
        return ZProcess_DEC(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_INCBY:
        return ZProcess_INCBY(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_DECBY:
        return ZProcess_DECBY(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_SElECT:
        return ZProcess_SElECT(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_KEYS:
        return ZProcess_KEYS(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_DBSIZE:
        return ZProcess_DBSIZE(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_RENAME:
        return ZProcess_RENAME(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_LPUSH:
        return ZProcess_LPUSH(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_RPUSH:
        return ZProcess_RPUSH(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_LPOP:
        return ZProcess_LPOP(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_RPOP:
        return ZProcess_RPOP(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_LLEN:
        return ZProcess_LLEN(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_LRANGE:
        return ZProcess_LRANGE(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_TYPE:
        return ZProcess_TYPE(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_SADD:
        return ZProcess_SADD(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_SREM:
        return ZProcess_SREM(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_SSIZE:
        return ZProcess_SSIZE(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_REMOVE:
        return ZProcess_REMOVE(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_SEXIST:
        return ZProcess_SEXIST(pCmd, pSendCmd, sendBuf, maxSendBufLen);
    case ZCMD_CLEARDB:
        return ZProcess_CLEARDB(pCmd, pSendCmd, sendBuf, maxSendBufLen);

    default:
        INNER_LOG(CWARNING, "未知的命令 0x%04x", (long)pCmd->cmdType);
    }


    return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.nocmdErrM103);

}

int ZRejectCommand(ZCacheCommand * pCmd)
{
	return ZSENDSHAREDCMD(pCmd->client, g_sharedCommand.full);
}

size_t ZPackLong(char * buf, size_t maxBufLen, long val)
{
	*(long *)buf = val;
	return sizeof(long);
}

int ZUnPackLong(const char * buf, size_t bufLen, long & val)
{
	val = *(long *)buf;
	return sizeof(long);
}

size_t ZPackString(char * buf, size_t maxBufLen, const sds s)
{
	long sLen = sdslen(s);
	if (sLen + 8 > (long)maxBufLen)
	{
		return sLen + 8;
	}
	*(long *)buf = sLen;
	buf += sizeof(long);
	*(unsigned int *)buf = sdskey(s);
	buf += sizeof(unsigned int);
	memcpy(buf, s, sLen);
	return sLen + 8;
}

size_t ZPackStringAndGenKey(char * buf, size_t maxBufLen, const sds s)
{
	sdskeygen(s);

	return ZPackString(buf, maxBufLen, s);
}

int ZUnPackString(const char * buf, size_t bufLen, sds & s)
{
	long sLen = *(long *)buf;
	buf += sizeof(long);
	if (sLen + 8 < (long)bufLen)
	{
		return -1;
	}
	unsigned int sKey = *(unsigned int *)buf;
	buf += sizeof(unsigned int);
	s = sdsnewlen(buf, sLen);
	sdssetkey(s, sKey);
	return sLen + 8;
}
