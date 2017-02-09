#include "ZClient.h"

const int g_maxDelay = 2000;
long long g_timeDelay[2000] = {0};
long long g_totalLen = 0;
long g_printInterval = 100000;

clock_t g_lastPrintTime = -1;


bool g_clientPerformaceFlag = true;


ZRCRITICALSECTION g_performanceTimeCS = ZRCreateCriticalSection();


int ZCientTCPNetworkEventHook(EvoTCPNetworkEvent netEvent)
{
    switch (netEvent.type)
    {
    case EVO_TCP_EVENT_ACCEPT:
    {

    }
    break;
    case EVO_TCP_EVENT_RECV:
    {
        ZClient * pClient = (ZClient *)netEvent.id.idParam;
        ZCacheCommand * pCmd = pClient->getCmd();
        ZUnPackCommand(pCmd, netEvent.buf, netEvent.bufLength);
        pClient->onResponseCommand(pCmd);
    }
    break;
    case EVO_TCP_EVENT_LISTENER_ERR:
        break;

    case EVO_TCP_EVENT_CONNECTION_ERR:
    {
        INNER_LOG(CINFO, "失去与服务器的连接 %s %d", netEvent.buf, netEvent.port);
    }
    break;
    default:
        break;
    }
    return 0;
}


void printPerformance(clock_t lastRequestTime)
{
    clock_t nowTime = clock();
    clock_t deltaReqTime = nowTime - lastRequestTime;
    ZRCS_ENTER(g_performanceTimeCS);
    if (deltaReqTime >= g_maxDelay)
    {
        deltaReqTime = g_maxDelay - 1;
    }
    if (deltaReqTime < 0)
    {
        deltaReqTime = 0;
    }
    ++g_timeDelay[deltaReqTime];
    if (g_lastPrintTime == -1)
    {
        g_lastPrintTime = nowTime;
    }
    if (++g_totalLen % g_printInterval == 0)
    {


        clock_t timeLen = nowTime - g_lastPrintTime;
        float totalPercent = 0.f;
        for (int i = 0; i < 200; ++i)
        {
            totalPercent += g_timeDelay[i] * 100.f / g_printInterval;
            if ((i < 10 || i % 10 == 0) && totalPercent < 99.9)
            {
                INNER_LOG(CINFO, "%f % <= %d ms ", totalPercent, i);
            }
        }

        INNER_LOG(CINFO, "\n\nRequest %lld times, last %lu take %ld ms, %f per sec.", g_totalLen, g_printInterval, timeLen, g_printInterval * 1000.f / timeLen);


        g_lastPrintTime = nowTime;
        memset(g_timeDelay, 0, sizeof(g_timeDelay));
    }
    ZRCS_LEAVE(g_performanceTimeCS);
}


ZClient::ZClient(bool synFlag)
    :m_synFlag(synFlag), m_validFlag(false), m_callBackParam(NULL), m_callBackFunFlag(NULL)
{
    m_id = INIT_CONNECTION_ID;
    SEMA_INIT(m_responseSema, 0, 0x7FFFFFFF);
    m_sendBuf = (char *)malloc(m_maxSendBufLen);
    if (synFlag)
    {
        m_synRequestCmd = new ZCacheCommand;
        m_synResponseCmd = new ZCacheCommand;
    }
    else
    {
        m_synResponseCmd = NULL;
        m_synRequestCmd = NULL;
    }
}

ZClient::~ZClient()
{
    setInvalid();
    lock();

    for (auto pCmd : m_requestCmdList)
    {
        ZClearCommand(pCmd);
        delete pCmd;
    }


    for (auto pCmd : m_emptyCmdList)
    {
        ZClearCommand(pCmd);
        delete pCmd;
    }

    if (m_synRequestCmd != NULL)
    {
        delete m_synRequestCmd;
    }
    if (m_synResponseCmd != NULL)
    {
        delete m_synResponseCmd;
    }



    unlock();

    SEMA_DESTROY(m_responseSema);
    free(m_sendBuf);


}

bool ZClient::isValid()
{
    return m_validFlag;
}

bool ZClient::isSyn()
{
    return m_synFlag;
}

void ZClient::setInvalid()
{
    m_validFlag = false;
    if (IdIsValid(m_id))
    {
        UnRegisterConnection(m_id);
    }
}

bool ZClient::connectToServer(const char * ip, unsigned short port)
{
    m_id = RegisterConnection(ip, port);
    if (IdIsValid(m_id) == false)
    {
        INNER_LOG(CWARNING, "连接服务器 %s:%d失败.", ip, (int)port);
        m_validFlag = false;
    }
    else
    {
        m_id.idParam = (void *)this;
        SetConnectionParam(m_id);
        m_validFlag = true;

    }
    return m_validFlag;
}

ZCacheCommand * ZClient::getCmd()
{

    // @debug

    return new ZCacheCommand;

    // @debug end
    // syn下
    if (m_synFlag)
    {
        ZCacheCommand * pCmd = m_synRequestCmd;
        if (pCmd != NULL)
        {
            m_synRequestCmd = NULL;
            return pCmd;
        }
        else
        {
            pCmd = m_synResponseCmd;
            m_synResponseCmd = NULL;
            return pCmd;
        }
    }
    ZCacheCommand * pCmd = NULL;
    lock();
    if (!m_emptyCmdList.empty())
    {
        pCmd = m_emptyCmdList.back();
        m_emptyCmdList.pop_back();
    }
    unlock();
    if (pCmd == NULL)
    {
        pCmd = new ZCacheCommand;
        ZCheckPtr(pCmd);
    }
    return pCmd;
}

void ZClient::deleteCmd(ZCacheCommand * pCmd)
{
    if (pCmd == NULL)
    {
        return;
    }
    // @debug
    ZClearCommand(pCmd);
    delete pCmd;
    return;
    // @debug end
    ZClearCommand(pCmd);
    // syn下
    if (m_synFlag)
    {
        if (m_synRequestCmd == NULL)
        {
            m_synRequestCmd = pCmd;
            return;
        }
        if (m_synResponseCmd == NULL)
        {
            m_synResponseCmd = pCmd;
        }
        return;
    }
    lock();
    m_emptyCmdList.push_back(pCmd);
    unlock();
}

ZCacheCommand * ZClient::requestSyn(ZCacheCommand * pCmd)
{
    if (!isValid() || !m_synFlag)
    {
        return NULL;
    }
    clock_t reqTime = 0;
    if (g_clientPerformaceFlag)
    {
        reqTime = clock();
    }
    size_t bufLen = ZPackCommand(pCmd, m_sendBuf, m_maxSendBufLen);
    if (bufLen > m_maxSendBufLen)
    {
        m_sendBuf = (char *)realloc(m_sendBuf, bufLen);
        m_maxSendBufLen = bufLen;
        ZPackCommand(pCmd, m_sendBuf, m_maxSendBufLen);
    }
    //deleteCmd(pCmd);
    if (::SendData(m_id, m_sendBuf, bufLen) < 0)
    {
        INNER_LOG(CWARNING, "向服务器发送数据失败.");
        setInvalid();
        return NULL;
    }
    SEMA_WAIT(m_responseSema);
    ZCacheCommand * pResCmd = m_synResponseCmd;
    m_synResponseCmd = NULL;
    if (g_clientPerformaceFlag)
    {
        printPerformance(reqTime);
    }
    if (pResCmd->cmdType == ZCMD_FULL)
    {
        INNER_LOG(CWARNING, "服务器忙，请求被主动拒绝.");
        deleteCmd(pResCmd);
        pResCmd = NULL;
    }
    return pResCmd;
}



bool ZClient::requestAsyn(ZCacheCommand * pCmd)
{

    if (!isValid() || m_synFlag)
    {
        return false;
    }

    if (!m_callBackFunFlag)
    {
        return false;
    }

    if (m_requestCmdList.size() > m_maxRequestSize)
    {
        Sleep(0);
        return false;
    }

    size_t bufLen = ZPackCommand(pCmd, m_sendBuf, m_maxSendBufLen);
    if (bufLen > m_maxSendBufLen)
    {
        m_sendBuf = (char *)realloc(m_sendBuf, bufLen);
        m_maxSendBufLen = bufLen;
        ZPackCommand(pCmd, m_sendBuf, m_maxSendBufLen);
    }

    clock_t reqTime = 0;
    if (g_clientPerformaceFlag)
    {
        reqTime = clock();
    }
    lock();
    //deleteCmd(pCmd);
    m_requestCmdList.push_back(pCmd);
    if (g_clientPerformaceFlag)
    {
        m_requestTimeList.push_back(reqTime);
    }
    unlock();
    if (::SendData(m_id, m_sendBuf, bufLen) < 0)
    {
        INNER_LOG(CWARNING, "向服务器发送数据失败.");
        setInvalid();
        return false;
    }
    return true;
}

void ZClient::onResponseCommand(ZCacheCommand * pRespCmd)
{
    if (!isValid())
    {
        deleteCmd(pRespCmd);
        return;
    }
    if (m_synFlag)
    {
        m_synResponseCmd = pRespCmd;
        SEMA_POST(m_responseSema);
    }
    else
    {
        clock_t reqTime = 0;
        lock();
        if (m_requestCmdList.empty())
        {
            INNER_LOG(CWARNING, "请求队列为空时收到服务器未知命令.");
        }
        ZCacheCommand * pReqCmd = m_requestCmdList.front();
        if (g_clientPerformaceFlag)
        {
            reqTime = m_requestTimeList.front();
            m_requestTimeList.pop_front();
        }
        m_requestCmdList.pop_front();
        unlock();
        if (g_clientPerformaceFlag)
        {
            printPerformance(reqTime);
        }
        if (pRespCmd->cmdType == ZCMD_FULL)
        {
            INNER_LOG(CWARNING, "服务器忙，请求被主动拒绝.");
            deleteCmd(pRespCmd);
            pRespCmd = NULL;
        }

        if (m_callBackFunFlag)
        {
            if (m_callBackFun(pReqCmd, pRespCmd, m_callBackParam))
            {
                // 只有返回true的时候再删除
                deleteCmd(pRespCmd);
                deleteCmd(pReqCmd);
            }
        }
        else
        {
            INNER_LOG(CWARNING, "当前没有回调函数用于处理服务器返回的命令.");
            deleteCmd(pRespCmd);
            deleteCmd(pReqCmd);
        }
    }
}

void ZClient::registerCallBackFun(ZClientCallBackFun callBackFun, void * param)
{
    if (!isValid() || m_synFlag)
    {
        return;
    }
    m_callBackFun = callBackFun;
    m_callBackParam = param;
    m_callBackFunFlag = true;
}

bool ZClient::getRegisterCallBackFun(ZClientCallBackFun & callBackFun, void * & param)
{
    if (!isValid() || m_synFlag || !m_callBackFunFlag)
    {
        return false;
    }
    callBackFun = m_callBackFun;
    param = m_callBackParam;
    return true;
}

void ZClient::ungisterCallBackFun()
{
    if (!isValid() || m_synFlag)
    {
        return;
    }
    m_callBackFunFlag = false;
}

