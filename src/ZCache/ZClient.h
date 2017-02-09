#pragma once
#include "ZCacheCommand.h"

#include <functional>
#include <ZRLockAble.h>

int ZCientTCPNetworkEventHook(EvoTCPNetworkEvent netEvent);

using std::function;

// 目前支持两种方式：同步，异步
// 需要注意，如果使用同步，就需要一直使用同步，如果异步就一直异步
// 懒得分开实现了
class ZClient : public ZRLockAble
{
public:
    typedef function<bool(ZCacheCommand *, ZCacheCommand *, void *)> ZClientCallBackFun;
    ZClient(bool synFlag = false);
    ~ZClient();

    bool isValid();
    bool isSyn();
    void setInvalid();
    bool connectToServer(const char * ip, unsigned short port);

    ZCacheCommand * getCmd();
    void deleteCmd(ZCacheCommand * pCmd);

    // 同步请求默认不删除cmd
    ZCacheCommand * requestSyn(ZCacheCommand * pCmd);

    // 异步请求在回调完成后根据返回值删除两个CMD，用户若不想删除函数，函数返回false
    bool requestAsyn(ZCacheCommand * pCmd);

    void onResponseCommand(ZCacheCommand * pCmd);


    void registerCallBackFun(ZClientCallBackFun callBackFun, void * param);
    bool getRegisterCallBackFun(ZClientCallBackFun & callBackFun, void * & param);
    void ungisterCallBackFun();

protected:

    TCPConnectionId m_id;

    bool m_synFlag;
    bool m_validFlag;

    bool m_callBackFunFlag;
    ZClientCallBackFun m_callBackFun;
    void * m_callBackParam;

    SEMA m_responseSema;

    ZCacheCommand * m_synResponseCmd;
    ZCacheCommand * m_synRequestCmd;

    char * m_sendBuf;
    size_t m_maxSendBufLen = 1024;

    list<ZCacheCommand *> m_requestCmdList;
    list<clock_t> m_requestTimeList;

    list<ZCacheCommand *> m_emptyCmdList;

    const size_t m_maxRequestSize = 1024;



private:
};