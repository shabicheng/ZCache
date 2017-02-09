#include "ZClientTestCommon.h"



bool OnZASynTestCmd(ZCacheCommand *, ZCacheCommand *, void * pSema)
{
    SEMA_POST((SEMA)pSema);
    return true;
}

TEST(ZSynTest, Normal)
{
    ASSERT_TRUE(g_clientCommand[0]->ping());
}

TEST(ZASynTest, Normal)
{
    ZClient::ZClientCallBackFun originalFun;
    void * originalParam;
    bool originalFlag = g_asynClient[0]->getRegisterCallBackFun(originalFun, originalParam);

    SEMA sema;
    SEMA_INIT(sema, 0, 1);
    ZCacheCommand * pCmd = g_asynClient[0]->getCmd();
    pCmd->cmdType = ZCMD_PING;

    g_asynClient[0]->registerCallBackFun(OnZASynTestCmd, sema);
    
    ASSERT_TRUE(g_asynClient[0]->requestAsyn(pCmd));

    ASSERT_TRUE(SEMA_WAIT_TIME(sema, 1000) == SEMA_WAIT_OK);

    

    if (originalFlag)
    {
        g_asynClient[0]->registerCallBackFun(originalFun, originalParam);
    }
    else
    {
        g_asynClient[0]->ungisterCallBackFun();
    }

    SEMA_DESTROY(sema);




}