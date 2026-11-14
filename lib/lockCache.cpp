// AI - implementation of field locking

#include "wpch.hpp"
#include "AI/aiTypes.hpp"
#include "lockCache.hpp"
#include "AI/operMap.hpp"
#include "landscape.hpp"

#include "global.hpp"

#include "Es/Containers/quadtreeEx.hpp"

/*!
\patch_internal 1.45 Date 2/20/2002 by Ondra
- New: LandRange dependencies isolated - preparation of variable LandRange.
New interfaces introduced: IAIPathPlanner, ILockCache, IOperCache
*/


DEFINE_FAST_ALLOCATOR(LockField)

LockField::LockField(LandIndex x, LandIndex z)
{
	Assert(InRange(z, x));
	memset(_locks, 0, sizeof(_locks));
	_x = (WORD)x;
	_z = (WORD)z;
}

LockField::~LockField()
{
  if (GLandscape && GLandscape->LockingCache())
    GLandscape->LockingCache()->RemoveField(LandIndex(_x), LandIndex(_z));
}

int LockField::IsLocked(OperFieldIndex x, OperFieldIndex z, bool soldier)
{
	Assert(x >= 0 && x < OperItemRange && z >= 0 && z < OperItemRange);
	return soldier ?
		(_locks[z][x] & LockMaskAll) :	// soldiers not testing the LockMaskVeh layer
		_locks[z][x];
}

void LockField::Lock(OperFieldIndex x, OperFieldIndex z, bool soldier, bool lock)
{
	Assert(x >= 0 && x < OperItemRange && z >= 0 && z < OperItemRange);
	if (soldier)
	{
		if (lock)
		{
			if ((_locks[z][x] & LockMaskVeh) < 0x0f)
				_locks[z][x]++;
		}
		else
		{
			if ((_locks[z][x] & LockMaskVeh) > 0)
				_locks[z][x]--;
		}
	}
	else
	{
		if (lock)
		{
			if ((_locks[z][x] & LockMaskAll) < 0xf0)
				_locks[z][x]+=0x10;
		}
		else
		{
			if ((_locks[z][x] & LockMaskAll) > 0)
				_locks[z][x]-=0x10;
		}
	}
}

//#define FieldsRange		(LandRange / BigFieldSize)

// note: class definition is in cpp file, as interface ILockCache
// is used in other sources

TypeIsSimple(LockField*)

class LockCache: public ILockCache
{
private:
  QuadTreeExRoot<LockField *> _index;
  int _count;

public:
  LockCache(Landscape *land);

	int IsLocked(OperMapIndex x, OperMapIndex z, bool soldier);
	Ref<LockField> GetField(LandIndex x, LandIndex z, bool create); 
	bool IsEmpty() const;

private:
  void RemoveField(LandIndex x, LandIndex z);
};

ILockCache *CreateLockCache(Landscape *land){return new LockCache(land);}

// DEFINE_FAST_ALLOCATOR(QuadTreeExRoot<LockField *>::QuadTreeType)

LockCache::LockCache(Landscape *land)
: _index(land->GetLandRange(), land->GetLandRange(), NULL), _count(0)
{
};

int LockCache::IsLocked(OperMapIndex x, OperMapIndex z, bool soldier)
{
  if (x < 0 || x >= OperItemRange * LandRange || z < 0 || z >= OperItemRange * LandRange) return 0;
  SPLIT_INDICES(x, z, x0, z0, xx, zz);

  Ref<LockField> fld = GetField(x0, z0, false);
  if (!fld) return 0;

	return fld->IsLocked(xx, zz, soldier);
}

bool LockCache::IsEmpty() const
{
  return _count == 0;
}

Ref<LockField> LockCache::GetField(LandIndex x, LandIndex z, bool create) 
{
  if (!InRange(z, x)) return NULL;

  Ref<LockField> fld = _index(x,z);
  if (fld) return fld;

  if (!create) return NULL;

  // create a new entry
  fld = new LockField(x, z);
  _count++;
  _index.Set(x, z, fld);
  return fld;
}

void LockCache::RemoveField(LandIndex x, LandIndex z)
{
  Assert(InRange(z, x));
  Assert(_index(x, z));

  _index.Set(x, z, NULL);
  _count--;
}
