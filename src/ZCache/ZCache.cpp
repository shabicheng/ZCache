#include "ZCacheServer.h"


int main()
{
    ZCacheServer * pServer = ZCacheServer::getInstance();
    pServer->start();
    
    getchar();
    pServer->stop();
    ZCacheServer::finalizeInstance();
    return 0;
}