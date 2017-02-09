#ifndef BRTREE_H__
#define BRTREE_H__

#include "ZRPair.h"
#include <stdlib.h>
#include <new>
using std::nothrow;

// 判断小于的函数类
template<typename T>
class _less
{
public:
    bool operator()(const T& left, const T& right) const
    {
        return left < right;
    }
};

// 判断相等的函数类
template<typename T>
class _equal
{
public:
    bool operator()(const T& left, const T& right) const
    {
        return left == right;
    }
};


template<
    typename _NodeType,
    typename _KeyType,
    typename _NodeAllocator,
    typename _KeyCompare = _less<_KeyType> >
class ZRRBTree
{
    // 节点颜色枚举
    enum _NodeColor{ RED, BLACK };

    // 节点类型
    typedef _NodeType _RBTNode;
    // 节点指针类型
    typedef _RBTNode* _RBTNodePtr;
    // 节点的键类型
    typedef typename _RBTNode::_NKeyType _NKeyType;
    // 节点的值类型
    typedef typename _RBTNode::_NValType _NValType;
    // 插入查找的结果
    struct _FindRstType
    {
        _FindRstType(_RBTNodePtr curnode, _RBTNodePtr parentnode, bool left)
        :node(curnode), pnode(parentnode), addleft(left){}
        _RBTNodePtr node;
        _RBTNodePtr pnode;
        bool addleft;
    };

    // 获取指定节点的后继结点的函数类
    class _NodeForward
    {
    public:
        _RBTNodePtr operator()(_RBTNodePtr& ptr) const
        {
            return treeSuccessor(ptr);
        }
    };

#ifdef _VXWORKS
    friend class _NodeForward;
#endif

    // 获取节点保存的值的函数类
    class _GetNodeContent
    {
    public:
        _NValType& operator()(_RBTNodePtr& ptr) const
        {
            if (ptr->isNil)
            {
                // 
            }
            return (ptr->getVal());
        }
    };

    // 指向const类型的迭代器，不允许修改其指向的值
    class _ZRConstIterator
    {
        friend class ZRRBTree< _NodeType,
        _KeyType,
        _NodeAllocator,
        _KeyCompare >;
    public:
        _ZRConstIterator()
            :m_container(NULL)
        {}
        _ZRConstIterator& operator++()
        {
            m_val = m_forward(m_val);
            return *this;
        }
        _ZRConstIterator operator++(int)
        {
            _ZRConstIterator tmpIter = *this;
            m_val = m_forward(m_val);
            return tmpIter;
        }
        const _NValType& operator*()
        {
            return m_getcont(m_val);
        }
        const _NValType* operator->()
        {
            return &m_getcont(m_val);
        }
        bool operator==(const _ZRConstIterator& right) const
        {
            if (m_container != right.m_container)
            {
                return false;
            }
            return m_equalcmp(m_val, right.m_val);
        }
        bool operator!=(const _ZRConstIterator& right) const
        {
            return !(this->operator==(right));
        }
    protected:
        _RBTNodePtr m_val;
        _equal<_RBTNodePtr> m_equalcmp;
        _NodeForward m_forward;
        _GetNodeContent m_getcont;;
        const ZRRBTree< _NodeType,
            _KeyType,
            _NodeAllocator,
            _KeyCompare > *m_container;
    };

    // 普通迭代器
    class _ZRIterator : public _ZRConstIterator
    {
        friend class ZRRBTree < _NodeType,
        _KeyType,
        _NodeAllocator,
        _KeyCompare >;
    public:
        _ZRIterator()
        {}
        _ZRIterator& operator++()
        {
#ifdef _VXWORKS_55
            _ZRConstIterator::m_val = m_forward(_ZRConstIterator::m_val);
#else
            _ZRConstIterator::m_val = _ZRConstIterator::m_forward(_ZRConstIterator::m_val);
#endif
            return *this;
        }
        _ZRIterator operator++(int)
        {
            _ZRIterator tmpIter = *this;
#ifdef _VXWORKS_55
            _ZRConstIterator::m_val = m_forward(_ZRConstIterator::m_val);
#else
            _ZRConstIterator::m_val = _ZRConstIterator::m_forward(_ZRConstIterator::m_val);
#endif
            return tmpIter;
        }
        _NValType& operator*()
        {
#ifdef _VXWORKS_55
            return m_getcont(_ZRConstIterator::m_val);
#else
            return _ZRConstIterator::m_getcont(_ZRConstIterator::m_val);
#endif
        }
        _NValType* operator->()
        {
#ifdef _VXWORKS_55
            return &m_getcont(_ZRConstIterator::m_val);
#else
            return &_ZRConstIterator::m_getcont(_ZRConstIterator::m_val);
#endif
        }
        bool operator==(const _ZRIterator& right) const
        {
            if (_ZRConstIterator::m_container != right._ZRConstIterator::m_container)
            {
                return false;
            }
#ifdef _VXWORKS_55
            return m_equalcmp(_ZRConstIterator::m_val, right._ZRConstIterator::m_val);
#else
            return _ZRConstIterator::m_equalcmp(_ZRConstIterator::m_val, right._ZRConstIterator::m_val);
#endif
        }
        bool operator!=(const _ZRIterator& right) const
        {
            return !(this->operator==(right));
        }
    };

protected:
    // 普通迭代器
    typedef _ZRIterator _ZRTreeIterator;
    // const迭代器
    typedef _ZRConstIterator _ZRTreeConstIterator;
    // 插入结果
    typedef ZRPair<_ZRTreeIterator, bool> _InsertRstType;
public:
    ZRRBTree()
    {
        m_head = m_nodealloc.alloc();
        m_head->isNil = true;
        m_head->color = BLACK;
        root() = m_head;
        lmost() = m_head;
        rmost() = m_head;
        m_size = 0;
    }

    // 拷贝构造函数
    ZRRBTree(const ZRRBTree& oldtree)
    {
        m_head = m_nodealloc.alloc();
        root() = m_head;
        copytree(oldtree.root(), root());
        if (!root()->isNil)
        {
            lmost() = _tmin(root());
            rmost() = _tmax(root());
            root()->parent = m_head;
        }
        else
        {
            lmost() = m_head;
            rmost() = m_head;
        }
    }

    // 赋值操作符
    ZRRBTree& operator=(const ZRRBTree& oldtree)
    {
        if (this == &oldtree)
        {
            // 自赋值，不需要进行任何处理
            return *this;
        }
        // 清除所有原有的节点
        clear();
        // 开始进行复制
        copytree(oldtree.root(), root());
        if (!root()->isNil)
        {
            lmost() = _tmin(root());
            rmost() = _tmax(root());
            root()->parent = m_head;
        }
        else
        {
            lmost() = m_head;
            rmost() = m_head;
        }
        return *this;
    }

    // 析构函数，需要删除所有的节点
    ~ZRRBTree()
    {
        clear();
        m_nodealloc.dealloc(m_head);
    }

    // 获取第一个节点
    // 如果树为空，返回end()
    _ZRTreeIterator begin()
    {
        _ZRTreeIterator iter;
        iter.m_container = this;
        iter.m_val = lmost();
        return iter;
    }

    // 获取最后一个节点的下一个节点
    _ZRTreeIterator end()
    {
        _ZRTreeIterator iter;
        iter.m_container = this;
        iter.m_val = m_head;
        return iter;
    }

    // 获取第一个节点的const迭代器
    // 如果树为空，返回end()
    _ZRTreeConstIterator begin() const
    {
        _ZRTreeConstIterator iter;
        iter.m_container = this;
        iter.m_val = lmost();
        return iter;
    }

    // 获取最后一个节点的下一个节点的const迭代器
    _ZRTreeConstIterator end() const
    {
        _ZRTreeConstIterator iter;
        iter.m_container = this;
        iter.m_val = m_head;
        return iter;
    }

    // 删除指定key所代表的节点
    // 如果指定key未在树中找到，直接返回end()
    // 否则删除成功后返回原迭代器的后继迭代器
    _ZRTreeIterator erase(_NKeyType& val)
    {
        _RBTNodePtr findNode = find_i(val);
        if (findNode->isNil)
        {
            return end();
        }
        _ZRTreeIterator successor;
        successor.m_container = this;
        successor.m_val = findNode;
        ++successor;
        eraseNode(findNode);
        return successor;
    }

    // 删除迭代器所指向的节点
    // 如果迭代器不指向该树的节点或为end()，直接返回end()
    // 否则删除成功后返回原迭代器的后继迭代器
    _ZRTreeIterator erase(_ZRTreeIterator& iter)
    {
        if (iter.m_container != this || iter.m_val->isNil)
        {
            return end();
        }
        // 先保存后继结点
        _ZRTreeIterator successor = iter;
        ++successor;
        eraseNode(iter.m_val);
        return successor;
    }

    _ZRTreeConstIterator erase(_ZRTreeConstIterator& iter)
    {
        if (iter.m_container != this || iter.m_val->isNil)
        {
            return end();
        }
        // 先保存后继结点
        _ZRTreeConstIterator successor = iter;
        ++successor;
        eraseNode(iter.m_val);
        return successor;
    }

    // 根据key值查找结点
    // 如果存在，返回对应的迭代器
    // 否则返回end()
    _ZRTreeIterator find(_NKeyType& key)
    {
        _ZRTreeIterator rst;
        rst.m_container = this;
        rst.m_val = find_i(key);
        return rst;
    }
    // 根据key值查找结点
    // 如果存在，返回对应的迭代器
    // 否则返回end()
	_ZRTreeConstIterator find(_NKeyType& key) const
    {
        _ZRTreeConstIterator rst;
        rst.m_container = this;
        rst.m_val = find_i(key);
        return rst;
    }

	//@add by zc
	
	_ZRTreeIterator lower_bound(const _NKeyType& key)
	{
		_ZRTreeIterator rst;
		rst.m_container = this;
		rst.m_val = lower_bound_i(key);
		return rst;
	}


	_ZRTreeConstIterator lower_bound(const _NKeyType& key) const
	{
		_ZRTreeConstIterator rst;
		rst.m_container = this;
		rst.m_val = lower_bound_i(key);
		return rst;
	}


	_ZRTreeIterator upper_bound(const _NKeyType& key)
	{
		_ZRTreeIterator rst;
		rst.m_container = this;
		rst.m_val = upper_bound_i(key);
		return rst;
	}


	_ZRTreeConstIterator upper_bound(const _NKeyType& key) const
	{
		_ZRTreeConstIterator rst;
		rst.m_container = this;
		rst.m_val = upper_bound_i(key);
		return rst;
	}





	//@add end

    // 获取当前树的有效节点数
    unsigned int size() const
    {
        return m_size;
    }

    // 判断当前树是否为空
    bool empty() const
    {
        return m_size == 0UL;
    }

    // 清除所有数据
    void clear()
    {
        eraseSubTree(root());
        root() = m_head;
        lmost() = m_head;
        rmost() = m_head;
        m_size = 0;
    }
    // 获取树的高度
    unsigned int height()
    {
        return height(root());
    }
    // 递归获取树高度
    unsigned int height(_RBTNodePtr root)
    {
        if (root->isNil)
        {
            return 0;
        }
        unsigned int lHeight, rHeight;
        lHeight = height(root->leftChild);
        rHeight = height(root->rightChild);
        return lHeight > rHeight ? lHeight + 1 : rHeight + 1;
    }
protected:

    // 提供给map的插入
    _InsertRstType insert_i(const _NKeyType& key, const _NValType& val)
    {
        _FindRstType findRst = insertfind_i(key);
        _InsertRstType rst;
        rst.first.m_container = this;
        if (!findRst.node->isNil)
        {
            // 需要插入的key已经存在，直接返回
            rst.first.m_val = findRst.node;
            rst.second = false;
            return rst;
        }
        // 未找到key对应的节点，插入新节点
        _RBTNodePtr insertNode = m_nodealloc.alloc(key, val);
        rst.first.m_val = insertNode;
        rst.second = true;
        insertNode->isNil = false;
        insertNode->color = RED;
        insertNode->parent = findRst.pnode;
        insertNode->leftChild = m_head;
        insertNode->rightChild = m_head;
        ++m_size;
        if (findRst.pnode->isNil)
        {
            // 第一个节点
            insertNode->color = BLACK;
            root() = insertNode;
            lmost() = insertNode;
            rmost() = insertNode;
            return rst;
        }
        if (findRst.addleft)
        {
            findRst.pnode->leftChild = insertNode;
            if (lmost() == findRst.pnode)
            {
                lmost() = insertNode;
            }
        }
        else
        {
            findRst.pnode->rightChild = insertNode;
            if (rmost() == findRst.pnode)
            {
                rmost() = insertNode;
            }
        }
        rebalance(insertNode);
        return rst;
    }

    // 根据key值插入新节点
    // 如果key在树中不存在，则插入新节点，并返回新节点的迭代器
    // 否则返回对应key的节点迭代器
    _InsertRstType insert_i(const _NKeyType& key)
    {
        _FindRstType findRst = insertfind_i(key);
        _InsertRstType rst;
        if (!findRst.node->isNil)
        {
            // 需要插入的key已经存在
            rst.first.m_val = findRst.node;
            rst.second = false;
            return rst;
        }
        // 未找到key对应的节点，插入新节点
        _RBTNodePtr insertNode = m_nodealloc.alloc(key);
        rst.first.m_val = insertNode;
        rst.second = true;
        insertNode->isNil = false;
        insertNode->color = RED;
        insertNode->parent = findRst.pnode;
        insertNode->leftChild = m_head;
        insertNode->rightChild = m_head;
        ++m_size;
        if (findRst.pnode->isNil)
        {
            // 第一个节点
            insertNode->color = BLACK;
            root() = insertNode;
            lmost() = insertNode;
            rmost() = insertNode;
            return rst;
        }
        if (findRst.addleft)
        {
            findRst.pnode->leftChild = insertNode;
            if (lmost() == findRst.pnode)
            {
                // 插入节点的值比原来的最小值小
                lmost() = insertNode;
            }
        }
        else
        {
            findRst.pnode->rightChild = insertNode;
            if (rmost() == findRst.pnode)
            {
                // 插入节点的值比原来的最大值大
                rmost() = insertNode;
            }
        }
        rebalance(insertNode);
        return rst;
    }
protected:
    // 插入时的查找需要保留更多的信息
    _FindRstType insertfind_i(const _NKeyType& key)
    {
        // 需要保留查找结果的父节点和查找结果是否属于其父节点的左子树
        _RBTNodePtr curNode = root(), pnode = m_head;
        bool leftChild = true;
        while (!curNode->isNil)
        {
            if (m_ltcmp(key, (curNode->getKey())))
            {
                pnode = curNode;
                leftChild = true;
                curNode = curNode->leftChild;
            }
            else if(m_ltcmp((curNode->getKey()), key))
            {
                pnode = curNode;
                leftChild = false;
                curNode = curNode->rightChild;
            }
            else
            {
                // 查找成功
                return _FindRstType(curNode, pnode, leftChild);
            }
        }
        return _FindRstType(curNode, pnode, leftChild);
    }

    // 简单的查找操作，只保留查找结果
    _RBTNodePtr find_i(const _NKeyType& key) const
    {
        _RBTNodePtr curNode = root();
        while (!curNode->isNil)
        {
            if (m_ltcmp(key, (curNode->getKey())))
            {
                curNode = curNode->leftChild;
            }
            else if (m_ltcmp((curNode->getKey()), key))
            {
                curNode = curNode->rightChild;
            }
            else
            {
                // 查找成功
                return curNode;
            }
        }
        return curNode;
    }

	// @add by zc
	_RBTNodePtr lower_bound_i(_NKeyType& key) const
	{
		_RBTNodePtr curNode = root();
		_RBTNodePtr lastNode = m_head;
		while (!curNode->isNil)
		{
			if (m_ltcmp((curNode->getKey()), key))
			{
				curNode = curNode->rightChild;
			}
			else
			{
				lastNode = curNode;
				curNode = curNode->leftChild;
			}
		}
		return lastNode;
	}


	_RBTNodePtr upper_bound_i(_NKeyType& key) const
	{
		_RBTNodePtr curNode = root();
		_RBTNodePtr lastNode = m_head;
		while (!curNode->isNil)
		{
			if (m_ltcmp(key, curNode->getKey()))
			{
				lastNode = curNode;
				curNode = curNode->leftChild;
			}
			else
			{
				curNode = curNode->rightChild;
			}
		}
		return lastNode;
	}


	// @add end


    void rebalance(_RBTNodePtr baseNode)
    {
        // 针对插入操作导致的不平衡调整树结构
        for (_RBTNodePtr pnode = baseNode, tnode = NULL;
             pnode->parent->color == RED;)
        {
            if (pnode->parent == pnode->parent->parent->leftChild)
            {
                // 当前节点的父节点是其父节点的左子树
                tnode = pnode->parent->parent->rightChild;
                if (tnode->color == RED)
                {
                    // 如果当前节点的叔父节点为红色
                    // 只需对当前节点、其父节点和叔父节点、其祖父节点重新着色
                    // 并将判定节点选定为当前节点的祖父节点
                    pnode->parent->color = BLACK;
                    tnode->color = BLACK;
                    pnode->parent->parent->color = RED;
                    pnode = pnode->parent->parent;
                }
                else
                {
                    // 当前节点的叔父节点为黑色
                    if (pnode == pnode->parent->rightChild)
                    {
                        // 若当前节点为其父节点的右子树
                        // 需要先左旋后右旋
                        pnode = pnode->parent;
                        lRotate(pnode);
                    }
                    // 执行重新着色和右旋
                    pnode->parent->color = BLACK;
                    pnode->parent->parent->color = RED;
                    rRotate(pnode->parent->parent);
                }
            }
            else
            {
                // 当前节点的父节点是其父节点的右子树
                tnode = pnode->parent->parent->leftChild;
                if (tnode->color == RED)
                {
                    // 如果当前节点的叔父节点为红色
                    // 只需对当前节点、其父节点和叔父节点、其祖父节点重新着色
                    // 并将判定节点选定为当前节点的祖父节点
                    pnode->parent->color = BLACK;
                    tnode->color = BLACK;
                    pnode->parent->parent->color = RED;
                    pnode = pnode->parent->parent;
                }
                else
                {
                    // 当前节点的叔父节点为黑色
                    if (pnode == pnode->parent->leftChild)
                    {
                        // 若当前节点为其父节点的右子树
                        // 需要先右旋后左旋
                        pnode = pnode->parent;
                        rRotate(pnode);
                    }
                    // 执行重新着色和左旋
                    pnode->parent->color = BLACK;
                    pnode->parent->parent->color = RED;
                    lRotate(pnode->parent->parent);
                }
            }
        }
        root()->color = BLACK;
    }

    _RBTNodePtr eraseNode(_RBTNodePtr eraseNode)
    {
        // 删除节点
        if (eraseNode == NULL || eraseNode->isNil)
        {
            return NULL;
        }
        // 首先保存待删除节点的后继结点
        _RBTNodePtr successorNode = treeSuccessor(eraseNode);

        _RBTNodePtr fixnode;    // 需要重新着色的节点
        _RBTNodePtr fixnodeparent;  // fixnode的父节点，可能为空
        _RBTNodePtr pnode = eraseNode;

        // 确定删除的节点
        if (pnode->leftChild->isNil)
        {
            // 待删除节点的左子树为空，需要将右子树连接到该节点的位置
            fixnode = pnode->rightChild;
        }
        else if (pnode->rightChild->isNil)
        {
            // 待删除节点的右子树为空，需要将左子树连接到该节点的位置
            fixnode = pnode->leftChild;
        }
        else
        {
            // 待删除节点的两个子树都不为空
            // 实际删除的节点为当前节点的后继结点
            // 并需要将当前节点替换为后继结点
            pnode = successorNode;
            fixnode = pnode->rightChild;
        }

        if (pnode == eraseNode)
        {
            // 待删除节点最多只有一个子树，直接重连接子树
            fixnodeparent = eraseNode->parent;
            if (!fixnode->isNil)
            {
                fixnode->parent = fixnodeparent;
            }

            if (eraseNode == root())
            {
                root() = fixnode;
            }
            else if (fixnodeparent->leftChild == eraseNode)
            {
                fixnodeparent->leftChild = fixnode;
            }
            else
            {
                fixnodeparent->rightChild = fixnode;
            }

            if (lmost() == eraseNode)
            {
                lmost() = fixnode->isNil ? fixnodeparent : _tmin(fixnode);
            }
            if (rmost() == eraseNode)
            {
                rmost() = fixnode->isNil ? fixnodeparent : _tmax(fixnode);
            }
        }
        else
        {
            // 待删除的节点两个子树都不为空
            // 需要将待删除的节点替换为后继结点并删除原来的后继结点
            eraseNode->leftChild->parent = pnode;
            pnode->leftChild = eraseNode->leftChild;

            if (pnode == eraseNode->rightChild)
            {
                fixnodeparent = pnode;
            }
            else
            {
                fixnodeparent = pnode->parent;
                if (!fixnode->isNil)
                {
                    fixnode->parent = fixnodeparent;
                }
                fixnodeparent->leftChild = fixnode;
                pnode->rightChild = eraseNode->rightChild;
                eraseNode->rightChild->parent = pnode;
            }

            if (eraseNode == root())
            {
                root() = pnode;
            }
            else if (eraseNode->parent->leftChild == eraseNode)
            {
                eraseNode->parent->leftChild = pnode;
            }
            else
            {
                eraseNode->parent->rightChild = pnode;
            }

            pnode->parent = eraseNode->parent;
            // 交换pnode和delnode的颜色
            eraseNode->color ^= pnode->color;
            pnode->color ^= eraseNode->color;
            eraseNode->color ^= pnode->color;
        }

        if (eraseNode->color == BLACK)
        {
            // 删除了一个黑色节点，需要调整树以重新达到平衡
            for (; fixnode != root() && fixnode->color == BLACK; 
                 fixnodeparent = fixnode->parent)
            {
                if (fixnode == fixnodeparent->leftChild)
                {
                    // 当前节点为其父节点的右子树
                    pnode = fixnodeparent->rightChild;
                    if (pnode->color == RED)
                    {
                        pnode->color = BLACK;
                        fixnodeparent->color = RED;
                        lRotate(fixnodeparent);
                        pnode = fixnodeparent->rightChild;
                    }

                    if (pnode->isNil)
                    {
                        fixnode = fixnodeparent;
                    }
                    else if (pnode->leftChild->color == BLACK && pnode->rightChild->color == BLACK)
                    {
                        pnode->color = RED;
                        fixnode = fixnodeparent;
                    }
                    else
                    {
                        if (pnode->rightChild->color == BLACK)
                        {
                            pnode->leftChild->color = BLACK;
                            pnode->color = RED;
                            rRotate(pnode);
                            pnode = fixnodeparent->rightChild;
                        }
                        pnode->color = fixnodeparent->color;
                        fixnodeparent->color = BLACK;
                        pnode->rightChild->color = BLACK;
                        lRotate(fixnodeparent);
                        break;
                    }
                }
                else
                {
                    // 当前节点为其父节点的左子树
                    pnode = fixnodeparent->leftChild;
                    if (pnode->color == RED)
                    {
                        pnode->color = BLACK;
                        fixnodeparent->color = RED;
                        rRotate(fixnodeparent);
                        pnode = fixnodeparent->leftChild;
                    }

                    if (pnode->isNil)
                    {
                        fixnode = fixnodeparent;
                    }
                    else if (pnode->rightChild->color == BLACK && pnode->leftChild->color == BLACK)
                    {
                        pnode->color = RED;
                        fixnode = fixnodeparent;
                    }
                    else
                    {
                        if (pnode->leftChild->color == BLACK)
                        {
                            pnode->rightChild->color = BLACK;
                            pnode->color = RED;
                            lRotate(pnode);
                            pnode = fixnodeparent->leftChild;
                        }

                        pnode->color = fixnodeparent->color;
                        fixnodeparent->color = BLACK;
                        pnode->leftChild->color = BLACK;
                        rRotate(fixnodeparent);
                        break;
                    }
                }
            }
            fixnode->color = BLACK;
        }
        // 删除节点
        m_nodealloc.dealloc(eraseNode);
        if (m_size > 0)
        {
            --m_size;
        }
        return successorNode;
    }

    void eraseSubTree(_RBTNodePtr rootnode)
    {
        // 删除以指定节点为根的子树（包含根节点）
        for (_RBTNodePtr pnode = rootnode;
             !pnode->isNil;
             rootnode = pnode)
        {
            // 递归删除
            eraseSubTree(pnode->rightChild);
            pnode = pnode->leftChild;
            m_nodealloc.dealloc(rootnode);
        }
    }

    static _RBTNodePtr treePrecursor(_RBTNodePtr node)
    {
        // 获取指定节点的前驱节点
        if (node->isNil)
        {
            return rmost();
        }
        else if (!node->leftChild->isNil)
        {
            return _tmax(node->leftChild);
        }
        else
        {
            _RBTNodePtr pnode;
            while (!(pnode = node->parent)->isNil
                   && node == pnode->leftChild)
            {
                node = pnode;
            }
            if (node->isNil)
            {
                return node;
            }
            else
            {
                return pnode;
            }
        }
    }

    static _RBTNodePtr treeSuccessor(_RBTNodePtr node)
    {
        // 获取指定节点的后继结点
        if (node->isNil)
        {
            return node;
        }
        else if (!node->rightChild->isNil)
        {
            return _tmin(node->rightChild);
        }
        else
        {
            _RBTNodePtr pnode;
            while (!(pnode = node->parent)->isNil
                   && node == pnode->rightChild)
            {
                node = pnode;
            }
            node = pnode;
            return node;
        }
    }

    void lRotate(_RBTNodePtr node)
    {
        // 对指定节点进行左旋操作
        _RBTNodePtr pnode = node->rightChild;
        node->rightChild = pnode->leftChild;

        if (!pnode->leftChild->isNil)
        {
            pnode->leftChild->parent = node;
        }
        pnode->parent = node->parent;

        if (node == root())
        {
            root() = pnode;
        }
        else if (node == node->parent->leftChild)
        {
            node->parent->leftChild = pnode;
        }
        else
        {
            node->parent->rightChild = pnode;
        }

        pnode->leftChild = node;
        node->parent = pnode;
    }
    void rRotate(_RBTNodePtr node)
    {
        // 对指定节点进行右旋操作
        _RBTNodePtr pnode = node->leftChild;
        node->leftChild = pnode->rightChild;

        if (!pnode->rightChild->isNil)
        {
            pnode->rightChild->parent = node;
        }
        pnode->parent = node->parent;

        if (node == root())
        {
            root() = pnode;
        }
        else if (node == node->parent->rightChild)
        {
            node->parent->rightChild = pnode;
        }
        else
        {
            node->parent->leftChild = pnode;
        }

        pnode->rightChild = node;
        node->parent = pnode;
    }

    const _RBTNodePtr& root() const
    {
        // 获取根节点
        return m_head->parent;
    }
    _RBTNodePtr& root()
    {
        // 获取根节点
        return m_head->parent;
    }
    const _RBTNodePtr& lmost() const
    {
        // 获取最小值节点
        return m_head->leftChild;
    }
    _RBTNodePtr& lmost()
    {
        // 获取最小值节点
        return m_head->leftChild;
    }
    const _RBTNodePtr& rmost() const
    {
        // 获取最大值节点
        return m_head->rightChild;
    }
    _RBTNodePtr& rmost()
    {
        // 获取最大值节点
        return m_head->rightChild;
    }
    bool copytree(_RBTNodePtr srcroot, _RBTNodePtr& dstroot)
    {
        // 不拷贝空节点
        if (srcroot->isNil)
        {
            // 根节点未拷贝
            return false;
        }
        // 首先拷贝根节点
        dstroot = m_nodealloc.alloc(srcroot);
        ++m_size;
        // 递归拷贝左子树
        if (copytree(srcroot->leftChild, dstroot->leftChild))
        {
            // 如果源树的左子树不为空，设置其父节点
            dstroot->leftChild->parent = dstroot;
        }
        else
        {
            // 否则将新根节点的左子树置为空
            dstroot->leftChild = m_head;
        }
        // 递归拷贝右子树
        if (copytree(srcroot->rightChild, dstroot->rightChild))
        {
            // 如果源树的右子树不为空，设置其父节点
            dstroot->rightChild->parent = dstroot;
        }
        else
        {
            // 否则将新根节点的左子树置为空
            dstroot->rightChild = m_head;
        }
        // 根节点已拷贝
        return true;
    }
    static _RBTNodePtr _tmin(_RBTNodePtr node)
    {
        // 查找以指定节点为根的树最小值指点
        while (!node->leftChild->isNil)
        {
            node = node->leftChild;
        }
        return node;
    }
    static _RBTNodePtr _tmax(_RBTNodePtr node)
    {
        // 查找以指定节点为根的树最大值指点
        while (!node->rightChild->isNil)
        {
            node = node->rightChild;
        }
        return node;
    }

    // 头节点，整棵树的入口点
    // 如果树为空
    //  头节点的父节点指针、左子节点指针和右子节点自身均指向自身
    // 如果树不为空
    //  头节点的父节点指针指向树的根节点
    //  左子节点指针指向整棵树的最小值节点（begin()获取的节点）
    //  右子节点指针指向整棵树的最大值节点（end()的前一个节点）
    // 整棵树只有头节点为空，所有的叶节点的左右子树指针均指向该节点
    _RBTNodePtr m_head;
    // 键值比较函数对象
    _KeyCompare m_ltcmp;
    // 节点分配器
    _NodeAllocator m_nodealloc;
    // 当前树的有效节点数
    unsigned int m_size;
};

#endif // BRTREE_H__
