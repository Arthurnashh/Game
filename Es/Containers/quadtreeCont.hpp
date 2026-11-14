#ifdef _MSC_VER
#pragma once
#endif

#ifndef __QUAD_TREE_CONT_HPP
#define __QUAD_TREE_CONT_HPP

#include "array.hpp"

//! Quad tree container, continous indexing
template <class Type, class Traits=FindArrayKeyTraits<Type>, class Allocator=MemAllocD>
class QuadTreeCont
{
protected:
	AutoArray<Type, Allocator> _values;
	AutoArray<int, Allocator> _index;
	AutoArray<int, Allocator> _free;

	int _maxLevel;
	//int _rootIndex;
	int GetRootIndex() const {return _index[0];}

public:
	//! Constructor
	/*!
	\param defaultValue default value - returned if indices are out of bound
	*/
	QuadTreeCont()
	{
		Clear();
	}
	//! Clear whole content of container
	void Clear();

	//! Return value at [x, y]
	//const Type &Get(int x, int y);
	//! Set value at [x, y] to value
	void Set(int x, int y, const Type &value);

	template <class DetectRegionF, class ActionF>
	bool ForEachInRegion
	(
		int index, int size, const int x, const int y, DetectRegionF detect, ActionF &action
	)
	{
		size >>= 1;
		// check into which children we should descend
		if (detect(x,y,size))
		{
			int nindex = _index[index + 0];
			if (nindex>=0)
			{
				if (ForEachInRegion(nindex,size,x,y,detect,action)) return true;
			}
			else
			{
				Assert(nindex < 0 && -nindex <= _values.Size());
				if (nindex!=-1)
				{
					if (action(_values[-nindex - 1])) return true;
				}
			}
		}
		if (detect(x,y+size,size))
		{
			int nindex = _index[index + 1];
			if (nindex>=0)
			{
				if (ForEachInRegion(nindex,size,x,y+size,detect,action)) return true;;
			}
			else
			{
				Assert(nindex < 0 && -nindex <= _values.Size());
				if (nindex!=-1)
				{
					if (action(_values[-nindex - 1])) return true;
				}
			}
		}
		if (detect(x+size,y,size))
		{
			int nindex = _index[index + 2];
			if (nindex>=0)
			{
				if (ForEachInRegion(nindex,size,x+size,y,detect,action)) return true;;
			}
			else
			{
				Assert(nindex < 0 && -nindex <= _values.Size());
				if (nindex!=-1)
				{
					if (action(_values[-nindex - 1])) return true;
				}
			}
		}
		if (detect(x+size,y+size,size))
		{
			int nindex = _index[index + 3];
			if (nindex>=0)
			{
				if (ForEachInRegion(nindex,size,x+size,y+size,detect,action)) return true;;
			}
			else
			{
				Assert(nindex < 0 && -nindex <= _values.Size());
				if (nindex!=-1)
				{
					if (action(_values[-nindex - 1])) return true;
				}
			}
		}
		return false;
	}

	//! execute action on all elements selected by detect
	/*!
	When action returns true on any element, no more elements are processed and true is returned.
	*/
	template <class DetectRegionF, class ActionF>
	bool ForEachInRegion
	(
		DetectRegionF detect, ActionF &action
	)
	{
    int rootIndex = GetRootIndex();
    if (rootIndex == -1) return false;
		return ForEachInRegion(rootIndex,1<<_maxLevel,0,0,detect,action);
	}

  // constant variant of ForEachInRegion
  template <class DetectRegionF, class ActionF>
  bool ForEachInRegion
  (
    int index, int size, const int x, const int y, DetectRegionF detect, const ActionF &action
  ) const
  {
    size >>= 1;
    // check into which children we should descend
    if (detect(x,y,size))
    {
      int nindex = _index[index + 0];
      if (nindex>=0)
      {
        if (ForEachInRegion(nindex,size,x,y,detect,action)) return true;
      }
      else
      {
        Assert(nindex < 0 && -nindex <= _values.Size());
        if (nindex!=-1)
        {
          if (action(_values[-nindex - 1])) return true;
        }
      }
    }
    if (detect(x,y+size,size))
    {
      int nindex = _index[index + 1];
      if (nindex>=0)
      {
        if (ForEachInRegion(nindex,size,x,y+size,detect,action)) return true;;
      }
      else
      {
        Assert(nindex < 0 && -nindex <= _values.Size());
        if (nindex!=-1)
        {
          if (action(_values[-nindex - 1])) return true;
        }
      }
    }
    if (detect(x+size,y,size))
    {
      int nindex = _index[index + 2];
      if (nindex>=0)
      {
        if (ForEachInRegion(nindex,size,x+size,y,detect,action)) return true;;
      }
      else
      {
        Assert(nindex < 0 && -nindex <= _values.Size());
        if (nindex!=-1)
        {
          if (action(_values[-nindex - 1])) return true;
        }
      }
    }
    if (detect(x+size,y+size,size))
    {
      int nindex = _index[index + 3];
      if (nindex>=0)
      {
        if (ForEachInRegion(nindex,size,x+size,y+size,detect,action)) return true;;
      }
      else
      {
        Assert(nindex < 0 && -nindex <= _values.Size());
        if (nindex!=-1)
        {
          if (action(_values[-nindex - 1])) return true;
        }
      }
    }
    return false;
  }

  //! execute action on all elements selected by detect
  /*!
  When action returns true on any element, no more elements are processed and true is returned.
  */
  template <class DetectRegionF, class ActionF>
  bool ForEachInRegion
  (
    DetectRegionF detect, const ActionF &action
  ) const
  {
    int rootIndex = GetRootIndex();
    if (rootIndex == -1) return false;
    return ForEachInRegion(rootIndex,1<<_maxLevel,0,0,detect,action);
  }

protected:
	//! Add new node to index
	int AddNode();
	//! Compare values
	int IsEqual(const Type &a, const Type &b)
	{
		return Traits::IsEqual(Traits::GetKey(a), Traits::GetKey(b));
	}
};

template <class Type, class Traits, class Allocator>
void QuadTreeCont<Type, Traits, Allocator>::Clear()
{
	_values.Clear();
	_index.Clear();
	_free.Clear();

	// _values[0] is always default value
	_values.Add();

	_maxLevel = 0;
	_index.Add(-1);
}

/*
template <class Type, class Traits, class Allocator>
const Type &QuadTreeCont<Type, Traits, Allocator>::Get(int x, int y)
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
*/

template <class Type, class Traits, class Allocator>
void QuadTreeCont<Type, Traits, Allocator>::Set(int x, int y, const Type &value)
{
  int size = 1 << _maxLevel;

  // check bounds
  //if (_rootIndex!=-1)
  {
    while ((x | y) & ~(size - 1))
    {
      // out of bounds - extend
      int n = AddNode();
      _index[n + 0] = GetRootIndex();
      _index[n + 1] = -1;	// default value
      _index[n + 2] = -1;	// default value
      _index[n + 3] = -1;	// default value

      _index[0] = n;

      _maxLevel++;
      size <<= 1;
    }
  }

  // find current leaf
  int startX = 0, startY = 0;

  //int pathIndex = 0;
  //int path[8 * sizeof(int)];

  int iindex = 0;
  while (_index[iindex] >= 0)
  {
    //path[pathIndex++] = *index;

    size >>= 1;
    if (x < startX + size)
    {
      if (y < startY + size)
      {
        iindex = _index[iindex] + 0;
      }
      else
      {
        iindex = _index[iindex] + 1;
        startY += size;
      }
    }
    else
    {
      if (y < startY + size)
      {
        iindex = _index[iindex] + 2;
      }
      else
      {
        iindex = _index[iindex] + 3;
        startY += size;
      }
      startX += size;
    }
  }

  // check if change must be done

  int currentIndex = _index[iindex];
  int indexValue = -currentIndex - 1;
  Assert(indexValue >= 0 && indexValue < _values.Size());

  // leaf found
  int newIndex = _values.Add(value);
  if (indexValue==0)
  {
    // no value found
    _index[iindex] = -newIndex -1;
    return;
  }

  const Type &leaf = _values[indexValue];
  int leafX = leaf.GetX();
  int leafY = leaf.GetY();
  if (leafX==x && leafY==y)
  {
    // value can be replaced
    _index[iindex] = -newIndex -1;
    return;
  }

  // aggregation can be done
  //bool aggregate = size == 1;

  // split current leaf node
  while (size > 1)
  {
    // add new node to hierarchy
    int n = AddNode();
    _index[iindex] = n;

    size >>= 1;

    int xIndex = x >= startX + size;
    int yIndex = y >= startY + size;

    int leafXIndex = leafX >= startX + size;
    int leafYIndex = leafY >= startY + size;

    _index[n + 0] = -1;
    _index[n + 1] = -1;
    _index[n + 2] = -1;
    _index[n + 3] = -1;

    if (xIndex!=leafXIndex || yIndex!=leafYIndex)
    {
      // we have found a distinct place for a new leaf
      _index[n + leafXIndex*2+leafYIndex] = currentIndex;
      _index[n + xIndex*2+yIndex] = -newIndex -1;
      return;
    }

    iindex = n + xIndex*2+yIndex;
    startX += xIndex*size;
    startY += yIndex*size;
  }

  Fail("Not reached");
  _index[iindex] = -1;
}

template <class Type, class Traits, class Allocator>
int QuadTreeCont<Type, Traits, Allocator>::AddNode()
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

