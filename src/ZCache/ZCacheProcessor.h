#pragma once
#include "ZCacheCommand.h"
#include "EvoTCPCommon.h"
#include <ZRlibrary.h>


class ZCacheProcessor : public ZRThread
{
public:
	ZCacheProcessor();
	~ZCacheProcessor();

    virtual void Run();

    void onDataArrive(TCPConnectionId id, const char * buf, size_t bufLen);


    ZCacheCommand * getFreeCmd();
    void returnFreeCmd(ZCacheCommand * frerCmd);

protected:
    list<ZCacheCommand *> m_freeCmdList;
    ZRObjectList<ZCacheCommand *> m_cmdList;
    ZCacheCommand * m_sendCmd;
    char * m_sendBuf;
    size_t m_maxSendBufSize;

    int m_maxCmdListSize;
    int m_maxFreeCmdList;
    int m_maxGetWaitMS;
    int m_maxInsertWaitMS;
private:
};