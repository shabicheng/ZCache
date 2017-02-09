#ifndef ZRPair_h__
#define ZRPair_h__

// 二元组类
template<typename _FirstType, typename _SecondType>
struct ZRPair
{
	ZRPair()
		:first(), second()
	{
	}
	ZRPair(const _FirstType& val1, const _SecondType& val2)
		:first(val1), second(val2)
	{
	}
	_FirstType first;
	_SecondType second;
};

// 创建一个ZRPair
template<typename _FirstType, typename _SecondType>
ZRPair<_FirstType, _SecondType> ZRMakePair(const _FirstType& first, const _SecondType& second)
{
	return ZRPair<_FirstType, _SecondType>(first, second);
}


#endif // ZRPair_h__
