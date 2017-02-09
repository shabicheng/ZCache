#include "ZClientTestCommon.h"

TEST(ZCmdSadd, Normal)
{
    ASSERT_TRUE(g_clientCommand[0]->sadd(g_setKey, g_setKeyValues[0]) > 0);
    ASSERT_TRUE(g_clientCommand[0]->sadd(g_noKey, g_setKeyValues[0]) > 0);

    ASSERT_TRUE(g_clientCommand[0]->sexist(g_setKey, g_setKeyValues[0]) > 0);
    ASSERT_TRUE(g_clientCommand[0]->sexist(g_noKey, g_setKeyValues[0]) > 0);

    g_clientCommand[0]->remove(g_noKey);
    g_clientCommand[0]->srem(g_setKey, g_setKeyValues[0]);
}

TEST(ZCmdSadd, Exist)
{
    ASSERT_TRUE(g_clientCommand[0]->sadd(g_setKey, g_setKeyValues[0]) > 0);
    ASSERT_TRUE(g_clientCommand[0]->sadd(g_setKey, g_setKeyValues[0]) == ZExistError);

    g_clientCommand[0]->srem(g_setKey, g_setKeyValues[0]);
}

TEST(ZCmdSadd, TypeError)
{
    ASSERT_TRUE(g_clientCommand[0]->sadd(g_strKey, g_setKeyValues[0]) == ZWrongTypeError);
}

TEST(ZCmdSrem, Normal)
{
    ASSERT_TRUE(g_clientCommand[0]->sadd(g_setKey, g_setKeyValues[0]) > 0);
    ASSERT_TRUE(g_clientCommand[0]->sadd(g_noKey, g_setKeyValues[0]) > 0);

    ASSERT_TRUE(g_clientCommand[0]->sexist(g_setKey, g_setKeyValues[0]) > 0);
    ASSERT_TRUE(g_clientCommand[0]->sexist(g_noKey, g_setKeyValues[0]) > 0);

    ASSERT_TRUE(g_clientCommand[0]->srem(g_setKey, g_setKeyValues[0]) > 0);
    ASSERT_TRUE(g_clientCommand[0]->srem(g_noKey, g_setKeyValues[0]) > 0);
    g_clientCommand[0]->remove(g_noKey);
}

TEST(ZCmdSrem, NoExist)
{

    ASSERT_TRUE(g_clientCommand[0]->srem(g_noKey, g_setKeyValues[0]) == ZNoKeyError);
    ASSERT_TRUE(g_clientCommand[0]->srem(g_setKey, g_setKeyValues[0]) == ZMinus1Error);
}

TEST(ZCmdSrem, TypeError)
{
    ASSERT_TRUE(g_clientCommand[0]->srem(g_strKey, g_setKeyValues[0]) == ZWrongTypeError);
}

TEST(ZCmdSsize, Normal)
{
    for (int i = 0; i < 10; ++i)
    {
        ASSERT_TRUE(g_clientCommand[0]->set(g_noKey, g_set[i]) > 0);

        ASSERT_TRUE(g_clientCommand[0]->ssize(g_noKey) == i);

        ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);
    }
}

TEST(ZCmdSsize, NoExist)
{
    ASSERT_TRUE(g_clientCommand[0]->ssize(g_noKey) == ZNoKeyError);
}

TEST(ZCmdSsize, TypeError)
{
    ASSERT_TRUE(g_clientCommand[0]->ssize(g_strKey) == ZWrongTypeError);
}

TEST(ZCmdSexist, Normal)
{
    for (int i = 0; i < 10; ++i)
    {
        ASSERT_TRUE(g_clientCommand[0]->set(g_noKey, g_set[i]) > 0);

        ASSERT_TRUE(g_clientCommand[0]->ssize(g_noKey) == i);

        for (int j = 0; j < i; ++j)
        {
            ASSERT_TRUE(g_clientCommand[0]->sexist(g_noKey, g_setKeyValues[j]) > 0);
        }

        ASSERT_TRUE(g_clientCommand[0]->remove(g_noKey) > 0);
    }
}

TEST(ZCmdSexist, NoExist)
{
    ASSERT_TRUE(g_clientCommand[0]->sexist(g_noKey, g_setKeyValues[0]) == ZNoKeyError);
}

TEST(ZCmdSexist, TypeError)
{
    ASSERT_TRUE(g_clientCommand[0]->sexist(g_strKey, g_setKeyValues[0]) == ZWrongTypeError);
}
