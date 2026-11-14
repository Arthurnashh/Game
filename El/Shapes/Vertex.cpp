#include <Es/essencepch.hpp>
#include "StdAfx.h"
#include ".\vertex.h"

namespace ShapeFiles
{
	unsigned int VertexArray::GetNextFreeAtPos(unsigned int pos) const
	{
		Assert(pos < Size() && (_flags[pos] & fUsed) == 0);
		const double& p = _v[pos].x;
		const unsigned int& n = reinterpret_cast<const unsigned int&>(p);
		return n;
	}

	unsigned int VertexArray::GetPrevFreeAtPos(unsigned int pos) const
	{
		Assert(pos < Size() && (_flags[pos] & fUsed) == 0);
		const double& p = _v[pos].x;
		const unsigned int& n = reinterpret_cast<const unsigned int&>(p);
		return n;
	}

	void VertexArray::SetNextFreeAtPos(unsigned int pos, unsigned int nextFree)
	{
		Assert(pos < Size() && (_flags[pos] & fUsed) == 0);
		{    
			double& p = _v[pos].x;
			unsigned int& n = reinterpret_cast<unsigned int&>(p);
			n = nextFree;    
		}
		{
			double& p = _v[pos].y;
			unsigned int& n = reinterpret_cast<unsigned int&>(p);
			n = -1;
		}
		if (nextFree<Size())
		{
			double& p = _v[nextFree].y;
			unsigned int& n = reinterpret_cast<unsigned int&>(p);
			n = pos;
		}
	}

	unsigned int VertexArray::AllocVertex()
	{
		if (_nextFree < Size())
		{
			unsigned int nn = _nextFree;
			_nextFree = GetNextFreeAtPos(_nextFree);
			if (_nextFree > Size()) _nextFree = Size();
			_flags[nn] = fUsed;
			return nn;
		}
		else
		{
			if (_nextFree > Size()) _nextFree = Size();
			_v.Access(_nextFree);
			_flags.Access(_nextFree);
			_flags[_nextFree] = fUsed;
			return _nextFree++;
		}
	}

	void VertexArray::FreeVertex(unsigned int pos)
	{
		Assert(pos < Size());
		if (pos + 1 == Size())
		{
			_v.Resize(pos);
			_flags.Resize(pos);
			if (_nextFree > Size()) _nextFree = Size();
			while (Size() > 0 && _flags[Size() - 1] == 0)
			{
				unsigned int k = Size() - 1;
				unsigned int prev = GetPrevFreeAtPos(k);
				unsigned int next = GetNextFreeAtPos(k);
				if (prev != -1) SetNextFreeAtPos(prev, next);
				if (_nextFree == k) _nextFree = next;
				if (_nextFree > k) _nextFree = k;
				_v.Resize(k);
				_flags.Resize(k);
			}
		}
		else
		{
			Assert((_flags[pos] & fUsed) != 0);
			_flags[pos] = 0;
			SetNextFreeAtPos(pos, _nextFree);
			_nextFree = pos;
		}
	}

	unsigned int VertexArray::AddVertex(const DVertex& vx)
	{
		unsigned int vxp = AllocVertex();
		SetVertex(vxp, vx);
		return vxp;
	}

	bool VertexArray::DeleteVertex(unsigned int id)
	{
		if (id < Size() && (_flags[id] & fUsed) != 0)
		{
			FreeVertex(id);
			return true;
		}
		else
		{
			return false;
		}
	}

	static DVertex SNullVertex;

	bool DVertex::IsInvalidVertex() const
	{
		return &SNullVertex == this;
	}

	const DVertex& DVertex::GetInvalidVertex()
	{
		return SNullVertex;
	}

	const DVertex& VertexArray::GetVertex(unsigned int pos) const
	{
		if (this && pos < Size() && (_flags[pos] & fUsed) != 0)
		{
			return _v[pos];
		}
		else
		{
			return SNullVertex;
		}
	}

	bool VertexArray::SetVertex(unsigned int pos, const DVertex& vx)
	{
		if (this && pos < Size() && (_flags[pos] & fUsed) != 0)
		{
			_v[pos] = vx;
			return true;
		}
		else
		{
			return false;
		}
	}

  VertexArray::Iter VertexArray::First()
  {
    for (unsigned int i=0;i<Size();i++)
      if (_flags[i] & fUsed) return Iter(this,i);
    return Iter(this,-1);
  }

  VertexArray::IterC VertexArray::First() const
  {
    for (unsigned int i=0;i<Size();i++)
      if (_flags[i] & fUsed) return IterC(this,i);
    return IterC(this,-1);
  }

  VertexArray::Iter VertexArray::Last()
  {
    for (unsigned int i=Size()-1;i>=0;i--)
      if (_flags[i] & fUsed) return Iter(this,i);
    return Iter(this,-1);
  }

  VertexArray::IterC VertexArray::Last() const
  {
    for (unsigned int i=Size()-1;i>=0;i--)
      if (_flags[i] & fUsed) return IterC(this,i);
    return IterC(this,-1);
  }

  void VertexArray::NextVertex(unsigned int &vxPos) const
  {
    if (vxPos==-1) return;
    for (unsigned int i=vxPos+1;i<Size();i++)
      if (_flags[i] & fUsed) 
      {
        vxPos=i;
        return;
      }
    vxPos=-1;
  }

  void VertexArray::PrevVertex(unsigned int &vxPos) const
  {
    if (vxPos==-1) return;
    if (vxPos>=Size()) vxPos=Size();
    for (unsigned int i=vxPos-1;i>=0;i--)
      if (_flags[i] & fUsed) 
      {
        vxPos=i;
        return;
      }
      vxPos=-1;
  }

}