#include "ZClientTestCommon.h"

int g_inner_performanceThreadCount = 0;

class ZInnerPerformanceTestThread : public ZRThread
{
public:
    int index;

    ZRLoadMeasure lm;
    virtual void Run()
    {
        lm.setOutputRound(100000000);
        if (g_synFlag)
        {
            lm.setBlockName("SynPerformance");
            do
            {
                for (int i = 0; i < g_performanceRunTime; ++i)
                {
                    //lm.begin(0);
                    for (int j = index; j < g_clientNum; j += g_inner_performanceThreadCount)
                    {
                        g_clientCommand[j]->ping();
                    }
                    
                    //lm.end(0);
                }

                for (int i = 0; i < g_performanceRunTime; ++i)
                {
                    //lm.begin(1);
                    for (int j = index; j < g_clientNum; j += g_inner_performanceThreadCount)
                    {
                        g_clientCommand[j]->setNx(g_strKey, g_performanceStrItem);
                    }
                    //lm.end(1);
                }

                for (int i = 0; i < g_performanceRunTime; ++i)
                {
                    //lm.begin(2);
                    for (int j = index; j < g_clientNum; j += g_inner_performanceThreadCount)
                    {

                        ZCacheItem * pItem = g_clientCommand[j]->get(g_strKey);
                        ZDeleteItem(pItem);
                    }
                    //lm.end(2);
                }

                for (int i = 0; i < g_performanceRunTime; ++i)
                {
                    for (int j = index; j < g_clientNum; j += g_inner_performanceThreadCount)
                    {
                        //lm.begin(3);
                        g_clientCommand[j]->lpush(g_listEmptyKey, g_performanceStrItem);
                        //lm.end(3);
                        //lm.begin(4);
                        ZCacheItem * pItem = g_clientCommand[j]->lpop(g_listEmptyKey);
                        //lm.end(4);
                        ZDeleteItem(pItem);
                        //lm.begin(5);
                        g_clientCommand[j]->rpush(g_listEmptyKey, g_performanceStrItem);
                        //lm.end(5);
                        //lm.begin(6);
                        pItem = g_clientCommand[j]->rpop(g_listEmptyKey);
                        //lm.end(6);
                        ZDeleteItem(pItem);
                    }
                }

                for (int i = 0; i < g_performanceRunTime; ++i)
                {
                    for (int j = index; j < g_clientNum; j += g_inner_performanceThreadCount)
                    {
                        //lm.begin(7);
                        g_clientCommand[j]->sadd(g_setEmptyKey, g_performanceStrItem->cacheBuffer);
                        //lm.end(7);
                        //lm.begin(8);
                        g_clientCommand[j]->srem(g_setEmptyKey, g_performanceStrItem->cacheBuffer);
                        //lm.end(8);
                    }
                }

                for (int i = 0; i < g_performanceRunTime; ++i)
                {
                    for (int j = index; j < g_clientNum; j += g_inner_performanceThreadCount)
                    {

                        //lm.begin(9);
                        g_clientCommand[j]->dbSize();
                        //lm.end(9);
                    }
                }

                
            } while (g_performanceCircleFlag);
        }
        else
        {
            lm.setBlockName("AsynPerformance");
            do
            {
//                 for (int i = 0; i < g_performanceRunTime; ++i)
//                 {
//                     for (int j = index; j < g_clientNum; j += g_inner_performanceThreadCount)
//                     {
// 
//                         ZCacheCommand * pCmd = g_asynClient[j]->getCmd();
//                         pCmd->cmdType = ZCMD_PING;
//                         if (g_asynClient[j]->requestAsyn(pCmd) == false)
//                         {
//                             g_asynClient[j]->deleteCmd(pCmd);
//                         }
//                     }
//                 }


                for (int i = 0; i < g_performanceRunTime; ++i)
                {
                    for (int j = index; j < g_clientNum; j += g_inner_performanceThreadCount)
                    {


                        ZCacheCommand * pCmd = g_asynClient[j]->getCmd();
                        pCmd->cmdType = ZCMD_SETNX;
                        pCmd->itemLen = 2;
                        pCmd->pItemList[0] = g_listValues[1][0];
                        pCmd->pItemList[1] = g_performanceStrItem;
                        if (g_asynClient[j]->requestAsyn(pCmd) == false)
                        {
                            pCmd->itemLen = 0;
                            g_asynClient[j]->deleteCmd(pCmd);
                        }
                    }
                }


                for (int i = 0; i < g_performanceRunTime; ++i)
                {
                    for (int j = index; j < g_clientNum; j += g_inner_performanceThreadCount)
                    {


                        ZCacheCommand * pCmd = g_asynClient[j]->getCmd();
                        pCmd->cmdType = ZCMD_GET;
                        pCmd->itemLen = 1;
                        pCmd->pItemList[0] = g_listValues[1][0];
                        if (g_asynClient[j]->requestAsyn(pCmd) == false)
                        {
                            pCmd->itemLen = 0;
                            g_asynClient[j]->deleteCmd(pCmd);
                        }
                    }
                }

                INNER_LOG(CINFO, "不想写了，异步就测这么多吧.");

            } while (g_performanceCircleFlag);
        }
    }
};

bool OnAsynPerformanceCmd(ZCacheCommand * pCmd, ZCacheCommand *, void * )
{
    // 防止被删掉
    pCmd->itemLen = 0;
    return true;
}

ZClient::ZClientCallBackFun g_inner_originalFun[1024];
void * g_inner_originalParam[1024];
bool g_inner_originalFlag[1024];
ZInnerPerformanceTestThread * g_inner_perThread[1024];

void ZPerformanceSyn()
{
    g_inner_performanceThreadCount = 8;
    for (int i = 0; i < g_inner_performanceThreadCount; ++i)
    {
        g_inner_perThread[i] = new ZInnerPerformanceTestThread;
        g_inner_perThread[i]->index = i;
    }


    for (int i = 0; i < g_inner_performanceThreadCount; ++i)
    {
        g_inner_perThread[i]->Start();
    }


    for (int i = 0; i < g_inner_performanceThreadCount; ++i)
    {
        g_inner_perThread[i]->Join();
    }

    for (int i = 0; i < g_inner_performanceThreadCount; ++i)
    {
        delete g_inner_perThread[i];

    }
}

void ZPerformanceAsyn()
{
    g_inner_performanceThreadCount = 2;

    for (int i = 0; i < g_inner_performanceThreadCount; ++i)
    {
        g_inner_perThread[i] = new ZInnerPerformanceTestThread;
        g_inner_perThread[i]->index = i;
    }

    for (int i = 0; i < g_clientNum; ++i)
    {
        g_inner_originalFlag[i] = g_asynClient[i]->getRegisterCallBackFun(g_inner_originalFun[i], g_inner_originalParam[i]);
        
        g_asynClient[i]->registerCallBackFun(OnAsynPerformanceCmd, g_inner_perThread[i % g_inner_performanceThreadCount]);
    }


    for (int i = 0; i < g_inner_performanceThreadCount; ++i)
    {
        g_inner_perThread[i]->Start();
    }


    for (int i = 0; i < g_inner_performanceThreadCount; ++i)
    {
        g_inner_perThread[i]->Join();
    }

    for (int i = 0; i < g_inner_performanceThreadCount; ++i)
    {
        delete g_inner_perThread[i];
    }

    for (int i = 0; i < g_clientNum; ++i)
    {

        if (g_inner_originalFlag[i])
        {
            g_asynClient[0]->registerCallBackFun(g_inner_originalFun[i], g_inner_originalParam[i]);
        }
        else
        {
            g_asynClient[0]->ungisterCallBackFun();
        }
    }
}

TEST(ZAPerformanceTest, Normal)
{ 
    if (!g_performanceFlag)
    {
        return;
    }

    if (g_synFlag)
    {
        ZPerformanceSyn();
    }
    else
    {
        ZPerformanceAsyn();
    }

    






}