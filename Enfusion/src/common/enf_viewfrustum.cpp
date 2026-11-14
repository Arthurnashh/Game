//-----------------------------------------------------------------------------
// File: enf_viewfrustum.cpp
//
// Desc: Mathematics library
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "common/enf_viewfrustum.h"

namespace enf
{
//-----------------------------------------------------------------------
void ViewFrustum::InitPerspective(uint mask, const Matrix43& mat, float fovh, float fovv, float nearplane, float farplane)
{
	float c = cosf(DEG2RAD(90.0f) - (fovh * 0.5f));
	float s = sinf(DEG2RAD(90.0f) - (fovh * 0.5f));

	Clear();

	if(mask & PLANE_NEAR)
	{
		//near plane
		m_ClipPlanes[m_iNumClipPlanes] = mat[0];
		m_ClipPlanes[m_iNumClipPlanes].w = m_ClipPlanes[m_iNumClipPlanes].Dot(mat[3]);
		m_iNumClipPlanes++;
	}

	if(mask & PLANE_FAR)
	{
		//far plane
		m_ClipPlanes[m_iNumClipPlanes] = -mat[0];
		m_ClipPlanes[m_iNumClipPlanes].w = m_ClipPlanes[m_iNumClipPlanes].Dot(mat[3]) - farplane;
		m_iNumClipPlanes++;
	}

	if(mask & PLANE_LEFT)
	{
		//left plane
		m_ClipPlanes[m_iNumClipPlanes] = mat.TransformVector(Vector3(c, -s, 0.0f));
		m_ClipPlanes[m_iNumClipPlanes].w = m_ClipPlanes[m_iNumClipPlanes].Dot(mat[3]);
		m_iNumClipPlanes++;
	}

	if(mask & PLANE_RIGHT)
	{
	   //right
		m_ClipPlanes[m_iNumClipPlanes] = mat.TransformVector(Vector3(c, s, 0.0f));
		m_ClipPlanes[m_iNumClipPlanes].w = m_ClipPlanes[m_iNumClipPlanes].Dot(mat[3]);
		m_iNumClipPlanes++;
	}

	c = cosf(DEG2RAD(90.0f) - (fovv * 0.5f));
	s = sinf(DEG2RAD(90.0f) - (fovv * 0.5f));

	if(mask & PLANE_BOTTOM)
	{
	   //bottom
		m_ClipPlanes[m_iNumClipPlanes] = mat.TransformVector(Vector3(c, 0.0f, -s));
		m_ClipPlanes[m_iNumClipPlanes].w = m_ClipPlanes[m_iNumClipPlanes].Dot(mat[3]);
		m_iNumClipPlanes++;
	}

	if(mask & PLANE_TOP)
	{
		//top
		m_ClipPlanes[m_iNumClipPlanes] = mat.TransformVector(Vector3(c, 0.0f, s));
		m_ClipPlanes[m_iNumClipPlanes].w = m_ClipPlanes[m_iNumClipPlanes].Dot(mat[3]);
		m_iNumClipPlanes++;
	}
}

//-----------------------------------------------------------------------
void ViewFrustum::InitOrthographic(uint mask, const Matrix43& mat, float fovh, float fovv, float nearplane, float farplane)
{
	Clear();

	/*for orthographic projection we make just
	oriented cube made of 6 perpedicular planes
	*/
	if(mask & PLANE_NEAR)
	{
		//near plane
		m_ClipPlanes[m_iNumClipPlanes] = mat[0];
		m_ClipPlanes[m_iNumClipPlanes].w = m_ClipPlanes[m_iNumClipPlanes].Dot(mat[3]); //TODO: nearplane
		m_iNumClipPlanes++;
	}

	if(mask & PLANE_FAR)
	{
		//far plane
		m_ClipPlanes[m_iNumClipPlanes] = -mat[0];
		m_ClipPlanes[m_iNumClipPlanes].w = m_ClipPlanes[m_iNumClipPlanes].Dot(mat[3]) - farplane;
		m_iNumClipPlanes++;
	}

	if(mask & PLANE_LEFT)
	{
		//left plane
		m_ClipPlanes[m_iNumClipPlanes] = -mat[1];
		m_ClipPlanes[m_iNumClipPlanes].w = m_ClipPlanes[m_iNumClipPlanes].Dot(mat[3]) - (fovh * 0.5f);
		m_iNumClipPlanes++;
	}

	if(mask & PLANE_RIGHT)
	{
	   //right
		m_ClipPlanes[m_iNumClipPlanes] = mat[1];
		m_ClipPlanes[m_iNumClipPlanes].w = m_ClipPlanes[m_iNumClipPlanes].Dot(mat[3]) - (fovh * 0.5f);
		m_iNumClipPlanes++;
	}

	if(mask & PLANE_BOTTOM)
	{
	   //bottom
		m_ClipPlanes[m_iNumClipPlanes] = -mat[2];
		m_ClipPlanes[m_iNumClipPlanes].w = m_ClipPlanes[m_iNumClipPlanes].Dot(mat[3]) - (fovv * 0.5f);
		m_iNumClipPlanes++;
	}

	if(mask & PLANE_TOP)
	{
		//top
		m_ClipPlanes[m_iNumClipPlanes] = mat[2];
		m_ClipPlanes[m_iNumClipPlanes].w = m_ClipPlanes[m_iNumClipPlanes].Dot(mat[3]) - (fovv * 0.5f);
		m_iNumClipPlanes++;
	}
}

//-----------------------------------------------------------------------
void ViewFrustum::AddPlane(const MPlane& plane)
{
	m_ClipPlanes[m_iNumClipPlanes] = plane;
	m_iNumClipPlanes++;
}


//-----------------------------------------------------------------------------
/*!
directional light 
*/
//-----------------------------------------------------------------------------
uint ViewFrustum::EnlargeByDirectionalLight(const Vector3& dir, float length)
{
	enf_assert2(fabs(dir.Length() - 1.0f) < 0.0001f, "dir must be normalized");

	Vector3 move (dir*length);
	uint enlarged = 0;

	//for each plane
	for (uint i = 0; i < m_iNumClipPlanes; i++)
	{
		float angle = m_ClipPlanes[i].Dot(dir);
		
		//normal of plane and direction vector of light are in the same direction 
		if (angle > 0.0f)
		{
			//point on plane
			Vector3 P(m_ClipPlanes[i].x*m_ClipPlanes[i].w, m_ClipPlanes[i].y*m_ClipPlanes[i].w, m_ClipPlanes[i].z*m_ClipPlanes[i].w);

			//move plane
			P -= move;

			m_ClipPlanes[i].w = m_ClipPlanes[i].Dot(P);
			enlarged |= (1 << i);
		}
	}
	return enlarged;
}



}