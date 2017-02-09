#pragma once
#include "ZCacheCommand.h"

#include <functional>
#include <ZRLockAble.h>

int ZCientTCPNetworkEventHook(EvoTCPNetworkEvent netEvent);

using std::function;

// Ŀǰ֧�����ַ�ʽ��ͬ�����첽
// ��Ҫע�⣬���ʹ��ͬ��������Ҫһֱʹ��ͬ��������첽��һֱ�첽
// ���÷ֿ�ʵ����
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

    // ͬ������Ĭ�ϲ�ɾ��cmd
    ZCacheCommand * requestSyn(ZCacheCommand * pCmd);

    // �첽�����ڻص���ɺ���ݷ���ֵɾ������CMD���û�������ɾ����������������false
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