//-----------------------------------------------------------------------------
// File: enf_mathrect.h
//
// Desc: rectangles
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------


#ifndef _ENF_MATHRECT_H
#define _ENF_MATHRECT_H

#include <math.h>
#include "enf_config.h"
#include "common/enf_inttypes.h"
#include "common/enf_intersection.h"

namespace enf
{
/** @addtogroup Common
@{*/

const uint CLIP_INSIDE		= 0;

/*!
TestRect return values
 */
enum TestRectResult
{
	RECT_OUTSIDE		= 0,
	RECT_LEFT_BOTTOM	= BITF(0),
	RECT_LEFT_TOP		= BITF(1),
	RECT_RIGHT_BOTTOM	= BITF(2),
	RECT_RIGHT_TOP		= BITF(3),
	RECT_INSIDE			= RECT_LEFT_BOTTOM | RECT_LEFT_TOP | RECT_RIGHT_BOTTOM | RECT_LEFT_TOP
};


//-----------------------------------------------------------------------------
/*!
rectangle for clipping, the borders are inside clipping region
*/
//-----------------------------------------------------------------------------
template<class T> class RectangleTemplate
{
private: 
	T m_MinX, m_MaxX;				///< minimum and maximum of x
	T m_MinY, m_MaxY;				///< minimum and maximum of y

public:
	//-----------------------------------------------------------------------------
	/*!
	empty constructor, create invalid rect
	*/
	//-----------------------------------------------------------------------------
	RectangleTemplate() 
	{
		m_MinX = (T) 0;
		m_MinY = (T) 0;
		m_MaxX = (T) -1;
		m_MaxY = (T) -1;
	}


	//-----------------------------------------------------------------------------
	/*!
	constructor, init
	\param minx		minimum x
	\param miny		minimum y
	\param maxx		maximum x
	\param maxy		maximum y
	*/
	//-----------------------------------------------------------------------------
	RectangleTemplate(T minx, T miny, T maxx, T maxy) 
	{
		m_MinX = minx;
		m_MinY = miny;
		m_MaxX = maxx;
		m_MaxY = maxy;
	}


	//-----------------------------------------------------------------------------
	/*!
	constructor, init, minimum is set to 0
	\param maxx		maximum x
	\param maxy		maximum y
	*/
	//-----------------------------------------------------------------------------
	RectangleTemplate(T maxx, T maxy) 
	{
		m_MinX = (T) 0;
		m_MinY = (T) 0;
		m_MaxX = maxx;
		m_MaxY = maxy;
	}


	//-----------------------------------------------------------------------------
	/*!
	constructor, copy
	\param src	source
	*/
	//-----------------------------------------------------------------------------
	RectangleTemplate(const RectangleTemplate<T> &src) 
	{
		*this = src;
	}



	//-----------------------------------------------------------------------------
	/*!
	operator =
	\param src	source
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE RectangleTemplate & operator= (const RectangleTemplate<T> &src) 
	{
		m_MinX = src.m_MinX;
		m_MinY = src.m_MinY;
		m_MaxX = src.m_MaxX;
		m_MaxY = src.m_MaxY;

		return *this;
	}


	//-----------------------------------------------------------------------------
	/*!
	invalidate rect -> fill it with 0,0,-1,-1
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void Invalidate()
	{
		m_MinX = m_MinY = (T)  0;
		m_MaxX = m_MaxY = (T) -1;
	}

	//-----------------------------------------------------------------------------
	/*!
	initialize clip space	
	\param minx		minimum x
	\param miny		minimum y
	\param maxx		maximum x
	\param maxy		maximum y
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void Init(T minx, T miny, T maxx, T maxy)
	{
		m_MinX = minx;
		m_MinY = miny;
		m_MaxX = maxx;
		m_MaxY = maxy;
	}

	//-----------------------------------------------------------------------------
	/*!
	initialize clip space, to point now
	\param x		x
	\param y		y
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void Init(T x, T y)
	{
		m_MinX = m_MaxX = x;
		m_MinY = m_MaxY = y;
	}


	//-----------------------------------------------------------------------------
	/*!
	initialize minimum of clip space	
	\param minx		minimum x
	\param miny		minimum y
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void InitMin(T minx, T miny)
	{
		m_MinX = minx;
		m_MinY = miny;
	}

	//-----------------------------------------------------------------------------
	/*!
	initialize maximum of clip space	
	\param maxx		maximum x
	\param maxy		maximum y
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void InitMax(T maxx, T maxy)
	{
		m_MaxX = maxx;
		m_MaxY = maxy;
	}


	//-----------------------------------------------------------------------------
	/*!
	initialize y values of clip space	
	\param miny		minimum y
	\param maxy		maximum y
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void InitY(T miny, T maxy)
	{
		m_MinY = miny;
		m_MaxY = maxy;
	}


	//-----------------------------------------------------------------------------
	/*!
	initialize x values of clip space	
	\param minx		minimum x
	\param maxx		maximum x
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void InitX(T minx, T maxx)
	{
		m_MinX = minx;
		m_MaxX = maxx;
	}


	//-----------------------------------------------------------------------------
	/*!
	add point 
	\param x		x coord
	\param y		y coord
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void Add(T x, T y)
	{
		m_MinX = enf_min(x, m_MinX);
		m_MaxX = enf_max(x, m_MaxX);
		m_MinY = enf_min(y, m_MinY);
		m_MaxY = enf_max(y, m_MaxY);
	}

	//-----------------------------------------------------------------------------
	/*!
	get values
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE T GetMinX() const	{	return m_MinX;	}
	ENF_INLINE T GetMinY() const 	{	return m_MinY;	}
	ENF_INLINE T GetMaxX() const 	{	return m_MaxX;	}
	ENF_INLINE T GetMaxY() const	{	return m_MaxY;	}

	//-----------------------------------------------------------------------------
	/*!
	set values
	\param val
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void SetMinX(T val ) 	{	Assert(val <= m_MaxX ); m_MinX = val; }
	ENF_INLINE void SetMinY(T val ) 	{	Assert(val <= m_MinY ); m_MinY = val;	}
	ENF_INLINE void SetMaxX(T val ) 	{	Assert(val >= m_MinX ); m_MaxX = val;	}
	ENF_INLINE void SetMaxY(T val ) 	{	Assert(val >= m_MinY ); m_MaxY = val;	}


	//-----------------------------------------------------------------------------
	/*!
	get width
	\return width
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE T GetWidth() const
	{
		return m_MaxX - m_MinX;
	}


	//-----------------------------------------------------------------------------
	/*!
	get height
	\return height
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE T GetHeight() const
	{
		return m_MaxY - m_MinY;
	}

	//-----------------------------------------------------------------------------
	/*!
	get center
	\return center
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void GetCenter(T& x, T& y) const
	{
		x = (m_MinX + m_MaxX)/2;
		y = (m_MinY + m_MaxY)/2;
	}


	//-----------------------------------------------------------------------------
	/*!
	return true if is valid, the width and height are > 0
	\return 
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE bool IsValid() const
	{
		return !(GetHeight() < 0 || GetWidth() < 0);
	}


	//-----------------------------------------------------------------------------
	/*!
	clamp coordinate
	\param x		x coord
	\param y		y coord
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void Clamp(T &x, T &y) const
	{
		x = enf_limit<T> (x, m_MinX, m_MaxX);
		y = enf_limit<T> (y, m_MinY, m_MaxY);
	}

	//-----------------------------------------------------------------------------
	/*!
	clamp coordinate
	\param x		x coord
	\param y		y coord
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void ClampInt(int &x, int &y) const
	{
		x = enf_limit<int> (x, (int) m_MinX, (int) m_MaxX);
		y = enf_limit<int> (y, (int) m_MinY, (int) m_MaxY);
	}


	//-----------------------------------------------------------------------------
	/*!
	clamp coordinate
	\param x		x coord
	\param y		y coord
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void ClampFloat(float &x, float &y) const
	{
		x = enf_limit<float> (x, (float) m_MinX, (float) m_MaxX);
		y = enf_limit<float> (y, (float) m_MinY, (float) m_MaxY);
	}


	//--------------------------------------------------------------------------
	/*!
	Wrap values to value usable for indexing in array
	\param x		x index
	\param y		y index
	*/
	//--------------------------------------------------------------------------
	ENF_INLINE void Wrap(T &x, T &y) const
	{
		x = enf_wrap<T> (x, m_MinX, m_MaxX);
		y = enf_wrap<T> (y, m_MinY, m_MaxY);
	}


	//--------------------------------------------------------------------------
	/*!
	Wrap values to value usable for indexing in array
	\param x		x index
	\param y		y index
	*/
	//--------------------------------------------------------------------------
	ENF_INLINE void WrapFloat(float &x, float &y) const
	{
		x = enf_wrap<float> (x, m_MinX, m_MaxX);
		y = enf_wrap<float> (y, m_MinY, m_MaxY);
	}


	//--------------------------------------------------------------------------
	/*!
	Wrap values to value usable for indexing in array
	\param x		x index
	\param y		y index
	*/
	//--------------------------------------------------------------------------
	ENF_INLINE void WrapInt(int &x, int &y) const
	{
		x = enf_wrap<int> (x, m_MinX, m_MaxX + 1);
		y = enf_wrap<int> (y, m_MinY, m_MaxY + 1);
	}


	//--------------------------------------------------------------------------
	/*!
	Intersect with another rectangle, do not test for validity, thus faster
	\param src		rectangle to intersect
	\return intersection
	*/
	//--------------------------------------------------------------------------
	ENF_INLINE RectangleTemplate<T> IntersectNoTest(const RectangleTemplate<T> &src) const
	{
		Assert(!(src.m_MinX > m_MaxX || src.m_MaxX < m_MinX || src.m_MinY > m_MaxY || src.m_MaxY < m_MinY));
	
		T x1 = (T) enf_limit<T> (src.m_MinX, m_MinX, m_MaxX);
		T y1 = (T) enf_limit<T> (src.m_MinY, m_MinY, m_MaxY);
		T x2 = (T) enf_limit<T> (src.m_MaxX, m_MinX, m_MaxX);
		T y2 = (T) enf_limit<T> (src.m_MaxY, m_MinY, m_MaxY);

		return RectangleTemplate<T> (x1, y1, x2, y2);
	}

	//--------------------------------------------------------------------------
	/*!
	Intersect with another rectangle
	\param src		rectangle to intersect
	\return intersection
	*/
	//--------------------------------------------------------------------------
	ENF_INLINE RectangleTemplate<T> Intersect(const RectangleTemplate<T> &src) const
	{
		//create invalid rect if src is totally out
		if (src.m_MinX > m_MaxX || src.m_MaxX < m_MinX || src.m_MinY > m_MaxY || src.m_MaxY < m_MinY)
			return RectangleTemplate<T> (-1, -1);

		return IntersectNoTest(src);
	}


	//--------------------------------------------------------------------------
	/*!
	Intersect with another rectangle
	\param src		rectangle to intersect
	\return this intersection
	*/
	//--------------------------------------------------------------------------
	ENF_INLINE RectangleTemplate<T>& IntersectWith(const RectangleTemplate<T> &src)
	{
		//create invalid rect if src is totally out
		if (src.m_MinX > m_MaxX || src.m_MaxX < m_MinX || src.m_MinY > m_MaxY || src.m_MaxY < m_MinY)
		{
			m_MinX = m_MinY = 0;
			m_MaxX = m_MaxY = -1;
			return *this;
		}

		m_MinX = (T) enf_limit<T> (src.m_MinX, m_MinX, m_MaxX);
		m_MinY = (T) enf_limit<T> (src.m_MinY, m_MinY, m_MaxY);
		m_MaxX = (T) enf_limit<T> (src.m_MaxX, m_MinX, m_MaxX);
		m_MaxY = (T) enf_limit<T> (src.m_MaxY, m_MinY, m_MaxY);

		return *this;
	}


	//--------------------------------------------------------------------------
	/*!
	union with another rectangle
	\param src		rectangle to create union
	\return union
	*/
	//--------------------------------------------------------------------------
	ENF_INLINE RectangleTemplate<T> Union(const RectangleTemplate<T> &src) const
	{
		T x1 = (T) enf_min<T> (src.m_MinX, m_MinX);
		T y1 = (T) enf_min<T> (src.m_MinY, m_MinY);
		T x2 = (T) enf_max<T> (src.m_MaxX, m_MaxX);
		T y2 = (T) enf_max<T> (src.m_MaxY, m_MaxY);

		return RectangleTemplate<T> (x1, y1, x2, y2);
	}


	//--------------------------------------------------------------------------
	/*!
	union with another rectangle
	\param src		rectangle to create union
	\return this union
	*/
	//--------------------------------------------------------------------------
	ENF_INLINE RectangleTemplate<T>& UnionWith(const RectangleTemplate<T> &src)
	{
		m_MinX = (T) enf_min<T> (src.m_MinX, m_MinX);
		m_MinY = (T) enf_min<T> (src.m_MinY, m_MinY);
		m_MaxX = (T) enf_max<T> (src.m_MaxX, m_MaxX);
		m_MaxY = (T) enf_max<T> (src.m_MaxY, m_MaxY);

		return *this;
	}


	//-----------------------------------------------------------------------------
	/*!
	enlarge rect in both directions
	\param value	value 
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE RectangleTemplate<T>& Enlarge(const T &value)
	{
		m_MinX -= value;
		m_MinY -= value;
		m_MaxX += value;
		m_MaxY += value;

		return *this;
	}


	//-----------------------------------------------------------------------------
	/*!
	enlarge rect by another rect
	\param enlarge		enlarge by this
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE RectangleTemplate<T>& Enlarge(const RectangleTemplate<T>& enlarge)
	{
		m_MinX -= enlarge.GetMinX();
		m_MinY -= enlarge.GetMinY();
		m_MaxX += enlarge.GetMaxX();
		m_MaxY += enlarge.GetMaxY();

		return *this;
	}


	//-----------------------------------------------------------------------------
	/*!
	scale in both directions
	\param scalex scale param in x
	\param scaley scale param in y
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE RectangleTemplate<T>& Scale(const T &scalex, const T &scaley)
	{
		m_MinX *= scalex;
		m_MinY *= scaley;
		m_MaxX *= scalex;
		m_MaxY *= scaley;

		return *this;
	}


	//-----------------------------------------------------------------------------
	/*!
	add offset 
	\param offsx	offset param in x
	\param offsy	offset param in y
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE RectangleTemplate<T>& Offset(const T &offsx, const T &offsy)
	{
		m_MinX += offsx;
		m_MinY += offsy;
		m_MaxX += offsx;
		m_MaxY += offsy;

		return *this;
	}


	//-----------------------------------------------------------------------------
	/*!
	test point whether it has valid coordinates
	\param px	x coord
	\param py	y coord
	\return flag where is the point
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE uint TestPoint(const T &px, const T &py) const
	{
		uint flag = CLIP_INSIDE;

		if (px < m_MinX)			
			flag = CLIP_LEFT;
		else 
			if (px > m_MaxX)	
				flag = CLIP_RIGHT;

		if (py < m_MinY)			
			flag |= CLIP_BOTTOM;
		else
			if (py > m_MaxY)	
				flag |= CLIP_TOP;

		return flag;
	}


	//-----------------------------------------------------------------------------
	/*!
	test point whether it has valid coordinates with epsilon value
	\param px	x coord
	\param py	y coord
	\param eps	epsilon val
	\return flag where is the point
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE uint TestPointEps(const T &px, const T &py, float eps) const
	{
		uint flag = CLIP_INSIDE;

		if (px < m_MinX + eps)
			flag = CLIP_LEFT;
		else 
			if (px > m_MaxX - eps)	
				flag = CLIP_RIGHT;

		if (py < m_MinY + eps)			
			flag |= CLIP_BOTTOM;
		else
			if (py > m_MaxY - eps)	
				flag |= CLIP_TOP;

		return flag;
	}


	//-----------------------------------------------------------------------------
	/*!
	\param px	x coord
	\param py	y coord
	\return flag where is the point
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE bool IsInside(const T &px, const T &py) const
	{
		return TestPoint(px, py) == CLIP_INSIDE;
	}


	//-----------------------------------------------------------------------------
	/*!
	test other rectangle if is inside (RECT_INSIDE), outside (RECT_OUTSIDE) or 
	is intersected (RECT_INTERSECT)
	\param src	source rectangle
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE TestRectResult TestRect(const RectangleTemplate<T> &src) const
	{
		bool minx = m_MinX <= src.GetMinX();
		bool maxx = m_MaxX >= src.GetMaxX();
		bool miny = m_MinY <= src.GetMinY();
		bool maxy = m_MaxY >= src.GetMaxY();

		if (minx & maxx & miny & maxy)
			return RECT_INSIDE;

		uint ret = RECT_OUTSIDE;
		if (TestPoint(src.GetMinX(), src.GetMinY()) == CLIP_INSIDE) ret |= RECT_LEFT_BOTTOM;
		if (TestPoint(src.GetMinX(), src.GetMaxY()) == CLIP_INSIDE) ret |= RECT_LEFT_TOP;
		if (TestPoint(src.GetMaxX(), src.GetMinY()) == CLIP_INSIDE) ret |= RECT_RIGHT_BOTTOM;
		if (TestPoint(src.GetMaxX(), src.GetMaxY()) == CLIP_INSIDE) ret |= RECT_RIGHT_TOP;

		return (TestRectResult) ret;
	}


	//-----------------------------------------------------------------------------
	/*!
	return nearest border point for some point
	\param px	x coord
	\param py	y coord
	\param outx corner point
	\param outy corner point
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void GetNearestPoint(const T &px, const T &py, T &outx, T &outy) const
	{
		T halfx = m_MinX + (m_MaxX - m_MinX)/2;
		T halfy = m_MinY + (m_MaxY - m_MinY)/2;

		if (px > halfx) outx = m_MaxX; else outx = m_MinX;
		if (py > halfy) outy = m_MaxY; else outy = m_MinY;
	}
};


/*!
type definition for Rectangle with int 
*/
typedef RectangleTemplate<int>		RectangleInt;

/*!
type definition for Rectangle with float 
*/
typedef RectangleTemplate<float>		RectangleFloat;
//@}
}
#endif //ENF_MATHRECT
