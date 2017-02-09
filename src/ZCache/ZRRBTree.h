#ifndef BRTREE_H__
#define BRTREE_H__

#include "ZRPair.h"
#include <stdlib.h>
#include <new>
using std::nothrow;

// �ж�С�ڵĺ�����
template<typename T>
class _less
{
public:
    bool operator()(const T& left, const T& right) const
    {
        return left < right;
    }
};

// �ж���ȵĺ�����
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
    // �ڵ���ɫö��
    enum _NodeColor{ RED, BLACK };

    // �ڵ�����
    typedef _NodeType _RBTNode;
    // �ڵ�ָ������
    typedef _RBTNode* _RBTNodePtr;
    // �ڵ�ļ�����
    typedef typename _RBTNode::_NKeyType _NKeyType;
    // �ڵ��ֵ����
    typedef typename _RBTNode::_NValType _NValType;
    // ������ҵĽ��
    struct _FindRstType
    {
        _FindRstType(_RBTNodePtr curnode, _RBTNodePtr parentnode, bool left)
        :node(curnode), pnode(parentnode), addleft(left){}
        _RBTNodePtr node;
        _RBTNodePtr pnode;
        bool addleft;
    };

    // ��ȡָ���ڵ�ĺ�̽��ĺ�����
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

    // ��ȡ�ڵ㱣���ֵ�ĺ�����
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

    // ָ��const���͵ĵ��������������޸���ָ���ֵ
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

    // ��ͨ������
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
    // ��ͨ������
    typedef _ZRIterator _ZRTreeIterator;
    // const������
    typedef _ZRConstIterator _ZRTreeConstIterator;
    // ������
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

    // �������캯��
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

    // ��ֵ������
    ZRRBTree& operator=(const ZRRBTree& oldtree)
    {
        if (this == &oldtree)
        {
            // �Ը�ֵ������Ҫ�����κδ���
            return *this;
        }
        // �������ԭ�еĽڵ�
        clear();
        // ��ʼ���и���
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

    // ������������Ҫɾ�����еĽڵ�
    ~ZRRBTree()
    {
        clear();
        m_nodealloc.dealloc(m_head);
    }

    // ��ȡ��һ���ڵ�
    // �����Ϊ�գ�����end()
    _ZRTreeIterator begin()
    {
        _ZRTreeIterator iter;
        iter.m_container = this;
        iter.m_val = lmost();
        return iter;
    }

    // ��ȡ���һ���ڵ����һ���ڵ�
    _ZRTreeIterator end()
    {
        _ZRTreeIterator iter;
        iter.m_container = this;
        iter.m_val = m_head;
        return iter;
    }

    // ��ȡ��һ���ڵ��const������
    // �����Ϊ�գ�����end()
    _ZRTreeConstIterator begin() const
    {
        _ZRTreeConstIterator iter;
        iter.m_container = this;
        iter.m_val = lmost();
        return iter;
    }

    // ��ȡ���һ���ڵ����һ���ڵ��const������
    _ZRTreeConstIterator end() const
    {
        _ZRTreeConstIterator iter;
        iter.m_container = this;
        iter.m_val = m_head;
        return iter;
    }

    // ɾ��ָ��key������Ľڵ�
    // ���ָ��keyδ�������ҵ���ֱ�ӷ���end()
    // ����ɾ���ɹ��󷵻�ԭ�������ĺ�̵�����
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

    // ɾ����������ָ��Ľڵ�
    // �����������ָ������Ľڵ��Ϊend()��ֱ�ӷ���end()
    // ����ɾ���ɹ��󷵻�ԭ�������ĺ�̵�����
    _ZRTreeIterator erase(_ZRTreeIterator& iter)
    {
        if (iter.m_container != this || iter.m_val->isNil)
        {
            return end();
        }
        // �ȱ����̽��
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
        // �ȱ����̽��
        _ZRTreeConstIterator successor = iter;
        ++successor;
        eraseNode(iter.m_val);
        return successor;
    }

    // ����keyֵ���ҽ��
    // ������ڣ����ض�Ӧ�ĵ�����
    // ���򷵻�end()
    _ZRTreeIterator find(_NKeyType& key)
    {
        _ZRTreeIterator rst;
        rst.m_container = this;
        rst.m_val = find_i(key);
        return rst;
    }
    // ����keyֵ���ҽ��
    // ������ڣ����ض�Ӧ�ĵ�����
    // ���򷵻�end()
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

    // ��ȡ��ǰ������Ч�ڵ���
    unsigned int size() const
    {
        return m_size;
    }

    // �жϵ�ǰ���Ƿ�Ϊ��
    bool empty() const
    {
        return m_size == 0UL;
    }

    // �����������
    void clear()
    {
        eraseSubTree(root());
        root() = m_head;
        lmost() = m_head;
        rmost() = m_head;
        m_size = 0;
    }
    // ��ȡ���ĸ߶�
    unsigned int height()
    {
        return height(root());
    }
    // �ݹ��ȡ���߶�
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

    // �ṩ��map�Ĳ���
    _InsertRstType insert_i(const _NKeyType& key, const _NValType& val)
    {
        _FindRstType findRst = insertfind_i(key);
        _InsertRstType rst;
        rst.first.m_container = this;
        if (!findRst.node->isNil)
        {
            // ��Ҫ�����key�Ѿ����ڣ�ֱ�ӷ���
            rst.first.m_val = findRst.node;
            rst.second = false;
            return rst;
        }
        // δ�ҵ�key��Ӧ�Ľڵ㣬�����½ڵ�
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
            // ��һ���ڵ�
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

    // ����keyֵ�����½ڵ�
    // ���key�����в����ڣ�������½ڵ㣬�������½ڵ�ĵ�����
    // ���򷵻ض�Ӧkey�Ľڵ������
    _InsertRstType insert_i(const _NKeyType& key)
    {
        _FindRstType findRst = insertfind_i(key);
        _InsertRstType rst;
        if (!findRst.node->isNil)
        {
            // ��Ҫ�����key�Ѿ�����
            rst.first.m_val = findRst.node;
            rst.second = false;
            return rst;
        }
        // δ�ҵ�key��Ӧ�Ľڵ㣬�����½ڵ�
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
            // ��һ���ڵ�
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
                // ����ڵ��ֵ��ԭ������СֵС
                lmost() = insertNode;
            }
        }
        else
        {
            findRst.pnode->rightChild = insertNode;
            if (rmost() == findRst.pnode)
            {
                // ����ڵ��ֵ��ԭ�������ֵ��
                rmost() = insertNode;
            }
        }
        rebalance(insertNode);
        return rst;
    }
protected:
    // ����ʱ�Ĳ�����Ҫ�����������Ϣ
    _FindRstType insertfind_i(const _NKeyType& key)
    {
        // ��Ҫ�������ҽ���ĸ��ڵ�Ͳ��ҽ���Ƿ������丸�ڵ��������
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
                // ���ҳɹ�
                return _FindRstType(curNode, pnode, leftChild);
            }
        }
        return _FindRstType(curNode, pnode, leftChild);
    }

    // �򵥵Ĳ��Ҳ�����ֻ�������ҽ��
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
                // ���ҳɹ�
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
        // ��Բ���������µĲ�ƽ��������ṹ
        for (_RBTNodePtr pnode = baseNode, tnode = NULL;
             pnode->parent->color == RED;)
        {
            if (pnode->parent == pnode->parent->parent->leftChild)
            {
                // ��ǰ�ڵ�ĸ��ڵ����丸�ڵ��������
                tnode = pnode->parent->parent->rightChild;
                if (tnode->color == RED)
                {
                    // �����ǰ�ڵ���常�ڵ�Ϊ��ɫ
                    // ֻ��Ե�ǰ�ڵ㡢�丸�ڵ���常�ڵ㡢���游�ڵ�������ɫ
                    // �����ж��ڵ�ѡ��Ϊ��ǰ�ڵ���游�ڵ�
                    pnode->parent->color = BLACK;
                    tnode->color = BLACK;
                    pnode->parent->parent->color = RED;
                    pnode = pnode->parent->parent;
                }
                else
                {
                    // ��ǰ�ڵ���常�ڵ�Ϊ��ɫ
                    if (pnode == pnode->parent->rightChild)
                    {
                        // ����ǰ�ڵ�Ϊ�丸�ڵ��������
                        // ��Ҫ������������
                        pnode = pnode->parent;
                        lRotate(pnode);
                    }
                    // ִ��������ɫ������
                    pnode->parent->color = BLACK;
                    pnode->parent->parent->color = RED;
                    rRotate(pnode->parent->parent);
                }
            }
            else
            {
                // ��ǰ�ڵ�ĸ��ڵ����丸�ڵ��������
                tnode = pnode->parent->parent->leftChild;
                if (tnode->color == RED)
                {
                    // �����ǰ�ڵ���常�ڵ�Ϊ��ɫ
                    // ֻ��Ե�ǰ�ڵ㡢�丸�ڵ���常�ڵ㡢���游�ڵ�������ɫ
                    // �����ж��ڵ�ѡ��Ϊ��ǰ�ڵ���游�ڵ�
                    pnode->parent->color = BLACK;
                    tnode->color = BLACK;
                    pnode->parent->parent->color = RED;
                    pnode = pnode->parent->parent;
                }
                else
                {
                    // ��ǰ�ڵ���常�ڵ�Ϊ��ɫ
                    if (pnode == pnode->parent->leftChild)
                    {
                        // ����ǰ�ڵ�Ϊ�丸�ڵ��������
                        // ��Ҫ������������
                        pnode = pnode->parent;
                        rRotate(pnode);
                    }
                    // ִ��������ɫ������
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
        // ɾ���ڵ�
        if (eraseNode == NULL || eraseNode->isNil)
        {
            return NULL;
        }
        // ���ȱ����ɾ���ڵ�ĺ�̽��
        _RBTNodePtr successorNode = treeSuccessor(eraseNode);

        _RBTNodePtr fixnode;    // ��Ҫ������ɫ�Ľڵ�
        _RBTNodePtr fixnodeparent;  // fixnode�ĸ��ڵ㣬����Ϊ��
        _RBTNodePtr pnode = eraseNode;

        // ȷ��ɾ���Ľڵ�
        if (pnode->leftChild->isNil)
        {
            // ��ɾ���ڵ��������Ϊ�գ���Ҫ�����������ӵ��ýڵ��λ��
            fixnode = pnode->rightChild;
        }
        else if (pnode->rightChild->isNil)
        {
            // ��ɾ���ڵ��������Ϊ�գ���Ҫ�����������ӵ��ýڵ��λ��
            fixnode = pnode->leftChild;
        }
        else
        {
            // ��ɾ���ڵ��������������Ϊ��
            // ʵ��ɾ���Ľڵ�Ϊ��ǰ�ڵ�ĺ�̽��
            // ����Ҫ����ǰ�ڵ��滻Ϊ��̽��
            pnode = successorNode;
            fixnode = pnode->rightChild;
        }

        if (pnode == eraseNode)
        {
            // ��ɾ���ڵ����ֻ��һ��������ֱ������������
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
            // ��ɾ���Ľڵ�������������Ϊ��
            // ��Ҫ����ɾ���Ľڵ��滻Ϊ��̽�㲢ɾ��ԭ���ĺ�̽��
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
            // ����pnode��delnode����ɫ
            eraseNode->color ^= pnode->color;
            pnode->color ^= eraseNode->color;
            eraseNode->color ^= pnode->color;
        }

        if (eraseNode->color == BLACK)
        {
            // ɾ����һ����ɫ�ڵ㣬��Ҫ�����������´ﵽƽ��
            for (; fixnode != root() && fixnode->color == BLACK; 
                 fixnodeparent = fixnode->parent)
            {
                if (fixnode == fixnodeparent->leftChild)
                {
                    // ��ǰ�ڵ�Ϊ�丸�ڵ��������
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
                    // ��ǰ�ڵ�Ϊ�丸�ڵ��������
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
        // ɾ���ڵ�
        m_nodealloc.dealloc(eraseNode);
        if (m_size > 0)
        {
            --m_size;
        }
        return successorNode;
    }

    void eraseSubTree(_RBTNodePtr rootnode)
    {
        // ɾ����ָ���ڵ�Ϊ�����������������ڵ㣩
        for (_RBTNodePtr pnode = rootnode;
             !pnode->isNil;
             rootnode = pnode)
        {
            // �ݹ�ɾ��
            eraseSubTree(pnode->rightChild);
            pnode = pnode->leftChild;
            m_nodealloc.dealloc(rootnode);
        }
    }

    static _RBTNodePtr treePrecursor(_RBTNodePtr node)
    {
        // ��ȡָ���ڵ��ǰ���ڵ�
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
        // ��ȡָ���ڵ�ĺ�̽��
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
        // ��ָ���ڵ������������
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
        // ��ָ���ڵ������������
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
        // ��ȡ���ڵ�
        return m_head->parent;
    }
    _RBTNodePtr& root()
    {
        // ��ȡ���ڵ�
        return m_head->parent;
    }
    const _RBTNodePtr& lmost() const
    {
        // ��ȡ��Сֵ�ڵ�
        return m_head->leftChild;
    }
    _RBTNodePtr& lmost()
    {
        // ��ȡ��Сֵ�ڵ�
        return m_head->leftChild;
    }
    const _RBTNodePtr& rmost() const
    {
        // ��ȡ���ֵ�ڵ�
        return m_head->rightChild;
    }
    _RBTNodePtr& rmost()
    {
        // ��ȡ���ֵ�ڵ�
        return m_head->rightChild;
    }
    bool copytree(_RBTNodePtr srcroot, _RBTNodePtr& dstroot)
    {
        // �������սڵ�
        if (srcroot->isNil)
        {
            // ���ڵ�δ����
            return false;
        }
        // ���ȿ������ڵ�
        dstroot = m_nodealloc.alloc(srcroot);
        ++m_size;
        // �ݹ鿽��������
        if (copytree(srcroot->leftChild, dstroot->leftChild))
        {
            // ���Դ������������Ϊ�գ������丸�ڵ�
            dstroot->leftChild->parent = dstroot;
        }
        else
        {
            // �����¸��ڵ����������Ϊ��
            dstroot->leftChild = m_head;
        }
        // �ݹ鿽��������
        if (copytree(srcroot->rightChild, dstroot->rightChild))
        {
            // ���Դ������������Ϊ�գ������丸�ڵ�
            dstroot->rightChild->parent = dstroot;
        }
        else
        {
            // �����¸��ڵ����������Ϊ��
            dstroot->rightChild = m_head;
        }
        // ���ڵ��ѿ���
        return true;
    }
    static _RBTNodePtr _tmin(_RBTNodePtr node)
    {
        // ������ָ���ڵ�Ϊ��������Сֵָ��
        while (!node->leftChild->isNil)
        {
            node = node->leftChild;
        }
        return node;
    }
    static _RBTNodePtr _tmax(_RBTNodePtr node)
    {
        // ������ָ���ڵ�Ϊ���������ֵָ��
        while (!node->rightChild->isNil)
        {
            node = node->rightChild;
        }
        return node;
    }

    // ͷ�ڵ㣬����������ڵ�
    // �����Ϊ��
    //  ͷ�ڵ�ĸ��ڵ�ָ�롢���ӽڵ�ָ������ӽڵ������ָ������
    // �������Ϊ��
    //  ͷ�ڵ�ĸ��ڵ�ָ��ָ�����ĸ��ڵ�
    //  ���ӽڵ�ָ��ָ������������Сֵ�ڵ㣨begin()��ȡ�Ľڵ㣩
    //  ���ӽڵ�ָ��ָ�������������ֵ�ڵ㣨end()��ǰһ���ڵ㣩
    // ������ֻ��ͷ�ڵ�Ϊ�գ����е�Ҷ�ڵ����������ָ���ָ��ýڵ�
    _RBTNodePtr m_head;
    // ��ֵ�ȽϺ�������
    _KeyCompare m_ltcmp;
    // �ڵ������
    _NodeAllocator m_nodealloc;
    // ��ǰ������Ч�ڵ���
    unsigned int m_size;
};

#endif // BRTREE_H__
