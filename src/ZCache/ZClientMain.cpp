#include "EvoTCPCommon.h"
#include <CLog.h>
#include <ZRLoadMeasure.h>
#include "ZCacheCommand.h"
#include "ZClient.h"

bool g_performanceSynFlag = false;

class ZInnerClientTestThread : public ZRThread
{
public:
    ZClient * client;
    virtual void Run()
    {
        if (client->isSyn())
        {
            ZCacheCommand * pCmd = client->getCmd();
            while (1)
            {
                pCmd->cmdType = ZCMD_PING;
                ZCacheCommand * pRespCmd = client->requestSyn(pCmd);
                if (pRespCmd != NULL)
                {
                    client->deleteCmd(pRespCmd);
                }
            }
        }
        else
        {
            while (1)
            {
                ZCacheCommand * pCmd = client->getCmd();
                pCmd->cmdType = ZCMD_PING;
                if (client->requestAsyn(pCmd) == false)
                {
                    client->deleteCmd(pCmd);
                }
            }
        }
    }
};


bool OnCmd(ZCacheCommand *, ZCacheCommand *, void *)
{
    return true;
}




int clientPerformanceMain()
{
    ZInitSharedCommand();
    EvoTCPInitialize();

    printf("同步(1) 异步(0) \n");
    int synInt = 0;
    scanf("%d", &synInt);
    g_performanceSynFlag = synInt != 0;

    int clientNum = 0;

    printf("并发数 (<= 1024) \n");
    scanf("%d", &clientNum);
    if (clientNum > 1024)
    {
        clientNum = 1024;
    }

    char * serverIp = "127.0.0.1";
    unsigned short serverPort = 6000;


    ZClient * client[1024];
    ZInnerClientTestThread * testThread[1024];


    for (int i = 0; i < clientNum; ++i)
    {
        client[i] = new ZClient(g_performanceSynFlag);
        client[i]->connectToServer(serverIp, serverPort);
        client[i]->registerCallBackFun(OnCmd, NULL);
        testThread[i] = new ZInnerClientTestThread;
        testThread[i]->client = client[i];
    }

    RegisterTCPEventHook(ZCientTCPNetworkEventHook);

    for (int i = 0; i < clientNum; ++i)
    {
        testThread[i]->Start();
    }

    system("pause");

//     if (g_synFlag)
//     {
//         while (1)
//         {
//             for (int i = 0; i < clientNum; ++i)
//             {
//                 ZCacheCommand * pCmd = client[i]->getCmd();
//                 pCmd->cmdType = ZCMD_PING;
//                 ZCacheCommand * pRespCmd = client[i]->requestSyn(pCmd);
//                 if (pRespCmd != NULL)
//                 {
//                     client[i]->deleteCmd(pRespCmd);
//                 }
//             }
//         }
//     }
//     else
//     {
//         while (1)
//         {
//             for (int i = 0; i < clientNum; ++i)
//             {
//                 ZCacheCommand * pCmd = client[i]->getCmd();
//                 pCmd->cmdType = ZCMD_PING;
//                 client[i]->requestAsyn(pCmd);
//             }
//         }
//     }


    EvoTCPFinalize();
    return 1;
}