#include "ZCacheDataBase.h"
#include "sds.h"
#include <string.h>
#include "dict.h"
#include "ZCacheObject.h"
#include "ZCacheItem.h"
#include <ZRMultiThreadUtility.h>



ZCacheDataBase::ZCacheDataBase()
{
    m_cs = ZRCreateCriticalSection();
}

ZCacheDataBase::~ZCacheDataBase()
{
    ZRDeleteCriticalSection(m_cs);
}
