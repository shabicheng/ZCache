#pragma once
#include "ZCacheClient.h"
#include "ZCommon.h"


struct ZCacheCommand
{
    ZCacheCommand()
        :client(NULL), itemLen(0)
    {

    }
	ZCacheItem * pItemList[MAX_COMMAND_ITEM_SIZE];
	ZCacheClient * client;
	long itemLen;
	unsigned short cmdType;
};

const unsigned short ZCMD_OK = 0x0000;
const unsigned short ZCMD_ERR = 0x0001;
const unsigned short ZCMD_PING = 0x0002;
const unsigned short ZCMD_PONG = 0x0003;
const unsigned short ZCMD_FULL = 0x0004;
const unsigned short ZCMD_LONG = 0x0005;
const unsigned short ZCMD_OBJ = 0x0006;


const unsigned short ZCMD_BASE      = 0x1000;
const unsigned short ZCMD_SET       = 0x1000;   // sds obj          OK
const unsigned short ZCMD_SETNX     = 0x1001;   // sds obj          OK
const unsigned short ZCMD_GET       = 0x1002;   // sds              obj
const unsigned short ZCMD_EXIST     = 0x1003;   // sds              OK
const unsigned short ZCMD_INC       = 0x1004;   // sds              long
const unsigned short ZCMD_DEC       = 0x1005;   // sds              long
const unsigned short ZCMD_INCBY     = 0x1006;   // sds long         long
const unsigned short ZCMD_DECBY     = 0x1007;   // sds long         long
const unsigned short ZCMD_SElECT    = 0x1008;   // long             OK
const unsigned short ZCMD_KEYS      = 0x1009;   // null             list<sds>
const unsigned short ZCMD_DBSIZE    = 0x100A;   // null             long
const unsigned short ZCMD_RENAME    = 0x100B;   // sds sds          OK
const unsigned short ZCMD_LPUSH     = 0x100C;   // sds obj          long
const unsigned short ZCMD_RPUSH     = 0x100D;   // sds obj          long
const unsigned short ZCMD_LPOP      = 0x100E;   // sds              obj
const unsigned short ZCMD_RPOP      = 0x100F;   // sds              obj
const unsigned short ZCMD_LLEN      = 0x1010;   // sds              long
const unsigned short ZCMD_LRANGE    = 0x1011;   // sds long long    list<obj>
const unsigned short ZCMD_TYPE      = 0x1012;   // sds              long
const unsigned short ZCMD_SADD      = 0x1013;   // sds sds          OK
const unsigned short ZCMD_SREM      = 0x1014;   // sds sds          OK
const unsigned short ZCMD_SSIZE     = 0x1015;   // sds              long
const unsigned short ZCMD_REMOVE    = 0x1016;   // sds              OK
const unsigned short ZCMD_SEXIST    = 0x1017;   // sds              OK
const unsigned short ZCMD_CLEARDB   = 0x1018;   // null             OK



int ZProcess_PING(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_SET(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_SETNX(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_GET(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_EXIST(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_INC(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_DEC(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_INCBY(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_DECBY(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_SElECT(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_KEYS(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_DBSIZE(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_RENAME(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_LPUSH(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_RPUSH(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_LPOP(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_RPOP(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_LLEN(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_LRANGE(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_TYPE(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_SADD(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_SREM(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_SSIZE(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_REMOVE(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_SEXIST(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZProcess_CLEARDB(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);



int ZPackAndSendCommand(ZCacheClient * pClient, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZPackLongAndSendCommand(ZCacheClient * pClient, unsigned short cmdType, long value);


int ZUnPackCommand(ZCacheCommand * pCmd, const char * buf, size_t bufLen);
size_t ZPackCommand(ZCacheCommand * pCmd, char * buf, size_t maxBufLen);


int ZClearCommand(ZCacheCommand * pCmd);
int ZProcessCommand(ZCacheCommand * pCmd, ZCacheCommand * pSendCmd, char * & sendBuf, size_t & maxSendBufLen);
int ZRejectCommand(ZCacheCommand * pCmd);

size_t ZPackLong(char * buf, size_t maxBufLen, long val);
int ZUnPackLong(const char * buf, size_t bufLen, long & val);
size_t ZPackString(char * buf, size_t maxBufLen, const sds s);
size_t ZPackStringAndGenKey(char * buf, size_t maxBufLen, const sds s);
int ZUnPackString(const char * buf, size_t bufLen, sds & s);

struct ZCacheConstCommand
{
	char buf[32];
	int bufLen;
};

struct ZCacheCommandShared
{
	ZCacheConstCommand ok, full, zero, one, ping, pong,
		minus1Err, minus2Err, minus3Err, minus4Err,
		wrongtypeErrM100, nokeyErrM101, nilErrorM102, nocmdErrM103,
        cmdparamErrM104, existErrM105, overflowErr106;
};

const int ZMinus1Error = -1;
const int ZMinus2Error = -2;
const int ZMinus3Error = -3;
const int ZMinus4Error = -4;

const int ZWrongTypeError = -100;
const int ZNoKeyError = -101;
const int ZNilError = -102;
const int ZNoCmdError = -103;
const int ZParamError = -104;
const int ZExistError = -105;
const int ZOverflowError = -106;

extern ZCacheCommandShared g_sharedCommand;

void ZInitSharedCommand();