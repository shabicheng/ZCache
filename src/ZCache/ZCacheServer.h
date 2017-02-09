#pragma once
#include "ZCacheDataBase.h"


class ZCacheServer
{
public:

	static ZCacheServer * getInstance();
	static void finalizeInstance();

    ZCacheServer();
    ~ZCacheServer();

    void start();
    void stop();

    ZCacheDataBase * getDB(int index);
    void clearDB(ZCacheDataBase * pDB);

protected:

    ZCacheDataBase ** databases;

private:
};
