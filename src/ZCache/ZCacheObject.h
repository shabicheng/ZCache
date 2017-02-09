#pragma once

#include <functional>

typedef void(*ZObjectFreeFun)(void *);

template <typename ObjType>
class ZCacheObjectDefaultDestroyer
{
public:
	void operator()(ObjType * pObj)
	{
		delete pObj;
	}
};

template <typename _ObjType, typename _Destroyer = ZCacheObjectDefaultDestroyer<_ObjType> >
class ZCacheObject
{
public:
	ZCacheObject()
		: pObj(NULL)
	{
		m_refCount = new int(1);
	}

	ZCacheObject(_ObjType * ptr)
		: pObj(ptr)
	{
		m_refCount = new int(1);
	}

	~ZCacheObject()
	{
		removeSelf();
	}

    // ��keys�������Ҫ��list��dict�����keyȫ��װ���������������keyȫ������sds
    // Ϊ�˼����ڴ濽������keyֱ�Ӹ�ֵ��list���У�����list�ͷ�ʱ��ɾ����key
    // ������ɾ��list֮ǰ���ɵ��ô˽ӿڰ������sds��ΪNULL��������ɾ��OBJ��ʱ�򲻻�delete ��SDS
    void removeObj()
    {
        pObj = NULL;
    }

	void init(_ObjType * p)
	{
		removeSelf();
		pObj = p;
		m_refCount = new int(1);
	}

	ZCacheObject(const ZCacheObject & o)
	{
		pObj = o.pObj;
		m_refCount = o.m_refCount;
		++(*m_refCount);

		return;
	}

	ZCacheObject & operator = (const ZCacheObject & o)
	{
        if (this == &o)
        {
            return *this;
        }
		removeSelf();

		pObj = o.pObj;
		m_refCount = o.m_refCount;
		++(*m_refCount);

		return *this;
	}

	_ObjType * pObj;

protected:

	void removeSelf()
	{
		if (--(*m_refCount) == 0)
		{
			m_destroyer(pObj);
			delete m_refCount;
		}
	}

	_Destroyer m_destroyer;

    int * m_refCount;
private:
};

