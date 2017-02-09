#pragma once


#include "gtest/gtest.h"
#include "EvoTCPCommon.h"
#include <CLog.h>
#include <ZRLoadMeasure.h>
#include "ZCacheCommand.h"
#include "ZClient.h"
#include "ZClientCommand.h"

#include "ZCacheItemHelper.h"

extern bool g_synFlag;
extern ZClient * g_client[1024];
extern ZClient * g_asynClient[1024];
extern ZClientCommand * g_clientCommand[1024];
extern int g_clientNum;

extern bool g_performanceFlag;
extern int g_performanceRunTime;
extern int g_performanceBufferSize;
extern bool g_performanceCircleFlag;
extern ZCacheBuffer * g_performanceStrItem;

extern sds g_noKey;
extern sds g_strKey;
extern sds g_longKey;
extern sds g_longMinKey;
extern sds g_longMaxKey;
extern sds g_listKey;
extern sds g_listEmptyKey;
extern sds g_setKey;
extern sds g_setEmptyKey;
extern sds g_setOneKey;

extern ZCacheBuffer * g_strItem;
extern ZCacheLong * g_longItem;
extern ZCacheLong * g_longMinItem;
extern ZCacheLong * g_longMaxItem;
extern ZCacheBuffer * g_listItem;


extern int g_listKeyNum;
extern int g_setKeyNum;

extern ZCacheBuffer * g_str[16];
extern ZCacheLong * g_long[20];
extern ZCacheList * g_list[10];
extern ZCacheBuffer * g_listValues[10][10];
extern ZCacheSet * g_set[10];
extern sds g_setKeyValues[10];