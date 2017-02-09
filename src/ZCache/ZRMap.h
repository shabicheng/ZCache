#ifndef ZRMAP_H__
#define ZRMAP_H__

#include "ZRRBTree.h"

// 树节点类型
// 针对映射（map）使用
template <
    typename _KeyType,
    typename _MappedType>
struct _ZRMapNode
{
    // 键不允许修改
    typedef const _KeyType _NKeyType;
    // 实际的值为一个二元组
    // 第一个元素为键类型，不允许修改，第二个元素为映射值类型，允许修改
    typedef ZRPair<const _KeyType, _MappedType> _NValType;
    _ZRMapNode()
        :parent(NULL),
        leftChild(NULL),
        rightChild(NULL),
        color('\0'),
        isNil(true)
    {}
    explicit _ZRMapNode(const _NValType& value)
        :parent(NULL),
        leftChild(NULL),
        rightChild(NULL),
        color('\0'),
        isNil(true),
        val(value)
    {}
    _ZRMapNode(const _KeyType& key, const _MappedType& mapped)
        :parent(NULL),
        leftChild(NULL),
        rightChild(NULL),
        color('\0'),
        isNil(true),
        val(key, mapped)
    {}
    _ZRMapNode<_KeyType, _MappedType>* parent;
    _ZRMapNode<_KeyType, _MappedType>* leftChild;
    _ZRMapNode<_KeyType, _MappedType>* rightChild;
    char color;
    bool isNil;
    _NValType val;
    const _KeyType& getKey() const
    {
        return val.first;
    }
    _NValType& getVal()
    {
        return val;
    }
    void setVal(const _NValType& newval)
    {
        val.second = newval.second;
    }
};

// 节点分配器
template <typename _KeyType, typename _MappedType>
class _ZRMapNodeAllocator
{
public:
    _ZRMapNode<_KeyType, _MappedType>* alloc()
    {
        return new(nothrow) _ZRMapNode<_KeyType, _MappedType>;
    }
    _ZRMapNode<_KeyType, _MappedType>* alloc(const _KeyType& key)
    {
        return new(nothrow) _ZRMapNode<_KeyType, _MappedType>(key, _MappedType());
    }
    _ZRMapNode<_KeyType, _MappedType>* alloc(const _KeyType& key, const ZRPair<const _KeyType, _MappedType>& val)
    {
        return new(nothrow) _ZRMapNode<_KeyType, _MappedType>(val);
    }
    _ZRMapNode<_KeyType, _MappedType>* alloc(const _ZRMapNode<_KeyType, _MappedType>* node)
    {
        _ZRMapNode<_KeyType, _MappedType> *newnode = new(nothrow) _ZRMapNode<_KeyType, _MappedType>(*node);
        return newnode;
    }
    void dealloc(_ZRMapNode<_KeyType, _MappedType> *node)
    {
        delete node;
    }
};

template <
    typename _KeyType,
    typename _MappedType,
    typename _KeyCompare = _less<_KeyType> >
class ZRMap
    : public ZRRBTree<
        _ZRMapNode<_KeyType, _MappedType>, 
        const _KeyType, 
        _ZRMapNodeAllocator<_KeyType, _MappedType>,
        _KeyCompare>
{
public:

    // g++要求必须使用这种难看的形式声明父类中定义的类型
    typedef typename ZRRBTree<
        _ZRMapNode<_KeyType, _MappedType>,
        const _KeyType,
        _ZRMapNodeAllocator<_KeyType, _MappedType>,
        _KeyCompare>::_ZRTreeIterator iterator;
    typedef typename ZRRBTree<
        _ZRMapNode<_KeyType, _MappedType>,
        const _KeyType,
        _ZRMapNodeAllocator<_KeyType, _MappedType>,
        _KeyCompare>::_ZRTreeConstIterator const_iterator;

    typename ZRRBTree<
        _ZRMapNode<_KeyType, _MappedType>,
        const _KeyType,
        _ZRMapNodeAllocator<_KeyType, _MappedType>,
        _KeyCompare>::_InsertRstType insert(const ZRPair<const _KeyType, _MappedType>& val)
    {
        return insert_i(val.first, val);
    }
    typename ZRRBTree<
        _ZRMapNode<_KeyType, _MappedType>,
        const _KeyType,
        _ZRMapNodeAllocator<_KeyType, _MappedType>,
        _KeyCompare>::_InsertRstType insert(const ZRPair<_KeyType, _MappedType>& val)
    {
        return this->insert_i(val.first, ZRPair<const _KeyType, _MappedType>(val.first, val.second));
    }
    // 映射类型的下标操作符
    _MappedType& operator[](const _KeyType& key)
    {
        return this->insert_i(key).first->second;
    }
};

#endif // ZRMAP_H__
