
#include "ZClientTestCommon.h"

extern ZClientCommand * g_clientCommand[1024];

TEST(ZCmdPing, PingPong)
{
    ASSERT_TRUE(g_clientCommand[0]->ping());
}


TEST(ZCmdExist, ExistFalse)
{
    sds key = sdsnew("ZCmdExist_123");
    ASSERT_TRUE(!g_clientCommand[0]->exist(key));
    sdsfree(key);
}

TEST(ZCmdExist, ExistTrue)
{
    sds key = sdsnew("ZCmdExist");
    ASSERT_TRUE(!g_clientCommand[0]->exist(key));
    ASSERT_TRUE(g_clientCommand[0]->set(key, g_strItem) > 0);
    ASSERT_TRUE(g_clientCommand[0]->exist(key));
    ASSERT_TRUE(g_clientCommand[0]->remove(key) > 0);
    ASSERT_TRUE(!g_clientCommand[0]->exist(key));
    sdsfree(key);

    ASSERT_TRUE(g_clientCommand[0]->exist(g_strKey));
    ASSERT_TRUE(g_clientCommand[0]->exist(g_longKey));
    ASSERT_TRUE(g_clientCommand[0]->exist(g_longMinKey));
    ASSERT_TRUE(g_clientCommand[0]->exist(g_longMaxKey));
    ASSERT_TRUE(g_clientCommand[0]->exist(g_listKey));
    ASSERT_TRUE(g_clientCommand[0]->exist(g_listEmptyKey));
    ASSERT_TRUE(g_clientCommand[0]->exist(g_setKey));
    ASSERT_TRUE(g_clientCommand[0]->exist(g_setEmptyKey));
}



TEST(ZCmdInc, Normal)
{
    long rst = g_clientCommand[0]->inc(g_longKey);
    ASSERT_TRUE(rst == 1);

    rst = g_clientCommand[0]->dec(g_longKey);
    ASSERT_TRUE(rst == 0);


    rst = g_clientCommand[0]->incBy(g_longKey, 99);
    ASSERT_TRUE(rst == 99);


    rst = g_clientCommand[0]->incBy(g_longKey, -10);
    ASSERT_TRUE(rst == 89);


    rst = g_clientCommand[0]->decBy(g_longKey, -10);
    ASSERT_TRUE(rst == 99);


    rst = g_clientCommand[0]->decBy(g_longKey, 199);
    ASSERT_TRUE(rst == -100);


    rst = g_clientCommand[0]->decBy(g_longKey, -100);
    ASSERT_TRUE(rst == 0);


}

TEST(ZCmdInc, NoKey)
{
    int errorFlag = 0;

    errorFlag = 0;
    g_clientCommand[0]->inc(g_noKey, &errorFlag);
    ASSERT_TRUE(errorFlag == ZNoKeyError);

    errorFlag = 0;
    g_clientCommand[0]->dec(g_noKey, &errorFlag);
    ASSERT_TRUE(errorFlag == ZNoKeyError);

    errorFlag = 0;
    g_clientCommand[0]->incBy(g_noKey, 10, &errorFlag);
    ASSERT_TRUE(errorFlag == ZNoKeyError);

    errorFlag = 0;
    g_clientCommand[0]->decBy(g_noKey, 10, &errorFlag);
    ASSERT_TRUE(errorFlag == ZNoKeyError);
}

TEST(ZCmdInc, ErrType)
{
    int errorFlag = 0;

    errorFlag = 0;
    g_clientCommand[0]->inc(g_strKey, &errorFlag);
    ASSERT_TRUE(errorFlag == ZWrongTypeError);

    errorFlag = 0;
    g_clientCommand[0]->dec(g_strKey, &errorFlag);
    ASSERT_TRUE(errorFlag == ZWrongTypeError);

    errorFlag = 0;
    g_clientCommand[0]->incBy(g_strKey, 10, &errorFlag);
    ASSERT_TRUE(errorFlag == ZWrongTypeError);

    errorFlag = 0;
    g_clientCommand[0]->decBy(g_strKey, 10, &errorFlag);
    ASSERT_TRUE(errorFlag == ZWrongTypeError);
}

TEST(ZCmdInc, Overflow)
{
    int errorFlag = 0;

    errorFlag = 0;
    g_clientCommand[0]->inc(g_longMaxKey, &errorFlag);
    ASSERT_TRUE(errorFlag == ZOverflowError);

    errorFlag = 0;
    g_clientCommand[0]->dec(g_longMinKey, &errorFlag);
    ASSERT_TRUE(errorFlag == ZOverflowError);

    errorFlag = 0;
    g_clientCommand[0]->setNx(g_longMaxKey, g_longMaxItem);
    g_clientCommand[0]->incBy(g_longMaxKey, 10, &errorFlag);
    ASSERT_TRUE(errorFlag == ZOverflowError);

    errorFlag = 0;
    g_clientCommand[0]->setNx(g_longMinKey, g_longMinItem);
    g_clientCommand[0]->decBy(g_longMinKey, 10, &errorFlag);
    ASSERT_TRUE(errorFlag == ZOverflowError);

    // overflow之后再回来
    g_clientCommand[0]->setNx(g_longKey, g_longItem);
    g_clientCommand[0]->setNx(g_longMaxKey, g_longMaxItem);
    g_clientCommand[0]->setNx(g_longMinKey, g_longMinItem);
}

TEST(ZCmdSelect, Normal)
{
    for (int i = 0; i < 10; ++i)
    {
        ASSERT_TRUE(g_clientCommand[0]->select(i));
    }
    ASSERT_TRUE(g_clientCommand[0]->select(0));
}


TEST(ZCmdSelect, InvalidIndex)
{
    ASSERT_FALSE(g_clientCommand[0]->select(-1));
    ASSERT_FALSE(g_clientCommand[0]->select(1000));
}

TEST(ZCmdDBSize, Normal)
{
    ASSERT_TRUE(g_clientCommand[0]->dbSize() > 0);
    g_clientCommand[0]->select(1);
    ASSERT_TRUE(g_clientCommand[0]->dbSize() == 0);
    g_clientCommand[0]->select(0);

}

TEST(ZCmdRename, Normal)
{
    ASSERT_TRUE(g_clientCommand[0]->rename(g_strKey, g_noKey) > 0);
    ASSERT_TRUE(g_clientCommand[0]->rename(g_noKey, g_strKey) > 0);

}


TEST(ZCmdRename, NoOldKey)
{
    ASSERT_TRUE(g_clientCommand[0]->rename(g_noKey, g_strKey) == ZNoKeyError);
}



TEST(ZCmdRename, ExistNewKey)
{
    ASSERT_TRUE(g_clientCommand[0]->rename(g_strKey, g_listKey) == ZExistError);
}


TEST(ZCmdType, Normal)
{
    ASSERT_TRUE(g_clientCommand[0]->type(g_strKey) == ZITEM_BUFFER);
    ASSERT_TRUE(g_clientCommand[0]->type(g_longKey) == ZITEM_LONG);
    ASSERT_TRUE(g_clientCommand[0]->type(g_listKey) == ZITEM_LIST);
    ASSERT_TRUE(g_clientCommand[0]->type(g_setKey) == ZITEM_SET);
}


TEST(ZCmdType, NoKey)
{
    ASSERT_TRUE(g_clientCommand[0]->type(g_noKey) == ZITEM_INVALID);
}












