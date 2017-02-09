#include "ZCacheItemHelper.h"

ZCacheItemHelper::ZCacheItemHelper()
{
    m_lock = ZRCreateCriticalSection(4000);
}

ZCacheItemHelper::~ZCacheItemHelper()
{

    ZRDeleteCriticalSection(m_lock);
}

ZCacheItemHelper g_itemHelper;
