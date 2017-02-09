#pragma once
#include "dict.h"
#include "ZCacheItem.h"
#include <ZRMultiThreadUtility.h>
class ZCacheDataBase : public ZCacheHash
{
public:
	ZCacheDataBase();
	~ZCacheDataBase();

    void lock()
    {
        ZRCS_ENTER(m_cs);
    }
    void unlock()
    {

        ZRCS_LEAVE(m_cs);
    }


protected:


    ZRCRITICALSECTION m_cs;

private:
};
