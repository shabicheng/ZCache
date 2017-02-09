#ifndef ZRSET_H__
#define ZRSET_H__

#include "ZRRBTree.h"

// 树节点类型
// 针对集合（set）使用
template <typename _KeyType>
struct _ZRSetNode
{
    // 集合只有键，没有映射值
    // 集合的键不允许修改
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
        // 不需要做任何操作
    }
};

// 节点的分配器
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
    // g++要求必须使用这种难看的形式声明父类中定义的类型
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
    // 集合的插入操作只需要一个参数
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
