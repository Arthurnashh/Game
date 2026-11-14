//-----------------------------------------------------------------------------
// File: enf_mathlib2d.h
//
// Desc: Mathematics library
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_MATHLIB2D_H
#define ENF_MATHLIB2D_H

#include <float.h>
#include <math.h>
#include "enf_config.h"
//#include "common/enf_types.h"
#include "common/enf_mathlib.h"

class Vector3P;
typedef Vector3P Vector3;

namespace enf
{
/** @defgroup Math2D 2D Mathematic library
@ingroup Common
@{*/

//-----------------------------------------------------------------------------
#define BadVec2(a) (BadFloat(a[0]) || BadFloat(a[1]))

//class Vector3;

/*!
Plane in 2D
*/
class Plane2
{
public:
	union
	{
		float v[2];				///< array of three floats

		struct
		{
			float x, y;		///< x, y of the vector
		};
	};

	float d;

	//-----------------------------------------------------------------------------
	/*!
	Constructor, from two numbers
	\param ix	x
	\param iy	y
	\param id	distance
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE Plane2(float ix, float iy, float id)
	{
		x = ix; y = iy; d = id;
	}

	//-----------------------------------------------------------------------------
	/*!
	empty constructor
	*/
	//-----------------------------------------------------------------------------
	Plane2()
	{
	}
	
};

//-----------------------------------------------------------------------------
/*!
class Vector2
*/
//-----------------------------------------------------------------------------
class Vector2
{
public:

	//!vector data
	union
	{
		float v[2];				///< array of two floats

		struct
		{
			float x, y;		///< x, y of the vector
		};
	};

	//-----------------------------------------------------------------------------
	/*!
	Constructor, from two numbers
	\param ix	x
	\param iy	y
	*/
	//-----------------------------------------------------------------------------
  ENF_INLINE Vector2(float ix, float iy): x(ix), y(iy) {}

	//-----------------------------------------------------------------------------
	/*!
	Constructor, 'f' is copied to all components	
	\param f		f,f
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE Vector2(float f): x(f), y(f) {}

	Vector2(const Vector3& vec);

	//-----------------------------------------------------------------------------
	/*!
	empty constructor
	*/
	//-----------------------------------------------------------------------------
	Vector2()
	{
	}


	//-----------------------------------------------------------------------------
	/*!
	constructor, from a pointer to float[2]
	\param ptr	pointer to floats
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE Vector2(const float *ptr): x(ptr[0]), y(ptr[1]) {}

	//--------------------------------------------------------------
	/*!
	Is vector zero
	\return true if vector has all components zeroed
	*/
	//--------------------------------------------------------------
	ENF_INLINE bool IsZero() const
	{
		return (x == 0 && y == 0);
	}

	//--------------------------------------------------------------
	/*!
	Is vector epsilon zero
	\param eps	epsilon for testing
	\return true if vector has all components zeroed
	*/
	//--------------------------------------------------------------
	ENF_INLINE bool IsZero(float eps) const
	{
		return (fabs(x) < eps && fabs(y) < eps);
	}


	//--------------------------------------------------------------
	/*!
	Zero vector
	\return reference
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2& Zero()
	{
		x = y = 0.0f;
		return *this;
	}


	//--------------------------------------------------------------
	/*!
	operator [], get component
	\return component
	*/
	//--------------------------------------------------------------
	ENF_INLINE float operator[](uint index) const
	{
		Assert(index <= 1);
		return v[index];
	}


	//--------------------------------------------------------------
	/*!
	operator ==, Are vectors equal?
	\return true if they are equal
	*/
	//--------------------------------------------------------------
	ENF_INLINE bool operator==(const Vector2& in) const
	{
		return (x == in.x && y == in.y);
	}


	//--------------------------------------------------------------
	/*!
	operator !, Are not vectors equal?
	\return true if they are not equal
	*/
	//--------------------------------------------------------------
	ENF_INLINE bool operator!=(const Vector2& in) const
	{
		return (x != in.x || y != in.y);
	}


	//--------------------------------------------------------------
	/*!
	operator <, return lesser components from both operands
	\return vector 
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2 operator<(const Vector2& in)
	{
		return Vector2(x < in.x ? x : in.x, y < in.y ? y : in.y);
	}


	//--------------------------------------------------------------
	/*!
	operator >, return bigger components from both operands
	\return vector 
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2 operator>(const Vector2& in)
	{
		return Vector2(x > in.x ? x : in.x, y > in.y ? y : in.y);
	}


	//--------------------------------------------------------------
	/*!
	operator =, copy from reference
	\return reference
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2& operator=(const Vector2& in)
	{
		x = in.x; y = in.y;
		return *this;
	}

	Vector2& operator=(const Vector3& vec);

	//--------------------------------------------------------------
	/*!
	operator =, copy from pointer to Vector2
	\return reference
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2& operator=(const Vector2* in)
	{
		x = in->x; y = in->y;
		return *this;
	}

	//--------------------------------------------------------------
	/*!
	operator =, copy from pointer to floats
	\return reference
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2& operator=(const float *ptr)
	{
		x = ptr[0]; y = ptr[1];
		return *this;
	}

	//--------------------------------------------------------------
	/*!
	Linear interpolation between two vectors, lerp == 0 -> return this
	\param vec		second vector3 for interpolation as reference
	\param lerp		parameter
	\return interpolated vector
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2 Lerp(const Vector2& vec, float lerp) const
	{
		return Vector2((vec.x - x) * lerp + x, (vec.y - y) * lerp + y);
	}


	//--------------------------------------------------------------
	/*!
	operator *, scale vector
	\param fac		scale factor
	\return			Vector2(x * fac, y * fac, z * fac)
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2 operator*(float fac) const
	{
		return Vector2(x * fac, y * fac);
	}


	//--------------------------------------------------------------
	/*!
	operator *=, scale vector and set
	\param fac		scale factor
	\return			reference
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2& operator*=(float fac)
	{
		x *= fac; y *= fac;
		return *this;
	}

	//--------------------------------------------------------------
	/*!
	operator +, add two vectors
	\param in		vector to add to this one
	\return			new vector
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2 operator+(const Vector2& in) const
	{
		return Vector2(x + in.x, y + in.y);
	}

	//--------------------------------------------------------------
	/*!
	Multiply and add
	\param add		vector to add
	\param mult		vector to multiply
	\return			new vector, add*mult + this
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2 MA(const Vector2& add, float mult) const
	{
		if(mult == 0)
			return *this;

		if(mult == 1.0f)
			return *this + add;

		return add * mult + *this;
	}

	//--------------------------------------------------------------
	/*!
	operator +=, add vector and set
	\param in		vector to add to this one, pointer to float
	\return			reference
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2& operator+=(const Vector2& in)
	{
		x += in.x; y += in.y;
		return *this;
	}

	//--------------------------------------------------------------
	/*!
	operator -, subtract two vectors
	\param in		vector to subtract from this one
	\return			new vector
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2 operator-(const Vector2 in) const
	{
		return Vector2(x - in.x, y - in.y);
	}

	//--------------------------------------------------------------
	/*!
	operator -, subtract two vectors and set
	\param in		vector to subtract from this one
	\return			reference
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2& operator-=(const Vector2& in)
	{
		x -= in.x; y -= in.y;
		return *this;
	}

	//--------------------------------------------------------------
	/*!
	Dot product
	\param in	vector for dot product
	\return		dot product
	*/
	//--------------------------------------------------------------
	ENF_INLINE float Dot(const Vector2& in) const
	{
		return x * in.x + y * in.y;
	}

	//--------------------------------------------------------------
	/*!
	Dot product in double
	\param in	vector for dot product
	\return		dot product
	*/
	//--------------------------------------------------------------
	ENF_INLINE double Dotd(const Vector2& in) const
	{
		return x * in.x + y * in.y;
	}

	//--------------------------------------------------------------
	/*!
	operator -, make inverse of vector
	\return		new inverted vector
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2 operator-() const
	{
		return Vector2(-x, -y);
	}

	//--------------------------------------------------------------
	/*!
	Euclidean length of vector
	\return		euclidean length
	*/
	//--------------------------------------------------------------
	ENF_INLINE float Length() const
	{
		double d = x * x + y * y;

		if(d == 0.0)
			return 0.0f;

		if(d == 1.0)
			return 1.0f;

		return (float)sqrt(d);
	}

	float SquaredLength() const
	{
		return x * x + y * y;
	}

	//--------------------------------------------------------------
	/*!
	L1(Manhattan) length of vector (sum)
	\return		euclidean length
	*/
	//--------------------------------------------------------------
	ENF_INLINE float LengthL1() const
	{
		return fabs(x) + fabs(y);
	}

	//--------------------------------------------------------------
	/*!
	Fast euclidean length of vector (fastsqrt lookup function used)
	\return		fast euclidean length
	*/
	//--------------------------------------------------------------
	ENF_INLINE float QLength() const
	{
		return enf_sqrte(x * x + y * y);
	}

	//--------------------------------------------------------------
	/*!
	Euclidean square length of vector
	\return		square euclidean length
	*/
	//--------------------------------------------------------------
	ENF_INLINE float LengthSq() const
	{
		return (x * x + y * y);
	}

	//--------------------------------------------------------------
	/*!
	Distance from another point
	\param dest second point
	\return		euclidean distance between points
	*/
	//--------------------------------------------------------------
	ENF_INLINE float Distance(const Vector2& dest) const
	{
		return sqrtf(sqr(x - dest.x) + sqr(y - dest.y));
	}

	//--------------------------------------------------------------
	/*!
	L1(Manhattan) distance from another point
	\param dest second point
	\return		L1 distance between points
	*/
	//--------------------------------------------------------------
	ENF_INLINE float DistanceL1(const Vector2& dest) const
	{
		return fabs(x - dest.x) + fabs(y - dest.y);
	}

	//--------------------------------------------------------------
	/*!
	Euclidean square distance between two points
	\param dest second point
	\return		euclidean distance between points
	*/
	//--------------------------------------------------------------
	ENF_INLINE float SquaredDistance(const Vector2& dest) const
	{
		return sqr(x - dest.x) + sqr(y - dest.y);
	}

	//--------------------------------------------------------------
	/*!
	Normalize vector
	\return		length of vector
	*/
	//--------------------------------------------------------------
	ENF_INLINE float Normalize()
	{
		float length = x * x + y * y;

		if(length != 1 && length > 0)
		{
			length = sqrtf(length);

			float ilength = 1.0f / length;
			x *= ilength;
			y *= ilength;
		}

		return length;
	}

	//!Returns normalized vector
	ENF_INLINE Vector2 Normalized() const
	{
		float lx = x, ly = y;
		double length = sqr(lx) + sqr(ly);

		if(length > 0)
		{
			double ilength = 1.0 / sqrt(length);
			return Vector2((float)(lx * ilength), (float)(ly * ilength));
		}

		return *this;
	}

	//--------------------------------------------------------------
	/*!
	Normalize vector using fastsqrt
	\return		length of vector
	*/
	//--------------------------------------------------------------
	ENF_INLINE float QNormalize()
	{
		float length = x * x + y * y;

		if(length != 1 && length > 0)
		{
			length = enf_sqrte(length);

			float ilength = 1.0f / length;
			x *= ilength;
			y *= ilength;
		}

		return length;
	}


	//--------------------------------------------------------------
	/*!
	Clamp vector to specified interval
	\param minv	 minimum
	\param maxv	 maximum
	\return self clamped vector
	*/
	//--------------------------------------------------------------
	ENF_INLINE Vector2 Clamp(float minv, float maxv)
	{
		x = enf_limit(x, minv, maxv);
		y = enf_limit(y, minv, maxv);
		return *this;
	}


	/*!
	Make one perpendicular vector to this one
	\return	perpendicular vector
	*/
	Vector2 Perpendicular() const;
};
TypeIsSimple(Vector2);

/*!
Axis aligned bounding volume. Bounding box
is defined by minimal and maximal bounds.
*/
class AAB2
{
protected:
	Vector2	Mins; ///<Minimal bounds
	Vector2	Maxs; ///<Maximal bounds

public:
/*!
Tells whether is bounding volume valid. Valid AAB2 must
not be negative sized (minimums greater than maximums)
\return
True when AAB2 is valid, false otherwise
*/
	ENF_INLINE bool IsValid() const
	{
		return (Mins.x <= Maxs.x &&
				Mins.y <= Maxs.y);
	}

/*!
Tells whether is bounding volume zero sized.
(Minimums are equal to maximums)
\return
True when AAB2 is zero sized, false otherwise
*/
	ENF_INLINE bool IsZero() const
	{
		return (Mins.IsZero() && Maxs.IsZero());
	}

/*!
Copy operator.
*/
	ENF_INLINE const AAB2& operator=(const AAB2& aabb)
	{
		Mins = aabb.Mins;
		Maxs = aabb.Maxs;
		return *this;
	}

	/*!
	Equality test.
	*/
	ENF_INLINE bool operator==( const AAB2& aabb )
	{
		return (Mins == aabb.Mins && Maxs == aabb.Maxs);
	}

/*!
Gets center of AAB2. It's the middle between
maximums and minimums of volume.
\return
Point in the center of AAB2
*/
	ENF_INLINE Vector2 GetCenter() const
	{
		return (Mins + Maxs) * 0.5f;
	}

/*!
Gets minimum of AAB2
\return
Minimums.
*/
	ENF_INLINE const Vector2& GetMins() const
	{
		return Mins;
	}

/*!
Gets maximum of AAB2
\return
Maximums.
*/
	ENF_INLINE const Vector2& GetMaxs() const
	{
		return Maxs;
	}

/*!
Gets maximum absolute extent in each axis
\return
Maximum extents
*/
	Vector2 GetExtents()
	{
		return Vector2((-Mins.x > Maxs.x) ? -Mins.x : Maxs.x,
							(-Mins.y > Maxs.y) ? -Mins.y : Maxs.y);		
	}

/*!
Gets size of AAB2. It's distance from
minimum to maximum.
\return
Size of AAB2
*/
	ENF_INLINE Vector2 GetSize() const
	{
		return (Maxs - Mins);
	}

	ENF_INLINE AAB2 GetEnlargeQuantized(float xsize, float ysize) const
	{
		return AAB2(
			floor(Mins.x / xsize) * xsize - (xsize * 0.5f),
			floor(Mins.y / ysize) * ysize - (ysize * 0.5f),
			ceil(Maxs.x / xsize) * xsize + (xsize * 0.5f),
			ceil(Maxs.y / ysize) * ysize + (ysize * 0.5f));
	}

/*!
Tells whether is AAB2 completelly inside
of this AAB2.
\param bbox
AAB2 which we are testing whether is inside of this AAB2
\return
True when is inside, false otherwise
*/
	bool Inside(const AAB2& bbox)
	{
		return (Mins.x <= bbox.Mins.x &&
			Maxs.x >= bbox.Maxs.x &&
			Mins.y <= bbox.Mins.y &&
			Maxs.y >= bbox.Maxs.y);
	}

/*!
Tells whether is point inside of this AAB2.
\param point Point we are testing.
\return
True when is inside, false otherwise.
*/
	bool Intersects(const Vector2& point) const
	{
		return !(Mins.x > point.x ||
			Mins.y > point.y ||
			Maxs.x < point.x ||
			Maxs.y < point.y);
	}

/*!
Tells whether is AAB2 intersecting this AAB2.
\param aabb AAB2 we are testing.
\return
True when is intersecting, false otherwise.
*/
	bool Intersects(const AAB2& aabb) const
	{
		return !((Mins.x > aabb.Maxs.x) ||
			(Mins.y > aabb.Maxs.y) ||
			(Maxs.x < aabb.Mins.x) ||
			(Maxs.y < aabb.Mins.y));
	}

/*!
Tells whether is bounding volume intersecting this AAB2.
\param mins
Minimums of volume we are testing.
\param maxs
Maximums of volume we are testing.
\return
True when is intersecting, false otherwise.
*/
	bool Intersects(const Vector2& mins, const Vector2& maxs) const
	{
		return !((Mins.x > maxs.x) ||
			(Mins.y > maxs.y) ||
			(Maxs.x < mins.x) ||
			(Maxs.y < mins.y));
	}

/*!
Updates minimums of this AAB2 in each axis
where point is lesser than minimum bounds
\param point
the point we are extending volume by
\return
Updated AAB2
*/
	ENF_INLINE AAB2& AddMin(const Vector2& point)
	{
		Mins.x = enf_min(Mins.x, point.x);
		Mins.y = enf_min(Mins.y, point.y);
		return *this;
	}

/*!
Updates maximums of this AAB2 in each axis
where point is greater than maximum bounds
\param point
the point we are extending volume by
\return
Updated AAB2
*/
	ENF_INLINE AAB2& AddMax(const Vector2& point)
	{
		Maxs.x = enf_max(Maxs.x, point.x);
		Maxs.y = enf_max(Maxs.y, point.y);
		return *this;
	}


/*!
Updates volume of this AAB2 by extents of
other AAB2 where maximums are greater than
maximums of this AAB2 and minimums are
lesser than minimums of this AAB2.
\param bbox
The AAB2 we are extending volume by
\return
Updated AAB2
*/
	ENF_INLINE AAB2& Add(const AAB2& bbox)
	{
		AddMin(bbox.Mins);
		AddMax(bbox.Maxs);
		return *this;
	}

/*!
Updates volume of this AAB2 where point axes
are greater than maximums of this AAB2
or lesser than minimums of this AAB2.
\param point
the point we are extending volume by
\return
Updated AAB2
*/
	ENF_INLINE AAB2& Add(const Vector2& point)
	{
		Mins.x = enf_min(Mins.x, point.x);
		Mins.y = enf_min(Mins.y, point.y);
		Maxs.x = enf_max(Maxs.x, point.x);
		Maxs.y = enf_max(Maxs.y, point.y);
		return *this;
	}

/*!
Updates volume of this AAB2 by point array,
where point axes are greater than maximums
of this AAB2 or lesser than minimums of this
AAB2.
\param points
Array of points we are extending volume by
\param numpoints
Number of points in the array
\param stride
optional parameter, when stride of data in array
is different from sizeof(Vector2)
\return
Updated AAB2
*/
	AAB2& AddPoints(const Vector2* points, uint numpoints, uint stride = sizeof(Vector2));

/*!
Updates volume of this AAB2 where axes
are greater than maximums of this AAB2
or lesser than minimums of this AAB2.
\param x
Position on X axis
\param y
Position on Y axis
\return
Updated AAB2
*/
	ENF_INLINE AAB2& Add(float x, float y)
	{
		Add(Vector2(x, y));
		return *this;
	}

/*!
Updates volume of this AAB2 where circle
is exceeding bounds.
\param point
The origin of circle
\param radius
The radius of circle
\return
Updated AAB2
*/
	ENF_INLINE AAB2& AddSphere(const Vector2& point, float radius)
	{
		AddMin(Vector2(point.x - radius, point.y - radius));
		AddMax(Vector2(point.x + radius, point.y + radius));
		return *this;
	}

/*!
Translates whole AAB2 by offset
\param offset
The offset we are translating AAB2 by
\return
Updated AAB2
*/
	ENF_INLINE AAB2& Translate(const Vector2& offset)
	{
		Mins += offset;
		Maxs += offset;
		return *this;
	}

/*!
Zeroes the AAB2. Minimums and maximums
are zeroed.
\return
Updated AAB2
*/
	ENF_INLINE AAB2& Zero()
	{
		Mins.Zero();
		Maxs.Zero();
		return *this;
	}

/*!
Initializes the AAB2. Minimums are set to numeric maximum
and vice versa. The AAB2 is invalid until some point or volume
is added in.
\return
Updated AAB2
*/
	ENF_INLINE AAB2& Init()
	{
		Mins.x = Mins.y =  9999999.0f;
		Maxs.x = Maxs.y = -9999999.0f;
		return *this;
	}

/*!
Initializes the AAB2. Minimums and maximums are
set to defined extents.
\param mins
New minimums
\param maxs
New maximums
\return
Updated AAB2
*/
	ENF_INLINE AAB2& Init(const Vector2& mins, const Vector2& maxs)
	{
		Mins = mins;
		Maxs = maxs;
		return *this;
	}

/*!
Initializes the AAB2. Minimums and maximums are
set to defined extents.
\param minx
New minimums in X axis
\param miny
New minimums in Y axis
\param maxx
New maximums in X axis
\param maxy
New maximums in Y axis
\return
Updated AAB2
*/
	ENF_INLINE AAB2& Init(float minx, float miny, float maxx, float maxy)
	{
		Mins = Vector2(minx, miny);
		Maxs = Vector2(maxx, maxy);
		return *this;
	}

/*!
Returns corner based on signs in each axis. Returns maximum
for negative sign and minimum for positive sign
\param x
The sign on which is X axis of corner selected
\param y
The sign on which is Y axis of corner selected
\return
Selected corner of this AAB2
*/
	ENF_INLINE Vector2 GetCornerBySigns(float x, float y) const
	{
		return Vector2(x < 0 ? Maxs.x : Mins.x,
							y < 0 ? Maxs.y : Mins.y);
	}

/*!
Initializes the AAB2. Minimums and maximums are
both set to defined point.
\param point
New minimums and maximums
\return
Updated AAB2
*/
	ENF_INLINE AAB2& Init(const Vector2& point)
	{
		Mins = Maxs = point;
		return *this;
	}

/*!
Initializes the AAB2. Minimums and maximums are
defined from trajectory of moving circle. Could
be used also as bounding volume of line, when
radius is set to zero.
\param from
Position where trajectory begins
\param to
Position where trajectory ends
\param radius
Radius of circle
\return
Updated AAB2
*/
	AAB2& InitFromMovingSphere(const Vector2& from, const Vector2& to, float radius = 0.0f)
	{
		for(int i = 0; i < 2; i++)
		{
			if(to[i] > from[i])
			{
				Mins.v[i] = from[i] - radius;
				Maxs.v[i] = to[i] + radius;
			}
			else
			{
				Mins.v[i] = to[i] - radius;
				Maxs.v[i] = from[i] + radius;
			}
		}
		return *this;
	}

/*!
Initializes the AAB2. Minimums and maximums are
defined from trajectory of moving AAB2. 
\param from
Position where trajectory begins
\param to
Position where trajectory ends
\param box
AAB2 of moving box
\param epsilon
Optional epsilon by which is resulting bounding box enlarged
\return
Updated AAB2
*/
	AAB2& InitFromMovingBox(const Vector2& from, const Vector2& to, const AAB2& box, float epsilon = 0.0f)
	{
		for(int i = 0; i < 2; i++)
		{
			if(to[i] > from[i])
			{
				Mins.v[i] = from.v[i] + box.Mins.v[i] - epsilon;
				Maxs.v[i] = to.v[i] + box.Maxs.v[i] + epsilon;
			}
			else
			{
				Mins.v[i] = to.v[i] + box.Mins.v[i] - epsilon;
				Maxs.v[i] = from.v[i] + box.Maxs.v[i] + epsilon;
			}
		}

		return *this;
	}

/*!
Initializes the AAB2. Minimums and maximums are
defined by radius of circle.
\param radius
Radius of circle
\return
Updated AAB2
*/
	AAB2& InitFromSphere(float radius)
	{
		Mins.x = Mins.y = -radius;
		Maxs.x = Maxs.y = radius;
		return *this;
	}

/*!
Initializes the AAB2. Minimums and maximums are
defined by AAB2 translated by offset.
\param pos
Position where AAB2 is translated
\param box
AAB2 of box
\param epsilon
Optional epsilon by which is resulting bounding box enlarged
\return
Updated AAB2
*/
	AAB2& InitFromBox(const Vector2& pos, const AAB2& box, float epsilon = 0.0f)
	{
		Mins = pos + box.Mins;
		Maxs = pos + box.Maxs;

		if(epsilon != 0.0f)
		{
			Mins.x -= epsilon;
			Mins.y -= epsilon;
			Maxs.x += epsilon;
			Maxs.y += epsilon;
		}
		return *this;
	}

	float	PlaneDist(const Plane2 *pl) const
	{
		Vector2 p;
		float d;

		p.x = (pl->x < 0) ? Mins.x : Maxs.x;
		p.y = (pl->y < 0) ? Mins.y : Maxs.y;

		d = p.x * pl->x + p.y * pl->y;

		return (d - pl->d);
	}
	
	ENF_INLINE int OnPlane(const Plane2 *pl) const
	{
		float d1, d2;

		float x1, y1, x2, y2;

		if(pl->x < 0)
		{
			x1 = Mins.x;
			x2 = Maxs.x;
		}
		else
		{
			x1 = Maxs.x;
			x2 = Mins.x;
		}

		if(pl->y < 0)
		{
			y1 = Mins.y;
			y2 = Maxs.y;
		}
		else
		{
			y1 = Maxs.y;
			y2 = Mins.y;
		}

		d1 = pl->x * x1 + pl->y * y1;
		d2 = pl->x * x2 + pl->y * y2;

		int t = 0;
		if(d1 >= pl->d)
			t = 1;

		if(d2 < pl->d)
			t |= 2;

		return t;
	}

/*!
Tells whether is AAB2 crossed by plane.
\param plane
Normal vector of plane.
\param pdist
Distance of plane to zero
\return
True when is intersected, false otherwise.
*/
	bool CrossingPlane(const Vector2& plane, float pdist) const
	{
		float x1, y1, x2, y2;

		if(plane.x < 0)
		{
			x1 = Mins.x;
			x2 = Maxs.x;
		}
		else
		{
			x1 = Maxs.x;
			x2 = Mins.x;
		}

		if(plane.y < 0)
		{
			y1 = Mins.y;
			y2 = Maxs.y;
		}
		else
		{
			y1 = Maxs.y;
			y2 = Mins.y;
		}

		if((plane.x * x2 + plane.y * y2) >= pdist)
			return false;

		return ((plane.x * x1 + plane.y * y1) >= pdist);
	}

/*!
Gets position on, or inside plane which is nearest in each axis to
the specified point.
\param pos
Position we are finding nearest position to
\return
the nearest position on box (or inside box, when source point
is inside too).
*/
	Vector2 GetNearestPosition(const Vector2& pos) const
	{
		//get nearest corner of AABB
		return Vector2((pos.x <= Mins.x) ? Mins.x : ((pos.x >= Maxs.x) ? Maxs.x : pos.x),
							(pos.y <= Mins.y) ? Mins.y : ((pos.y >= Maxs.y) ? Maxs.y : pos.y));
	}

/*!
Constructor creating AAB2 set to defined point
*/
	AAB2(const Vector2& point)
	{
		Mins = point;
		Maxs = point;
	}

/*!
Constructor creating AAB2 with defined extents
*/
	AAB2(const Vector2& mins, const Vector2& maxs)
	{
		Mins = mins;
		Maxs = maxs;
	}

/*!
Constructor creating AAB2 with defined extents
*/
	AAB2(float minx, float miny, float maxx, float maxy)
	{
		Mins.x = minx; Mins.y = miny;
		Maxs.x = maxx; Maxs.y = maxy;
	}

/*!
Basic constructor.
\param init
When set to true, AAB2 is initialized with maximums
on numeric minimum and vice versa.
*/
	AAB2(bool init = false)
	{
		if(init)
			Init();
	}
};
//@}

/*!
Intersect line pt1-pt2 with line pt3-pt4.
Intersection point is returned in intersection param.
\return -1: no intersection; 0: parallel; 1: intersecting
*/
struct MathUtils2D
{
	static int IntersectLines( const enf::Vector2& pt1, const enf::Vector2& pt2, const enf::Vector2& pt3, const enf::Vector2& pt4, ebool bSegments, Vector2& intersection )
	{
		float a1 = pt2.y - pt1.y;
		float a2 = pt4.y - pt3.y;
		float b1 = pt1.x - pt2.x;
		float b2 = pt3.x - pt4.x;
		float c1 = a1 * pt1.x + b1 * pt1.y;
		float c2 = a2 * pt3.x + b2 * pt3.y;

		float det = (a1 * b2) - (a2 * b1);
		if( fabsf( det ) < 0.001f )
		{
			// parallel
			return 0;
		}
		else
		{
			// possible intersection
			float x = (b2 * c1 - b1 * c2) / det;
			float y = (a1 * c2 - a2 * c1) / det;
			intersection.x = x;
			intersection.y = y;

			if( bSegments )
			{
				static const float TRESHOLD = .001f;
				if( x < enf::enf_min( pt1.x, pt2.x ) - TRESHOLD || x > enf::enf_max( pt1.x, pt2.x ) + TRESHOLD ) return -1;
				if( x < enf::enf_min( pt3.x, pt4.x ) - TRESHOLD || x > enf::enf_max( pt3.x, pt4.x ) + TRESHOLD ) return -1;
				if( y < enf::enf_min( pt1.y, pt2.y ) - TRESHOLD || y > enf::enf_max( pt1.y, pt2.y ) + TRESHOLD ) return -1;
				if( y < enf::enf_min( pt3.y, pt4.y ) - TRESHOLD || y > enf::enf_max( pt3.y, pt4.y ) + TRESHOLD ) return -1;
			}

			return 1;
		}
	}


	//-----------------------------------------------------------------------------
	/*!
	returns etrue	if edges intersects each other

	\param P1		first point of first edge
	\param P2		second point of first edge
	\param P3		first point of second edge
	\param P4		second point of second edge
	\param inters	OUT, intersection

	TODO: looks like previous function, but simpler, update this one for common line intersection
	*/
	//-----------------------------------------------------------------------------
	static ebool IntersectEdges(const Vector2& P1, const Vector2& P2, const Vector2& P3, const Vector2& P4, Vector2 &inters)
	{
		Vector2 dir1 (P2 - P1);
		Vector2 dir2 (P4 - P3);

		//(P4.y - P3.y)*(P2.x - P1.x) - (P4.x - P3.x)*(P2.y - P1.y)
		float den = dir2.y*dir1.x - dir2.x*dir1.y;  

		if (fabs(den) < 1e-5)
		{
			return efalse;
		}

		//(P4.x - P3.x)*(P1.y - P3.y) - (P4.y - P3.y)*(P1.x - P3.x)
		float nom1	= dir2.x*(P1.y - P3.y) - dir2.y*(P1.x - P3.x);
		float t		= nom1/den;

		if (enf_in(t, 0.0f, 1.0f))
		{
			float nom2 = dir1.x*(P1.y - P3.y) - dir1.y*(P1.x - P3.x);
			float t2   = nom2/den;

			if (enf_in(t2, 0.0f, 1.0f))
			{
				inters = P1 + dir1*t;
				return etrue;
			}
		}

		return efalse;
	}


	//-----------------------------------------------------------------------------
	/*!
	compute intersection of line and circle, classic way using quadratic equation

	\param P			line start point
	\param dir		direction of line
	\param S			circle center
	\param radius	radius of circle
	\param I1		first output of intersection (positive direction of dir when P is inside circle)
	\param I2		second output of intersection
	\return			number of intersections (0, 1, 2)
	*/
	//-----------------------------------------------------------------------------
	static uint IntersectLineCircle(const Vector2 &P, const Vector2 &dir, const Vector2& S, float radius, Vector2 &I1, Vector2 &I2)
	{
		//center to zero
		Vector2 PP1 (P - S);

		//coefficients of quadratic equation
		float a = dir.Dot(dir);
		float b = 2.0f*dir.Dot(PP1);
		float c = PP1.Dot(PP1) - sqr(radius);

		float D = sqr(b) - 4.0f*a*c;

		//no intersection
		if (D < 0.0f)
		{
			return 0;
		}

		//on intersection
		float inv_2a = 1.0f/(2.0f*a);
		if (D < 1e-5)
		{
			float t = -b*inv_2a;
			I1 = P + dir*t;
			return 1;
		}

		//two intersections
		float sqrtD = sqrt(D);
		float t1 = (-b + sqrtD)*inv_2a;
		float t2 = (-b - sqrtD)*inv_2a;

		I1 = P + dir*t1;
		I2 = P + dir*t2;

		return 2;
	}


	//-----------------------------------------------------------------------------
	/*!
	get barycentric coords

	\param p1				first point of triangle
	\param p2				second point of triangle
	\param p3				third point of triangle
	\param point			tested point
	\param barycentric	OUT, 3 barycentric coords
	*/
	//-----------------------------------------------------------------------------
	static void GetBarycentricCoords(const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& point, float *barycentric)
	{
		//area of large triangle
		Vector2 dir[3];
		dir[0]  = p1 - p2; 
		dir[1]  = p3 - p2;
		float sum = 0.5f*fabs(dir[0].x*dir[1].y - dir[1].x*dir[0].y);

		//points are in the same position
		if (sum == 0.0f)
		{
			barycentric[0] = 1.0f; barycentric[1] = barycentric[2] = 0.0f;
			return;
		}

		//vectors to point
		dir[0] = p1 - point;
		dir[1] = p2 - point;
		dir[2] = p3 - point;

		//and the area of inner triangles
		float area[3];
		area[0] = 0.5f*fabs(dir[0].x*dir[1].y - dir[1].x*dir[0].y);
		area[1] = 0.5f*fabs(dir[1].x*dir[2].y - dir[2].x*dir[1].y);
		area[2] = 0.5f*fabs(dir[2].x*dir[0].y - dir[0].x*dir[2].y);

		float invsum = 1.0f/sum;

		barycentric[0] = area[1]*invsum;
		barycentric[1] = area[2]*invsum;
		barycentric[2] = area[0]*invsum;
	}


	//-----------------------------------------------------------------------------
	/*!
	computation if point is inside triangle, the result is true/false and possible
	barycentric coordinates

	\param p1				first point of triangle
	\param p2				second point of triangle
	\param p3				third point of triangle
	\param point			tested point
	\param barycentric	OUT, 3 barycentric coords
	*/
	//-----------------------------------------------------------------------------
	static ebool IsPointInTriangle(const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& point, float *barycentric)
	{
		float innerbar[3];

		if (!barycentric)
		{
			barycentric = innerbar;
		}

		GetBarycentricCoords(p1, p2,p3, point, barycentric);

		const float EPS = 1.0f + 1e-5f;
		if (barycentric[0] + barycentric[1] + barycentric[2] < EPS)
		{
			return etrue;
		}

		return efalse;
	}


	//-----------------------------------------------------------------------------
	/*!
	find two circles intersection using triangles (simpler then by computing two circles
	equations)

	\param P1			center of first circle
	\param r1			radius of first circle
	\param P2			center of second circle
	\param r2			radius of second circle
	\param I1			OUT, first possible intersection
	\param I2			OUT, second possible intersection
	\param outcenter	OUT, center of intersection, not needed, if null is not filled

	\return 
	 -3 = no intersection, one inside the other
	 -2 = no intersection, are far away
	 -1 = infinity intersections = circles are equal 
	  0 = no intersection, points are equal, but radius different
	  1 = 1 intersection at the point of touch
	  2 = two intersections
	  in I1 and I2 are possible intersections
	*/
	//-----------------------------------------------------------------------------
	enum CircleIntersection {
		CI_INSIDE				= -3,
		CI_AWAY					= -2,
		CI_INFINITY				= -1,
		CI_EQUAL					=  0,
		CI_ONE_INTERSECTION	=  1,
		CI_TWO_INTERSECTION	=  2
	};

	static CircleIntersection IntersectCircles (const Vector2& P1, float r1, const Vector2& P2, float r2, Vector2& I1, Vector2& I2, Vector2* outcenter = NULL) 
	{
		const float EPSILON = 1e-8f;

		Vector2 delta  = P2 - P1;
		float distance = delta.Length();

		//points are equal, thus no intersection or infinity
		if (distance < EPSILON)
		{
			//no intersection
			if (r1 == r2)
				return CI_INFINITY;

			return CI_EQUAL;
		}

		//no intersection, when are far away
		if (distance > (r1 + r2))
		{
			return CI_AWAY;
		}

		//no intersection, one is inside the other
		if (distance < fabs(r1 - r2))
		{
			return CI_INSIDE;
		}

		//one intersection
		if (fabs(distance - (r1 + r2)) < EPSILON)
		{
			I1 = P1 + delta*(r1/distance);
			I2 = I1;

			return CI_ONE_INTERSECTION;
		}


		//the distance from point P1 to point in the center of intersection
		float r1sqr = r1*r1;
		float a = (r1sqr - r2*r2 + distance*distance) / (2.0f*distance) ;

		//center of intersection
		Vector2 dir    = delta*(1.0f/distance);
		Vector2 center = P1 + dir*a; 

		if (outcenter)
		{
			*outcenter = center;
		}

		//distance from center to one intersection point
		float sqrh = r1sqr - a*a;

		//safety test
		if (sqrh < EPSILON)
		{
			I1 = I2 = center;
			return CI_ONE_INTERSECTION;
		}

		float h = sqrt(sqrh);

		//create normal and normalize it
		Vector2 normal (-dir.y, dir.x);

		Vector2 vec = normal*h;
		I1 = center + vec;
		I2 = center - vec;

		return CI_TWO_INTERSECTION;	
	}




};

class Rect2D
{
public:
	float		Left;
   float		Top;
   float		Width;
   float		Height;

	Rect2D(): Left(0), Top(0), Width(0), Height(0) {}
	Rect2D(float left, float top, float width, float height): Left(left), Top(top), Width(width), Height(height) {}
	Rect2D(const Vector2& pos, const Vector2& size): Left(pos.x), Top(pos.y), Width(size.x), Height(size.y) {}

	bool operator==(const Rect2D& rect)
	{
		return Left == rect.Left && Top == rect.Top && Width == rect.Width && Height == rect.Height;
	}

	bool operator!=(const Rect2D& rect)
	{
		return Left != rect.Left || Top != rect.Top || Width != rect.Width || Height != rect.Height;
	}

	Rect2D operator=(const Rect2D& rect)
	{
		Left = rect.Left;
		Top = rect.Top;
		Width = rect.Width;
		Height = rect.Height;
		return *this;
	}
	//!returns position
	Vector2 GetPosition() const
	{
		return Vector2(Left, Top);
	}
	//!returns size
	Vector2 GetSize() const
	{
		return Vector2(Width, Height);
	}

	//!returns position
	Rect2D& SetPosition(const Vector2& pos)
	{
		Left = pos.x; Top = pos.y;
		return *this;
	}
	//!returns size
	Rect2D& SetSize(const Vector2& size)
	{
		Width = size.x; Height = size.y;
		return *this;
	}

	//!Translates current position
	Rect2D& Translate(const Vector2& offset)
	{
		Left += offset.x; Top += offset.y;
		return *this;
	}

	//!Interpolates between this and the other rect
	Rect2D Lerp(const Rect2D& other, float lerp) const
	{
		return Rect2D(enf_lerp(Left, other.Left, lerp), enf_lerp(Top, other.Top, lerp),
				enf_lerp(Width, other.Width, lerp), enf_lerp(Height, other.Height, lerp));
	}

	ENF_INLINE bool IsInside(float x, float y) const
	{
		return (x >= Left && x <= Left + Width && y >= Top && y <= Top + Height);
	}
};


}
#endif //ENF_MATHLIB2D_H

