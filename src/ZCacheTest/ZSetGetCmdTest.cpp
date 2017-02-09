
#include "ZClientTestCommon.h"



TEST(ZCmdSet, Normal)
{
    ASSERT_TRUE(g_clientCommand[0]->set(g_noKey, g_strItem) > 0);
    ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);
    ASSERT_TRUE(g_clientCommand[0]->set(g_noKey, g_longItem) > 0);
    ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);
    ASSERT_TRUE(g_clientCommand[0]->set(g_noKey, g_list[5]) > 0);
    ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);
    ASSERT_TRUE(g_clientCommand[0]->set(g_noKey, g_set[5]) > 0);
    ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);
}


TEST(ZCmdSet, ExistFalse)
{
    ASSERT_TRUE(g_clientCommand[0]->set(g_strKey, g_strItem) == ZExistError);
}





TEST(ZCmdSetNx, Normal)
{
    ASSERT_TRUE(g_clientCommand[0]->setNx(g_noKey, g_strItem) > 0);
    ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);
    ASSERT_TRUE(g_clientCommand[0]->setNx(g_noKey, g_longItem) > 0);
    ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);
    ASSERT_TRUE(g_clientCommand[0]->setNx(g_noKey, g_list[5]) > 0);
    ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);
    ASSERT_TRUE(g_clientCommand[0]->setNx(g_noKey, g_set[5]) > 0);
    ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);

    ASSERT_EQ(g_clientCommand[0]->setNx(g_strKey, g_strItem), ZExistError);
    ASSERT_EQ(g_clientCommand[0]->setNx(g_longKey, g_longItem), ZExistError);
    ASSERT_EQ(g_clientCommand[0]->setNx(g_longMinKey, g_longMinItem), ZExistError);
    ASSERT_EQ(g_clientCommand[0]->setNx(g_longMaxKey, g_longMaxItem), ZExistError);


}


TEST(ZCmdGet, Normal)
{
    ZCacheItem * pItem = NULL;

    pItem = g_clientCommand[0]->get(g_strKey);
    ASSERT_TRUE(pItem != NULL);
    ASSERT_TRUE(pItem->itemType == ZITEM_BUFFER);
    ASSERT_TRUE(sdscmp(((ZCacheBuffer *)pItem)->cacheBuffer, g_strItem->cacheBuffer) == 0);
    ZDeleteItem(pItem);



    pItem = g_clientCommand[0]->get(g_longKey);
    ASSERT_TRUE(pItem != NULL);
    ASSERT_TRUE(pItem->itemType == ZITEM_LONG);
    ASSERT_TRUE(((ZCacheLong *)pItem)->cacheLong == 0);
    ZDeleteItem(pItem);


    pItem = g_clientCommand[0]->get(g_longMinKey);
    ASSERT_TRUE(pItem != NULL);
    ASSERT_TRUE(pItem->itemType == ZITEM_LONG);
    ASSERT_TRUE(((ZCacheLong *)pItem)->cacheLong == LONG_MIN);
    ZDeleteItem(pItem);

    pItem = g_clientCommand[0]->get(g_longMaxKey);
    ASSERT_TRUE(pItem != NULL);
    ASSERT_TRUE(pItem->itemType == ZITEM_LONG);
    ASSERT_TRUE(((ZCacheLong *)pItem)->cacheLong == LONG_MAX);
    ZDeleteItem(pItem);
}


TEST(ZCmdGet, NoKey)
{
    ZCacheItem * pItem = NULL;

    pItem = g_clientCommand[0]->get(g_noKey);
    ASSERT_TRUE(pItem == NULL);
}


TEST(ZCmdSetGetCheck, T_LONG)
{
    ASSERT_TRUE(g_clientCommand[0]->set(g_noKey, g_longItem) > 0);
    ZCacheItem * pItem = g_clientCommand[0]->get(g_noKey);
    ASSERT_TRUE(pItem != NULL);
    ASSERT_TRUE(pItem->itemType == ZITEM_LONG);
    ASSERT_TRUE(((ZCacheLong *)pItem)->cacheLong == 0);
    ZDeleteItem(pItem);
    ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);
}


TEST(ZCmdSetGetCheck, T_BUFFER)
{
    ASSERT_TRUE(g_clientCommand[0]->set(g_noKey, g_strItem) > 0);
    ZCacheItem * pItem = g_clientCommand[0]->get(g_noKey);
    ASSERT_TRUE(pItem != NULL);
    ASSERT_TRUE(pItem->itemType == ZITEM_BUFFER);
    ASSERT_TRUE(sdscmp(((ZCacheBuffer *)pItem)->cacheBuffer, g_strItem->cacheBuffer) == 0);
    ZDeleteItem(pItem);
    ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);

}


TEST(ZCmdSetGetCheck, T_LIST)
{
    for (int i = 0; i < 10; ++i)
    {
        ASSERT_TRUE(g_clientCommand[0]->set(g_noKey, g_list[i]) > 0);
        ZCacheItem * pItem = g_clientCommand[0]->get(g_noKey);


        ASSERT_TRUE(pItem != NULL);
        ASSERT_TRUE(pItem->itemType == ZITEM_LIST);
        ZCacheList * pList = (ZCacheList *)(pItem);
        ASSERT_EQ(pList->getSize(), i);

        int j = 0;
        for (auto pObj : pList->cacheList)
        {
            ASSERT_EQ(pObj->pObj->itemType, ZITEM_BUFFER);
            ZCacheBuffer * pBuffer = (ZCacheBuffer *)(pObj->pObj);
            ASSERT_TRUE(*g_listValues[i][j++] == *pBuffer);
        }

        ZDeleteItem(pItem);
        ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);

    }
}


TEST(ZCmdSetGetCheck, T_SET)
{
    for (int i = 0; i < 10; ++i)
    {
        ASSERT_TRUE(g_clientCommand[0]->set(g_noKey, g_set[i]) > 0);
        ZCacheItem * pItem = g_clientCommand[0]->get(g_noKey);


        ASSERT_TRUE(pItem != NULL);
        ASSERT_TRUE(pItem->itemType == ZITEM_SET);
        ZCacheSet * pSet = ZItemCast<ZCacheSet>(pItem);
        ASSERT_TRUE(pSet != NULL);

        dictIterator * pIter = dictGetIterator(pSet->cacheSet);
        dictEntry * pEntry = NULL;
        while (NULL != (pEntry = dictNext(pIter)))
        {
            // ≤È’“
            bool findRst = false;
            for (int j = 0; j < i; ++j)
            {
                if (sdscmp((sds)pEntry->key, g_setKeyValues[j]) == 0)
                {
                    findRst = true;
                    break;
                }
            }
            ASSERT_TRUE(findRst);
        }


        ZDeleteItem(pItem);
        ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);

    }
}
