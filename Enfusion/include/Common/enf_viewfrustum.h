//-----------------------------------------------------------------------------
// File: enf_viewfrustum.h
//
// Desc: Mathematics library
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_VIEWFRUSTUM_H
#define ENF_VIEWFRUSTUM_H

#include "enf_config.h"
#include "common/enf_mathlib.h"

namespace enf
{
/** @addtogroup Math3D
@{*/
/*!
Clipping frustum for volume culling
*/
class PUBLIC_API ViewFrustum
{
private:
	uint		m_iNumClipPlanes;
	uint		m_iMask;
	Vector4	m_ClipPlanes[8];

public:
	enum PlaneMask
	{
		PLANE_NEAR = BITF(0),
		PLANE_FAR = BITF(1),
		PLANE_LEFT = BITF(2),
		PLANE_RIGHT = BITF(3),
		PLANE_BOTTOM = BITF(4),
		PLANE_TOP = BITF(5),
		PLANE_ALL = PLANE_NEAR|PLANE_LEFT|PLANE_RIGHT|PLANE_BOTTOM|PLANE_TOP|PLANE_FAR
	};

	void operator=(ViewFrustum vf)
	{
		m_iNumClipPlanes = vf.m_iNumClipPlanes;
		m_iMask = vf.m_iMask;

		for(uint n = 0; n < vf.m_iNumClipPlanes; n++)
		{
			m_ClipPlanes[n] = vf.m_ClipPlanes[n];
		}
	}

	/*!
	Tests whether is AAB inside frustum
	\param bbox Bounding box
	\return True when it's inside
	*/
	ENF_INLINE bool TestAABB(const AAB& bbox) const
	{
		float extx = bbox.GetMaxs().x, exty = bbox.GetMaxs().y, extz = bbox.GetMaxs().z;
		float cx = (bbox.GetMins().x + extx) * 0.5f;
		float cy = (bbox.GetMins().y + exty) * 0.5f;
		float cz = (bbox.GetMins().z + extz) * 0.5f;
		extx -= cx;
		exty -= cy;
		extz -= cz;
		for(uint n = 0; n < m_iNumClipPlanes; n++)
		{
			float px = m_ClipPlanes[n].x;
			float py = m_ClipPlanes[n].y;
			float pz = m_ClipPlanes[n].z;
			float d2 = fabsf(extx * px) + fabsf(exty * py) + fabsf(extz * pz);
			float d1 = (px * cx + py * cy + pz * cz) - m_ClipPlanes[n].w;
			if ( d1 + d2 < 0 )
			{
				return false;
			}
		}
		return true;
	}

	/*!
	Tests whether is AAB inside frustum
	\param bbox Bounding box
	\param mask Mask of planes to be tested
	\return True when it's inside
	*/
	ENF_INLINE bool TestAABB(const AAB& bbox, uint mask) const
	{
		if(mask == 0) return true;

		float extx = bbox.GetMaxs().x, exty = bbox.GetMaxs().y, extz = bbox.GetMaxs().z;
		float cx = (bbox.GetMins().x + extx) * 0.5f;
		float cy = (bbox.GetMins().y + exty) * 0.5f;
		float cz = (bbox.GetMins().z + extz) * 0.5f;
		extx -= cx;
		exty -= cy;
		extz -= cz;
		for(uint n = 0, b = 1; n < m_iNumClipPlanes; n++, b <<= 1)
		{
			if((b & mask) == 0)
				continue;
			float px = m_ClipPlanes[n].x;
			float py = m_ClipPlanes[n].y;
			float pz = m_ClipPlanes[n].z;
			float d2 = fabsf(extx * px) + fabsf(exty * py) + fabsf(extz * pz);
			float d1 = (px * cx + py * cy + pz * cz) - m_ClipPlanes[n].w;

			if ( d1 + d2 < 0 )
			{
				return false;
			}
		}
		return true;
	}

	/*!
	Tests whether is AAB inside frustum. Check plane-mask
	and clear bits of planes that are completely out
	\param bbox Bounding box
	\param _mask Mask of planes to be tested and updated
	\return True when it's inside
	*/
	ENF_INLINE bool TestAABBUpdate(const AAB& bbox, uint& _mask) const
	{
		uint mask = _mask;

		float extx = bbox.GetMaxs().x, exty = bbox.GetMaxs().y, extz = bbox.GetMaxs().z;
		float cx = (bbox.GetMins().x + extx) * 0.5f;
		float cy = (bbox.GetMins().y + exty) * 0.5f;
		float cz = (bbox.GetMins().z + extz) * 0.5f;
		extx -= cx;
		exty -= cy;
		extz -= cz;
		for(uint n = 0, b = 1; n < m_iNumClipPlanes; n++, b <<= 1)
		{
			if((b & mask) == 0)
				continue;
			float px = m_ClipPlanes[n].x;
			float py = m_ClipPlanes[n].y;
			float pz = m_ClipPlanes[n].z;
			float d2 = fabsf(extx * px) + fabsf(exty * py) + fabsf(extz * pz);
			float d1 = (px * cx + py * cy + pz * cz) - m_ClipPlanes[n].w;

			if ( d1 + d2 < 0 )
			{
				_mask = mask;
				return false;
			}
			if ( d1 - d2 > 0 )
				mask ^= b;
		}
		_mask = mask;
		return true;
	}

	ENF_INLINE float Distance(const Vector4& plane, const Vector3& point) const
	{
		return plane.Dot(point) - plane.w;
	}
	/*!
	Tests whether is point inside frustum. 
	\param point	Point
	\param maxDist	maximum distance from plane where point is selected as to be inside, default is 1 unit
	\return True when it's inside
	*/
	ENF_INLINE bool TestPoint(const Vector3& point, const float maxDist = 0.0f) const
	{
		for(uint n = 0; n < m_iNumClipPlanes; n++)
		{
			if(Distance(m_ClipPlanes[n], point) < maxDist)
				return false;
		}
		return true;
	}

	/*!
	Tests whether is point inside frustum. 
	\param point Point
	\param mask Mask of planes to be tested
	\return True when it's inside
	*/
	ENF_INLINE bool TestPoint(const Vector3& point, uint mask) const
	{
		if(mask == 0) return true;
		for(uint n = 0, b = 1; n < m_iNumClipPlanes; n++, b <<= 1)
		{
			if(b & mask)
			{
				if(Distance(m_ClipPlanes[n], point) < 0)
					return false;
			}
		}
		return true;
	}

	/*!
	Tests whether is sphere inside frustum
	\param origin Origin of sphere
	\param radius Radius of sphere
	\return True when it's inside
	*/
	ENF_INLINE bool TestSphere(const Vector3& origin, float radius) const
	{
		for(uint n = 0; n < m_iNumClipPlanes; n++)
		{
			float d = Distance(m_ClipPlanes[n], origin);

			if(d < -radius)
				return false;
		}
		return true;
	}
	/*!
	Tests whether is sphere inside frustum
	\param origin Origin of sphere
	\param radius Radius of sphere
	\param mask Mask of planes to be tested
	\return True when it's inside
	*/
	ENF_INLINE bool TestSphere(const Vector3& origin, float radius, uint mask) const
	{
		if(mask == 0) return true;
		for(uint n = 0, b = 1; n < m_iNumClipPlanes; n++, b <<= 1)
		{
			if(b & mask)
			{
				float d = Distance(m_ClipPlanes[n], origin);

				if(d < -radius)
					return false;
			}
		}
		return true;
	}

	/*!
	Tests whether is sphere inside frustum
	\param origin Origin of sphere
	\param radius Radius of sphere
	\param mask Mask of planes to be tested and updated
	\return True when it's inside
	*/
	ENF_INLINE bool TestSphereUpdate(const Vector3& origin, float radius, uint& mask) const
	{
		if(mask == 0) return true;
		for(uint n = 0, b = 1; n < m_iNumClipPlanes; n++, b <<= 1)
		{
			if(b & mask)
			{
				float d = Distance(m_ClipPlanes[n], origin);
				if(d < -radius)
					return false;
				if ( d > radius )
					mask ^= b;
			}
		}
		return true;
	}

	/*!
	Adds user clip-plane to frustum
	\param plane
	Plane to be added
	*/
	void AddPlane(const MPlane& plane);
	
	/*!
	Initialize frustum for perspective projection
	\param mask
	Mask of clipplanes we want to set up (see ViewFrustum::PlaneMask)
	\param mat
	Camera matrix
	\param fovh
	Horizontal FOV in radians
	\param fovv
	Vertical FOV in radians
	\param nearplane
	Distance of near clipping plane
	\param farplane
	Distance of far clipping plane
	*/
	void InitPerspective(uint mask, const Matrix43& mat, float fovh, float fovv, float nearplane = 0, float farplane = 0);
	/*!
	Initialize frustum for orthographic projection
	\param mask
	Mask of clipplanes we want to set up (see ViewFrustum::PlaneMask)
	\param mat
	Camera matrix
	\param fovh
	Horizontal FOV in units
	\param fovv
	Vertical FOV in units
	\param nearplane
	Distance of near clipping plane
	\param farplane
	Distance of far clipping plane
	*/
	void InitOrthographic(uint mask, const Matrix43& mat, float fovh, float fovv, float nearplane = 0, float farplane = 0);

	//!Returns number of clip-planes
	uint GetNumClipPlanes() const
	{
		return m_iNumClipPlanes;
	}
	const Vector4* GetClipPlanes() const
	{
		return m_ClipPlanes;
	}
	/*!
	Returns clip-plane
	\param num
	Number of plane
	\return
	Pointer to plane
	*/
	const Vector4& GetClipPlane(int num) const
	{
		return m_ClipPlanes[num];
	}

	//!Clears all clip-planes
	void Clear()
	{
		m_iNumClipPlanes = 0;
	}

	//!Returns mask of all set planes defined as (2^GetNumClipPlanes()) - 1)
	uint GetMask() const
	{
		return (1 << m_iNumClipPlanes) - 1;
	}

	ebool ClipConvex(){return false;};

	ViewFrustum():
		m_iNumClipPlanes(0),
		m_iMask(0)
	{
	}

	/*!
	enlarges frustum by directional light
	\param dir		directional vector of lights
	\param length	max distance of shadows
	\return
	Bit-mask of changed planes
	*/
	uint EnlargeByDirectionalLight(const Vector3& dir, float length);
};

//@}
}
#endif //ENF_VIEWFRUSTUM_H
