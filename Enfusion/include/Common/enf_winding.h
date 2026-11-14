//-----------------------------------------------------------------------------
// File: enf_winding.h
//
// Desc: Polygon library.
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_WINDING_H
#define ENF_WINDING_H

#include "enf_config.h"
#include "common/enf_mathlib.h"

namespace enf
{
/** @addtogroup Math3D
@{*/

enum ClipSide
{
	SIDE_ON		= 0,
	SIDE_FRONT	= 1,
	SIDE_BACK	= 2,
	SIDE_CROSS	= 3
};

const int MAX_WINDING_POINTS = 32767;

//! n-point polygon.
class Winding
{
private:
	void* operator new(NEW_SIZE_T size, char const * file, int line) THROW_NEW;
	void* operator new[](NEW_SIZE_T size, char const * file, int line) THROW_NEW;
	void* operator new(NEW_SIZE_T size) THROW_NEW;
	void* operator new[](NEW_SIZE_T size) THROW_NEW;

	short		NumPoints;
	short		MaxPoints;

public:
	Winding*	Next;
	Vector3	p[1];
	int GetNumPoints() const
	{
		return NumPoints;
	}

	Winding(): Next(NULL), NumPoints(0), MaxPoints(0) {}
	~Winding() {}

	static Winding* Alloc(int np)
	{
		Assert(np <= MAX_WINDING_POINTS);
		int size = sizeof(Vector3) * (np - 1) + sizeof(Winding);
    Winding* w = (Winding*)GMemFunctions->New(size, __FILE__, __LINE__);
		w->Next = NULL;
		w->NumPoints = 0;
		w->MaxPoints = np;
		return w;
	}

	static Winding* Create(const Vector3* pts, int np)
	{
		Winding* w = Alloc(np);
		w->SetPoints(pts, np);
		return w;
	}

	const Vector3& AddPoint(const Vector3& np)
	{
		Assert(NumPoints < MaxPoints);
		p[NumPoints] = np;
		return p[NumPoints++];
	}
	
	void SetPoints(const Vector3* points, int numPoints)
	{
		Assert(numPoints <= MaxPoints);
		NumPoints = numPoints;
		CopyMem(p, points, numPoints * sizeof(Vector3));
	}

	Vector3* AppendPoints(int newPoints)
	{
		Assert((NumPoints + newPoints) <= MaxPoints);

		Vector3* ptr = p + NumPoints;
		NumPoints += newPoints;
		return ptr;
	}

	void Clear() { NumPoints = 0; }

	float Plane(Vector3& normal) const
	{
		Vector3 v1 = p[1] - p[0];
		Vector3 v2 = p[2] - p[0];
    normal = (v2.CrossProduct(v1)).Normalized();
    return normal.DotProduct(p[0]);
	}

	float	Area() const
	{
		float	total = 0;

		for (int i = 2; i < GetNumPoints() ; i++)
		{
			Vector3 d1 = p[i - 1] - p[0];
			Vector3 d2 = p[i] - p[0];
			Vector3 cross = d1.CrossProduct(d2);
      total += 0.5f * cross.Size();
		}
		return total;
	}

	AAB Bounds() const
	{
		AAB bounds(p[0]);

		for (int i = 1; i < GetNumPoints(); i++)
		{
			bounds.Add(p[i]);
		}

		return bounds;
	}

	Vector3	Center() const
	{
		Vector3 center(p[0]);

		for (int i = 1 ;i < GetNumPoints() ; i++)
			center += p[i];

		center *= 1.0f / (float)GetNumPoints();
		return center;
	}


	Winding* Copy() const
	{
		return Create(p, GetNumPoints());
	}

	Winding* CopyReversed() const;
	ClipSide OnPlaneSide(const Vector3& normal, float dist) const;
	void ClipEpsilon(const Vector3& normal, float dist, float epsilon, Winding** front, Winding** back) const;

	static Winding* BaseWindingForPlane (Vector4 plane);
	static Winding* ChopInPlace (Winding* in, const Vector3& normal, float dist, float epsilon);
	static Winding* Chop(Winding* in, const Vector3& normal, float dist);

  void operator delete(void* ptr, char const * file, int line){GMemFunctions->Delete(ptr);}
	void operator delete[](void* ptr, char const * file, int line){GMemFunctions->Delete(ptr);}
	void operator delete(void* ptr) { GMemFunctions->Delete(ptr);}
	void operator delete[](void* ptr) { GMemFunctions->Delete(ptr);}
	void* operator new(size_t size, void *p){ return p;}
	void* operator new[](size_t size, void *p){ return p;}
	void operator delete(void* ptr, void* p) { }
	void operator delete[](void* ptr, void* p) { }

	void RemoveColinearPoints();
	bool	Cleanup();
	bool	Check() const;
};
//@}

}

#endif //ENF_WINDING_H
