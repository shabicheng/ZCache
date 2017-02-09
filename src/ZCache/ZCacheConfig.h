#pragma once
#include "sds.h"



class ZCacheConfig
{
public:
	ZCacheConfig();
	~ZCacheConfig();

    static int readConfig(ZCacheConfig & config);


    int maxCommandListSize = 256;
    int maxGetWaitMS = 1000;
    int maxInsertWaitMS = 100;
    int maxInsertTryTimes = 5;
    int clientHeartBeatTimes = 5;
    int initSendPacketMaxSize = 1024;
	int databaseCount = 16;
    int itemCacheClearSize = 1024 * 128;
    int itemCacheClearPercent = 50;

	sds listenIp = "127.0.0.1";
	unsigned short listenPort = 6000;
	int processorNum = 4;

protected:
	    
private:
};

extern ZCacheConfig g_config;