#ifndef ZRSET_H__
#define ZRSET_H__

#include "ZRRBTree.h"

// ���ڵ�����
// ��Լ��ϣ�set��ʹ��
template <typename _KeyType>
struct _ZRSetNode
{
    // ����ֻ�м���û��ӳ��ֵ
    // ���ϵļ��������޸�
    typedef const _KeyType _NKeyType;
    typedef const _KeyType _NValType;
    _ZRSetNode()
        :parent(NULL),
        leftChild(NULL),
        rightChild(NULL),
        color('\0'),
        isNil(true)
    {}
    explicit _ZRSetNode(_NValType& value)
        :parent(NULL),
        leftChild(NULL),
        rightChild(NULL),
        color('\0'),
        isNil(true),
        val(value)
    {}
    _ZRSetNode<_KeyType>* parent;
    _ZRSetNode<_KeyType>* leftChild;
    _ZRSetNode<_KeyType>* rightChild;
    char color;
    bool isNil;
    _KeyType val;
    const _KeyType& getKey() const
    {
        return val;
    }
    const _KeyType& getVal() const
    {
        return val;
    }
    void setVal(const _KeyType& newval)
    {
        // ����Ҫ���κβ���
    }
};

// �ڵ�ķ�����
template <typename _KeyType>
class _ZRSetNodeAllocator
{
public:
    _ZRSetNode<_KeyType>* alloc()
    {
        return new(nothrow) _ZRSetNode<_KeyType>;
    }
    _ZRSetNode<_KeyType>* alloc(const _KeyType& key)
    {
        _ZRSetNode<_KeyType> *node = new(nothrow) _ZRSetNode<_KeyType>(key);
        return node;
    }
    _ZRSetNode<_KeyType>* alloc(const _ZRSetNode<_KeyType>* node)
    {
        _ZRSetNode<_KeyType> *newnode = new(nothrow) _ZRSetNode<_KeyType>(*node);
        return newnode;
    }
    void dealloc(_ZRSetNode<_KeyType> *node)
    {
        delete node;
    }
};

template <
    typename _ValType,
    typename _KeyCompare = _less<_ValType> >
class ZRSet
    : public ZRRBTree<
        _ZRSetNode<_ValType>, 
        const _ValType, 
        _ZRSetNodeAllocator<_ValType>, 
        _KeyCompare>
{
public:
    // g++Ҫ�����ʹ�������ѿ�����ʽ���������ж��������
    typedef typename ZRRBTree<
        _ZRSetNode<_ValType>,
        const _ValType,
        _ZRSetNodeAllocator<_ValType>,
        _KeyCompare>::_ZRTreeIterator iterator;
    typedef typename ZRRBTree<
        _ZRSetNode<_ValType>,
        const _ValType,
        _ZRSetNodeAllocator<_ValType>,
        _KeyCompare>::_ZRTreeConstIterator const_iterator;
    // ���ϵĲ������ֻ��Ҫһ������
    typename ZRRBTree<
        _ZRSetNode<_ValType>,
        const _ValType,
        _ZRSetNodeAllocator<_ValType>,
        _KeyCompare>::_InsertRstType insert(const _ValType& val)
    {
        return this->insert_i(val);
    }
};

#endif // ZRSET_H__
