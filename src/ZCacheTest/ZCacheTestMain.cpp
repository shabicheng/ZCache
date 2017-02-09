#include "gtest/gtest.h"
#include "EvoTCPCommon.h"
#include <CLog.h>
#include <ZRLoadMeasure.h>
#include "ZCacheCommand.h"
#include "ZClient.h"
#include "ZClientCommand.h"

#include "ZCacheItemHelper.h"

extern dictType setDictType;

bool g_synFlag = true;
ZClient * g_client[1024];
ZClient * g_asynClient[1024];
ZClientCommand * g_clientCommand[1024];
int g_clientNum = 50;

bool g_performanceFlag = true;
int g_performanceRunTime = 100000;
bool g_performanceCircleFlag = true;
int g_performanceBufferSize = 256;
ZCacheBuffer * g_performanceStrItem = NULL;

clock_t g_firstRunTime = clock();
int g_runTimes = 0;

sds g_noKey = NULL;
sds g_strKey = NULL;
sds g_longKey = NULL;
sds g_longMinKey = NULL;
sds g_longMaxKey = NULL;
sds g_listKey = NULL;
sds g_listEmptyKey = NULL;
sds g_setKey = NULL;
sds g_setEmptyKey = NULL;
sds g_setOneKey = NULL;

ZCacheBuffer * g_strItem = NULL;
ZCacheLong * g_longItem= NULL;
ZCacheLong * g_longMinItem = NULL;
ZCacheLong * g_longMaxItem = NULL;
ZCacheBuffer * g_listItem = NULL;


int g_listKeyNum = 1;
int g_setKeyNum = 1;

ZCacheBuffer * g_str[16];
ZCacheLong * g_long[20];
ZCacheList * g_list[10];
ZCacheBuffer * g_listValues[10][10];
ZCacheSet * g_set[10];
sds g_setKeyValues[10];


bool OnCmd(ZCacheCommand *, ZCacheCommand *, void *)
{
    return true;
}


class ZCacheClientEnvironment : public testing::Environment
{

public:

    virtual void SetUp()
    {

        printf("Init client .\n");

        if (!g_performanceFlag)
        {
            g_clientNum = 1;
        }

//         printf("客户端数量 (<= 1024) \n");
//         scanf("%d", &g_clientNum);
        if (g_clientNum > 1024)
        {
            g_clientNum = 1024;
        }
        if (g_clientNum <= 0)
        {
            g_clientNum = 1;
        }

        char * serverIp = "127.0.0.1";
        unsigned short serverPort = 6000;

        memset(g_client, 0, sizeof(g_client));
        memset(g_clientCommand, 0, sizeof(g_clientCommand));
        memset(g_asynClient, 0, sizeof(g_asynClient));

        for (int i = 0; i < 1; ++i)
        {
            g_client[i] = new ZClient(true);
            g_client[i]->connectToServer(serverIp, serverPort);
            g_client[i]->registerCallBackFun(OnCmd, g_client[i]);
            g_clientCommand[i] = new ZClientCommand();
            g_clientCommand[i]->setSynClient(g_client[i]);
            assert(g_client[i]->isValid());


            g_asynClient[i] = new ZClient(false);
            g_asynClient[i]->connectToServer(serverIp, serverPort);
            g_asynClient[i]->registerCallBackFun(OnCmd, g_asynClient[i]);
            assert(g_asynClient[i]->isValid());
        }

        for (int i = 1; i < g_clientNum; ++i)
        {
            if (g_synFlag)
            {
                g_client[i] = new ZClient(true);
                g_client[i]->connectToServer(serverIp, serverPort);
                g_client[i]->registerCallBackFun(OnCmd, g_client[i]);
                g_clientCommand[i] = new ZClientCommand();
                g_clientCommand[i]->setSynClient(g_client[i]);
                assert(g_client[i]->isValid());
            }
            else
            {
                g_asynClient[i] = new ZClient(false);
                g_asynClient[i]->connectToServer(serverIp, serverPort);
                g_asynClient[i]->registerCallBackFun(OnCmd, g_asynClient[i]);
                assert(g_asynClient[i]->isValid());

            }
        }

        RegisterTCPEventHook(ZCientTCPNetworkEventHook);

        printf("Init test context .\n");

        g_noKey = sdsnew("g_noKey");
        sdskeygen(g_noKey);
        g_strKey = sdsnew("g_strKey");
        sdskeygen(g_strKey);
        g_longKey = sdsnew("g_longKey");
        sdskeygen(g_longKey);
        g_longMinKey = sdsnew("g_longMinKey");
        sdskeygen(g_longMinKey);
        g_longMaxKey = sdsnew("g_longMaxKey");
        sdskeygen(g_longMaxKey);
        g_listKey = sdsnew("g_listKey");
        sdskeygen(g_listKey);
        g_listEmptyKey = sdsnew("g_listEmptyKey");
        sdskeygen(g_listEmptyKey);
        g_setKey = sdsnew("g_setKey");
        sdskeygen(g_setKey);
        g_setEmptyKey = sdsnew("g_setEmptyKey");
        sdskeygen(g_setEmptyKey);
        g_setOneKey = sdsnew("g_setOneKey");
        sdskeygen(g_setOneKey);

        sds performanceStr = sdsnewlen(NULL, g_performanceBufferSize);
        for (int i = 0; i < g_performanceBufferSize; ++i)
        {
            performanceStr[i] = i % 256;
        }
        sdskeygen(performanceStr);
        g_performanceStrItem = ZNewBuffer();
        g_performanceStrItem->setStringNoCopy(performanceStr);

        g_strItem = ZNewBuffer();

        sds str = sdsnew("zc");
        sdskeygen(str);
        g_strItem->setStringNoCopy(str);


        g_longItem = ZNewLong();
        g_longItem->setValue(0);
        g_longMinItem = ZNewLong();
        g_longMinItem->setValue(LONG_MIN);
        g_longMaxItem = ZNewLong();
        g_longMaxItem->setValue(LONG_MAX);


        g_listItem = ZNewBuffer();

        sds listStr = sdsnew("zclist");
        sdskeygen(listStr);
        g_listItem->setStringNoCopy(listStr);

        g_clientCommand[0]->clearDB();

        g_clientCommand[0]->set(g_strKey, g_strItem);

        g_clientCommand[0]->set(g_longKey, g_longItem);
        g_clientCommand[0]->set(g_longMaxKey, g_longMaxItem);
        g_clientCommand[0]->set(g_longMinKey, g_longMinItem);

        g_clientCommand[0]->rpush(g_listKey, g_listItem);

        // 构造空的key
        g_clientCommand[0]->rpush(g_listEmptyKey, g_listItem);
        ZDeleteItem(g_clientCommand[0]->rpop(g_listEmptyKey));

        g_clientCommand[0]->sadd(g_setKey, g_setOneKey);

        g_clientCommand[0]->sadd(g_setEmptyKey, g_setOneKey);
        g_clientCommand[0]->srem(g_setEmptyKey, g_setOneKey);


        for (int i = 0; i < 16; ++i)
        {
            int bufSize = 1 >> i;
            g_str[i] = ZNewBuffer();
            g_str[i]->setStringNoCopy(sdsnewlen(NULL, bufSize));
        }

        for (int i = 0; i < 20; ++i)
        {
            int bufSize = 1 >> i;
            g_long[i] = ZNewLong();
            g_long[i]->setValue(-10 + i);
        }


        for (int i = 0; i < 10; ++i)
        {
            
            g_list[i] = ZNewList();
            for (int j = 0; j < i; ++j)
            {
                ZCacheBuffer * pBuf = ZNewBuffer();
                g_listValues[i][j] = pBuf;
                char tmpBuf[32];
                sprintf(tmpBuf, "listItem_%d", j);
                pBuf->setStringNoCopy(sdsnew(tmpBuf));
                sdskeygen(pBuf->cacheBuffer);
                g_list[i]->pushBack(new ZCacheObj(pBuf));
            }
        }

        memset(g_setKeyValues, 0, sizeof(g_setKeyValues));
        for (int i = 0; i < 10; ++i)
        {
            g_set[i] = ZNewSet();
            g_set[i]->cacheSet = dictCreate(&setDictType, NULL);
            for (int j = 0; j < i; ++j)
            {
                if (g_setKeyValues[j] == NULL)
                {
                    char tmpBuf[32];
                    sprintf(tmpBuf, "setItem_%d", j);
                    g_setKeyValues[j] = sdsnew(tmpBuf);
                    sdskeygen(g_setKeyValues[j]);
                }
                g_set[i]->add(g_setKeyValues[j]);
            }
        }




        printf("Init done .\n");
    }

    virtual void TearDown()
    {
        for (int i = 0; i < g_clientNum; ++i)
        {
            if (g_clientCommand[i] != NULL)
            {
                delete g_clientCommand[i];

            }
            if (g_asynClient[i] != NULL)
            {
                delete g_asynClient[i];
            }
            // 在 CLIENT CMD中删除
            //delete g_client[i];
        }

        INNER_LOG(CINFO, "测试用例运行 %d次 已运行 %d ms", ++g_runTimes, clock() - g_firstRunTime);
    }

};



int main(int argc, char ** argv)
{

    ZInitSharedCommand();
    EvoTCPInitialize();


    printf("Running main() from gtest_main.cc\n");
    testing::AddGlobalTestEnvironment(new ZCacheClientEnvironment);
    testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();

    system("pause");

    EvoTCPFinalize();
    return 0;
}