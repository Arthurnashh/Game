/*
(c) Copyright 2011, RTDynamics
All Rights Reserved.

Product:		RTDynamics
Subsystem:		RTDynamics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

namespace RTD
{
	template <class VecType>
	class LineSegment
	{
		VecType m_p0, m_p1;
		VecType m_u;
	public:
		LineSegment( const VecType& p1, const VecType& p2 )
			: m_p0(p1), m_p1(p2)
		{
			// Let L be given by the parametric equation: P(s) = P0 + s(P1-P0) = P0 + su.
			m_u = ( m_p1 - m_p0 );
		}

		const VecType getU( ) const	{ return m_u; }
		const VecType getP0( ) const	{ return m_p0; }
		const VecType getP1( ) const	{ return m_p1; }

		VecType getPointOnLine( double s ) const
		{
			return m_p0 + m_u * s;
		}
	};

	typedef LineSegment<smVec3r>	LineSegment3r;
	typedef LineSegment<smVec3d>	LineSegment3d;
}