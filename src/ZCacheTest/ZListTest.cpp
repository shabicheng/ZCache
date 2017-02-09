
#include "ZClientTestCommon.h"


TEST(ZCmdPush, Normal)
{
    ASSERT_EQ(g_clientCommand[0]->lpush(g_listEmptyKey, g_strItem), 1);
    ASSERT_EQ(g_clientCommand[0]->lpush(g_listKey, g_strItem), 2);
    ASSERT_EQ(g_clientCommand[0]->rpush(g_listEmptyKey, g_strItem), 2);
    ASSERT_EQ(g_clientCommand[0]->rpush(g_listKey, g_strItem), 3);

    // pop

    ZCacheItem * pItem = NULL;

    pItem = g_clientCommand[0]->lpop(g_listEmptyKey);
    ASSERT_TRUE(pItem != NULL);
    ZDeleteItem(pItem);


    pItem = g_clientCommand[0]->lpop(g_listKey);
    ASSERT_TRUE(pItem != NULL);
    ZDeleteItem(pItem);


    pItem = g_clientCommand[0]->rpop(g_listEmptyKey);
    ASSERT_TRUE(pItem != NULL);
    ZDeleteItem(pItem);

    
    pItem = g_clientCommand[0]->rpop(g_listKey);
    ASSERT_TRUE(pItem != NULL);
    ZDeleteItem(pItem);


    ASSERT_EQ(g_clientCommand[0]->llen(g_listKey), 1);
    ASSERT_EQ(g_clientCommand[0]->llen(g_listEmptyKey), 0);


}


TEST(ZCmdPush, NoKey)
{
    ASSERT_EQ(g_clientCommand[0]->lpush(g_noKey, g_strItem), 1);
    ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);
}

TEST(ZCmdPush, WrongType)
{
    ASSERT_EQ(g_clientCommand[0]->lpush(g_strKey, g_strItem), ZWrongTypeError);
    ASSERT_EQ(g_clientCommand[0]->rpush(g_strKey, g_strItem), ZWrongTypeError);
}


TEST(ZCmdPop, Normal)
{

    ASSERT_EQ(g_clientCommand[0]->lpush(g_listEmptyKey, g_strItem), 1);
    ASSERT_EQ(g_clientCommand[0]->lpush(g_listKey, g_strItem), 2);
    ASSERT_EQ(g_clientCommand[0]->rpush(g_listEmptyKey, g_longItem), 2);
    ASSERT_EQ(g_clientCommand[0]->rpush(g_listKey, g_longItem), 3);

    // pop

    ZCacheItem * pItem = NULL;

    pItem = g_clientCommand[0]->lpop(g_listEmptyKey);
    ASSERT_TRUE(pItem != NULL);
    ASSERT_TRUE(pItem->itemType == ZITEM_BUFFER);
    ASSERT_TRUE(sdscmp(((ZCacheBuffer *)pItem)->cacheBuffer, g_strItem->cacheBuffer) == 0);

    ZDeleteItem(pItem);


    pItem = g_clientCommand[0]->lpop(g_listKey);
    ASSERT_TRUE(pItem != NULL);
    ASSERT_TRUE(pItem->itemType == ZITEM_BUFFER);
    ASSERT_TRUE(sdscmp(((ZCacheBuffer *)pItem)->cacheBuffer, g_strItem->cacheBuffer) == 0);

    ZDeleteItem(pItem);


    pItem = g_clientCommand[0]->rpop(g_listEmptyKey);
    ASSERT_TRUE(pItem != NULL);
    ASSERT_TRUE(pItem->itemType == ZITEM_LONG);
    ASSERT_TRUE(((ZCacheLong *)pItem)->cacheLong == 0);
    ZDeleteItem(pItem);


    pItem = g_clientCommand[0]->rpop(g_listKey);
    ASSERT_TRUE(pItem != NULL);
    ASSERT_TRUE(pItem->itemType == ZITEM_LONG);
    ASSERT_TRUE(((ZCacheLong *)pItem)->cacheLong == 0);
    ZDeleteItem(pItem);


    ASSERT_EQ(g_clientCommand[0]->llen(g_listKey), 1);
    ASSERT_EQ(g_clientCommand[0]->llen(g_listEmptyKey), 0);
}


TEST(ZCmdPop, NoKey)
{
    int errorFlag = 0;
    ASSERT_TRUE(g_clientCommand[0]->lpop(g_noKey, &errorFlag) == NULL);
    ASSERT_EQ(errorFlag, ZNoKeyError);
    errorFlag = 0;
    ASSERT_TRUE(g_clientCommand[0]->rpop(g_noKey, &errorFlag) == NULL);
    ASSERT_EQ(errorFlag, ZNoKeyError);

}

TEST(ZCmdPop, WrongType)
{
    int errorFlag = 0;
    ASSERT_TRUE(g_clientCommand[0]->lpop(g_longKey, &errorFlag) == NULL);
    ASSERT_EQ(errorFlag, ZWrongTypeError);
    errorFlag = 0;
    ASSERT_TRUE(g_clientCommand[0]->rpop(g_strKey, &errorFlag) == NULL);
    ASSERT_EQ(errorFlag, ZWrongTypeError);
}


TEST(ZCmdPop, EmptyErr)
{
    int errorFlag = 0;
    ASSERT_TRUE(g_clientCommand[0]->lpop(g_listEmptyKey, &errorFlag) == NULL);
    ASSERT_EQ(errorFlag, ZNilError);
    errorFlag = 0;
    ASSERT_TRUE(g_clientCommand[0]->rpop(g_listEmptyKey, &errorFlag) == NULL);
    ASSERT_EQ(errorFlag, ZNilError);
}



TEST(ZCmdLLen, Normal)
{
    ASSERT_EQ(g_clientCommand[0]->llen(g_listKey), 1);
    ASSERT_EQ(g_clientCommand[0]->llen(g_listEmptyKey), 0);
    
}


TEST(ZCmdLLen, NoKey)
{
    ASSERT_EQ(g_clientCommand[0]->llen(g_noKey), ZNoKeyError);
}


TEST(ZCmdLLen, WrongType)
{
    ASSERT_EQ(g_clientCommand[0]->llen(g_setKey), ZWrongTypeError);
}



TEST(ZCmdLRange, Normal)
{
    ZCacheList * pList = NULL;
    
    g_clientCommand[0]->set(g_noKey, g_list[9]);
    for (int i = 0; i < 9; ++i)
    {
        for (int j = i + 1; j < 10; ++j)
        {
            pList = NULL;
            ASSERT_EQ(j - i, g_clientCommand[0]->lrange(g_noKey, i, j, pList));
            ASSERT_TRUE(pList != NULL);
            ASSERT_EQ(pList->getSize(), j - i);

            int k = i;
            for (auto pObj : pList->cacheList)
            {
                ASSERT_EQ(pObj->pObj->itemType, ZITEM_BUFFER);
                ZCacheBuffer * pBuffer = (ZCacheBuffer *)(pObj->pObj);
                ASSERT_TRUE(*g_listValues[9][k++] == *pBuffer);
            }
            ZDeleteItem(pList);

        }
    }
    g_clientCommand[0]->remove(g_noKey);
}


TEST(ZCmdLRange, NoKey)
{
    ZCacheList * pList = NULL;
    ASSERT_EQ(ZNoKeyError, g_clientCommand[0]->lrange(g_noKey, 0, 1, pList));
}


TEST(ZCmdLRange, WrongType)
{
    ZCacheList * pList = NULL;
    ASSERT_EQ(ZWrongTypeError, g_clientCommand[0]->lrange(g_strKey, 0, 1, pList));
}


TEST(ZCmdLRange, InvalidParam)
{
    ZCacheList * pList = NULL;
    ASSERT_EQ(ZParamError, g_clientCommand[0]->lrange(g_listKey, 0, 100, pList));
}

