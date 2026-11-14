#ifdef _MSC_VER
#pragma once
#endif

#ifndef __QUAD_TREE_HPP
#define __QUAD_TREE_HPP

#include "array.hpp"

//! Quad tree container
template <class Type, class Traits=FindArrayKeyTraits<Type>, class Allocator=MemAllocD>
class QuadTree
{
protected:
	FindArrayKey<Type, Traits, Allocator> _values;
	AutoArray<int, Allocator> _index;
	AutoArray<int, Allocator> _free;

	Type _defaultValue;
	int _maxLevel;
	int _rootIndex;

public:
	//! Constructor
	/*!
	\param defaultValue default value - returned if indices are out of bound
	*/
	QuadTree(const Type &defaultValue=Type()) : _defaultValue(defaultValue)
	{
		Clear();
	}
	//! Clear whole content of container
	void Clear();

	//! Return value at [x, y]
	const Type &Get(int x, int y) const;
	//! Set value at [x, y] to value
	void Set(int x, int y, const Type &value);

protected:
	//! Add new node to index
	int AddNode();
	//! Compare values
	int IsEqual(const Type &a, const Type &b)
	{
		return Traits::IsEqual(Traits::GetKey(a), Traits::GetKey(b));
	}
  //@{ helper to allow single index pointing to _rootIndex as well
  int GetIndexed(int ii) const {return ii<0 ? _rootIndex : _index[ii];}
  void SetIndexed(int ii, int val) {if (ii<0) _rootIndex = val; else _index[ii]=val;}
  //@}
};

template <class Type, class Traits, class Allocator>
void QuadTree<Type, Traits, Allocator>::Clear()
{
	_values.Clear();
	_index.Clear();
	_free.Clear();

	// _values[0] is always default value
	_values.Add(_defaultValue);

	_maxLevel = 0;
	_rootIndex = -1;
}

template <class Type, class Traits, class Allocator>
const Type &QuadTree<Type, Traits, Allocator>::Get(int x, int y) const
{
	int size = 1 << _maxLevel;

	// check bounds
	if ((x | y) & ~(size - 1)) return _defaultValue; // out of bounds

	int startX = 0, startY = 0;

	int index = _rootIndex;
	while (index >= 0)
	{
		size >>= 1;
		if (x < startX + size)
		{
			if (y < startY + size)
			{
				index = _index[index + 0];
			}
			else
			{
				index = _index[index + 1];
				startY += size;
			}
		}
		else
		{
			if (y < startY + size)
			{
				index = _index[index + 2];
			}
			else
			{
				index = _index[index + 3];
				startY += size;
			}
			startX += size;
		}
	}
	Assert(index < 0 && -index <= _values.Size());
	return _values[-index - 1];
}

template <class Type, class Traits, class Allocator>
void QuadTree<Type, Traits, Allocator>::Set(int x, int y, const Type &value)
{
	int size = 1 << _maxLevel;

	// check bounds
	if ((x | y) & ~(size - 1))
	{
		if (IsEqual(value, _defaultValue)) return;
		do
		{
			// out of bounds - extend
			int n = AddNode();
			_index[n + 0] = _rootIndex;
			_index[n + 1] = -1;	// default value
			_index[n + 2] = -1;	// default value
			_index[n + 3] = -1;	// default value
			
			_rootIndex = n;
			_maxLevel++;
			size <<= 1;
		} while ((x | y) & ~(size - 1));
	}

	// find current leaf
	int startX = 0, startY = 0;

	int pathIndex = 0;
	int path[8 * sizeof(int)];

  // negative value means _rootIndex, non-negative index into _index array
  // note: we cannot use ordinary pointer, as AddNote may invalidate the content
	int indexIndex = -1;
	while (GetIndexed(indexIndex) >= 0)
	{
		path[pathIndex++] = GetIndexed(indexIndex);

		size >>= 1;
		if (x < startX + size)
		{
			if (y < startY + size)
			{
				indexIndex = GetIndexed(indexIndex) + 0;
			}
			else
			{
				indexIndex = GetIndexed(indexIndex) + 1;
				startY += size;
			}
		}
		else
		{
			if (y < startY + size)
			{
				indexIndex = GetIndexed(indexIndex) + 2;
			}
			else
			{
				indexIndex = GetIndexed(indexIndex) + 3;
				startY += size;
			}
			startX += size;
		}
	}
	
	// check if change must be done
	int currentIndex = GetIndexed(indexIndex);
	int indexValue = -currentIndex - 1;
	Assert(indexValue >= 0 && indexValue < _values.Size());
	if (IsEqual(_values[indexValue], value)) return;

	// aggregation can be done
	bool aggregate = size == 1;

	// split current leaf node
	while (size > 1)
	{
		// add new node to hierarchy
		int n = AddNode();
		SetIndexed(indexIndex,n);

		size >>= 1;
		if (x < startX + size)
		{
			if (y < startY + size)
			{
				// _index[n + 0] = 0;
				_index[n+ 1] = currentIndex;
				_index[n+ 2] = currentIndex;
				_index[n+ 3] = currentIndex;
				indexIndex = n+0;
			}
			else
			{
				_index[n+ 0] = currentIndex;
				// _index[n+ 1] = 0;
				_index[n+ 2] = currentIndex;
				_index[n+ 3] = currentIndex;
				indexIndex = n+ 1;
				startY += size;
			}
		}
		else
		{
			if (y < startY + size)
			{
				_index[n+ 0] = currentIndex;
				_index[n+ 1] = currentIndex;
				// _index[n+ 2] = 0;
				_index[n+ 3] = currentIndex;
				indexIndex = n+ 2;
			}
			else
			{
				_index[n+ 0] = currentIndex;
				_index[n+ 1] = currentIndex;
				_index[n+ 2] = currentIndex;
				// _index[n+ 3] = 0;
				indexIndex = n+ 3;
				startY += size;
			}
			startX += size;
		}
	}

	// set new value
	indexValue = _values.Find(value);
	if (indexValue < 0) indexValue = _values.Add(value);
	currentIndex = -indexValue - 1;
  SetIndexed(indexIndex,currentIndex);

	// aggregation
	if (aggregate)
	{
		for (int i=pathIndex-1; i>=0; i--)
		{
			int index = path[i];
			if (_index[index + 0] != currentIndex) break;
			if (_index[index + 1] != currentIndex) break;
			if (_index[index + 2] != currentIndex) break;
			if (_index[index + 3] != currentIndex) break;

			// aggregation is possible
			_free.Add(index);
			if (i == 0)
			{
				Assert(_rootIndex == index);
				_rootIndex = currentIndex;
			}
			else
			{
				int levelUp = path[i - 1];
				if (_index[levelUp + 0] == index) _index[levelUp + 0] = currentIndex;
				else if (_index[levelUp + 1] == index) _index[levelUp + 1] = currentIndex;
				else if (_index[levelUp + 2] == index) _index[levelUp + 2] = currentIndex;
				else if (_index[levelUp + 3] == index) _index[levelUp + 3] = currentIndex;
			}
		}
	}
	int i = _rootIndex;
	while
	(
		i >= 0 &&
		_index[i + 1] == -1 &&
		_index[i + 2] == -1 &&
		_index[i + 3] == -1
	)
	{
		_free.Add(i);
		_rootIndex = _index[i + 0];
		_maxLevel--;
		i = _rootIndex;
	}
}

template <class Type, class Traits, class Allocator>
int QuadTree<Type, Traits, Allocator>::AddNode()
{
	int n = _free.Size();
	if (n > 0)
	{
		int i = _free[n - 1];
		_free.Delete(n - 1);
		return i;
	}

	int i = _index.Size();
	_index.Access(i + 3);
	return i;
}

#endif
