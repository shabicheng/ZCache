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

    // 在keys命令里，需要用list把dict里面的key全部装起来，但是里面的key全部都是sds
    // 为了减少内存拷贝，将key直接赋值到list里中，但在list释放时会删除此key
    // 所以在删除list之前，可调用此接口把里面的sds置为NULL，这样在删除OBJ的时候不会delete 掉SDS
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

