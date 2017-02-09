#include "ZCacheProcessor.h"
#include "ZCacheClient.h"
#include "ZCacheConfig.h"

ZCacheProcessor::ZCacheProcessor()
    : m_maxGetWaitMS(g_config.maxGetWaitMS), m_maxInsertWaitMS(g_config.maxInsertWaitMS),
    m_maxCmdListSize(g_config.maxCommandListSize), m_maxFreeCmdList(g_config.maxCommandListSize + 4),
    m_cmdList(g_config.maxCommandListSize)
{
    for (int i = 0; i < m_maxFreeCmdList; ++i)
    {
        m_freeCmdList.push_back(new ZCacheCommand);
    }

    m_maxSendBufSize = g_config.initSendPacketMaxSize;
    m_sendBuf = (char *)malloc(m_maxSendBufSize);
    m_sendCmd = new ZCacheCommand;
}

ZCacheProcessor::~ZCacheProcessor()
{
    for (auto pCmd : m_freeCmdList)
    {
        delete pCmd;
    }
    ZCacheCommand * pCmd = NULL;
    while (m_cmdList.getObject(pCmd, 0))
    {
        delete pCmd;
    }
    delete m_sendCmd;
    free(m_sendBuf);
}

void ZCacheProcessor::Run()
{
    while (!IsStoping())
    {
        ZCacheCommand * pCmd = NULL;
        if (m_cmdList.getObject(pCmd, m_maxGetWaitMS))
        {
            ZProcessCommand(pCmd, m_sendCmd, m_sendBuf, m_maxSendBufSize);
            returnFreeCmd(pCmd);
            ZClearCommand(m_sendCmd);
        }
    }
}

void ZCacheProcessor::onDataArrive(TCPConnectionId id, const char * buf, size_t bufLen)
{
    ZCacheCommand * pCmd = getFreeCmd();
    if (pCmd == NULL)
    {
        INNER_LOG(CERROR, "获取空的cmd失败.");
        return;
    }
    ZUnPackCommand(pCmd, buf, bufLen);
    pCmd->client = (ZCacheClient *)id.idParam;

    int tryTimes = 0;
    while (!m_cmdList.insertObject(pCmd, m_maxInsertWaitMS) && tryTimes++ < g_config.maxInsertTryTimes)
    {
        INNER_LOG(CINFO, "CMD队列拥塞，尝试再次插入.");
    }
    if (tryTimes > g_config.maxInsertTryTimes)
    {
        INNER_LOG(CWARNING, "CMD队列已满，拒绝本次命令.");
        ZRejectCommand(pCmd);
        returnFreeCmd(pCmd);
    }
}


ZCacheCommand * ZCacheProcessor::getFreeCmd()
{
    m_cmdList.lock();
    if (m_freeCmdList.empty())
    {
        m_cmdList.unlock();
        return NULL;
    }
    ZCacheCommand * pFreeCmd = m_freeCmdList.front();
    m_freeCmdList.pop_front();
    m_cmdList.unlock();
    return pFreeCmd;
}

void ZCacheProcessor::returnFreeCmd(ZCacheCommand * frerCmd)
{
    ZClearCommand(frerCmd);
    m_cmdList.lock();
    m_freeCmdList.push_back(frerCmd);
    m_cmdList.unlock();
}

