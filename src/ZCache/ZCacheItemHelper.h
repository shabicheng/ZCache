#pragma once

#include <list>
#include "ZCacheItem.h"
#include "ZRlibrary.h"
#include "ZCommon.h"
#include "ZCacheConfig.h"
using std::list;


class ZCacheItemHelper
{
public:
	ZCacheItemHelper();
	~ZCacheItemHelper();

	ZCacheItem * createItem(unsigned char type)
	{
		if (type < 1 || type > ZITEM_TYPE_NUM)
		{
			INNER_LOG(CERROR, "未知的数据类型 0x%2x", (long)type);
            return NULL;
		}
		size_t index = type - 1;
        // @debug
        //ZRCS_ENTER(m_lock);
		if (false && !m_cacheItemList[index].empty())
		{
			ZCacheItem * pItem = m_cacheItemList[index].back();
			m_cacheItemList[index].pop_back();
            //ZRCS_LEAVE(m_lock);
			return pItem;
		}
		else
		{
            //ZRCS_LEAVE(m_lock);
            // @debug end
			switch (type)
			{
			case ZITEM_BUFFER:
			{
				ZCacheBuffer * pItem = new ZCacheBuffer;
				ZCheckPtr(pItem);
				return pItem;
			}
			case ZITEM_LONG:
			{
				ZCacheLong * pItem = new ZCacheLong;
				ZCheckPtr(pItem);
				return pItem;
			}
			case ZITEM_LIST:
			{
				ZCacheList * pItem = new ZCacheList;
				ZCheckPtr(pItem);
				return pItem;
			}
			case ZITEM_SET:
			{
				ZCacheSet * pItem = new ZCacheSet;
				ZCheckPtr(pItem);
				return pItem;
			}
			case ZITEM_HASH:
			{
				ZCacheHash * pItem = new ZCacheHash;
				ZCheckPtr(pItem);
				return pItem;
			}
			}
		}
		return NULL;
	}

	void deleteItem(ZCacheItem * pItem)
	{
		if (pItem == NULL)
		{
			return;
		}

        if (pItem->itemType <= ZITEM_INVALID || pItem->itemType >= ZITEM_TYPE_NUM)
        {
            INNER_LOG(CWARNING, "未知的Item类型，无法释放.");
            return;
        }
        // @debug
        pItem->clear();
        delete pItem;
        return;
        // @debug end

        pItem->clear();

        ZRCS_ENTER(m_lock);

#ifndef NDEBUG
        assert(find(m_cacheItemList[pItem->itemType - 1].begin(), m_cacheItemList[pItem->itemType - 1].end(), pItem) == m_cacheItemList[pItem->itemType - 1].end());
#endif

        list<ZCacheItem *> & itemList = m_cacheItemList[pItem->itemType - 1];
        itemList.push_back(pItem);

        // 缓存太多，进行清理
        if (itemList.size() > (size_t)g_config.itemCacheClearSize)
        {
            INNER_LOG(CINFO, "缓存超出阈值，开始清理.");
            int popSize = (int)(g_config.itemCacheClearSize * (g_config.itemCacheClearPercent / 100.f));
            for (int i = 0; i < popSize; ++i)
            {
                ZCacheItem * pDeleteItem = itemList.back();
                delete pDeleteItem;
                itemList.pop_back();
            }
        }
        ZRCS_LEAVE(m_lock);
	}

protected:

    ZRCRITICALSECTION m_lock;

	list<ZCacheItem *> m_cacheItemList[ZITEM_TYPE_NUM];
	
private:
};

#define ZNewBuffer() (ZCacheBuffer *)g_itemHelper.createItem(ZITEM_BUFFER)
#define ZNewLong() (ZCacheLong *)g_itemHelper.createItem(ZITEM_LONG)
#define ZNewList() (ZCacheList *)g_itemHelper.createItem(ZITEM_LIST)
#define ZNewSet() (ZCacheSet *)g_itemHelper.createItem(ZITEM_SET)
#define ZNewHash() (ZCacheHash *)g_itemHelper.createItem(ZITEM_HASH)

#define ZDeleteItem(pItem) g_itemHelper.deleteItem((pItem))

template<typename T, typename P>
inline T * ZItemCast(P * p)
{
    return NULL;
}


// 开始特化
template<>
inline ZCacheBuffer * ZItemCast<ZCacheBuffer, ZCacheItem>(ZCacheItem * p)
{
    if (p->itemType != ZITEM_BUFFER)
    {
        return NULL;
    }
    return (ZCacheBuffer*)p;
}


template<>
inline ZCacheLong * ZItemCast<ZCacheLong, ZCacheItem>(ZCacheItem * p)
{
    if (p->itemType != ZITEM_LONG)
    {
        return NULL;
    }
    return (ZCacheLong*)p;
}


template<>
inline ZCacheList * ZItemCast<ZCacheList, ZCacheItem>(ZCacheItem * p)
{
    if (p->itemType != ZITEM_LIST)
    {
        return NULL;
    }
    return (ZCacheList*)p;
}


template<>
inline ZCacheSet * ZItemCast<ZCacheSet, ZCacheItem>(ZCacheItem * p)
{
    if (p->itemType != ZITEM_SET)
    {
        return NULL;
    }
    return (ZCacheSet*)p;
}


template<>
inline ZCacheHash * ZItemCast<ZCacheHash, ZCacheItem>(ZCacheItem * p)
{
    if (p->itemType != ZITEM_HASH)
    {
        return NULL;
    }
    return (ZCacheHash*)p;
}


extern ZCacheItemHelper g_itemHelper;
